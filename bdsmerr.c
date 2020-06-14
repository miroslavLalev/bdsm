#include <string.h>
#include <stdlib.h>

struct fs_error_str {
    char *message;
    int errnum;
};

typedef struct fs_error_str fs_error;

fs_error fs_err_create(char *message, int errnum) {
    fs_error err;
    err.errnum = errnum;
    err.message = (char*)malloc(strlen(message)+1);
    strcpy(err.message, message);
    return err;
}

fs_error fs_no_err() {
    fs_error err;
    err.errnum = 0;
    return err;
}
