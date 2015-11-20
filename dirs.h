#ifndef __DIRS_H__
#define __DIRS_H__

#ifdef __cplusplus
extern "C" {
#endif

int mkdir_r(const char *);
int is_exist(char *);


#define METADATA_SAVE_PATH "c:/cygwin64/home/sdo_kasi"
#define IMAGEDATA_SAVE_PATH "c:/cygwin64/home/sdo_kasi"
#define LOG_PATH "c:/cygwin64/home/sdo_kasi/sync_logs"

#ifdef __cplusplus
}
#endif

#endif
