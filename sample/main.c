#include <stdio.h>
#include <unistd.h>
#define CWF_IMPLEMENTATION
#include "../cwf.h"

// MAYBE LATER
// - telnet localhost 8000 & not typing would block other connections
// - serve static files from a directory
// - sanitize ../../etc/passwd-like paths
// - path arguments
// - query arguments
// - POST, JSON?
// - do a chat site?
//   - registering
//   - basic auth
//   - getting messages
//   - displaying messages
//   - writing messages
// - cookies
// - try auth?

cwf_Response api_index(cwf_Request request)
{
    const char *user_agent = cwf_headers_get(request.headers, "user-agent");
    if (user_agent == NULL) user_agent = "unknown agent";

    cwf_Response response = {
        .code = 200,
        .body = sv_cstr(cwf_sprintf(
            "<html>\n"
            "<head>\n"
            "  <title>CWF example</title>\n"
            "  <link rel=\"stylesheet\" href=\"/simple.css\" />\n"
            "</head>\n"
            "<body>\n"
            "  <p>Your user agent is: %s</p>\n"
            "</body>\n"
            "</html>\n",
            user_agent
        )),
    };

    cwf_headers_push(&response.headers, "content-type", "text/html");
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
    cwf_headers_push(&response.headers, "content-type", "image/png");
    return response;
}

cwf_Response api_favicon_redirect(cwf_Request request)
{
    (void)request;
    return cwf_response_found("/favicon.png");
}

cwf_Response api_simple_css(cwf_Request request)
{
    (void)request;
    cwf_StringBuilder sb = {0};
    if (!cwf_file_read("simple.css", &sb)) {
        return (cwf_Response) { .code = 500 };
    }

    cwf_Response response = {
        .code = 200,
        .body = cwf_sv_from_sb(sb),
    };
    cwf_headers_push(&response.headers, "content-type", "text/css");
    return response;
}

int main()
{
    // TODO only chdir if needed
    chdir("sample");

    cwf_App app = {0};
    cwf_app_route(&app, "GET", "/", api_index);
    cwf_app_route(&app, "GET", "/favicon.png", api_favicon);
    cwf_app_route(&app, "GET", "/favicon.ico", api_favicon_redirect);
    cwf_app_route(&app, "GET", "/simple.css", api_simple_css);
    cwf_app_run(app, "localhost", 8000);
    printf("Program finished.\n");
    return 0;
}
