#ifndef PUTHTTPD_TYPES_H
#define PUTHTTPD_TYPES_H

typedef struct {
    unsigned long length;
    char *position;
} s_string;

s_string create_string(char *buf, unsigned long len);

void delete_string(s_string s);

s_string concat_string(s_string s1, s_string s2);

#endif //PUTHTTPD_TYPES_H
