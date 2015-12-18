#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "flv.h"

void av_dump_packet(av_packet_t *pkt)
{
    printf("pkt->media = %d\n", pkt->media);
    printf("pkt->codec = %d\n", pkt->codec);
    printf("pkt->keyframe = %d\n", pkt->keyframe);
    printf("pkt->size = %d\n", pkt->size);
    printf("pkt->time_ms = %lld\n", (long long)pkt->time_ms);
}

void dump_tag(FlvTag *tag)
{
    printf("---------\n");
    printf("tag->TagType   = %d\n", tag->TagType);
    printf("tag->DataSize  = %d\n", tag->DataSize);
    printf("tag->Timestamp = %d ***\n", tag->Timestamp);
    printf("tag->StreamID  = %d\n", tag->StreamID);
}

void dump_video_tag_header(VideoTagHeader *th)
{
    printf("th->FrameType = %d\n", th->FrameType);
    printf("th->CodecID   = %d\n", th->CodecID);
}

void dump_audio_tag_header(AudioTagHeader *th)
{
    printf("\nth->CodecID    = %d\n", th->CodecID);
    printf("th->SampleRate = %d\n", th->SampleRate);
    printf("th->SampleBits = %d\n", th->SampleBits);
    printf("th->Stereo     = %d\n", th->Stereo);
}

void dump_avcc(AVCConfiguration *avcc)
{
    printf("avcc->configurationVersion = %d\n", avcc->configurationVersion);
    printf("avcc->AVCProfileIndication = %d\n", avcc->AVCProfileIndication);
    printf("avcc->profile_compatibility = %d\n", avcc->profile_compatibility);
    printf("avcc->AVCLevelIndication = %d\n", avcc->AVCLevelIndication);
    printf("avcc->lengthSizeMinusOne = %d\n", avcc->lengthSizeMinusOne);
}

