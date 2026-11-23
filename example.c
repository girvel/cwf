#include <stdio.h>
#define CWF_IMPLEMENTATION
#include "cwf.h"

cwf_Response api_get_hw()
{
    cwf_Response response = {
      .code = 200,
      .body = "<h1>Hello, world!</h1>\r\n",
    };

    cwf_response_set_header(&response, "content-type", "text/html");
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
