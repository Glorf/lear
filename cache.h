#ifndef PUTHTTPD_CACHE_H
#define PUTHTTPD_CACHE_H

#include "types.h"

s_string read_file(char *filename);
int is_directory(const char *path);

#endif //PUTHTTPD_CACHE_H
