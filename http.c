#include "http.h"
#include "logger.h"

int parse_request_line(char *bareLine, int lineSize, s_http_request *request) {
    bareLine[lineSize]='\0';
    message_log(bareLine, INFO);
    return 0;

    /*
     * TODO: Parse request here
     */
}

int process_http_request(s_http_request *request, s_http_response *response) {
    /*
     * TODO: Process request and generate response here
     */

    return 0;
}

int generate_bare_response(s_http_response *response, char *bareResponse, int *responseSize) {
    /*
     * TODO: Serialize response and return char table here
     */

    return 0;
}