#ifndef CWF_H
#define CWF_H
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


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
}

#endif

#endif
