#include <stdio.h>
#define CWF_IMPLEMENTATION
#include "cwf.h"

cwf_Response api_get_hw()
{
    return (cwf_Response) {
      .code = 200,
      .body = "Hello, world!\r\n",
    };
}

int main()
{
    cwf_App app = {0};
    cwf_app_route(&app, "/api/get", api_get_hw);
    cwf_app_run(app, "localhost", 8000);
    printf("Program finished.\n");
    return 0;
}
