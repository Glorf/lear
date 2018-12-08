#ifndef PUTHTTPD_HTTP_H
#define PUTHTTPD_HTTP_H

#include <glob.h>

typedef enum {
    OPTIONS,
    GET,
    HEAD,
    UNKNOWN
} e_http_methods;

typedef enum {
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    REQUEST_TIMEOUT = 408,
    URI_TOO_LONG = 414,
    INTERNAL_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    HTTP_VERSION_NOT_SUPPORTED = 505
} e_http_status;

typedef struct {
    e_http_methods method;
    char hostname[128];
    char parameters[128][64];
    char resource[128];
} s_http_request;

typedef struct  {
    e_http_status status;
    size_t body_length;
    char body[8192];
} s_http_response;

int parse_request_line(char *bareLine, int lineSize, s_http_request *request);
int process_http_request(s_http_request *request, s_http_response *response);
size_t generate_bare_response(s_http_response *response, char *bareResponse);

#endif //PUTHTTPD_HTTP_H
