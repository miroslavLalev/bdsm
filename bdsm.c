#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "fs_types.h"

#define BSSM_FS_ENV "BDSM_FS"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Missing command parameter.\n");
        return 1;
    }
    char *command = argv[1];
    char **argv_rem = NULL;
    int argc_rem = argc - 2;
    if (argc_rem > 0) {
        argv_rem = &argv[2];
    }

    char *fs_file = getenv(BSSM_FS_ENV);
    if (fs_file == NULL) {
        fprintf(stderr, "Missing environment variable '%s'\n", BSSM_FS_ENV);
        return 1;
    }

    if (strcmp(command, "mkfs") == 0) {
        fs_error err = bdsm_mkfs(fs_file);
        if (err != NO_ERR) {
            fprintf(stderr, "mkfs error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "fsck") == 0) {
        fs_error err = bdsm_fsck(fs_file);
        if (err != NO_ERR) {
            fprintf(stderr, "fsck error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "debug") == 0) {
        fs_debug dbg;
        fs_error err = bdsm_debug(fs_file, &dbg);
        if (err != NO_ERR) {
            fprintf(stderr, "debug error\n");
            return 1;
        }
        printf("BDSM Debug information:\n");
        printf("\tmax size: %lu\n", dbg.max_size);
        printf("\tblock size: %u\n", dbg.block_size);
        printf("\tmax inodes: %u\n", dbg.n_inodes);
        return 0;
    }
    if (strcmp(command, "lsobj") == 0) {
        if (argc_rem < 1) {
            fprintf(stderr, "lsobj requires 1 argument\n");
            return 1;
        }

        fs_error err = bdsm_lsobj(fs_file, argv_rem[0]);
        if (err != NO_ERR) {
            fprintf(stderr, "lsobj error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "lsdir") == 0) {
        if (argc_rem < 1) {
            fprintf(stderr, "lsdir requires 1 argument\n");
            return 1;
        }

        fs_error err = bdsm_lsdir(fs_file, argv_rem[0]);
        if (err != NO_ERR) {
            fprintf(stderr, "lsdir error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "stat") == 0) {
        if (argc_rem < 1) {
            fprintf(stderr, "stat requires 1 argument\n");
            return 1;
        }

        fs_error err = bdsm_stat(fs_file, argv_rem[0]);
        if (err != NO_ERR) {
            fprintf(stderr, "stat error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "mkdir") == 0) {
        if (argc_rem < 1) {
            fprintf(stderr, "mkdir requires 1 argument\n");
            return 1;
        }

        fs_error err = bdsm_mkdir(fs_file, argv_rem[0]);
        if (err != NO_ERR) {
            fprintf(stderr, "mkdir error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "rmdir") == 0) {
        if (argc_rem < 1) {
            fprintf(stderr, "rmdir requires 1 argument\n");
            return 1;
        }

        fs_error err = bdsm_rmdir(fs_file, argv_rem[0]);
        if (err != NO_ERR) {
            fprintf(stderr, "rmdir error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "cpfile") == 0) {
        if (argc_rem < 2) {
            fprintf(stderr, "cpfile requires 2 arguments\n");
            return 1;
        }

        fs_error err = bdsm_cpfile(fs_file, argv_rem[0], argv_rem[1]);
        if (err != NO_ERR) {
            fprintf(stderr, "cpfile error\n");
            return 1;
        }
        return 0;
    }
    if (strcmp(command, "rmfile") == 0) {
        if (argc_rem < 1) {
            fprintf(stderr, "rmfile requires 1 arguments\n");
            return 1;
        }

        fs_error err = bdsm_rmfile(fs_file, argv_rem[0]);
        if (err != NO_ERR) {
            fprintf(stderr, "rmfile error\n");
            return 1;
        }
        return 0;
    }

    fprintf(stderr, "Invalid command '%s'\n", command);
    return 1;
}
