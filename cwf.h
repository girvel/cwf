#ifndef CWF_H
#define CWF_H
#include <stddef.h>
#include <stdlib.h>


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

#define CWF_DA_FOREACH(Type, it, da) \
    for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

#ifdef CWF_IMPLEMENTATION
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

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

    printf("Listening on %s:%i\n", address, port);

    char buffer[1024] = {0};
    socklen_t addrlen = sizeof(address);
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            perror("accept failed");
            close(server_fd);
            exit(1);
        }

        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read < 0) {
            perror("recv failed");
            close(client_fd);
            close(server_fd);
            exit(1);
        }

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Client says: %s\n", buffer);
            const char *reply = "Hello dude";
            send(client_fd, reply, strlen(reply), 0);
        }

        close(client_fd);
    }

    close(server_fd);
}

#endif

#endif
