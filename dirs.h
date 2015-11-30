#ifndef __DIRS_H__
#define __DIRS_H__

#ifdef __cplusplus
extern "C" {
#endif

int mkdir_r(const char *);
int is_exist(char *);


#ifdef __CYGWIN__
#define METADATA_SAVE_PATH "./data"
#define IMAGEDATA_SAVE_PATH "./data"
#define LOG_PATH "./data/logs"
#else
#define METADATA_SAVE_PATH "/home/sdo_kasi/http"
#define IMAGEDATA_SAVE_PATH "/home/sdo_kasi/http"
#define LOG_PATH "/home/sdo_kasi/sync_sdo/logs"
#endif

#ifdef __cplusplus
}
#endif

#endif
