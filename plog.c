#include <time.h>
#include <stdio.h>
#include <limits.h>

#include "log.h"
#include "dirs.h"

static FILE *_log_fp = NULL;

char* get_formated_time(void) 
{
    time_t rawtime;
    struct tm *timeinfo;
    static char _retval[20];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(_retval, sizeof(_retval), "%Y-%m-%d %H:%M:%S", timeinfo);

    return _retval;
}
/*
void log_file_set(char *filename)
{
    char lfile[PATH_MAX]; 

    if (_log_fp)
        log_file_close();

    sprintf(lfile, "%s/%s", LOG_PATH, filename);
    _log_fp = fopen(lfile, "wb");
    if (!_log_fp) {
        _log_fp = stderr;
        fprintf(stderr, "Fail to create log file %s\n", lfile);
    }
}

inline FILE* log_file_fp(void)
{
    if (!_log_fp)
        _log_fp = stderr;

    return _log_fp;
}

void log_file_close(void)
{
    if (_log_fp != stderr) {
        fclose(_log_fp);
        _log_fp = NULL;
    }
}
*/
FILE* open_log(char *filename)
{
    char lfile[PATH_MAX]; 
    FILE *fp;

    sprintf(lfile, "%s/%s", LOG_PATH, filename);
    fp = fopen(lfile, "wb");
    if (fp) {
        fp = stderr;
        fprintf(stderr, "Fail to create log file %s\n", lfile);
    }

    return fp;
}

void close_log(FILE *fp)
{
    if (fp)
        fclose(fp);
}

