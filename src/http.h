#ifndef PUTHTTPD_HTTP_H
#define PUTHTTPD_HTTP_H

#include "types.h"

typedef enum {
    OPTIONS,
    GET,
    HEAD,
    UNKNOWN,
} e_http_methods;

typedef enum {
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    REQUEST_TIMEOUT = 408, //TODO: handle it somehow
    REQUEST_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    INTERNAL_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    HTTP_VERSION_NOT_SUPPORTED = 505
} e_http_status;

typedef enum {
    V1_0 = 0,
    V1_1 = 1
} e_http_version;

typedef struct {
    e_http_methods method;
    e_http_version version;
    s_string hostname;
    s_string_list *headers_first;
    s_string_list *headers_last; //we have both pointers so that we won't have to iterate while parsing
    s_string resource;
    e_http_status status;
    void *next;
} s_http_request;

typedef struct  {
    e_http_status status;
    e_http_version version;
    s_string_list *headers_first;
    s_string_list *headers_last; //we have both pointers so that we won't have to iterate while parsing
    unsigned long body_length;
    char *body;
} s_http_response;

s_http_request *parse_request(s_string *bareRequest);
void parse_request_line(s_string *bareLine, s_http_request *request);
int process_http_request(s_http_request *request, s_http_response *response);
s_string generate_bare_header(s_http_response *response);
void forge_status_line(const char protocol[], const char status[], s_string_list *headers, s_string *result);
void delete_request(s_http_request *request);

#endif //PUTHTTPD_HTTP_H
