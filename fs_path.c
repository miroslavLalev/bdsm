#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int fs_path_errno;

size_t fs_path_split(char *path, char ***segments) {
    size_t p_size = strlen(path);
    if (p_size == 0) {
        fs_path_errno = 1; // empty path
        return 0;
    }
    if (path[0] != '+') {
        fs_path_errno = 2; // invalid path
        return 0;
    }

    size_t i=1, res_size=0;
    char *curr_segment = (char*)malloc(100);
    while (i<p_size) {
        if (!(
            // allowed symbols are [a-z],[A-Z],[0-9],[-_.]
            (path[i] >= 'a' && path[i] <= 'z') ||
            (path[i] >= 'A' && path[i] <= 'z') ||
            (path[i] >= '0' && path[i] <= '9') ||
            (path[i] == '-' || path[i] == '_' || path[i] == '.' || path[i] == '/')
        )) {
            fs_path_errno = 3; // invalid characters in path
            return 0;
        }

        if (path[i] != '/') {
            curr_segment = strncat(curr_segment, &path[i], 1);
            if (i != p_size-1) {
                i++;
                continue;
            }
        }

        size_t seg_len = strlen(curr_segment);
        if (seg_len == 0) {
            // skip empty segments
            i++;
            continue;
        }

        *segments = (char**)realloc(*segments, res_size+1);
        (*segments)[res_size] = (char*)malloc(seg_len+1);
        strcpy((*segments)[res_size], curr_segment);
        res_size++;
        i++;
        free(curr_segment);
        curr_segment = (char*)malloc(100);
    }

    fs_path_errno = 0;
    return res_size;
}

int is_fs_path(char *path) {
    if (strlen(path) > 0 && path[0] == '+') {
        return 0;
    }
    return -1;
}
