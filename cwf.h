/* CWF stands for C Web Framework */

#ifndef CWF_H
#define CWF_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// =================================================================================================
// Utility functions
// =================================================================================================

#define CWF_DA_RESERVE(da, min_capacity) do { \
    if ((min_capacity) >= (da)->capacity) { \
        if ((da)->capacity == 0) (da)->capacity = 16; \
        while ((min_capacity) > (da)->capacity) (da)->capacity *= 2; \
        (da)->items = realloc((da)->items, sizeof(*(da)->items) * (da)->capacity); \
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

typedef struct cwf_StringView {
    const char *items;
    size_t count;
} cwf_StringView;

cwf_StringView cwf_sv_from_parts(const char *items, size_t count);

cwf_StringView cwf_sv_chop_by_delim(cwf_StringView *sv, char delim);

#ifdef CWF_IMPLEMENTATION

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

#endif

// =================================================================================================
// API
// =================================================================================================

typedef struct cwf_Response {
    const char *body;
    int code;
} cwf_Response;

typedef cwf_Response (*cwf_ApiHandler)();

typedef struct _cwf_Endpoint {
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

void cwf_app_route(cwf_App *app, const char *endpoint, cwf_ApiHandler handler);
void cwf_app_run(cwf_App app, const char *address, int port);

// =================================================================================================
// Implementation
// =================================================================================================

#ifdef CWF_IMPLEMENTATION
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

void cwf_app_route(cwf_App *app, const char *endpoint, cwf_ApiHandler handler)
{
    _cwf_Endpoint e = {endpoint, handler};
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

        char *response;
        CWF_DA_FOREACH (_cwf_Endpoint, e, &app._endpoints) {
            if (strncmp(e->address, address.items, strlen(e->address)) == 0) {
                response = "HTTP/2 200\n";
                goto found;
            }
        }
        response = "HTTP/2 404\n";

    found:
        printf("> RESPONSE\n%s\n", response);
        send(client_fd, response, strlen(response), 0);

    finish:
        close(client_fd);
    }

    close(server_fd);
}

#endif

#endif
