#include "http.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>

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
    /*
     * TODO: Process request and generate response here
     */

    static const char body[] =
            "<html>\n"
            "<head>\n"
            "<title>Test</title>\n"
            "</head>\n"
            "<body>\n"
            "Hello World!\n"
            "</body>\n"
            "</html>";

    response->status = OK;
    strcpy(response->body, body);
    response->body_length = strlen(body);

    return 0;
}

size_t generate_bare_response(s_http_response *response, char *bareResponse) {
    /*
     * TODO: Serialize response and return char table here
     */
    static const char header_OK[] = "HTTP/1.1 200 OK";
    static const char header_NOT_FOUND[] = "HTTP/1.1 404 Not Found";


    if(response->status == OK)
        sprintf(bareResponse, "%s\r\nContent-Length: %d\r\n\r\n%s", header_OK, (int)response->body_length, response->body);
    else if(response->status == NOT_FOUND)
        sprintf(bareResponse, "%s\r\nContent-Length: %d\r\n\r\n%s", header_NOT_FOUND, (int)response->body_length, response->body);

    return strlen(bareResponse);
}