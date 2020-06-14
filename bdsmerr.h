#ifndef BDSM_ERR_H
#define BDSM_ERR_H

#include <stdbool.h>

struct fs_error_str {
    char *message;
    int errnum;
};

typedef struct fs_error_str fs_error;

fs_error fs_err_create(char *message, int errnum);

fs_error fs_no_err();

#endif
