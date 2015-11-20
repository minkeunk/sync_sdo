#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

char* get_formated_time(void);
FILE* get_log_fp(void);
void close_log_file(void);
//#define __SHORT_FILE__ (strrchr(__FILE__, '/')? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __LOG__(format, loglevel, ...) fprintf(get_log_fp(), "%s %-1s [%s:%d] " format, get_formated_time(), loglevel, __FILE__, __LINE__, ## __VA_ARGS__)

#define LOGDEBUG(format, ...) __LOG__(format, "D", ## __VA_ARGS__)
#define LOGWARN(format,...) __LOG__(format, "W", ## __VA_ARGS__)
#define LOGERROR(format, ...) __LOG__(format, "E", ## __VA_ARGS__)
#define LOGINFO(format, ...) __LOG__(format, "I", ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

