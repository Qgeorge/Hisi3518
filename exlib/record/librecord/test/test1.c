#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "flvdec.h"
#include "flvenc.h"

int main(void)
{
    FlvDec *dec = flvdec_open("video.flv");
    if (!dec) {
        printf("flvdec_open failed\n");
        return -1;
    }

    MediaDesc desc[2] = {
        { .media = MediaType_Video, .codec = CodecType_H264 },
        { .media = MediaType_Audio, .codec = CodecType_ALAW },
    };

    FlvEnc *enc = flvenc_open("video2.flv", desc, 2);
    if (!enc) {
        printf("flvenc_open failed\n");
        return -1;
    }

    int video_fd = open("video.h264", O_CREAT | O_TRUNC | O_WRONLY, 0644);
    int audio_fd = open("audio.g711", O_CREAT | O_TRUNC | O_WRONLY, 0644);

    int i;
    for (i = 0; i < 10; i++) {
        //        printf("+++++++++++\n");
        av_packet_t pkt;
        if (flvdec_read(dec, &pkt) < 0) {
            printf("flvdec_read failed\n");
            break;
        }

        switch (pkt.media) {
        case MediaType_Video:
            write(video_fd, pkt.data, pkt.size);
            break;
        case MediaType_Audio:
            write(audio_fd, pkt.data, pkt.size);
            break;
        default:
            printf("unsupported media type %d\n", pkt.media);
        }

        av_dump_packet(&pkt);

        if (flvenc_write(enc, &pkt) < 0)
            return -1;
    }

    flvdec_close(dec);
    flvenc_close(enc);

    return 0;
}

