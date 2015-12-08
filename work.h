#ifndef __WORK_H__
#define __WORK_H__

#include <time.h>

#include "list.h"
#include "sdo_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WORK_FILE "./work.list"

struct WORK_DESC {
    time_t time;
    sdo_image_type type;
    struct list_head list;
};

struct WORK_DESC* work_list_init(void);
void work_list_del(void);
struct WORK_DESC* work_list_load(void);
void work_list_save(void);

void work_list_add(int year, int month, int day, int image_type);

#ifdef __cplusplus
}
#endif

#endif

