    #include <stdio.h>
    #include <sys/types.h>
    #include <dirent.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include "reveal.h"

    extern char prev_dir[];  // from hop.c

    static int cmp_names(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}


    void reveal_command(int argc, char **argv) {
        int show_all = 0, line = 0;
        char *target = NULL;

        // Parse arguments
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-") == 0) {
                if (!target) target = "-";
                else { printf("reveal: Invalid Syntax!\n"); return; }
            } else if (argv[i][0] == '-') {
                for (int j = 1; argv[i][j]; j++) {
                    if (argv[i][j] == 'a') show_all = 1;
                    else if (argv[i][j] == 'l') line = 1;
                    else { printf("reveal: Invalid option %c\n", argv[i][j]); return; }
                }
            } else {
                if (!target) target = argv[i];
                else { printf("reveal: Invalid Syntax!\n"); return; }
            }
        }

        char path[512];
        if (!target || strcmp(target, ".") == 0) {
            if (!getcwd(path, sizeof(path))) { printf("Error getting current directory!\n"); return; }
        } else if (strcmp(target, "-") == 0) {
            if (prev_dir[0] == '\0') { printf("No such directory!\n"); return; }
            strcpy(path, prev_dir);
        } else strcpy(path, target);

        struct dirent **namelist;
        int n = scandir(path, &namelist, NULL, cmp_names);
        if (n < 0) { printf("No such directory!\n"); return; }

        for (int i = 0; i < n; i++) {
            if (!show_all && namelist[i]->d_name[0] == '.') {
                free(namelist[i]);
                continue;
            }
            if (line) printf("%s\n", namelist[i]->d_name);
            else printf("%s  ", namelist[i]->d_name);

            free(namelist[i]);
        }

        if (!line) printf("\n");
        free(namelist);
    }
