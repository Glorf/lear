#ifndef PUTHTTPD_TYPES_H
#define PUTHTTPD_TYPES_H

typedef struct {
    unsigned long length;
    char *position;
} s_string;

typedef struct s_string_list s_string_list;

struct s_string_list {
    s_string key;
    s_string value;
    s_string_list *next;
};

s_string create_string(char *buf, unsigned long len);
void delete_string(s_string *s);
s_string concat_string(s_string s1, s_string s2);
s_string concat_string_const(s_string str, const char *con);
s_string substring(s_string *haystack, const char *needle);
long compare_string(s_string *str1, s_string *str2);
long compare_string_const(s_string *str, const char *con);
char *to_c_string(s_string *str);
void clear_string_list(s_string_list *first);

typedef struct {
    char *payload;
    unsigned long size;
    unsigned long offset;
} s_buffer;

s_buffer initialize_buffer();
int expand_buffer(s_buffer *buffer, long howMuch);
void clean_buffer(s_buffer *buffer);

#endif //PUTHTTPD_TYPES_H
