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

int main(int argc, char *argv[])
{
    FlvDec *dec = flvdec_open(argv[1]);
    if (!dec) {
        printf("flvdec_open failed\n");
        return -1;
    }

    int video_fd = open("video.h264", O_CREAT | O_TRUNC | O_WRONLY, 0644);
    int audio_fd = open("audio.g711", O_CREAT | O_TRUNC | O_WRONLY, 0644);

    while (1) {
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
    }

    flvdec_close(dec);

    return 0;
}
