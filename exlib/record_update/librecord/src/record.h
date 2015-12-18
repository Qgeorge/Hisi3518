#ifndef __RECORD_H__
#define __RECORD_H__

int av_record_init(char *mount_point);
int av_record_quit(void);
typedef long long int64_t;
/**
 * codec: 0 - H264, 1 - G.711ALaw
 */
int av_record_write(int codec, void *buf, int len, int64_t time_ms, int keyframe);

typedef struct av_record_s av_record_t;

av_record_t *av_record_open(int year, int month, int day, int hour, int minute);
int av_record_close(av_record_t *rec);

typedef struct {
    int codec;  /* 0 - h264, 1 - G.711ALaw */
    int keyframe;
    int64_t time_ms;

    int size;
    char data[1024 * 1024];
} av_frame_t;

int av_record_read(av_record_t *rec, av_frame_t *frame);

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
} record_info_t;

int av_record_search(int year, int month, int day, int *timeinfos, int nmemb);

#endif
