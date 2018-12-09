#include "http.h"
#include "logger.h"
#include "config.h"
#include "cache.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int parse_request_line(char *bareLine, int lineSize, s_http_request *request) {
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

        request->resource = create_string(resource, strlen(resource));

        char *protocol = strtok(NULL, " ");
        if(protocol == NULL) return BAD_REQUEST;
        if(strcmp(protocol, "HTTP/1.1") == 0) {
            message_log("Requested resource: ", DEBUG);
            message_log(request->resource.position, DEBUG);
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

    return OK;
}

int process_http_request(s_http_request *request, s_http_response *response) {

    s_string webdir = read_config_string("host.webDir", "/var/www");
    s_string nfdir = read_config_string("host.notFound", "/404.html");
    s_string index = create_string("/index.html", 11);

    if(request->method == GET) {
        //TODO: handle multiple hostnames


        s_string resourceDir = concat_string(webdir, request->resource);

        if(is_directory(resourceDir)) { //if it's directory, look for index.html inside
            s_string indexDir = concat_string(resourceDir, index);

            delete_string(resourceDir);

            resourceDir = indexDir;
        }

        if(access(resourceDir.position , F_OK ) == -1) { //File not exist
            delete_string(resourceDir);

            resourceDir = concat_string(webdir, nfdir);

            message_log("Not found!", WARN);
            message_log(resourceDir.position, WARN);

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
            message_log(resourceDir.position, WARN);
            message_log("Error while reading file", ERR);
            response->status = INTERNAL_ERROR;
        }

    }

    /*
     * TODO: handle other requests
     */


    delete_string(webdir);
    delete_string(nfdir);
    delete_string(index);
    delete_string(request->resource);

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
            result.position = malloc(result.length+1);
            snprintf(result.position, result.length+1, "%s\r\nContent-Length: %lu\r\n\r\n", header_OK, response->body_length);
            return result;
        case NOT_FOUND:
            result.length = (size_t)snprintf(NULL, 0, "%s\r\nContent-Length: %lu\r\n\r\n", header_NOT_FOUND, response->body_length);
            result.position = malloc(result.length+1);
            snprintf(result.position, result.length+1, "%s\r\nContent-Length: %lu\r\n\r\n", header_NOT_FOUND, response->body_length);
            return result;
        case INTERNAL_ERROR:
        default:
            result.length = (size_t)snprintf(NULL, 0, "%s\r\n\r\n", header_INTERNAL_ERROR);
            result.position= malloc(result.length+1);
            snprintf(result.position, result.length+1, "%s\r\n\r\n", header_INTERNAL_ERROR);
            return result;
    }
}

void free_request(s_http_request *request) {
    delete_string(request->resource);
}

void free_response(s_http_response *response) {
    //NOTHING YET
}