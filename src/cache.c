#include "cache.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


s_string read_file(s_string filename) {
    s_string filecontent;
    filecontent.length = 0;
    filecontent.position = NULL;

    char *str_filename = to_c_string(&filename);

    int fd = open(str_filename, O_RDONLY);
    free(str_filename);

    if(fd < 0) {
        message_log("Failed to open file", ERR);
        return filecontent;
    }

    struct stat s;
    fstat(fd, &s);
    filecontent.length = (size_t)s.st_size;

    filecontent.position = mmap(NULL, filecontent.length, PROT_READ, MAP_SHARED, fd, 0);

    if(filecontent.position < 0) {
        message_log("Error while mapping file", ERR);
    }

    close(fd);

    return filecontent;
}

int is_directory(s_string path) {
    int res = 0;
    char *str_path = to_c_string(&path);
    struct stat statbuf;
    res = stat(str_path, &statbuf);
    free(str_path);

    if (res != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}
