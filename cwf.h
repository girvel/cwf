/* CWF stands for C Web Framework */

#ifndef CWF_H
#define CWF_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// =================================================================================================
// Utility functions
// =================================================================================================

typedef struct cwf_ArenaRegion cwf_ArenaRegion;
struct cwf_ArenaRegion {
    cwf_ArenaRegion *next;
    size_t count, capacity;
    uint8_t data[];
};

typedef struct cwf_Arena {
    cwf_ArenaRegion *begin, *end;
} cwf_Arena;

typedef struct cwf_ArenaMark {
    cwf_ArenaRegion *region;
    size_t count;
} cwf_ArenaMark;

#ifndef CWF_ARENA_DEFAULT_CAPACITY
#define CWF_ARENA_DEFAULT_CAPACITY 64*1024
#endif

cwf_ArenaRegion *_cwf_arena_region_new(size_t capacity);
void _cwf_arena_region_free(cwf_ArenaRegion *r);

void *cwf_arena_alloc(cwf_Arena *a, size_t size_bytes);
void *cwf_arena_realloc(cwf_Arena *a, void *prev, size_t prev_capacity, size_t next_capacity);
cwf_ArenaMark cwf_arena_snapshot(cwf_Arena *a);
void cwf_arena_reset(cwf_Arena *a);
void cwf_arena_rewind(cwf_Arena *a, cwf_ArenaMark m);
void cwf_arena_free(cwf_Arena *a);

void *cwf_alloc(size_t capacity);
void *cwf_realloc(void *ptr, size_t prev_capacity, size_t next_capacity);
void cwf_free(void *ptr);

#define CWF_DA_RESERVE(da, min_capacity) do { \
    if ((min_capacity) >= (da)->capacity) { \
        size_t prev_capacity = (da)->capacity; \
        if ((da)->capacity == 0) (da)->capacity = 16; \
        while ((min_capacity) > (da)->capacity) (da)->capacity *= 2; \
        (da)->items = cwf_realloc( \
            (da)->items, \
            sizeof(*(da)->items) * prev_capacity, \
            sizeof(*(da)->items) * (da)->capacity \
        ); \
    } \
} while (0)

#define CWF_DA_APPEND(da, item) do { \
    CWF_DA_RESERVE((da), (da)->count + 1); \
    (da)->items[(da)->count++] = (item); \
} while (0)

#define CWF_DA_APPEND_MANY(da, new_items, new_items_count) do { \
    CWF_DA_RESERVE((da), (da)->count + (new_items_count)); \
    memcpy((da)->items + (da)->count, (new_items), (new_items_count) * sizeof(*(da)->items)); \
    (da)->count += (new_items_count); \
} while (0)

#define CWF_DA_FOREACH(Type, it, da) \
    for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

typedef struct cwf_StringBuilder {
    char *items;
    size_t count, capacity;
} cwf_StringBuilder;

#define CWF_SB_APPEND_BUF(sb, buf, size) CWF_DA_APPEND_MANY(sb, buf, size)
#define CWF_SB_APPEND_NULL(sb) CWF_DA_APPEND_MANY(sb, "", 1)

int cwf_sb_appendf(cwf_StringBuilder *sb, const char *fmt, ...);

typedef struct cwf_StringView {
    const char *items;
    size_t count;
} cwf_StringView;

cwf_StringView cwf_sv_from_parts(const char *items, size_t count);
cwf_StringView cwf_sv_chop_by_delim(cwf_StringView *sv, char delim);
cwf_StringView cwf_sv_trim_space(cwf_StringView sv);
cwf_StringView cwf_sv_trim_space_left(cwf_StringView sv);
cwf_StringView cwf_sv_trim_space_right(cwf_StringView sv);
bool cwf_sv_eq(cwf_StringView sv, const char *str);
char *cwf_sv_strdup(cwf_StringView sv);

#ifdef CWF_IMPLEMENTATION
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

cwf_ArenaRegion *_cwf_arena_region_new(size_t capacity)
{
    size_t k = sizeof(uintptr_t);
    size_t total_capacity = (sizeof(cwf_ArenaRegion) + k - 1) / k * k + capacity;
    cwf_ArenaRegion *r = malloc(total_capacity);
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void _cwf_arena_region_free(cwf_ArenaRegion *r)
{
    free(r);
}

void *cwf_arena_alloc(cwf_Arena *a, size_t size_bytes)
{
    // rounding trick
    size_t size = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t) * sizeof(uintptr_t);

    if (a->end == NULL) {
        assert(a->begin == NULL);
        size_t capacity = CWF_ARENA_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->end = _cwf_arena_region_new(capacity);
        a->begin = a->end;
    } else {
        while (a->end->count + size > a->end->capacity && a->end->next != NULL) {
            a->end = a->end->next;
        }

        if (a->end->count + size > a->end->capacity) {
            size_t capacity = CWF_ARENA_DEFAULT_CAPACITY;
            if (capacity < size) capacity = size;
            a->end->next = _cwf_arena_region_new(capacity);
            a->end = a->end->next;
        }
    }

    void *result = a->end->data + a->end->count;
    a->end->count += size;
    return result;
}

void *cwf_arena_realloc(cwf_Arena *a, void *ptr, size_t prev_capacity, size_t next_capacity)
{
    if (next_capacity <= prev_capacity) return ptr;
    void *next_ptr = cwf_arena_alloc(a, next_capacity);
    memcpy(next_ptr, ptr, prev_capacity);
    return next_ptr;
}

static cwf_Arena cwf_arena = {0};

void *cwf_alloc(size_t capacity)
{
    return cwf_arena_alloc(&cwf_arena, capacity);
}

void *cwf_realloc(void *ptr, size_t prev_capacity, size_t next_capacity)
{
    return cwf_arena_realloc(&cwf_arena, ptr, prev_capacity, next_capacity);
}

cwf_StringView cwf_sv_from_parts(const char *items, size_t count)
{
    cwf_StringView result;
    result.items = items;
    result.count = count;
    return result;
}

cwf_StringView cwf_sv_chop_by_delim(cwf_StringView *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->items[i] != delim) i += 1;

    cwf_StringView result = cwf_sv_from_parts(sv->items, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->items += i + 1;
    } else {
        sv->count -= i;
        sv->items += i;
    }

    return result;
}

bool cwf_sv_eq(cwf_StringView sv, const char *str)
{
    size_t len = strlen(str);
    if (len != sv.count) return false;
    return strncmp(sv.items, str, len) == 0;
}

int cwf_sb_appendf(cwf_StringBuilder *sb, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    CWF_DA_RESERVE(sb, sb->count + n + 1);
    char *dest = sb->items + sb->count;
    va_start(args, fmt);
    vsnprintf(dest, n+1, fmt, args);
    va_end(args);

    sb->count += n;

    return n;
}

cwf_StringView cwf_sv_trim_space(cwf_StringView sv)
{
    return cwf_sv_trim_space_left(cwf_sv_trim_space_right(sv));
}

cwf_StringView cwf_sv_trim_space_left(cwf_StringView sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.items[i])) {
        i += 1;
    }

    // Apparently if sv.items == 0 and i == 0 adding them
    // together is an undefined behavior. According to
    // clang's UndefinedBehaviorSanitizer at least.
    if (i == 0) return sv;

    return cwf_sv_from_parts(sv.items + i, sv.count - i);
}

cwf_StringView cwf_sv_trim_space_right(cwf_StringView sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.items[sv.count - 1 - i])) {
        i += 1;
    }

    return cwf_sv_from_parts(sv.items, sv.count - i);
}

char *cwf_sv_strdup(cwf_StringView sv)
{
    char *result;
    if (sv.items[sv.count - 1] == '\0') {
        result = cwf_alloc(sv.count);
        memcpy(result, sv.items, sv.count);
    } else {
        result = cwf_alloc(sv.count + 1);
        memcpy(result, sv.items, sv.count);
        result[sv.count] = '\0';
    }
    return result;
}

#endif // CWF_IMPLEMENTATION

// =================================================================================================
// API
// =================================================================================================

typedef struct cwf_Header {
    const char *name, *value;
} cwf_Header;

typedef struct cwf_Headers {
    cwf_Header *items;
    size_t count, capacity;
} cwf_Headers;

typedef struct cwf_Request {
    cwf_Headers headers;
} cwf_Request;

typedef struct cwf_Response {
    const char *body;
    int code;
    cwf_Headers headers;
} cwf_Response;

typedef cwf_Response (*cwf_ApiHandler)(cwf_Request);

typedef struct _cwf_Endpoint {
    const char *method;
    const char *address;
    cwf_ApiHandler handler;
} _cwf_Endpoint;

typedef struct _cwf_Endpoints {
    _cwf_Endpoint *items;
    size_t count, capacity;
} _cwf_Endpoints;

typedef struct cwf_App {
    _cwf_Endpoints _endpoints;
} cwf_App;

void cwf_app_route(cwf_App *app, const char *method, const char *endpoint, cwf_ApiHandler handler);
void cwf_app_run(cwf_App app, const char *address, int port);
void cwf_headers_set(cwf_Headers *headers, const char *name, const char *value);
const char *cwf_headers_get(cwf_Headers headers, const char *name);

// =================================================================================================
// Implementation
// =================================================================================================

#ifdef CWF_IMPLEMENTATION
#include <arpa/inet.h>
#include <unistd.h>

void cwf_app_route(cwf_App *app, const char *method, const char *endpoint, cwf_ApiHandler handler)
{
    _cwf_Endpoint e = {
        .method = method,
        .address = endpoint,
        .handler = handler,
    };
    CWF_DA_APPEND(&app->_endpoints, e);
}

void cwf_app_run(cwf_App app, const char *address, int port)
{
    CWF_DA_FOREACH(_cwf_Endpoint, e, &app._endpoints) {
        printf("GET %s\n", e->address);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(1);
    }

    printf("Listening on %s:%i\n\n", address, port);

    cwf_StringBuilder response_sb = {0};
    CWF_DA_RESERVE(&response_sb, 4096);
    socklen_t addrlen = sizeof(address);
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            perror("accept failed");
            close(server_fd);
            exit(1);
        }

        // TODO string building
        char buffer[4096] = {0};
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read < 0) {
            perror("recv failed");
            close(client_fd);
            close(server_fd);
            exit(1);
        }

        if (bytes_read == 0) goto finish;

        buffer[bytes_read] = '\0';
        printf("> REQUEST\n%s", buffer);

        cwf_StringView sv = cwf_sv_from_parts(buffer, bytes_read);
        cwf_StringView method = cwf_sv_chop_by_delim(&sv, ' ');
        cwf_StringView address = cwf_sv_chop_by_delim(&sv, ' ');
        cwf_sv_chop_by_delim(&sv, '\n');

        cwf_Request request = {0};
        while (sv.count > 0) {
            cwf_StringView value = cwf_sv_chop_by_delim(&sv, '\n');
            cwf_StringView key = cwf_sv_chop_by_delim(&value, ':');

            value = cwf_sv_trim_space(value);
            key = cwf_sv_trim_space(key);
            sv = cwf_sv_trim_space(sv);

            cwf_headers_set(&request.headers, cwf_sv_strdup(key), cwf_sv_strdup(value));
        }

        response_sb.count = 0;
        CWF_DA_FOREACH (_cwf_Endpoint, e, &app._endpoints) {
            if (cwf_sv_eq(address, e->address) && cwf_sv_eq(method, e->method)) {
                cwf_Response response_data = e->handler(request);
                cwf_sb_appendf(&response_sb, "HTTP/1.1 %d\r\n", response_data.code);

                size_t content_length = strlen(response_data.body);
                cwf_sb_appendf(&response_sb, "content-length: %d\r\n", content_length + 1);
                
                const char *content_type = cwf_headers_get(response_data.headers, "content-type");
                if (content_type == NULL) content_type = "text/plain";
                cwf_sb_appendf(&response_sb, "content-type: %s\r\n", content_type);

                cwf_sb_appendf(&response_sb, "\r\n");
                CWF_SB_APPEND_BUF(&response_sb, response_data.body, content_length + 1);
                goto found;
            }
        }
        cwf_sb_appendf(&response_sb, "HTTP/1.1 404\r\n");
        CWF_SB_APPEND_NULL(&response_sb);

    found:
        printf("> RESPONSE\n%s\n", response_sb.items);
        send(client_fd, response_sb.items, response_sb.count, 0);

    finish:
        close(client_fd);
    }

    close(server_fd);
}

void cwf_headers_set(cwf_Headers *headers, const char *name, const char *value)
{
    cwf_Header header = {name, value};
    CWF_DA_APPEND(headers, header);
}

const char *cwf_headers_get(cwf_Headers headers, const char *name)
{
    CWF_DA_FOREACH (cwf_Header, h, &headers) {
        if (strcmp(name, h->name) == 0) {
            return h->value;
        }
    }
    return NULL;
}

#endif // CWF_IMPLEMENTATION
#endif // CWF_H
