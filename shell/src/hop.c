    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <limits.h>
    #include "hop.h"

    #ifndef PATH_MAX
    #define PATH_MAX 4096
    #endif

    char prev_dir[PATH_MAX] = "";  // made global so reveal.c can access it

    void hop_command(int argc, char **argv) {
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));

        if (argc == 1 || strcmp(argv[1], "~") == 0) {
            char *home = getenv("HOME");
            if (home && chdir(home) != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(prev_dir, cwd);
            }
        }
        else if (strcmp(argv[1], ".") == 0) {
            // stay in same dir
        }
        else if (strcmp(argv[1], "..") == 0) {
            if (chdir("..") != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(prev_dir, cwd);
            }
        }
        else if (strcmp(argv[1], "-") == 0) {
            if (strlen(prev_dir) == 0) {
                printf("No such directory!\n");
            } else {
                char temp[PATH_MAX];
                strcpy(temp, cwd);
                if (chdir(prev_dir) != 0) {
                    printf("No such directory!\n");
                } else {
                    strcpy(prev_dir, temp);  // swap dirs
                }
            }
        }
        else {
            if (chdir(argv[1]) != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(prev_dir, cwd);
            }
        }
    }
