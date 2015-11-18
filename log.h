#ifndef __LOG_H__
#define __LOG_H__

char* get_formated_time(void);

//#define __SHORT_FILE__ (strrchr(__FILE__, '/')? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __LOG__(format, loglevel, ...) printf("%s %-5s [%s] " format "\n", get_formated_time(), loglevel, __func__, ## __VA_ARGS__)

#define LOGDEBUG(format, ...) __LOG__(format, "DEBUG", ## __VA_ARGS__)
#define LOGWARN(format,...) __LOG__(format, "WARN", ## __VA_ARGS__)
#define LOGERROR(format, ...) __LOG__(format, "ERROR", ## __VA_ARGS__)
#define LOGINFO(format, ...) __LOG__(format, "INFO", ## __VA_ARGS__)

#endif

