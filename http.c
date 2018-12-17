#include "http.h"
#include "logger.h"
#include "config.h"
#include "cache.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static const char C_ENDLINE[] = "\r\n";
static const char C_GET[] = "GET";
static const char C_HEAD[] = "HEAD";
static const char C_OPTIONS[] = "OPTIONS";
static const char C_SPACE[] = " ";
static const char C_HEADER_STOPPER[] = ": ";

static const char C_HTTP_1_0[] = "HTTP/1.0";
static const char C_HTTP_1_1[] = "HTTP/1.1";

static const char header_OK[] = "200 OK";
static const char header_BAD_REQUEST[] = "400 Bad Request";
static const char header_NOT_FOUND[] = "404 Not Found";
static const char header_REQUEST_TIMEOUT[] = "408 Request Timeout";
static const char header_REQUEST_TOO_LARGE[] = "413 Request Entity Too Large";
static const char header_URI_TOO_LONG[] = "414 Request-URI Too Long";
static const char header_INTERNAL_ERROR[] = "500 Internal Server Error";
static const char header_NOT_IMPLEMENTED[] = "501 Not Implemented";
static const char header_HTTP_VERSION_NOT_SUPPORTED[] = "505 HTTP Version Not Supported";

s_http_request *parse_request(s_string *bareRequest) {
    s_http_request *request = malloc(sizeof(s_http_request));
    request->method = UNKNOWN;
    request->status = OK;
    request->headers = NULL;

    s_string line = substring(bareRequest, C_ENDLINE);

    long offset = 0;
    while(line.length > 0) {
        //attach the stopper
        line.length += 2;
        parse_request_line(&line, request);
        if(request->status != OK) break; //request failed anyway, no need to process it
        //detach the parsed head
        offset += line.length;
        line.position = bareRequest->position+offset;
        line.length = bareRequest->length - offset;

        line = substring(&line, C_ENDLINE);
    }

    return request;
}

void parse_request_line(s_string *bareLine, s_http_request *request) {
    s_string offset;
    offset.length = bareLine->length;
    offset.position = bareLine->position;
    if(request->method == UNKNOWN) { //no data yet, it's first line
        s_string type = substring(&offset, C_SPACE);
        if(type.length == 0) {
            request->status = BAD_REQUEST;
            return;
        }


        if(compare_string_const(&type, C_GET)) request->method = GET;
        else if(compare_string_const(&type, C_HEAD)) request->method = HEAD;
        else if(compare_string_const(&type, C_OPTIONS)) request->method = OPTIONS;
        else {
            message_log("Request method unsupported", INFO);
            string_log(&type, INFO);
            request->status = NOT_IMPLEMENTED;
            return;
        }

        offset.position += type.length + 1; //move to next part of line
        offset.length -= (type.length + 1);

        s_string resource = substring(&offset, C_SPACE);
        if(resource.length == 0) {
            message_log("Error while parsing request header", ERR);
            request->status = BAD_REQUEST;
            return;
        }

        long max_uri = read_config_long("maxURILength", "128");
        if(resource.length > max_uri) {
            message_log("Requested uri too long!", ERR);
            request->status = URI_TOO_LONG;
            return;
        }

        request->resource = create_string(resource.position, resource.length);

        offset.position += resource.length + 1;
        offset.length -= (resource.length + 1);

        s_string protocol = substring(&offset, C_ENDLINE);
        if(protocol.length == 0) {
            message_log("Bad protocol", DEBUG);
            request->status = BAD_REQUEST;
            return;
        }

        if(compare_string_const(&protocol, C_HTTP_1_1)) {
            request->status = OK;
            request->version = V1_1;
        }
        else if(compare_string_const(&protocol, C_HTTP_1_0)) {
            request->status = OK;
            request->version = V1_0;
        }
        else {
            message_log("HTTP version unsupported", DEBUG);
            request->status = HTTP_VERSION_NOT_SUPPORTED;
            return;
        }
    }
    else { //It's not status line, and we don't support request bodies so parse it as header (TODO: parse request bodies)
        s_string key = substring(&offset, C_HEADER_STOPPER);
        offset.position += key.length + 2;
        offset.length -= (key.length + 2);
        if(key.length == 0) {
            request->status = BAD_REQUEST;
            return;
        }

        s_string value = substring(&offset, C_ENDLINE);
        if(value.length == 0) {
            request->status = BAD_REQUEST;
            return;
        }

        s_string_list *last;

        if(request->headers == NULL) {
            request->headers = malloc(sizeof(s_string_list));
            last = request->headers;
        }
        else {
            for (last = request->headers; last != NULL && last->next != NULL; last = last->next); //go to last header
            request->headers->next = malloc(sizeof(s_string_list));
            last = request->headers->next;
        }

        last->next = NULL;
        last->key = create_string(key.position, key.length);
        last->value = create_string(value.position, value.length);
    }
}

int process_http_request(s_http_request *request, s_http_response *response) {

    s_string webdir = read_config_string("host.webDir", "/var/www");
    s_string nfdir = read_config_string("host.notFound", "/404.html");
    s_string index = create_string("/index.html", 11);

    response->version = request->version;

    if(request->status != OK) {
        //Sending error-like, body-less response
        response->status = request->status;
        response->body_length = 0;
        return 0;
    }


    if(request->method == GET || request->method == HEAD) {
        //TODO: handle multiple hostnames

        string_log(&request->resource, INFO);

        s_string resourceDir = concat_string(webdir, request->resource);


        if(is_directory(resourceDir)) { //if it's directory, look for index.html inside
            s_string indexDir = concat_string(resourceDir, index);

            delete_string(resourceDir);

            resourceDir = indexDir;
        }

        char *str_resource_dir = to_c_string(&resourceDir);
        if(access(str_resource_dir , F_OK ) == -1) { //File not exist
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
        free(str_resource_dir);

        string_log(&resourceDir, DEBUG);


        if(request->method == GET) {
            s_string page = read_file(resourceDir);
            response->body = page.position;
            response->body_length = (size_t) page.length;

            if (response->body_length == -1) {
                message_log(resourceDir.position, WARN);
                message_log("Error while reading file", ERR);
                response->status = INTERNAL_ERROR;
            }
        }

        delete_string(resourceDir);

    }
    else if(request->method == OPTIONS) {
        response->status = OK;
        /*
         * TODO: add response headers
         */
    }

    delete_string(webdir);
    delete_string(nfdir);
    delete_string(index);
    delete_string(request->resource);

    return 0;
}

s_string generate_bare_header(s_http_response *response) {
    s_string result;

    result.length = 0;
    result.position = NULL;

    const char *protocol = NULL;
    if(response->version == V1_0)
        protocol = C_HTTP_1_0;
    else if(response->version == V1_1)
        protocol = C_HTTP_1_1;

    switch(response->status) {
        case OK:
            forge_status_line(protocol, header_OK, response->body_length, &result);
            return result;
        case BAD_REQUEST:
            forge_status_line(protocol, header_BAD_REQUEST, response->body_length, &result);
            return result;
        case NOT_FOUND:
            forge_status_line(protocol, header_NOT_FOUND, response->body_length, &result);
            return result;
        case REQUEST_TIMEOUT:
            forge_status_line(protocol, header_REQUEST_TIMEOUT, response->body_length, &result);
            return result;
        case REQUEST_TOO_LARGE:
            forge_status_line(protocol, header_REQUEST_TOO_LARGE, response->body_length, &result);
            return result;
        case URI_TOO_LONG:
            forge_status_line(protocol, header_URI_TOO_LONG, response->body_length, &result);
            return result;
        case INTERNAL_ERROR:
            forge_status_line(protocol, header_INTERNAL_ERROR, response->body_length, &result);
            return result;
        case NOT_IMPLEMENTED:
        default:
            forge_status_line(protocol, header_NOT_IMPLEMENTED, response->body_length, &result);
            return result;
        case HTTP_VERSION_NOT_SUPPORTED:
            forge_status_line(protocol, header_HTTP_VERSION_NOT_SUPPORTED, response->body_length, &result);
            return result;
    }
}

void forge_status_line(const char protocol[], const char header[], unsigned long body_length, s_string *result) {
    result->length = (size_t) snprintf(NULL, 0, "%s %s\r\nContent-Length: %lu\r\n\r\n", protocol, header, body_length);
    result->position = malloc(result->length+1);
    snprintf(result->position, result->length + 1, "%s %s\r\nContent-Length: %lu\r\n\r\n", protocol, header, body_length);
}