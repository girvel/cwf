#include <stdio.h>
#include <unistd.h>
#define CWF_IMPLEMENTATION
#include "../cwf.h"
#include "sqlite3.h" 

// SIDE QUESTS

// QUEST: chat-site
// - registering
//   - check for account already existing
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
        .code = CWF_CODE_200_OK,
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
        return (cwf_Response) { .code = CWF_CODE_500_INTERNAL_SERVER_ERROR };
    }

    return (cwf_Response) {
        .code = CWF_CODE_200_OK,
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

sqlite3 *db;
char *db_error = NULL;

#define SQL_FAIL(rc) do { \
    int SQL_FAIL_rc = (rc); \
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db)); \
    fprintf(stderr, "%s\n", sqlite3_errstr(SQL_FAIL_rc)); \
    return (cwf_Response) {.code = CWF_CODE_500_INTERNAL_SERVER_ERROR}; \
} while (0)

cwf_Response api_register(cwf_Request request)
{
    cwf_StringTable form_data = {0};
    if (!cwf_form_read(request.body, &form_data)) {
        return (cwf_Response) {.code = CWF_CODE_400_BAD_REQUEST};
    }

    const char *username = cwf_st_get(form_data, "username");
    if (username == NULL) return (cwf_Response) {.code = CWF_CODE_400_BAD_REQUEST};
    const char *password = cwf_st_get(form_data, "password");
    if (password == NULL) return (cwf_Response) {.code = CWF_CODE_400_BAD_REQUEST};

    // TODO init & reuse it
    sqlite3_stmt *existence_statement;
    int rc;

    rc = sqlite3_prepare_v2(
      db, "SELECT EXISTS(SELECT * FROM users WHERE user = ?)", -1,
      &existence_statement, NULL
    );
    if (rc != SQLITE_OK) SQL_FAIL(rc);
    
    rc = sqlite3_bind_text(existence_statement, 1, username, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) SQL_FAIL(rc);

    rc = sqlite3_step(existence_statement);
    if (rc != SQLITE_ROW) SQL_FAIL(rc);

    int user_already_exists = sqlite3_column_int(existence_statement, 0);
    sqlite3_finalize(existence_statement);
    if (user_already_exists) return (cwf_Response) {.code = CWF_CODE_409_CONFLICT};

    sqlite3_stmt *insert_statement;

    // TODO fix the leak on SQL_FAIL
    rc = sqlite3_prepare_v2(
        db, "INSERT INTO users (user, password) VALUES (?, ?)", -1,
        &insert_statement, NULL
    );
    if (rc != SQLITE_OK) SQL_FAIL(rc);

    rc = sqlite3_bind_text(insert_statement, 1, username, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) SQL_FAIL(rc);

    rc = sqlite3_bind_text(insert_statement, 2, password, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) SQL_FAIL(rc);

    rc = sqlite3_step(insert_statement);
    if (rc != SQLITE_DONE) SQL_FAIL(rc);

    return cwf_response_found("/");
}

int main()
{
    if (sqlite3_open("db", &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    printf("Opened SQLite DB.\n");

    char *error = NULL;
    if (sqlite3_exec(
        db,
        "CREATE TABLE IF NOT EXISTS users (\n"
        "  user TEXT NOT NULL PRIMARY KEY,\n"
        "  password TEXT NOT NULL\n"
        ");",
        NULL, NULL,
        &error
    ) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", error);
        sqlite3_close(db);
        return 1;
    }

    {
        sqlite3_stmt *statement;
        assert(sqlite3_prepare_v2(
            db, "SELECT user, password FROM users", -1,
            &statement, NULL
        ) == SQLITE_OK);

        while (1) {
            int step_rc = sqlite3_step(statement);
            if (step_rc == SQLITE_DONE) break;
            assert(step_rc == SQLITE_ROW);
            const unsigned char *username = sqlite3_column_text(statement, 0);
            const unsigned char *password = sqlite3_column_text(statement, 1);
            printf("username: |%s| password: |%s|\n", username, password);
        }

        assert(sqlite3_finalize(statement) == SQLITE_OK);
    }

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
