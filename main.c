#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <time.h>

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

static char *_sdo_image_type_string[SDO_IMAGE_TYPE_MAX] = {
    "94", "131", "171", "193", "211", "304", "335", "1600", "1700", "4500",
    "Ic", "Ic_flat", "M", "M_color"
};

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

    LOGINFO("generating metaurl %s..done\n", url);
    return 0;
}

static int _generate_file_list(char *url, struct IMAGE_FILE *image_file_list)
{
    char metadata_path[PATH_MAX];
    struct IMAGE_FILE *file;
    FILE *fp;
    int count;

    LOGINFO("generating image file list.");
    sprintf(metadata_path, "%s%s\0", METADATA_SAVE_PATH, url);

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
        LOGINFO("Failed to open file %s\n", metadata_path);
        fclose(fp);
        return FAILED;
    }

    fclose(fp);

    LOGINFO("done.\n");
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

static int _download_file_from(char *server, char *url, char *to)
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

    LOGINFO("downloading %s\n", filename);
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

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        for (try = 0; try < 3; try++) {
            res = curl_easy_perform(curl);

            if (res == CURLE_OK) break;
            else {
                LOGINFO("curl told us %d, trying again (%d)", res, try);
            } 
        }

        if (try == 3) {
            LOGINFO("download failed\n");
        }

        curl_easy_cleanup(curl);

        if (file_desc->stream)
            fclose(file_desc->stream);

        free(file_desc);

        if (try == 3) {
            LOGINFO("download failed\n");
            return FAILED;
        } else {
            double speed;
            curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &speed);
            LOGINFO("successfully downloaded at %d kbytes/s\n", 
                    (int)speed/1000);
            return SUCCESS;
        }
    }

    LOGINFO("curl init failed\n");
    return FAILED;
}

static int _download_images_for_day(int year, int month, int day, 
                            sdo_image_type start_image)
{
    char *url;
    int i;
    struct IMAGE_FILE *image_file_list, *tmp;
    struct list_head *pos, *q;
    char file_name[PATH_MAX];
    int res;
    int result = SUCCESS;

    LOGINFO("starting download for %4d-%02d-%02d.\n", year, month, day);
    url = (char*)malloc(PATH_MAX);

    for (i = start_image; i < SDO_IMAGE_TYPE_MAX; i++) {
        _generate_metadata_url(year, month, day, i, url);
        sprintf(file_name, "%s%s", METADATA_SAVE_PATH, url);
        if (is_exist(file_name)) {
            LOGINFO("%s exist. deleting file and downloading.\n", file_name);
            remove(file_name);
        }
        res = _download_file_from(METADATA_SERVER, url, METADATA_SAVE_PATH);
        if (res == FAILED) {
            LOGWARN("downloading metadata file failed!.(%s%s)\n", url);
            work_list_add(year, month, day, i);
            result = FAILED;
            continue;
        }

        image_file_list = (struct IMAGE_FILE*)malloc(sizeof(struct IMAGE_FILE));
        INIT_LIST_HEAD(&(image_file_list->list));
        _generate_file_list(url, image_file_list);

        list_for_each_safe(pos, q, &(image_file_list->list)) {
            tmp = list_entry(pos, struct IMAGE_FILE, list);

            sprintf(file_name, "%s%s", IMAGEDATA_SAVE_PATH, tmp->file_name);
            if (!is_exist(file_name)) {
                res = _download_file_from(IMAGEDATA_SERVER, tmp->file_name,
                            IMAGEDATA_SAVE_PATH);
                if (res == FAILED) {
                    LOGWARN("downloading file failed!. (%s%s)\n", url);
                    work_list_add(year, month, day, i);
                    result = FAILED;
                    continue;
                }
            } else {
                LOGINFO("%s exists. skip this file\n", file_name);
            }

            list_del(pos);
            free(tmp);
        }

        free(image_file_list);
    }
    free(url);

    LOGINFO("download completed for %4d-%02d-%02d.\n", year, month, day);
    return result;
}

static void _check_storage(void)
{
    time_t today;
    struct tm *tm_s;
    char *url;
    int i;
    struct IMAGE_FILE *image_file_list, *tmp;
    struct list_head *pos, *q;
    char meta_name[PATH_MAX];
    char image_name[PATH_MAX];

    today = time(NULL);
    today -= 34128000L; // for 1year and 30days

    url = (char*)malloc(PATH_MAX);

    for (i = AIA_94; i < SDO_IMAGE_TYPE_MAX; i++) {
        tm_s = gmtime(&today);
        _generate_metadata_url(tm_s->tm_year + 1900, tm_s->tm_mon+1, 
                tm_s->tm_mday, i, url);
        sprintf(meta_name, "%s%s", METADATA_SAVE_PATH, url);
        if (is_exist(meta_name)) {
            image_file_list = 
                (struct IMAGE_FILE*)malloc(sizeof(struct IMAGE_FILE));
            INIT_LIST_HEAD(&(image_file_list->list));
            _generate_file_list(url, image_file_list);

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
}
    
int main(void)
{
    char logfile[PATH_MAX];
    struct WORK_DESC *work_list, *tmp;
    struct list_head *pos, *q;
    int res;
    time_t start, end, duration;

    if (!is_exist(LOG_PATH))
        mkdir(LOG_PATH, 0777);

    curl_global_init(CURL_GLOBAL_DEFAULT);


    do {
        work_list = work_list_init();
        
        start = time(NULL);
        list_for_each_safe(pos, q, &(work_list->list)) {
            struct tm* tm_s;
            time_t today;

            /* check storage 
             * delete data over 1year + 30days
             */
            _check_storage(); 

            /* if 1day pasts since last start. reschecule work */
            today = time(NULL);
            duration = today - start;
            if (duration > 86400L) 
                break; 

            /* try to download today's data */
            tm_s = gmtime(&today);
            sprintf(logfile, "%04d%02d%02d.log", tm_s->tm_year+1900,
                    tm_s->tm_mon+1, tm_s->tm_mday);
            log_file_set(logfile);

            _download_images_for_day(tm_s->tm_year + 1900, tm_s->tm_mon + 1,
                    tm_s->tm_mday, AIA_94);
            log_file_close();

            /* then download past data */
            tmp = list_entry(pos, struct WORK_DESC, list);
            tm_s = gmtime(&tmp->time);

            sprintf(logfile, "%04d%02d%02d.log", tm_s->tm_year+1900,
                    tm_s->tm_mon+1, tm_s->tm_mday);
            log_file_set(logfile);

            res = _download_images_for_day(tm_s->tm_year+1900, tm_s->tm_mon+1, 
                tm_s->tm_mday, tmp->type);
            if (res == SUCCESS) {
                list_del(pos);
                free(tmp);
            }
            log_file_close();
        }
        end = time(NULL);
        duration = 10800L - (end - start); //for 3hours
        if (duration >= 0 && duration < 10800L) 
            sleep(duration);
    } while(1);

    curl_global_cleanup();
    return 0;
}



