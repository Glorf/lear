#include "extras.h"
#include "stdlib.h"

#include <stdio.h>
#include <string.h>

s_string create_string(char *buf, unsigned long len) {
    s_string s;
    s.length = len+1;
    s.position = malloc(s.length);
    strcpy(s.position, buf);

    return s;
}

void delete_string(s_string s) {
    free(s.position);
}

s_string concat_string(s_string s1, s_string s2) {
    s_string s3;

    s3.length = s1.length + s2.length;
    s3.position = malloc(s3.length);
    strcpy(s3.position, s1.position);
    strcat(s3.position, s2.position);

    return s3;
}