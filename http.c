#include "http.h"
#include "logger.h"
#include "config.h"
#include "cache.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int parse_request_line(char *bareLine, int lineSize, s_http_request *request) {
    int offset = 0;
    if(request->method == UNKNOWN) { //no data yet, it's first line
        char *type = strtok(bareLine, " ");
        if(type == NULL) return BAD_REQUEST;

        if(strcmp(type, "GET") == 0) request->method = GET;
        else if(strcmp(type, "HEAD") == 0) request->method = HEAD;
        else if(strcmp(type, "OPTIONS") == 0) request->method = OPTIONS;
        else return NOT_IMPLEMENTED;

        char *resource = strtok(NULL, " ");
        if(resource == NULL) {
            message_log("Error while parsing request header", ERR);
            return BAD_REQUEST;
        }
        request->resource = malloc(sizeof(resource));
        strcpy(request->resource, resource);

        char *protocol = strtok(NULL, " ");
        if(protocol == NULL) return BAD_REQUEST;
        if(strcmp(protocol, "HTTP/1.1") == 0) {
            message_log("Requested resource: ", DEBUG);
            message_log(request->resource, DEBUG);
            return OK;
        }
        else {
            return HTTP_VERSION_NOT_SUPPORTED;
        }
    }
    else {

    }

    /*
     * TODO: Parse request headers here
     */

    return 0;
}

int process_http_request(s_http_request *request, s_http_response *response) {

    const char *webdir = read_config_string("host.webDir", "/var/www");
    const char *nfdir = read_config_string("host.notFound", "404.html");

    int larger = sizeof(request->resource) > sizeof(nfdir) ? sizeof(request->resource): sizeof(nfdir);

    if(request->method == GET) {
        //TODO: handle multiple hostnames
        char *resourceDir = malloc(larger + sizeof(webdir));
        sprintf(resourceDir, "%s%s", webdir, request->resource);

        if(is_directory(resourceDir)) { //if it's directory, look for index.html inside
            strcat(resourceDir, "/index.html");
        }

        message_log(resourceDir, INFO);


        if(access(resourceDir , F_OK ) == -1) { //File not exist
            sprintf(resourceDir, "%s%s", webdir, nfdir);
            response->status = NOT_FOUND;
        }
        else
        {
            response->status = OK;
        }



        s_string page = read_file(resourceDir);
        response->body = page.position;
        response->body_length = (size_t)page.length;

        if(response->body_length == -1) {
            message_log(resourceDir, WARN);
            message_log("Error while reading file", ERR);
            response->status = INTERNAL_ERROR;
        }

        free(resourceDir);
    }

    /*
     * TODO: handle other requests
     */

    free(request->resource);

    return 0;
}

s_string generate_bare_header(s_http_response *response) {
    static const char header_OK[] = "HTTP/1.1 200 OK";
    static const char header_NOT_FOUND[] = "HTTP/1.1 404 Not Found";
    static const char header_INTERNAL_ERROR[] = "HTTP/1.1 500 Internal Server Error";

    /*
     * TODO: Add other responses
     */

    s_string result;

    result.length = 0;
    result.position = NULL;

    switch(response->status) {
        case OK:
            result.length = (size_t)snprintf(NULL, 0, "%s\r\nContent-Length: %lu\r\n\r\n", header_OK, response->body_length);
            result.position = malloc(result.length);
            sprintf(result.position, "%s\r\nContent-Length: %lu\r\n\r\n", header_OK, response->body_length);
            return result;
        case NOT_FOUND:
            result.length = (size_t)snprintf(NULL, 0, "%s\r\nContent-Length: %lu\r\n\r\n", header_NOT_FOUND, response->body_length);
            result.position = malloc(result.length);
            sprintf(result.position, "%s\r\nContent-Length: %lu\r\n\r\n", header_NOT_FOUND, response->body_length);
            return result;
        case INTERNAL_ERROR:
            result.length = (size_t)snprintf(NULL, 0, "%s\r\n\r\n", header_INTERNAL_ERROR);
            result.position= malloc(result.length);
            sprintf(result.position, "%s\r\n\r\n", header_NOT_FOUND);
            return result;
    }

    return result;
}