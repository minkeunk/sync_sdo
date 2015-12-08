#include <stdio.h>
#include <stdlib.h>

#include "work.h"

static struct WORK_DESC _work_list;

static int _same_work_exist(long time, int type) 
{
    struct WORK_DESC *tmp;

    list_for_each_entry(tmp, &_work_list.list, list) {
        if (time == tmp->time) {
            tmp->type = (tmp->type < type) ? tmp->type : type;
            return TRUE;
        }
    }

    return FALSE;
}

struct WORK_DESC* work_list_init(void)
{
    time_t work_time;
    struct WORK_DESC *work;
    int i;
    struct tm* loctime;
    struct WORK_DESC *tmp;
    struct list_head *pos, *q;


    list_for_each_safe(pos, q, &_work_list.list) {
        tmp = list_entry(pos, struct WORK_DESC, list);
        list_del(pos);
        free(tmp);
    }


    INIT_LIST_HEAD(&_work_list.list);

    time(&work_time);

    /* for 1 year and 30 days */
    for (i = 0; i < 365 + 30; i++) {
        work_time -= 60*60*24;
        work = (struct WORK_DESC*)malloc(sizeof(struct WORK_DESC));
        work->time = work_time;
        work->type = AIA_94;

       list_add_tail(&(work->list), &(_work_list.list));
    }

    return &_work_list;
}

void work_list_save(void)
{
    FILE* fp;
    struct WORK_DESC *tmp;

    fp = fopen(WORK_FILE, "wt");
    if (fp)
        list_for_each_entry(tmp, &_work_list.list, list) 
            fprintf(fp, "%ld %d\n", tmp->time, tmp->type);
    fclose(fp);
}

struct WORK_DESC* work_list_load(void)
{
    FILE* fp;
    struct WORK_DESC *tmp;
    struct list_head *pos, *q;

    INIT_LIST_HEAD(&_work_list.list);
    /* empty list */
    list_for_each_safe(pos, q, &_work_list.list) {
        tmp = list_entry(pos, struct WORK_DESC, list);
        list_del(pos);
        free(tmp);
    }

    fp = fopen(WORK_FILE, "rt");
    if (fp) {
        long time;
        int type;
        int count;

        while (!feof(fp)) {
            count = fscanf(fp, "%ld %d\n", &time, &type);
            if (count != 2)
                break;

            if (!_same_work_exist(time, type)) {
                tmp = (struct WORK_DESC*)malloc(sizeof(struct WORK_DESC));
                tmp->time = time;
                tmp->type = type;

                list_add_tail(&(tmp->list), &(_work_list.list));
            }
        }
        fclose(fp);
        return &_work_list;
    }

    return NULL;
}

void work_list_add(int year, int month, int day, int type)
{
    struct WORK_DESC *new;
    struct tm tm_time;
    time_t time;

    tm_time.tm_year = year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = tm_time.tm_min = tm_time.tm_sec = tm_time.tm_isdst = 0;

    time = mktime(&tm_time);

    if (!_same_work_exist(time, type)) {
        new = (struct WORK_DESC*)malloc(sizeof(struct WORK_DESC));
        new->time = time;
        new->type = type;

        list_add(&(new->list), &(_work_list.list));
    }
}

       

