#include <stdio.h>
#include <stdlib.h>

int read_file(char *filename, char *filecontent) {
    FILE *fp = fopen(filename, "r");

    int len = 0;

    if(fp == NULL)
        return -1;

    if (fseek(fp, 0L, SEEK_END) == 0) {
        long bufsize = ftell(fp);

        if (fseek(fp, 0L, SEEK_SET) != 0) return  -1;

        len = (int)fread(filecontent, sizeof(char), (size_t)bufsize, fp);
        if ( ferror( fp ) != 0 ) return -1;
        else {
            filecontent[len++] = '\0';
        }
    }
    fclose(fp);

    return len;
}