#include <stdio.h>
#include <unistd.h>
#define CWF_IMPLEMENTATION
#include "../cwf.h"

// SIDE QUESTS
// - enum for http response codes

// QUEST: chat-site
// - registering
//   - connect sqlite
// - basic auth
// - getting messages
// - displaying messages
// - writing messages

// MAYBE LATER
// - telnet localhost 8000 & not typing would block other connections
// - serve static files from a directory
// - sanitize ../../etc/passwd-like paths
// - path arguments
// - query arguments
// - POST, JSON?
// - cookies
// - try auth?

cwf_Response site_index(cwf_Request request)
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

cwf_Response site_favicon_redirect(cwf_Request request)
{
    (void)request;
    return cwf_response_found("/favicon.png");
}

static inline cwf_Response serve_file(const char *path)
{
    cwf_StringBuilder sb = {0};
    if (!cwf_file_read(path, &sb)) {
        return (cwf_Response) { .code = 500 };
    }

    return (cwf_Response) {
        .code = 200,
        .body = cwf_sv_from_sb(sb),
    };
}

cwf_Response site_favicon(cwf_Request request)
{
    (void)request;
    cwf_Response response = serve_file("favicon.png");
    if (response.code == 200) cwf_headers_push(&response.headers, "content-type", "image/png");
    return response;
}

cwf_Response site_simple_css(cwf_Request request)
{
    (void)request;
    cwf_Response response = serve_file("simple.css");
    if (response.code == 200) cwf_headers_push(&response.headers, "content-type", "text/css");
    return response;
}

cwf_Response site_register(cwf_Request request)
{
    (void)request;
    cwf_Response response = serve_file("register.html");
    if (response.code == 200) cwf_headers_push(&response.headers, "content-type", "text/html");
    return response;
}

cwf_Response api_register(cwf_Request request)
{
    cwf_StringTable form_data = {0};
    if (!cwf_form_read(request.body, &form_data)) {
        return (cwf_Response) {.code = 400};
    }

    const char *username = cwf_st_get(form_data, "username");
    if (username == NULL) username = "<missing>";
    const char *password = cwf_st_get(form_data, "password");
    if (password == NULL) password = "<missing>";

    printf("username: |%s| password: |%s|\n", username, password);

    // TODO respond OK if username is not admin
    // cwf_Response response = cwf_response_found("/");
    return (cwf_Response) {.code = 204};
}

int main()
{
    // TODO only chdir if needed
    chdir("sample");

    cwf_App app = {0};
    cwf_app_route(&app, "GET", "/", site_index);
    cwf_app_route(&app, "GET", "/register", site_register);
    cwf_app_route(&app, "GET", "/favicon.png", site_favicon);
    cwf_app_route(&app, "GET", "/favicon.ico", site_favicon_redirect);
    cwf_app_route(&app, "GET", "/simple.css", site_simple_css);
    cwf_app_route(&app, "POST", "/api/register", api_register);
    cwf_app_run(app, "localhost", 8000);
    printf("Program finished.\n");
    return 0;
}
