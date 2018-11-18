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

void parse_request(char *bareRequest, s_http_request *request);

#endif //PUTHTTPD_HTTP_H
