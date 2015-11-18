#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <limits.h>

#include "dirs.h"

int mkdir_r(const char *dir)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len-1] == '/')
        tmp[len-1] = 0;
   
    for (p = tmp+1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir (tmp, 0777);
            *p = '/';
        }
    mkdir(tmp, 0777);

    return 0;
}

int is_exist(char *file)
{
    if (access(file, F_OK) != -1) {
        return 1;
    } else
        return 0;
}

