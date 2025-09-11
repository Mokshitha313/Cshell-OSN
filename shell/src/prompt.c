#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include "prompt.h"

// Fallbacks if not defined
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

static char home[PATH_MAX];

void init_prompt() {
    getcwd(home, sizeof(home));
}

void print_prompt() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    char *user = getpwuid(getuid())->pw_name;

    char host[HOST_NAME_MAX];
    gethostname(host, sizeof(host));

    // Replace home with ~
    char *disp = cwd;
    if (strncmp(cwd, home, strlen(home)) == 0) {
        static char tmp[PATH_MAX];
        snprintf(tmp, sizeof(tmp), "~%s", cwd + strlen(home));
        disp = tmp;
    }

    printf("<%s@%s:%s> ", user, host, disp);
    fflush(stdout);
}
