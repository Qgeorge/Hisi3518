#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "flvenc.h"
#include "flvdec.h"
#include "record.h"

#define min(x, y)    ((x) < (y) ? (x) : (y))

static char g_mount_point[1024] = {0};

static FlvEnc *enc = NULL;
static struct tm prev_tm;

int av_record_init(char *mount_point)
{
    strncpy(g_mount_point, mount_point, sizeof(g_mount_point) - 1);
    return 0;
}

int av_record_quit(void)
{
    if (enc) {
        flvenc_close(enc);
        enc = NULL;
    }
    return 0;
}

static void get_dirname(int year, int month, int day, int hour, char *dirname, int len)
{
    snprintf(dirname, len, "%s/%04d-%02d-%02d/%d", g_mount_point, year, month, day, hour);
}

static int find_oldest_dir(char *dirname, int len)
{
    int found = 0;
    struct dirent **namelist = NULL;

    int n = scandir(g_mount_point, &namelist, 0, alphasort);
    if (n <= 0)
        return 0;   /* no record */

    int i;
    for (i = 0; i < n; i++) {
        int year, month, day;
        if (sscanf(namelist[i]->d_name, "%04d-%02d-%02d", &year, &month, &day) == 3) {
            snprintf(dirname, len, "%s/%s", g_mount_point, namelist[i]->d_name);
            found = 1;
            break;
        }
    }

    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return found;
}

static int find_oldest_file(char *dirname, char *filepath, int len)
{
    int found = 0;
    struct dirent **namelist = NULL;

    int n = scandir(dirname, &namelist, 0, alphasort);
    if (n <= 0)
        return 0;   /* no record */

    int i;
    for (i = 0; i < n; i++) {
        int year, month, day, hour, minute;
        if (sscanf(namelist[i]->d_name, "%04d%02d%02dT%02d%02d.flv", &year, &month, &day, &hour, &minute) == 5) {
            snprintf(filepath, len, "%s/%s", dirname, namelist[i]->d_name);
            found = 1;
            break;
        }
    }

    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return found;
}

static int64_t freesize(void)
{
    struct statfs sfs;
    if (statfs(g_mount_point, &sfs) < 0)
        return -1;
    return (int64_t)sfs.f_bsize * (int64_t)sfs.f_bavail;
}

static int get_filepath(int year, int month, int day, int hour, int minute, char *filepath, int len)
{
    snprintf(filepath, len, "%s/%04d-%02d-%02d/%d/%04d%02d%02dT%02d%02d.flv",
            g_mount_point, year, month, day, hour, year, month, day, hour, minute);
    return 0;
}

static int needswitch(struct tm tm)
{
    if (tm.tm_year == prev_tm.tm_year && tm.tm_mon  == prev_tm.tm_mon  &&
        tm.tm_mday == prev_tm.tm_mday && tm.tm_hour == prev_tm.tm_hour &&
        tm.tm_min  == prev_tm.tm_min)
        return 0;
    return 1;
}

#define SIZE_1MB    (1024 * 1024)

static int prepare_recording(int keyframe)
{
    char dirname[1024];
    char filepath[1024];

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);

    int year   = tm.tm_year + 1900;
    int month  = tm.tm_mon + 1;
    int day    = tm.tm_mday;
    int hour   = tm.tm_hour;
    int minute = tm.tm_min;

    if (enc && !(needswitch(tm) && keyframe))
        return 0;

    /*
     * switch to next file
     */

    if (enc) {
        flvenc_close(enc); /* close prevous FLV file */
        enc = NULL;
    }

    get_dirname(year, month, day, hour, dirname, sizeof(dirname));

    if (access(dirname, R_OK | W_OK | X_OK) < 0) {
#if 0
        if (mkdir(dirname, 0777) < 0) {
            perror("mkdir");
            return -1;
        }
#else
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", dirname);
        system(cmd);
#endif
    }

    while (freesize() < 100LL * SIZE_1MB) {
        if (find_oldest_dir(dirname, sizeof(dirname)) != 1)
            return -1;

        if (find_oldest_file(dirname, filepath, sizeof(filepath)) != 1)
            return -1;

        unlink(filepath);   /* delete oldest file to earn some space */
    }

    get_filepath(year, month, day, hour, minute, filepath, sizeof(filepath));

    MediaDesc desc[2] = {
        { .media = MediaType_Video, .codec = CodecType_H264 },
        { .media = MediaType_Audio, .codec = CodecType_ALAW },
    };

    enc = flvenc_open(filepath, desc, 2);
    if (!enc) {
        perror("flvenc_open");
        return -1;
    }

    prev_tm = tm;

    return 0;
}

int av_record_write(int codec, void *buf, int len, int64_t time_ms, int keyframe)
{
    if (prepare_recording(keyframe))
        return -1;

    av_packet_t pkt;
    memset(&pkt, 0, sizeof(av_packet_t));

    switch (codec) {
    case 0:
        pkt.media = MediaType_Video;
        pkt.codec = CodecType_H264;
        break;
    case 1:
        pkt.media = MediaType_Audio;
        pkt.codec = CodecType_ALAW;
        break;
    case 2:
        pkt.media = MediaType_Audio;
        pkt.codec = CodecType_MULAW;
        break;
    default:
        printf("unknown codec type %d\n", codec);
        return -1;
    }

    pkt.keyframe = keyframe;
    memcpy(pkt.data, buf, len);
    pkt.size     = len;
    pkt.time_ms  = time_ms;

    if (flvenc_write(enc, &pkt) < 0)
        return -1;

    return 0;
}

struct av_record_s {
    FlvDec *dec;
};

av_record_t *av_record_open(int year, int month, int day, int hour, int minute)
{
    char filepath[1024];
    get_filepath(year, month, day, hour, minute, filepath, sizeof(filepath));

    av_record_t *rec = malloc(sizeof(av_record_t));
    if (!rec)
        return NULL;
    memset(rec, 0, sizeof(av_record_t));

    rec->dec = flvdec_open(filepath);
    if (!rec->dec) {
        free(rec);
        return NULL;
    }

    return rec;
}

int av_record_close(av_record_t *rec)
{
    flvdec_close(rec->dec);
    free(rec);
    return 0;
}

int av_record_read(av_record_t *rec, av_frame_t *frame)
{
    av_packet_t pkt;
    if (flvdec_read(rec->dec, &pkt) < 0)
        return -1;

    int size = min(pkt.size, sizeof(frame->data));

    frame->codec    = pkt.codec;
    frame->keyframe = pkt.keyframe;
    frame->time_ms  = pkt.time_ms;

    memcpy(frame->data, pkt.data, size);
    frame->size = size;

    return 0;
}

static int customFilter(const struct dirent *pDir)
{
	if (strcmp(pDir->d_name, ".") && strcmp(pDir->d_name, ".."))
	{
		return 1;
	}
	return 0;
}
int av_record_search(int year, int month, int day, int *timeinfos, int nmemb)
{
    int count = 0;
    int hour;
	char chartime[20]={0};

	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
	memset(tmp_time, 0, sizeof(tmp_time));

    for (hour = 0; hour < 24; hour++) {
        char dirname[1024];
        get_dirname(year, month, day, hour, dirname, sizeof(dirname));
        struct dirent **namelist = NULL;
        int n = scandir(dirname, &namelist, customFilter, alphasort);
        if (n <= 0)
            continue;

        int i;
        for (i = 0; i < n; i++) {
            record_info_t infos;
            record_info_t *info = &infos;
            memset(info, 0, sizeof(record_info_t));
			//if(strcmp())
            int ret = sscanf(namelist[i]->d_name, "%04d%02d%02dT%02d%02d.flv",
                    &info->year, &info->month, &info->day, &info->hour, &info->minute);
            if (ret != 5)
                continue;
			sprintf(chartime, "%04d-%02d-%02d-%02d-%02d", info->year, info->month, info->day, info->hour, info->minute);
			strptime(chartime, "%Y-%m-%d-%H-%M", tmp_time);
			timeinfos[count] = mktime(tmp_time);
            count++;
            if (count >= nmemb)
                break;
        }   
		
        for (i = 0; i < n; i++)
            free(namelist[i]);
        free(namelist);
    }
	free(tmp_time);
    return count;
}
