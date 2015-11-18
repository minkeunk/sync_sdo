#include <time.h>
#include <stdio.h>

#include "log.h"

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

