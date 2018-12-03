#ifndef PUTHTTPD_HTTP_H
#define PUTHTTPD_HTTP_H

typedef enum {
    OPTIONS,
    GET,
    HEAD
} e_http_methods;

typedef struct {
    e_http_methods method;
    char hostname[20];

} s_http_request;

typedef struct  {

} s_http_response;

int parse_request_line(char *bareLine, int lineSize, s_http_request *request);
int process_http_request(s_http_request *request, s_http_response *response);
int generate_bare_response(s_http_response *response, char *bareResponse, int *responseSize);

#endif //PUTHTTPD_HTTP_H
