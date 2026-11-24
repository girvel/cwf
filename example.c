#include <stdio.h>
#define CWF_IMPLEMENTATION
#include "cwf.h"

// BUGS
// - cwf_headers_set does not overwrite
// - 404 is malformed, should have content-length 0

// QUESTS
// - redirect endpoint/ -> endpoint and back
// - serve CSS
// - graceful shutdown
// - telnet localhost 8000 & not typing would block other connections

// MAYBE LATER
// - serve static files from a directory
// - sanitize ../../etc/passwd-like paths
// - path arguments
// - query arguments
// - POST, JSON?
// - host a real website
// - cookies
// - try auth?

cwf_Response api_index(cwf_Request request)
{
    const char *user_agent = cwf_headers_get(request.headers, "user-agent");
    if (user_agent == NULL) user_agent = "unknown agent";

    cwf_Response response = {
        .code = 200,
        .body = sv_cstr(cwf_sprintf(
            "<html><head><title>CWF example</title></head>"
            "<body><h1>Your user agent is: %s</h1></body></html>\r\n",
            user_agent
        )),
    };

    cwf_headers_set(&response.headers, "content-type", "text/html");
    return response;
}

cwf_Response api_favicon(cwf_Request request)
{
    (void)request;

    cwf_StringBuilder sb = {0};
    if (!cwf_file_read("favicon.png", &sb)) {
        return (cwf_Response) { .code = 500 };
    }

    cwf_Response response = {
        .code = 200,
        .body = cwf_sv_from_sb(sb),
    };
    cwf_headers_set(&response.headers, "content-type", "image/png");
    return response;
}

cwf_Response api_favicon_redirect(cwf_Request request)
{
    (void)request;
    return cwf_response_found("/favicon.png");
}

int main()
{
    cwf_App app = {0};
    cwf_app_route(&app, "GET", "/", api_index);
    cwf_app_route(&app, "GET", "/favicon.png", api_favicon);
    cwf_app_route(&app, "GET", "/favicon.ico", api_favicon_redirect);
    cwf_app_run(app, "localhost", 8000);
    printf("Program finished.\n");
    return 0;
}
