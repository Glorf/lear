#include "types.h"
#include "stdlib.h"
#include "logger.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

s_string create_string(char *buf, unsigned long len) {
    s_string s;
    s.length = len;
    s.position = malloc(s.length);
    memcpy(s.position, buf, len);

    return s;
}

void delete_string(s_string *s) {
    if(s->position == NULL)
        message_log("Tried to free null string!", WARN);
    else
        free(s->position);

    s->length = 0;
}

s_string concat_string(s_string s1, s_string s2) {
    s_string s3;

    s3.length = s1.length + s2.length;
    s3.position = malloc(s3.length);
    memcpy(s3.position, s1.position, s1.length);
    memcpy(s3.position+s1.length, s2.position, s2.length);

    return s3;
}

s_string concat_string_const(s_string str, const char *con) {
    s_string s3;

    unsigned long len = strlen(con);
    s3.length = str.length + len;
    s3.position = malloc(s3.length);
    memcpy(s3.position, str.position, str.length);
    memcpy(s3.position+str.length, con, len);

    return s3;
}

s_string substring(s_string *haystack, const char *needle) { //find index of first occurence, NULL if not found
    s_string sub;
    sub.length = 0;
    sub.position = NULL;

    long needle_len = strlen(needle);
    if(haystack->length<needle_len) return sub;

    for(unsigned long i=0; i<=haystack->length-needle_len; i++) {

        for(unsigned long j=0; j<needle_len+1; j++) {
            if(j == needle_len) {
                sub.length = i;
                sub.position = haystack->position;
                return sub;
            }
            if(haystack->position[i+j] == needle[j]) continue;
            else break;
        }
    }

    sub.length = haystack->length;
    return sub;
}

long compare_string(s_string *str1, s_string *str2) { //return 1 if equal, 0 otherwise
    if(str1->length != str2->length) return 0;
    for(long i = 0; i<str1->length; i++) {
        if(str1->position[i] == str2->position[i]) continue;
        return 0;
    }
    return 1;
}

long compare_string_const(s_string *str, const char con[]) {
    if(strlen(con) != str->length) return 0;
    for(long i = 0; i<str->length; i++) {
        if(str->position[i] == con[i]) continue;
        return 0;
    }
    return 1;
}

char *to_c_string(s_string *str) { //remember to free afterwards!
    char *new = malloc(str->length + 1);
    memcpy(new, str->position, str->length);
    new[str->length] = '\0';

    return new;
}

void clear_string_list(s_string_list *first) {

    for(s_string_list *current = first; current!=NULL; ) {
        s_string_list *prev = current;
        current = prev->next;
        delete_string(&prev->key);
        delete_string(&prev->value);
        free(prev);
    }

}

s_buffer initialize_buffer() {
    s_buffer buffer;
    buffer.size = 0;
    buffer.offset = 0;
    buffer.payload = NULL;
    return buffer;
}

int expand_buffer(s_buffer *buffer, long howMuch) {
    buffer->size += howMuch;
    if(howMuch > 0) { //expand
        if (buffer->size == howMuch) //Was inexistent
            buffer->payload = malloc((size_t) howMuch);
        else {
            buffer->payload = realloc(buffer->payload, (size_t) buffer->size);
        }
    }
    else { //shrink
        if(buffer->size <= 0) { //free the empty buffer
            free(buffer->payload);
            buffer->size = 0;
            buffer->offset = 0;
        }
        else {
            buffer->payload = realloc(buffer->payload, buffer->size);
        }
    }
    return 0;
}

void clean_buffer(s_buffer *buffer) {
    if(buffer->size > 0) free(buffer->payload);
    else {
        message_log("Tried to free empty buffer!", WARN);
    }
}

