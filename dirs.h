#ifndef __DIRS_H__
#define __DIRS_H__

#ifdef __cplusplus
extern "C" {
#endif

int mkdir_r(const char *);
int is_exist(char *);


#ifdef __CYGWIN__
#define METADATA_SAVE_PATH "c:/cygwin64/home/sdo_kasi"
#define IMAGEDATA_SAVE_PATH "c:/cygwin64/home/sdo_kasi"
#define LOG_PATH "c:/cygwin64/home/sdo_kasi/sync_logs"
#else
#define METADATA_SAVE_PATH "/home/sdo_kasi/http"
#define IMAGEDATA_SAVE_PATH "/home/sdo_kasi/http"
#define LOG_PATH "/home/sdo_kasi/sync_sdo/logs"
#endif

#ifdef __cplusplus
}
#endif

#endif
