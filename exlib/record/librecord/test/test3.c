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

static void dump_record(record_info_t *rec)
{
    printf("rec->year   = %d\n", rec->year);
    printf("rec->month  = %d\n", rec->month);
    printf("rec->day    = %d\n", rec->day);
    printf("rec->hour   = %d\n", rec->hour);
    printf("rec->minute = %d\n", rec->minute);
}

int main(void)
{
    av_record_init("/home/a15103/workspace/flv/flvenc/repo");

    FlvDec *dec = flvdec_open("video.flv");
    if (!dec) {
        printf("flvdec_open failed\n");
        return -1;
    }

    int i;
    for (i = 0; i < 1000; i++) {
        av_packet_t pkt;
        if (flvdec_read(dec, &pkt) < 0)
            return -1;

        av_record_write(pkt.codec, pkt.data, pkt.size, pkt.time_ms, pkt.keyframe);
    }

    record_info_t records[2048];
    int numrecs = av_record_search(2015, 9, 17, records, 2048);

    printf("numrecs = %d\n", numrecs);

    for (i = 0; i < numrecs; i++) {
        printf("---------------\n");
        dump_record(&records[i]);
    }

    flvdec_close(dec);

    //    20150917T1408
    av_record_t *rec = av_record_open(2015, 9, 17, 19, 19);
    if (!rec) {
        printf("av_record_open failed\n");
        return -1;
    }

    while (1) {
        av_frame_t frame;
        int ret = av_record_read(rec, &frame);
        if (ret < 0)
            break;
        printf("-------------\n");
        printf("frame.time_ms = %lld\n", (long long)frame.time_ms);
        printf("frame.codec   = %d\n", frame.codec);
        printf("frame.size    = %d\n", frame.size);
    }

    av_record_close(rec);

    return 0;
}
