#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#include <curl/curl.h>

#include "list.h"

#include "sdo_data.h"
#include "kasi_servers.h"
#include "dirs.h"
#include "log.h"
#include "work.h"

#define METAURL_FORMAT "/metadata/nasa/sdo/%s/%s/%s/%04d/%04d%02d%02d_%s_%s_%s.txt"

struct FILE_DESC {
    CURL *curl;
    char name[PATH_MAX];
    FILE *stream;
};

struct IMAGE_FILE {
    char file_name[256];
    char rec_time[32];
    struct list_head list;
};

typedef enum {
    TODAY_THREAD = 0,
    PAST_THREAD,
    MAX_THREAD
} THREAD_NO;

static char *_sdo_image_type_string[SDO_IMAGE_TYPE_MAX] = {
    "94", "131", "171", "193", "211", "304", "335", "1600", "1700", "4500",
    "Ic", "Ic_flat", "M", "M_color"
};

static int _thread_data[MAX_THREAD] = {0, 1};
static int _update_interval = 30L;

static int _generate_metadata_url(int year, int month, int day,
                            sdo_image_type image_type, char *url)
{
    char instrument[10] = "";
    char option[10] = "";
    char *format = "jpg";

    if (image_type >= AIA_94 && image_type <= AIA_4500) {
        sprintf(instrument, "aia");
        sprintf(option, "2048");
    } else {
        sprintf(instrument, "hmi");
        sprintf(option, "4096");
    }


    sprintf(url, METAURL_FORMAT, format, instrument, 
            _sdo_image_type_string[image_type], 
            year, year, month, day, format, _sdo_image_type_string[image_type], 
            option);

    //LOGINFO("generating metaurl %s..done\n", url);
    return 0;
}

static int _generate_file_list(char *url, struct IMAGE_FILE *image_file_list, 
        FILE *lp)
{
    char metadata_path[PATH_MAX];
    struct IMAGE_FILE *file;
    FILE *fp;
    int count;

    LOGINFO(lp, "generating image file list.");
    sprintf(metadata_path, "%s%s", METADATA_SAVE_PATH, url);

    fp = fopen(metadata_path, "rb");
    if (fp) {
        while (!feof(fp)) {
            file = (struct IMAGE_FILE*)malloc(sizeof(struct IMAGE_FILE));
            count = fscanf(fp, "%s %s\n", file->rec_time, file->file_name);
            if (count != 2) {
                free(file);
                break;
            }
            list_add(&(file->list), &(image_file_list->list));
        }
    } else {
        LOGWARN(lp, "Failed to open file %s\n", metadata_path);
        fclose(fp);
        return FAILED;
    }

    fclose(fp);

    LOGINFO(lp, "done.\n");
    return SUCCESS;
}


static size_t _file_write_func(void *buffer, size_t size, size_t nmemb,
        void *stream)
{
    struct FILE_DESC *out = (struct FILE_DESC*)stream;
    if (out && !out->stream) {
        char *fname, *dname;
        struct stat st = {0};
 
        fname = strdup(out->name);
        dname = dirname(fname);

        if (stat(dname, &st) == -1) {
            mkdir_r(dname);
        }

        free(fname);
        out->stream = fopen(out->name, "wb");
        if (!out->stream)
            return -1;
    }

    return fwrite(buffer, size, nmemb, out->stream);
}

static int _download_file_from(char *server, char *url, char *to, FILE *lp)
{
    CURL *curl;
    CURLcode res;
    char kasi_url[PATH_MAX] = "";
    struct FILE_DESC *file_desc;
    int try;
    char *bname, *filename;

    bname = strdup(url);
    filename = basename(bname);
    free(bname);

    LOGINFO(lp, "downloading %s\n", filename);
    curl = curl_easy_init();
    //LOGINFO("downloading from http://%s%s", server, url);
    if (curl) {

        sprintf(kasi_url, "http://%s%s", server, url);
        curl_easy_setopt(curl, CURLOPT_URL, kasi_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _file_write_func);
        
        file_desc = (struct FILE_DESC*)malloc(sizeof(struct FILE_DESC));
        sprintf(file_desc->name, "%s%s", to, url);
        file_desc->stream = NULL;
        file_desc->curl = curl;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file_desc);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 60L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);

        if (file_desc->stream)
            fclose(file_desc->stream);

        free(file_desc);

        if (res != CURLE_OK) {
            LOGWARN(lp, "download failed\n");
            LOGWARN(lp, "cURL returns %d\n", res);
            return FAILED;
        } else {
            double speed;
            curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &speed);
            LOGINFO(lp, "successfully downloaded at %d kbytes/s\n", 
                    (int)speed/1000);
            return SUCCESS;
        }
    }

    LOGWARN(lp, "curl init failed\n");
    return FAILED;
}

static int _download_images_for_day(int year, int month, int day, 
                            sdo_image_type start_image, FILE* lp)
{
    char *url;
    int i;
    struct IMAGE_FILE *image_file_list, *tmp;
    struct list_head *pos, *q;
    char file_name[PATH_MAX];
    int res;
    int result = SUCCESS;

    LOGINFO(lp, "starting download for %4d-%02d-%02d.\n", year, month, day);
    url = (char*)malloc(PATH_MAX);

    for (i = start_image; i < SDO_IMAGE_TYPE_MAX; i++) {
        _generate_metadata_url(year, month, day, i, url);
        sprintf(file_name, "%s%s", METADATA_SAVE_PATH, url);
        if (is_exist(file_name)) {
            LOGINFO(lp, "%s exist. deleting file and downloading.\n", file_name);
            remove(file_name);
        }
        res = _download_file_from(METADATA_SERVER, url, METADATA_SAVE_PATH, lp);
        if (res == FAILED) {
            LOGWARN(lp, "downloading metadata file failed!.(%s)\n", url);
            work_list_add(year, month, day, i);
            result = FAILED;
            continue;
        }
        image_file_list = (struct IMAGE_FILE*)malloc(sizeof(struct IMAGE_FILE));
        INIT_LIST_HEAD(&(image_file_list->list));
        _generate_file_list(url, image_file_list, lp);

        list_for_each_safe(pos, q, &(image_file_list->list)) {
            tmp = list_entry(pos, struct IMAGE_FILE, list);

            sprintf(file_name, "%s%s", IMAGEDATA_SAVE_PATH, tmp->file_name);
            if (!is_exist(file_name)) {
                res = _download_file_from(IMAGEDATA_SERVER, tmp->file_name,
                            IMAGEDATA_SAVE_PATH, lp);
                if (res == FAILED) {
                    LOGWARN(lp, "downloading file failed!. (%s)\n", tmp->file_name);
                    work_list_add(year, month, day, i);
                    list_del(pos);
                    free(tmp);
                    result = FAILED;
                    continue;
                }
            } else {
                LOGINFO(lp, "%s exists. skip this file\n", file_name);
            }

            list_del(pos);
            free(tmp);
        }

        free(image_file_list);
    }
    free(url);

    LOGINFO(lp, "download completed for %4d-%02d-%02d.\n", year, month, day);
    return result;
}

static void _check_storage(void)
{
    time_t today;
    struct tm tm_s;
    char *url;
    int i;
    struct IMAGE_FILE *image_file_list, *tmp;
    struct list_head *pos, *q;
    char meta_name[PATH_MAX];
    char image_name[PATH_MAX];
    char log_file[PATH_MAX];

    time(&today);
    today -= 34128000L; // for 1year and 30days

    url = (char*)malloc(PATH_MAX);

    for (i = AIA_94; i < SDO_IMAGE_TYPE_MAX; i++) {
        gmtime_r(&today, &tm_s);
        _generate_metadata_url(tm_s.tm_year + 1900, tm_s.tm_mon+1, 
                tm_s.tm_mday, i, url);
        sprintf(meta_name, "%s%s", METADATA_SAVE_PATH, url);
        
        if (is_exist(meta_name)) {
            image_file_list = 
                (struct IMAGE_FILE*)malloc(sizeof(struct IMAGE_FILE));
            INIT_LIST_HEAD(&(image_file_list->list));
            _generate_file_list(url, image_file_list, stderr);

            list_for_each_safe(pos, q, &(image_file_list->list)) {
                tmp = list_entry(pos, struct IMAGE_FILE, list);

                sprintf(image_name, "%s%s", 
                        IMAGEDATA_SAVE_PATH, tmp->file_name);
                if (is_exist(image_name)) {
                    fprintf(stderr, "removing %s\n", image_name);
                    remove(image_name);
                }
                list_del(pos);
                free(tmp);
            }
            free(image_file_list);
            fprintf(stderr, "removing %s\n", meta_name);
            remove(meta_name);
        }
    }
    free(url);
       
    /*
     * remove log file
     */
    gmtime_r(&today, &tm_s);
    sprintf(log_file, "%s/%04d%02d%02d.log", LOG_PATH, tm_s.tm_year+1900,
                   tm_s.tm_mon+1, tm_s.tm_mday);
    remove(log_file);
}

static _set_current_download_day(THREAD_NO no, struct tm dtime)
{
    dtime.tm_sec = 0;
    dtime.tm_min = 0;
    dtime.tm_hour = 0;
    dtime.tm_wday = 0;
    dtime.tm_yday = 0;
    dtime.tm_isdst = 0;

    _thread_data[no] = mktime(&dtime);
}

inline static int _same_day_downloading(void)
{
    return (_thread_data[TODAY_THREAD] == _thread_data[PAST_THREAD]);
}

static void* _download_today_data(void *arg)
{
    char logfile[PATH_MAX];
    time_t today, start, end;
    struct tm start_s;
    FILE *fp;

    do {
        time(&start);
        gmtime_r(&start, &start_s);

        do {} while (_same_day_downloading());
        _set_current_download_day(TODAY_THREAD, start_s); 
       
        sprintf(logfile, "latest.log");
        fp = open_log(logfile);
        _download_images_for_day(start_s.tm_year + 1900, start_s.tm_mon + 1,
                  start_s.tm_mday, AIA_94, fp);
        close_log(fp);

        sleep(300L); //for 5 minutes.
   
    } while (1);
}

static void* _download_past_data(void *arg)
{
    char logfile[PATH_MAX];
    struct WORK_DESC *work_list, *tmp;
    struct list_head *pos, *q;
    int res;
    time_t start, end, duration;
    FILE *lp;

    do {
        struct tm tm_s, today_s;
        time_t today;
        
        work_list = work_list_init();
        
        time(&start);
        list_for_each_safe(pos, q, &(work_list->list)) {

            _check_storage(); 

            /* if 1day pasts since last start. reschecule work */
            time(&today);
            duration = today - start;
            if (duration > 86400L) 
                break; 

            tmp = list_entry(pos, struct WORK_DESC, list);
            gmtime_r(&tmp->time, &tm_s);

            do {} while (_same_day_downloading());
            _set_current_download_day(PAST_THREAD, tm_s); 
 
            sprintf(logfile, "%04d%02d%02d.log", tm_s.tm_year+1900,
                    tm_s.tm_mon+1, tm_s.tm_mday);
            lp = open_log(logfile);

            res = _download_images_for_day(tm_s.tm_year+1900, tm_s.tm_mon+1, 
                tm_s.tm_mday, tmp->type, lp);
            if (res == SUCCESS) {
                list_del(pos);
                free(tmp);
            }
            close_log(lp);
        }
        end = time(NULL);
        duration =  1800L - (end - start); // for 30 min.
        if (duration >= 0 && duration < 1800L) 
            sleep(duration);
    } while (1);
}

static void _start_sync(void)
{
    char logfile[PATH_MAX];
    struct WORK_DESC *work_list, *tmp;
    struct list_head *pos, *q;
    int res;
    time_t start, end, duration;
    pthread_t today_tid, past_tid;

    if (!is_exist(LOG_PATH))
        mkdir(LOG_PATH, 0777);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    pthread_create(&today_tid, NULL, _download_today_data, NULL);
    pthread_create(&past_tid, NULL, _download_past_data, NULL);

    pthread_join(today_tid, 0);
    pthread_join(past_tid, 0);

    curl_global_cleanup();
    return;
}

static void _start_service(void)
{
    pid_t pid = 0;
    pid_t sid = 0;

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }

    if (pid > 0) {
        exit(0);
    }

    umask(0);
    sid = setsid();
    if (sid < 0) {
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    _start_sync();

}

static void _stop_service(void)
{
    pid_t pid;
    pid = get_pid_by_name("sync_kasi_sdo");
    if (pid < 0)
        return;

    kill(pid, SIGKILL);
    return;
}

static void _show_help(void)
{
    fprintf(stderr, "Sync Kasi SDO 1.0\n");
    fprintf(stderr, "Usage: sync_kasi_sdo [start|stop]\n");
    fprintf(stderr, "-start \t\t Start sync_kasi_sdo service.\n");
    fprintf(stderr, "-stop \t\t Stop sync_kasi_sdo service.\n");
}

enum {
    SERVICE_START,
    SERVICE_STOP,
    SERVICE_HELP
};

int main(int argc, char *argv[])
{
    long service;
    pid_t pid;

    if (argc == 2) {
        if (strcmp(argv[1], "start") == 0)
            service = SERVICE_HELP;
        else if (strcmp(argv[1], "stop") == 0)
            service = SERVICE_STOP;
        else
            service = SERVICE_HELP;
    } else
        service = SERVICE_HELP;

    switch (service) {
        case SERVICE_HELP:
            _show_help();
            break;

        case SERVICE_START:
            pid = get_pid_by_name("sync_kasi_sdo");
            if (pid > 0) {
                fprintf(stderr, "sync_kasi_sdo is already running.\n");
                return 0;
            }
            fprintf(stderr, "sync_kasi_sdo service starts.\n");
            _start_service();
            break;

        case SERVICE_STOP:
            pid = get_pid_by_name("sync_kasi_sdo");
            if (pid < 0) {
                fprintf(stderr, "sync_kasi_sdo is not running\n");
                return 0;
            }
            _stop_service();
            fprintf(stderr, "sync_kasi_sdo service stops.\n");
            break;
    }

    return 0;
}



