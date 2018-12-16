#include "http.h"
#include "logger.h"
#include "config.h"
#include "cache.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

s_http_request *parse_request(s_string *bareRequest) {
    s_http_request *request = malloc(sizeof(s_http_request));
    request->method = UNKNOWN;
    request->status = OK;

    s_string stopper = create_string("\r\n", 2);
    s_string line = substring(bareRequest, &stopper);

    long offset = 0;
    while(line.length > 0) {
        //attach the stopper
        line.length += 2;
        parse_request_line(&line, request);

        //detach the parsed head
        offset += line.length+2;
        line.position = line.position+offset;
        line.length = bareRequest->length - offset;

        line = substring(&line, &stopper);
    }

    delete_string(stopper);

    return request;
}

void parse_request_line(s_string *bareLine, s_http_request *request) {
    s_string stopper = create_string(" ", 1);
    s_string endline = create_string("\r\n", 2);
    s_string get = create_string("GET", 3);
    s_string head = create_string("HEAD", 4);
    s_string options = create_string("OPTIONS", 7);
    s_string http11 = create_string("HTTP/1.1", 8);

    s_string offset;
    offset.length = bareLine->length;
    offset.position = bareLine->position;
    if(request->method == UNKNOWN) { //no data yet, it's first line
        s_string type = substring(&offset, &stopper);
        if(type.length == 0) {
            request->status = BAD_REQUEST;
            return;
        }

        if(compare_string(&type, &get)) request->method = GET;
        else if(compare_string(&type, &head)) request->method = HEAD;
        else if(compare_string(&type, &options)) request->method = OPTIONS;
        else {
            message_log("Request method unsupported", INFO);
            string_log(&type, INFO);
            request->status = NOT_IMPLEMENTED;
            return;
        }

        offset.position += type.length + 1; //move to next part of line
        offset.length -= (type.length + 1);


        s_string resource = substring(&offset, &stopper);
        if(resource.length == 0) {
            message_log("Error while parsing request header", ERR);
            request->status = BAD_REQUEST;
            return;
        }

        request->resource = create_string(resource.position, resource.length);

        offset.position += resource.length + 1;
        offset.length -= (resource.length + 1);

        s_string protocol = substring(&offset, &endline);
        if(protocol.length == 0) {
            message_log("Bad protocol", DEBUG);
            request->status = BAD_REQUEST;
            return;
        }
        if(compare_string(&protocol, &http11)) {
            message_log("Requested resource: ", DEBUG);
            string_log(&request->resource, DEBUG);
            request->status = OK;
        }
        else {
            message_log("HTTP version unsupported", DEBUG);
            request->status = HTTP_VERSION_NOT_SUPPORTED;
            return;
        }
    }
    else {
        /*
         * TODO: Parse request headers here
        */
    }
}

int process_http_request(s_http_request *request, s_http_response *response) {

    s_string webdir = read_config_string("host.webDir", "/var/www");
    s_string nfdir = read_config_string("host.notFound", "/404.html");
    s_string index = create_string("/index.html", 11);

    if(request->status != OK) {
        message_log("REQUEST FAILED! SHOULD RETURN NON-200 RESPONSE!", WARN);
        return  -1;
    }


    if(request->method == GET) {
        //TODO: handle multiple hostnames


        string_log(&request->resource, INFO);

        s_string resourceDir = concat_string(webdir, request->resource);


        if(is_directory(resourceDir)) { //if it's directory, look for index.html inside
            s_string indexDir = concat_string(resourceDir, index);

            delete_string(resourceDir);

            resourceDir = indexDir;
        }

        char *str_resource_dir = to_c_string(&resourceDir);
        message_log("Trying file:", INFO);
        message_log(str_resource_dir, INFO);
        if(access(str_resource_dir , F_OK ) == -1) { //File not exist
            free(str_resource_dir);
            delete_string(resourceDir);

            resourceDir = concat_string(webdir, nfdir);

            message_log("Not found!", WARN);
            string_log(&resourceDir, WARN);

            response->status = NOT_FOUND;
        }
        else
        {
            response->status = OK;
        }

        string_log(&resourceDir, DEBUG);



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