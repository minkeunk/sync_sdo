#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

char* get_formated_time(void);
//FILE* log_file_fp(void);
//void log_file_close(void);
//void log_file_set(char *);
FILE* open_log(char *);
void close_log(FILE*);
//#define __SHORT_FILE__ (strrchr(__FILE__, '/')? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __LOG__(FP, format, loglevel, ...) fprintf(FP, "%s %-1s [%s:%d] " format, get_formated_time(), loglevel, __FILE__, __LINE__, ## __VA_ARGS__)

#define LOGDEBUG(FP, format, ...) __LOG__(FP, format, "D", ## __VA_ARGS__)
#define LOGWARN(FP, format,...) __LOG__(FP, format, "W", ## __VA_ARGS__); fflush(FP);
#define LOGERROR(FP, format, ...) __LOG__(FP, format, "E", ## __VA_ARGS__); fflush(FP);
#define LOGINFO(FP, format, ...) __LOG__(FP, format, "I", ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

