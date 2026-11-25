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
// [SECTION] Utility functions
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
char *cwf_strdup(const char *source);
char *cwf_sprintf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));

#define CWF_DA_RESERVE_EXACT(da, exact_capacity) do { \
    if ((exact_capacity) >= (da)->capacity) { \
        (da)->items = cwf_realloc( \
            (da)->items, \
            sizeof(*(da)->items) * (da)->capacity, \
            sizeof(*(da)->items) * (exact_capacity) \
        ); \
        (da)->capacity = (exact_capacity); \
    } \
} while (0)

#define CWF_DA_RESERVE(da, min_capacity) do { \
    size_t exact_capacity = (da)->capacity; \
    if (exact_capacity == 0) exact_capacity = 16; \
    while ((min_capacity) > exact_capacity) exact_capacity *= 2; \
    CWF_DA_RESERVE_EXACT((da), exact_capacity); \
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

typedef struct cwf_StringView {
    const char *items;
    size_t count;
} cwf_StringView;

void cwf_sb_append_null(cwf_StringBuilder *sb);
void cwf_sb_append_buf(cwf_StringBuilder *sb, const char *buf, size_t size);
void cwf_sb_append_sv(cwf_StringBuilder *sb, cwf_StringView sv);
int cwf_sb_appendf(cwf_StringBuilder *sb, const char *fmt, ...)
    __attribute__ ((format(printf, 2, 3)));

cwf_StringView cwf_sv_from_parts(const char *items, size_t count);
cwf_StringView cwf_sv_from_sb(cwf_StringBuilder sb);
cwf_StringView cwf_sv_chop_by_delim(cwf_StringView *sv, char delim);
cwf_StringView cwf_sv_trim_space(cwf_StringView sv);
cwf_StringView cwf_sv_trim_space_left(cwf_StringView sv);
cwf_StringView cwf_sv_trim_space_right(cwf_StringView sv);
bool cwf_sv_eq(cwf_StringView sv, const char *str);
char *cwf_sv_strdup(cwf_StringView sv);

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)(sv).count, (sv).items

static inline cwf_StringView sv_cstr(const char *source)
{
    return (cwf_StringView) {source, strlen(source)};
}

bool cwf_file_read(const char *path, cwf_StringBuilder *sb);

#define CWF_MAX2(A, B) ({ \
    __typeof__((A+B)) CWF_MAX2_a = (A), CWF_MAX2_b = (B); \
    CWF_MAX2_a > CWF_MAX2_b ? CWF_MAX2_a : CWF_MAX2_b; \
})

#define CWF_MIN2(A, B) ({ \
    __typeof__((A+B)) CWF_MIN2_a = (A), CWF_MIN2_b = (B); \
    CWF_MIN2_a < CWF_MIN2_b ? CWF_MIN2_a : CWF_MIN2_b; \
})

typedef struct cwf_StringPair {
    char *key, *value;
} cwf_StringPair;

typedef struct cwf_StringTable {
    cwf_StringPair *items;
    size_t count, capacity;
} cwf_StringTable;

const char *cwf_st_get(cwf_StringTable table, const char *key);
void cwf_st_set(cwf_StringTable *table, const char *key, const char *value);

// =================================================================================================
// [SECTION] API
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
    cwf_StringView body;
} cwf_Request;

typedef struct cwf_Response {
    // May contain null terminators => cwf_StringView
    cwf_Headers headers;
    cwf_StringView body;
    int code;
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
void cwf_headers_push(cwf_Headers *headers, const char *name, const char *value);
void cwf_headers_push_sv(cwf_Headers *headers, cwf_StringView name, cwf_StringView value);
const char *cwf_headers_get(cwf_Headers headers, const char *name);
cwf_Response cwf_response_moved_permanently(const char *uri);
cwf_Response cwf_response_found(const char *uri);
bool cwf_form_read(cwf_StringView body, cwf_StringTable *result);

// =================================================================================================
// [SECTION] Utility implementation
// =================================================================================================

#ifdef CWF_IMPLEMENTATION
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

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

cwf_ArenaMark cwf_arena_snapshot(cwf_Arena *a)
{
    cwf_ArenaMark result;
    if (a->end == NULL) {
        assert(a->begin == NULL);
        result.region = NULL;
        result.count = 0;
    } else {
        result.region = a->end;
        result.count = a->end->count;
    }
    return result;
}

void cwf_arena_reset(cwf_Arena *a)
{
    for (cwf_ArenaRegion *r = a->begin; r != NULL; r = r->next) {
        r->count = 0;
    }
    a->end = a->begin;
}

void cwf_arena_rewind(cwf_Arena *a, cwf_ArenaMark m)
{
    if (m.region == NULL) {
        cwf_arena_reset(a);
        return;
    }

    m.region->count = m.count;
    for (cwf_ArenaRegion *r = m.region->next; r != NULL; r = r->next) {
        r->count = 0;
    }

    a->end = m.region;
}

void cwf_arena_free(cwf_Arena *a);

static cwf_Arena cwf_arena = {0};

void *cwf_alloc(size_t capacity)
{
    return cwf_arena_alloc(&cwf_arena, capacity);
}

void *cwf_realloc(void *ptr, size_t prev_capacity, size_t next_capacity)
{
    return cwf_arena_realloc(&cwf_arena, ptr, prev_capacity, next_capacity);
}

char *cwf_strdup(const char *source)
{
    size_t len = strlen(source);
    char *result = cwf_alloc(len + 1);
    strcpy(result, source);
    return result;
}

char *cwf_sprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    int n = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    char *result = cwf_alloc(n + 1);
    vsnprintf(result, n + 1, fmt, args);

    va_end(args);
    return result;
}

cwf_StringView cwf_sv_from_parts(const char *items, size_t count)
{
    cwf_StringView result;
    result.items = items;
    result.count = count;
    return result;
}

cwf_StringView cwf_sv_from_sb(cwf_StringBuilder sb)
{
    cwf_StringView result;
    result.items = sb.items;
    result.count = sb.count;
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

void cwf_sb_append_null(cwf_StringBuilder *sb)
{
    CWF_DA_APPEND_MANY(sb, "", 1);
}

void cwf_sb_append_buf(cwf_StringBuilder *sb, const char *buf, size_t size)
{
    CWF_DA_APPEND_MANY(sb, buf, size);
}

void cwf_sb_append_sv(cwf_StringBuilder *sb, cwf_StringView sv)
{
    CWF_DA_APPEND_MANY(sb, sv.items, sv.count);
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
    char *result = cwf_alloc(sv.count + 1);
    memcpy(result, sv.items, sv.count);
    result[sv.count] = '\0';
    return result;
}

bool cwf_file_read(const char *path, cwf_StringBuilder *sb)
{
    bool result = true;

    FILE *f = fopen(path, "rb");
    size_t new_count = 0;
    long long m = 0;
    if (f == NULL) {
        result = false;
        goto defer;
    }
    if (fseek(f, 0, SEEK_END) < 0) {
        result = false;
        goto defer;
    }
#ifndef _WIN32
    m = ftell(f);
#else
    m = _telli64(_fileno(f));
#endif
    if (m < 0) {
        result = false;
        goto defer;
    }
    if (fseek(f, 0, SEEK_SET) < 0) {
        result = false;
        goto defer;
    }

    new_count = sb->count + m;
    if (new_count > sb->capacity) {
        sb->items = cwf_realloc(sb->items, sb->capacity, new_count);
        sb->capacity = new_count;
    }

    fread(sb->items + sb->count, m, 1, f);
    if (ferror(f)) {
        result = false;
        goto defer;
    }
    sb->count = new_count;

defer:
    if (f) fclose(f);
    return result;
}

const char *cwf_st_get(cwf_StringTable table, const char *key)
{
    for (cwf_StringPair *pair = table.items; pair < table.items + table.count; pair++) {
        if (strcmp(pair->key, key) == 0) return pair->value;
    }
    return NULL;
}

void cwf_st_set(cwf_StringTable *table, const char *key, const char *value)
{
    char *value_copy = cwf_strdup(value);
    for (cwf_StringPair *pair = table->items; pair < table->items + table->count; pair++) {
        if (strcmp(pair->key, key) == 0) {
            pair->value = value_copy;
            return;
        }
    }

    cwf_StringPair new_pair = {cwf_strdup(key), value_copy};
    CWF_DA_APPEND(table, new_pair);
}

// =================================================================================================
// [SECTION] Implementation
// =================================================================================================

void cwf_app_route(cwf_App *app, const char *method, const char *endpoint, cwf_ApiHandler handler)
{
    _cwf_Endpoint e = {
        .method = method,
        .address = endpoint,
        .handler = handler,
    };
    CWF_DA_APPEND(&app->_endpoints, e);
}

static volatile sig_atomic_t is_running = false;

static void _handle_sigint(int signum)
{
    (void)signum;
    is_running = false;
}

void cwf_app_run(cwf_App app, const char *address, int port)
{
    CWF_DA_FOREACH (_cwf_Endpoint, e, &app._endpoints) {
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

    struct sigaction sa_prev;
    struct sigaction sa = {
        .sa_handler = _handle_sigint,
        .sa_flags = 0,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, &sa_prev);

    cwf_ArenaMark pre_request_baseline = cwf_arena_snapshot(&cwf_arena);

    is_running = true;
    while (is_running) {
        socklen_t addrlen = sizeof(address);
        int client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
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
        if (buffer[bytes_read - 1] != '\n') {
            printf("\n");
        }

        cwf_StringView sv = cwf_sv_from_parts(buffer, bytes_read);
        cwf_StringView method = cwf_sv_chop_by_delim(&sv, ' ');
        cwf_StringView requested_address = cwf_sv_chop_by_delim(&sv, ' ');
        cwf_sv_chop_by_delim(&sv, '\n');

        cwf_Request request = {0};
        while (sv.count > 0) {
            cwf_StringView value = cwf_sv_trim_space(cwf_sv_chop_by_delim(&sv, '\n'));
            if (value.count == 0) break;
            cwf_StringView key = cwf_sv_trim_space(cwf_sv_chop_by_delim(&value, ':'));
            cwf_headers_push_sv(&request.headers, key, value);
        }
        request.body = sv;

        cwf_StringBuilder response_sb = {0};
        CWF_DA_RESERVE(&response_sb, 4096);

        CWF_DA_FOREACH (_cwf_Endpoint, endpoint, &app._endpoints) {
            if (!cwf_sv_eq(method, endpoint->method)) continue;

            cwf_Response response_data;
            if (cwf_sv_eq(requested_address, endpoint->address)) {
                response_data = endpoint->handler(request);
                const char *content_type = cwf_headers_get(response_data.headers, "content-type");
                if (content_type == NULL) {
                    cwf_headers_push(&response_data.headers, "content-type", "text/plain");
                }
                goto response_formed;
            }

            size_t endpoint_address_len = strlen(endpoint->address);
            if (endpoint->address[endpoint_address_len - 1] == '/'
                && endpoint_address_len == requested_address.count + 1
                && strncmp(endpoint->address, requested_address.items, requested_address.count) == 0
            ) {
                response_data = cwf_response_moved_permanently(endpoint->address);
                goto response_formed;
            }

            if (requested_address.items[requested_address.count - 1] == '/'
                && endpoint_address_len + 1 == requested_address.count
                && strncmp(endpoint->address, requested_address.items, endpoint_address_len) == 0
            ) {
                response_data = cwf_response_moved_permanently(endpoint->address);
                goto response_formed;
            }

            continue;

        response_formed:
            cwf_sb_appendf(&response_sb, "HTTP/1.1 %d\r\n", response_data.code);
            cwf_sb_appendf(&response_sb, "content-length: %ld\r\n", response_data.body.count);
            CWF_DA_FOREACH (cwf_Header, h, &response_data.headers) {
                cwf_sb_appendf(&response_sb, "%s: %s\r\n", h->name, h->value);
            }
            cwf_sb_appendf(&response_sb, "\r\n");
            cwf_sb_append_sv(&response_sb, response_data.body);
            goto found;
        }
        cwf_sb_appendf(&response_sb, "HTTP/1.1 404\r\ncontent-length: 0\r\n");

        cwf_StringView response_cropped;
    found:
        response_cropped = cwf_sv_from_parts(
            response_sb.items, CWF_MIN2(response_sb.count, 1024)
        );
        printf("> RESPONSE\n" SV_FMT "\n", SV_ARG(response_cropped));
        send(client_fd, response_sb.items, response_sb.count, 0);

    finish:
        close(client_fd);
        printf("Arena last region count: %ld\n", cwf_arena.end->count);
        cwf_arena_rewind(&cwf_arena, pre_request_baseline);
    }

    close(server_fd);
    sigaction(SIGINT, &sa_prev, NULL);
}

void cwf_headers_push_sv(cwf_Headers *headers, cwf_StringView name, cwf_StringView value)
{
    char *name_lower = cwf_alloc(name.count + 1);
    for (size_t i = 0; i < name.count; i++) name_lower[i] = tolower((unsigned char)name.items[i]);
    name_lower[name.count] = '\0';
    cwf_Header header = {name_lower, cwf_sv_strdup(value)};
    CWF_DA_APPEND(headers, header);
}

void cwf_headers_push(cwf_Headers *headers, const char *name, const char *value)
{
    cwf_headers_push_sv(headers, sv_cstr(name), sv_cstr(value));
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

cwf_Response cwf_response_found(const char *uri)
{
    cwf_Response result = {
        .code = 302,
        .body = sv_cstr(cwf_sprintf("<a href=\"%s\">Found</a>.\r\n", uri)),
    };
    
    cwf_headers_push(&result.headers, "content-type", "text/html");
    cwf_headers_push(&result.headers, "location", uri);
    return result;
}

cwf_Response cwf_response_moved_permanently(const char *uri)
{
    cwf_Response result = {
        .code = 301,
        .body = sv_cstr(cwf_sprintf("<a href=\"%s\">Moved Permanently</a>.\r\n", uri)),
    };
    
    cwf_headers_push(&result.headers, "content-type", "text/html");
    cwf_headers_push(&result.headers, "location", uri);
    return result;
}

static unsigned char _cwf_from_hex(char ch)
{
    if (ch <= '9') return ch - '0';
    if (ch <= 'F') return ch - 'A' + 0xA;
    if (ch <= 'f') return ch - 'a' + 0xA;
    return 0;
}

static bool _cwf_form_process_string(cwf_StringView str, char **result)
{
    cwf_StringBuilder sb = {0};
    CWF_DA_RESERVE_EXACT(&sb, str.count + 1);
    for (size_t i = 0; i < str.count; i++) {
        switch (str.items[i]) {
            case '+':
                CWF_DA_APPEND(&sb, ' ');
                break;
            case '%': {
                if (str.count - i <= 2) return false;
                unsigned char c1 = _cwf_from_hex(str.items[++i]);
                unsigned char c2 = _cwf_from_hex(str.items[++i]);
                CWF_DA_APPEND(&sb, (c1 << 4) | c2);
                break;
            }
            default:
                CWF_DA_APPEND(&sb, str.items[i]);
        }
    }
    cwf_sb_append_null(&sb);
    *result = sb.items;
    return true;
}

bool cwf_form_read(cwf_StringView body, cwf_StringTable *result)
{
    while (body.count > 0) {
        cwf_StringView value = cwf_sv_chop_by_delim(&body, '&');
        cwf_StringView key = cwf_sv_chop_by_delim(&value, '=');

        char *value_processed, *key_processed;
        if (!_cwf_form_process_string(value, &value_processed)) return false;
        if (!_cwf_form_process_string(key, &key_processed)) return false;
        
        cwf_st_set(result, key_processed, value_processed);
    }
    return true;
}

#endif // CWF_IMPLEMENTATION
#endif // CWF_H
