#ifndef PUTHTTPD_TYPES_H
#define PUTHTTPD_TYPES_H

typedef struct {
    unsigned long length;
    char *position;
} s_string;

s_string create_string(char *buf, unsigned long len);
void delete_string(s_string s);
s_string concat_string(s_string s1, s_string s2);
s_string substring(s_string *needle, s_string *haystack);
long compare_string(s_string *str1, s_string *str2);
char *to_c_string(s_string *str);

typedef struct {
    char *payload;
    unsigned long size;
    unsigned long offset;
} s_buffer;

s_buffer initialize_buffer();
int expand_buffer(s_buffer *buffer, long howMuch);
void clean_buffer(s_buffer *buffer);

#endif //PUTHTTPD_TYPES_H
