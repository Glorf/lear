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
        if(lineSize >3 && bareLine[0] == 'G' && bareLine[1] == 'E' && bareLine[2] == 'T') {
            request->method = GET;
            offset+=3;
        }
        else if(lineSize > 4 && bareLine[0] == 'H' && bareLine[1] == 'E' && bareLine[2] == 'A' && bareLine[3] == 'D') {
            request->method = HEAD;
            offset+=4;
        }
        else if(lineSize > 7 && bareLine[0] == 'O' && bareLine[1] == 'P' && bareLine[2] == 'T' && bareLine[3] == 'I' &&
                bareLine[4] == 'O' && bareLine[5] == 'N' && bareLine[6] == 'S') {
            request->method = OPTIONS;
            offset+=7;
        }
        else //Unknown, unhandled request, TODO: throw 501
            return -1;

        if(bareLine[offset] == ' ') offset++;
        else return -1; //omit space and handle resource name, TODO: throw 501

        for(int i = 0; i<lineSize - offset; i++) { //copy resource name
            if(bareLine[i+offset] == ' ') {
                request->resource[i] = '\0';
                offset+=(i+1);
                break;
            }

            request->resource[i] = bareLine[i+offset];
        }

        if(lineSize-offset >= 8 && bareLine[offset] == 'H' && bareLine[offset+1] == 'T' && bareLine[offset+2] == 'T' &&
            bareLine[offset+3] == 'P' && bareLine[offset+4] == '/' && bareLine[offset+5] == '1' &&
            bareLine[offset+6] == '.' && bareLine[offset+7] == '1') {
            message_log("Requested resource: ", DEBUG);
            message_log(request->resource, DEBUG);
            return 0;
        }
        else {
            return -1; //TODO: throw unsupported - 505
        }
    }

    /*
     * TODO: Parse request here
     */

    return 0;
}

int process_http_request(s_http_request *request, s_http_response *response) {

    const char *webdir = read_config_string("host.webDir", "/var/www");


    if(request->method == GET) {
        //TODO: handle multiple hostnames
        char resourceDir[256];

        sprintf(resourceDir, "%s%s", webdir, request->resource);

        if(is_directory(resourceDir)) { //if it's directory, look for index.html inside
            strcat(resourceDir, "/index.html");
        }

        message_log(resourceDir, INFO);


        if(access(resourceDir , F_OK ) == -1) { //File not exist
            sprintf(resourceDir, "%s/%s", webdir, read_config_string("host.notFound", "404.html"));
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
    }

    /*
     * TODO: handle other requests
     */

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