#include <stdio.h>
#define CWF_IMPLEMENTATION
#include "cwf.h"

// FUTURE QUESTS

char buffer[4096];

cwf_Response api_get_hw(cwf_Request request)
{
    const char *user_agent = cwf_headers_get(request.headers, "user-agent");
    if (user_agent == NULL) user_agent = "unknown agent";

    cwf_Response response = {
      .code = 200,
      .body = cwf_sprintf("<h1>Hello, %s!</h1>\r\n", user_agent),
    };

    cwf_headers_set(&response.headers, "content-type", "text/html");
    return response;
}

int main()
{
    cwf_App app = {0};
    cwf_app_route(&app, "GET", "/api/something", api_get_hw);
    cwf_app_run(app, "localhost", 8000);
    printf("Program finished.\n");
    return 0;
}
