#include "http.h"
#include "logger.h"
#include "config.h"
#include "cache.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static const char *C_ENDLINE = "\r\n";
static const char *C_GET = "GET";
static const char *C_HEAD = "HEAD";
static const char *C_OPTIONS = "OPTIONS";
static const char *C_SPACE = " ";
static const char *C_HEADER_STOPPER = ":";

static const char *C_HTTP[2] = {"HTTP/1.0", "HTTP/1.1"};

static const char *C_INDEX = "/index.html";

static const char *header_OK = "200 OK";
static const char *header_BAD_REQUEST = "400 Bad Request";
static const char *header_NOT_FOUND = "404 Not Found";
static const char *header_REQUEST_TIMEOUT = "408 Request Timeout";
static const char *header_REQUEST_TOO_LARGE = "413 Request Entity Too Large";
static const char *header_URI_TOO_LONG = "414 Request-URI Too Long";
static const char *header_INTERNAL_ERROR = "500 Internal Server Error";
static const char *header_NOT_IMPLEMENTED = "501 Not Implemented";
static const char *header_HTTP_VERSION_NOT_SUPPORTED = "505 HTTP Version Not Supported";

s_http_request *parse_request(s_string *bareRequest) {
    s_http_request *request = malloc(sizeof(s_http_request));
    request->method = UNKNOWN;
    request->status = OK;
    request->headers_first = NULL;
    request->headers_last = NULL;
    request->next = NULL;
    request->hostname.position = NULL;
    request->resource.position = NULL;

    s_string line = substring(bareRequest, C_ENDLINE);

    long offset = 0;
    while(line.position != NULL) {
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
        if(type.position == NULL) {
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
        if(resource.position == NULL) {
            message_log("Error while parsing request header", ERR);
            request->status = BAD_REQUEST;
            return;
        }

        long max_uri = get_global_config()->max_URI_length;
        if(resource.length > max_uri) {
            message_log("Requested uri too long!", ERR);
            request->status = URI_TOO_LONG;
            return;
        }

        //resource path traversal security check
        //TODO: Perform full path traversal security check!!
        request->resource = create_string(resource.position, resource.length);
        if(request->resource.length == 0) {
            request->status = BAD_REQUEST;
            return;
        }
        else {
            unsigned long tmp_offset = 0;
            s_string tmp_str = substring(&resource, "/");
            while(tmp_str.position != NULL) {

                if(tmp_str.length == 2 && compare_string_const(&tmp_str, "..")) { //path traversal!
                    message_log("Possible path traversal!", WARN);
                    request->status = BAD_REQUEST;
                    break;
                }
                else if(tmp_str.length == 1 && compare_string_const(&tmp_str, ".")) { //suspicious current directory access?
                    message_log("Possible path traversal!", WARN);
                    request->status = BAD_REQUEST;
                    break;
                }

                tmp_offset += tmp_str.length + 1;

                tmp_str.position = resource.position + tmp_offset;
                tmp_str.length = resource.length - tmp_offset;

                tmp_str = substring(&tmp_str, "/");
            }
            if(tmp_str.length > 0) { //there are no more slashes, but some characters were not verified yet
                tmp_str.position = resource.position + tmp_offset;

                //TODO: Duplicate path traversal detection! Treat it in one method somehow
                if(tmp_str.length == 2 && compare_string_const(&tmp_str, "..")) { //path traversal!
                    message_log("Possible path traversal!", WARN);
                    request->status = BAD_REQUEST;
                }
                else if(tmp_str.length == 1 && compare_string_const(&tmp_str, ".")) { //suspicious current directory access?
                    message_log("Possible path traversal!", WARN);
                    request->status = BAD_REQUEST;
                }
            }
        }


        offset.position += resource.length + 1;
        offset.length -= (resource.length + 1);

        s_string protocol = substring(&offset, C_ENDLINE);
        if(protocol.position == NULL) {
            message_log("Bad protocol", DEBUG);
            request->status = BAD_REQUEST;
            return;
        }

        if(compare_string_const(&protocol, C_HTTP[1])) {
            request->version = V1_1;
        }
        else if(compare_string_const(&protocol, C_HTTP[0])) {
            request->version = V1_0;
        }
        else {
            message_log("HTTP version unsupported", DEBUG);
            request->status = HTTP_VERSION_NOT_SUPPORTED;
        }
    }
    else { //headers parsing
        s_string key = substring(&offset, C_HEADER_STOPPER);
        offset.position += key.length + 1;
        offset.length -= (key.length + 1);
        if (key.position == NULL) {
            request->status = BAD_REQUEST;
            return;
        }

        //Let's parse spaces if they are after header name
        while (offset.position[0] == ' ') {
            offset.position++;
            offset.length--;
        }

        s_string value = substring(&offset, C_ENDLINE);
        if (value.position == NULL) {
            request->status = BAD_REQUEST;
            return;
        }

        //TODO: delegate duplicate to function
        if(request->headers_first == NULL) {
            request->headers_first = malloc(sizeof(s_string_list));
            request->headers_last = request->headers_first;
        }
        else {
            request->headers_last->next = malloc(sizeof(s_string_list));
            request->headers_last = request->headers_last->next;
        }

        request->headers_last->next = NULL;
        request->headers_last->key = create_string(key.position, key.length);
        request->headers_last->value = create_string(value.position, value.length);
    }
}

int process_http_request(s_http_request *request, s_http_response *response) {

    response->version = request->version;

    if(request->status != OK) {
        //Sending error-like, body-less response
        response->status = request->status;
        response->body_length = 0;
    }
    else if(request->method == OPTIONS && compare_string_const(&request->resource, "*")) //it's not a file but OPTIONS query
        response->status = OK;
    else {
        string_log(&request->resource, INFO);

        //TODO: Read it from map & add multiple hosts

        s_string resource_dir = concat_string(get_global_config()->host.root_path, request->resource);


        if (is_directory(resource_dir)) { //if it's directory, look for index.html inside
            s_string indexDir = concat_string_const(resource_dir, C_INDEX);

            delete_string(&resource_dir);

            resource_dir = indexDir;
        }

        char *str_resource_dir = to_c_string(&resource_dir);
        if (access(str_resource_dir, F_OK) == -1) { //File not exist
            delete_string(&resource_dir);

            resource_dir = concat_string(get_global_config()->host.root_path, get_global_config()->host.not_found_path);

            message_log("Not found!", WARN);
            string_log(&resource_dir, WARN);

            response->status = NOT_FOUND;
        } else {
            response->status = OK;
        }
        free(str_resource_dir);

        string_log(&resource_dir, DEBUG);


        if(request->method == GET || request->method == HEAD) { //we don't have to retrieve resource for OPTIONS
            s_string page = read_file(resource_dir);
            response->body = page.position;
            response->body_length = (size_t) page.length;
        }

        if (response->body_length == -1) {
            message_log(resource_dir.position, WARN);
            message_log("Error while reading file", ERR);
            response->status = INTERNAL_ERROR;
        }

        delete_string(&resource_dir);
    }

    //Process basic headers
    for(s_string_list *header = request->headers_first; header != NULL; ) {
        string_log(&header->value, DEBUG);
        if(request->version==V1_0 && compare_string_const(&header->key, "Connection") && compare_string_const(&header->value, "Keep-Alive")) { //send keepalive
            //TODO: Duplicate - pass this and the one below to function
            if(response->headers_first == NULL) {
                response->headers_first = malloc(sizeof(s_string_list));
                response->headers_last = response->headers_first;
            }
            else {
                response->headers_last->next = malloc(sizeof(s_string_list));
                response->headers_last = response->headers_last->next;
            }
            //We transmit header as-is
            response->headers_last->value = create_string(header->value.position, header->value.length);
            response->headers_last->key = create_string(header->key.position, header->key.length);
            response->headers_last->next = NULL;
            message_log("Keepalive processed", DEBUG);
        }

        header = header->next;
    }

    if(request->method == OPTIONS && (response->status==OK || compare_string_const(&request->resource, "*"))) { //Global options query, return default
        //TODO: duplicate!
        if(response->headers_first == NULL) {
            response->headers_first = malloc(sizeof(s_string_list));
            response->headers_last = response->headers_first;
        }
        else {
            response->headers_last->next = malloc(sizeof(s_string_list));
            response->headers_last = response->headers_last->next;
        }

        response->headers_last->key = create_string("Allow", 14);
        response->headers_last->value = create_string("GET, HEAD, OPTIONS", 18);
        string_log(&response->headers_last->key, DEBUG);
    }

    //Message has body - we should transmit its length
    if(response->headers_first == NULL) {
        response->headers_first = malloc(sizeof(s_string_list));
        response->headers_last = response->headers_first;
    }
    else {
        response->headers_last->next = malloc(sizeof(s_string_list));
        response->headers_last = response->headers_last->next;
    }

    response->headers_last->key = create_string("Content-Length", 14);
    char number[11]; //to surely fit long
    sprintf(number, "%lu", response->body_length);
    response->headers_last->value = create_string(number, strlen(number));
    response->headers_last->next = NULL;
    message_log("ContentLength added", DEBUG);
    string_log(&response->headers_last->key, DEBUG);

    return 0;
}

s_string generate_bare_header(s_http_response *response) {
    s_string result;

    result.length = 0;
    result.position = NULL;

    const char *protocol = C_HTTP[response->version];

    switch(response->status) {
        case OK:
            forge_status_line(protocol, header_OK, response->headers_first, &result);
            return result;
        case BAD_REQUEST:
            forge_status_line(protocol, header_BAD_REQUEST, response->headers_first, &result);
            return result;
        case NOT_FOUND:
            forge_status_line(protocol, header_NOT_FOUND, response->headers_first, &result);
            return result;
        case REQUEST_TIMEOUT:
            forge_status_line(protocol, header_REQUEST_TIMEOUT, response->headers_first, &result);
            return result;
        case REQUEST_TOO_LARGE:
            forge_status_line(protocol, header_REQUEST_TOO_LARGE, response->headers_first, &result);
            return result;
        case URI_TOO_LONG:
            forge_status_line(protocol, header_URI_TOO_LONG, response->headers_first, &result);
            return result;
        case INTERNAL_ERROR:
            forge_status_line(protocol, header_INTERNAL_ERROR, response->headers_first, &result);
            return result;
        case NOT_IMPLEMENTED:
        default:
            forge_status_line(protocol, header_NOT_IMPLEMENTED, response->headers_first, &result);
            return result;
        case HTTP_VERSION_NOT_SUPPORTED:
            forge_status_line(protocol, header_HTTP_VERSION_NOT_SUPPORTED, response->headers_first, &result);
            return result;
    }
}

void forge_status_line(const char protocol[], const char status[], s_string_list *headers, s_string *result) {
    //TODO: consider hardcoded request header size so we won't have to count it as below
    unsigned long stat_len = strlen(status);
    result->length = 8 /*HTTP/1.[0,1]*/ + 1 /* */ + stat_len /*200 OK*/ + 2 /*\r\n*/;
    for(s_string_list *header = headers; header != NULL; ) {
        result->length += header->key.length + 2 + header->value.length + 2;

        header = header->next;
    }
    result->length += 2; //\r\n

    result->position = malloc(result->length);
    memcpy(result->position, protocol, 8); //TODO: move protocol to s_string to avoid these hardcodings!
    memcpy(result->position+8, " ", 1);
    memcpy(result->position+9, status, stat_len); //TODO: move status to s_string to avoid these hardcodings!
    memcpy(result->position+9+stat_len, "\r\n", 2);
    unsigned long offset = 11 + stat_len;
    for(s_string_list *header = headers; header != NULL; ) {
        memcpy(result->position + offset, header->key.position, header->key.length);
        offset += header->key.length;
        memcpy(result->position + offset, ": ", 2);
        offset += 2;
        memcpy(result->position + offset, header->value.position, header->value.length);
        offset += header->value.length;
        string_log(&header->value, DEBUG);
        memcpy(result->position + offset, "\r\n", 2);
        offset += 2;

        header = header->next;
    }

    memcpy(result->position + offset, "\r\n", 2); //end response header

    string_log(result, DEBUG);
}

void delete_request(s_http_request *request) {
    clear_string_list(request->headers_first);
    delete_string(&request->hostname);
    delete_string(&request->resource);
    free(request);
}