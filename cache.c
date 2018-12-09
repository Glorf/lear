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

    int fd = open(filename.position, O_RDONLY);

    if(fd < 0) {
        message_log("Failed to open file", ERR);
        return filecontent;
    }

    struct stat s;
    fstat(fd, &s);
    filecontent.length = (size_t)s.st_size;

    if(filecontent.length < 0) {
        message_log("Error while getting filesize", ERR);
    }

    filecontent.position = mmap(NULL, filecontent.length, PROT_READ, MAP_SHARED, fd, 0);

    if(filecontent.position < 0) {
        message_log("Error while mapping file", ERR);
    }

    close(fd);

    return filecontent;
}

int is_directory(s_string path) {
    struct stat statbuf;
    if (stat(path.position, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}