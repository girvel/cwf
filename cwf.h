#ifndef CWF_H
#define CWF_H

typedef struct cwf_Response {
    const char *body;
    int code;
} cwf_Response;

typedef struct cwf_App {
} cwf_App;

typedef cwf_Response (*cwf_ApiHandler)();

void cwf_app_route(cwf_App *app, const char *endpoint, cwf_ApiHandler handler);
void cwf_app_run(cwf_App app, const char *address, int port);

#ifdef CWF_IMPLEMENTATION

void cwf_app_route(cwf_App *app, const char *endpoint, cwf_ApiHandler handler)
{

}

void cwf_app_run(cwf_App app, const char *address, int port)
{
    
}

#endif

#endif
