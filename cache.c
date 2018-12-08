#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

s_string read_file(char *filename) {
    s_string filecontent;
    filecontent.length = 0;
    filecontent.position = NULL;

    FILE *fp = fopen(filename, "r");

    if(fp == NULL)
        return filecontent;

    if (fseek(fp, 0L, SEEK_END) == 0) {
        filecontent.length = (size_t)ftell(fp);

        if (fseek(fp, 0L, SEEK_SET) != 0) return  filecontent;

        filecontent.position = malloc((size_t)filecontent.length+1);
        size_t len = fread(filecontent.position, sizeof(char), (size_t)filecontent.length, fp);
        if ( ferror( fp ) != 0 ) return filecontent;
        else {
            filecontent.position[len++] = '\0';
        }
    }
    fclose(fp);

    return filecontent;
}

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}