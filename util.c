#include "util.h"

#include <stdio.h>

char* load_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("while opening file");
        return NULL;
    }

    // get file size
    if (fseek(f, 0, SEEK_END) < 0) {
        perror("while seeking end of file");
        fclose(f);
        return NULL;
    }
    long length = ftell(f);
    if (length < 0) {
        perror("while reading file");
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) < 0) {
        perror("while seeking start of file");
        fclose(f);
        return NULL;
    }

    // read file
    char* ret = (char*) MALLOC((size_t) length + 1);
    if (fread(ret, 1, (size_t) length, f) < (size_t) length) {
        perror("incomplete read");
    }

    // finish
    ret[length] = '\0';
    if (fclose(f) < 0) {
        perror("failed to close file");
    }
    return ret;
}

