#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "h264.h"
#include "flvenc.h"

#define min(x, y)    ((x) < (y) ? (x) : (y))
#define NELEMS(x)    (sizeof(x) / sizeof((x)[0]))

struct FlvEnc {
    char filename[512];

    MediaDesc descs[4];
    int ndesc;

    int fd;

    int64_t first_ts;

    uint8_t sps[2048];
    uint16_t sps_len;

    uint8_t pps[1024];
    uint16_t pps_len;
};

inline int AV_WB32(uint8_t *p, uint32_t data)
{
    uint8_t *pdata = (uint8_t *)&data;
    *(p++) = *(pdata + 3);
    *(p++) = *(pdata + 2);
    *(p++) = *(pdata + 1);
    *(p++) = *(pdata + 0);
    return 4;
}

inline int AV_WB24(uint8_t *p, uint32_t data)
{
    uint8_t *pdata = (uint8_t *)&data;
    *(p++) = *(pdata + 2);
    *(p++) = *(pdata + 1);
    *(p++) = *(pdata + 0);
    return 3;
}

inline int AV_WB16(uint8_t *p, uint16_t data)
{
    uint8_t *pdata = (uint8_t *)&data;
    *(p++) = *(pdata + 1);
    *(p++) = *(pdata + 0);
    return 2;
}

inline int AV_WB8(uint8_t *p, uint8_t data)
{
    *(p++) = data;
    return 1;
}

static int AV_Write(uint8_t *p, void *buf, int len)
{
    memcpy(p, buf, len);
    p += len;
    return len;
}

FlvEnc *flvenc_open(char *filename, MediaDesc *descs, int nmemb)
{
    FlvEnc *enc = malloc(sizeof(FlvEnc));
    if (!enc)
        return NULL;
    memset(enc, 0, sizeof(FlvEnc));

    strncpy(enc->filename, filename, sizeof(enc->filename) - 1);
    memcpy(enc->descs, descs, min(nmemb, NELEMS(enc->descs)));

    enc->fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (enc->fd < 0) {
        free(enc);
        return NULL;
    }

    enc->first_ts = -1;

    uint8_t buf[128];
    uint8_t *p = buf;

    p += AV_Write(p, "FLV", 3);
    p += AV_WB8(p, 0x01);    /* Version */

    uint8_t TypeFlags = 0;

    int i;
    for (i = 0; i < enc->ndesc; i++) {
        if (enc->descs[i].media == MediaType_Video)
            TypeFlags |= 0x01;
        else if (enc->descs[i].media == MediaType_Audio)
            TypeFlags |= 0x03;
    }

    p += AV_WB8(p, TypeFlags);
    p += AV_WB32(p, 9);    /* DataOffset */
    p += AV_WB32(p, 0);    /* PreviousTagSize0 */

    if (write(enc->fd, buf, p - buf) != p - buf) {
        close(enc->fd);
        free(enc);
        return NULL;
    }

    return enc;
}

int flvenc_close(FlvEnc *enc)
{
    close(enc->fd);
    free(enc);
    return 0;
}

static uint8_t *avc_find_startcode_internal(uint8_t *p, uint8_t *end)
{
    uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(uint32_t*)p;
        //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
        //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

uint8_t *avc_find_startcode(uint8_t *p, uint8_t *end)
{
    uint8_t *out = avc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1]) out--;
    return out;
}

static int flvenc_write_sequence_header(FlvEnc *enc, uint8_t *sps, int sps_len, uint8_t *pps, int pps_len, int64_t ts_ms, int keyframe)
{
    uint8_t tag[128];
    uint8_t *p = tag;

    int time_ms = ts_ms - enc->first_ts;

    /* Tag */
    p += AV_WB8(p, TagType_Video);
    p += AV_WB24(p, 0);    /* DataSize */
    p += AV_WB24(p, time_ms);
    p += AV_WB8(p, (time_ms >> 24) & 0xf);
    p += AV_WB24(p, 0);    /* streamID */

    /* VideoTag */
    p += AV_WB8(p, ((keyframe ? 1 : 2) << 4) | 7);

    /* AVCPacket */
    p += AV_WB8(p, 0x00);           /* AVCPacketType: AVC Sequence Header */
    p += AV_WB24(p, 0);             /* CompositionTime */

    /* AVCConfiguration */
    p += AV_WB8(p, 0x01);           /* configurationVersion  */
    p += AV_WB8(p, sps[1]);         /* AVCProfileIndication  */
    p += AV_WB8(p, sps[2]);         /* profile_compatibility */
    p += AV_WB8(p, sps[3]);         /* AVCLevelIndication    */
    p += AV_WB8(p, 0xfa | 0x3);     /* lengthSizeMinusOne    */

    p += AV_WB8(p, 1);              /* numOfSequenceParameterSets */
    p += AV_WB16(p, sps_len);
    p += AV_Write(p, sps, sps_len);

    p += AV_WB8(p, 1);              /* numOfPictureParameterSets */
    p += AV_WB16(p, pps_len);
    p += AV_Write(p, pps, pps_len);

    /* fill DataSize field */
    uint8_t *p_size = &tag[1];
    AV_WB24(p_size, p - tag - 11);

    if (write(enc->fd, tag, p - tag) != p - tag)
        return -1;

    /* PreviousTagSize */
    uint32_t total = htonl(p - tag);  /* 11 for FlvTag */
    if (write(enc->fd, &total, sizeof(uint32_t)) != sizeof(uint32_t)) /* 1 for VideoTagHeader, 11 for FlvTag */
        return -1;

    return 0;
}

static int flvenc_write_nal(FlvEnc *enc, void *nal, int nalsize, int64_t ts_ms, int keyframe)
{
    uint8_t tag[128];
    uint8_t *p = tag;

    if (enc->first_ts == -1)
        enc->first_ts = ts_ms;
    int time_ms = ts_ms - enc->first_ts;

    int total_data_size = nalsize + 1 + 8;

    /* Tag */
    p += AV_WB8(p, TagType_Video);
    p += AV_WB24(p, total_data_size);    /* DataSize, 1 for VideoTagHeader */
    p += AV_WB24(p, time_ms);
    p += AV_WB8(p, (time_ms >> 24) & 0xf);
    p += AV_WB24(p, 0);    /* streamID */

    /* VideoTag */
    p += AV_WB8(p, ((keyframe ? 1 : 2) << 4) | 7);

    /* AVCPacket */
    p += AV_WB8(p, 0x01);   /* AVCPacketType: AVC NALU */
    p += AV_WB24(p, 0);     /* reserved */
    p += AV_WB32(p, nalsize);   /* NALU Size */

    if (write(enc->fd, tag, p - tag) != p - tag)
        return -1;

    if (write(enc->fd, nal, nalsize) != nalsize)
        return -1;

    /* PreviousTagSize */
    uint32_t total = htonl(total_data_size + 11);  /* 11 for FlvTag */
    if (write(enc->fd, &total, sizeof(uint32_t)) != sizeof(uint32_t)) /* 1 for VideoTagHeader, 11 for FlvTag */
        return -1;

    return 0;
}

static int flvenc_write_g711(FlvEnc *enc, CodecType codec, void *buf, int size, int64_t ts_ms)
{
    uint8_t tag[128];
    uint8_t *p = tag;

    if (enc->first_ts == -1)
        enc->first_ts = ts_ms;
    int time_ms = ts_ms - enc->first_ts;

    int total_data_size = size + 1;     /* 1 for AudioTagHeader */

    /* Tag */
    p += AV_WB8(p, TagType_Audio);      /* TagType */
    p += AV_WB24(p, total_data_size);    /* DataSize */
    p += AV_WB24(p, time_ms);
    p += AV_WB8(p, (time_ms >> 24) & 0xf);
    p += AV_WB24(p, 0);    /* streamID */

    /* AudioTag */
    AudioTagHeader h;
    memset(&h, 0, sizeof(AudioTagHeader));

    switch (codec) {
    case CodecType_ALAW:
        h.CodecID = 7;
        break;
    case CodecType_MULAW:
        h.CodecID = 8;
        break;
    default:
        printf("unsupport audio codec type %d\n", codec);
    }

    h.SampleRate = 0;
    h.SampleBits = 1;
    h.Stereo     = 0;

    p += AV_Write(p, &h, sizeof(AudioTagHeader));

    if (write(enc->fd, tag, p - tag) != p - tag)
        return -1;

    if (write(enc->fd, buf, size) != size)
        return -1;

    /* PreviousTagSize */
    uint32_t total = htonl(total_data_size + 11);  /* 11 for FlvTag */
    if (write(enc->fd, &total, 4) != 4)
        return -1;

    return 0;
}

static int flvenc_write_h264(FlvEnc *enc, uint8_t *buf_in, int size, int64_t ts_ms, int keyframe)
{
    uint8_t *p = buf_in;
    uint8_t *end = p + size;
    uint8_t *nal_start, *nal_end;

    nal_start = avc_find_startcode(p, end);
    for (;;) {
        while (nal_start < end && !*(nal_start++));
        if (nal_start == end)
            break;

        nal_end = avc_find_startcode(nal_start, end);
        flvenc_write_nal(enc, nal_start, nal_end - nal_start, ts_ms, keyframe);
        nal_start = nal_end;
    }

    return 0;
}

static void print_hex(char *prefix, void *buf, int len)
{
    uint8_t *ptr = buf;
    uint8_t *end = ptr + len;

    if (prefix)
        printf("%s", prefix);

    while (ptr < end) {
        printf("%02x ", *(ptr++));
    }
    printf("\n");
}

int flvenc_write(FlvEnc *enc, av_packet_t *pkt)
{
    if (!enc->sps_len || !enc->pps_len) {
        if (pkt->media != MediaType_Video)
            return 0;

        if (pkt->codec != CodecType_H264)
            return 0;

        if (!pkt->keyframe)
            return 0;

        enc->sps_len = h264_get_sps(pkt->data, pkt->size, enc->sps, sizeof(enc->sps));
        enc->pps_len = h264_get_pps(pkt->data, pkt->size, enc->pps, sizeof(enc->pps));

        print_hex("sps: ", enc->sps, enc->sps_len);
        print_hex("pps: ", enc->pps, enc->pps_len);

        if (!enc->sps_len || !enc->pps_len) {
            printf("sps or pps not found\n");
            return 0;
        }

        enc->first_ts = pkt->time_ms;

        if (flvenc_write_sequence_header(enc, enc->sps, enc->sps_len, enc->pps, enc->pps_len, pkt->time_ms, 1) < 0)
            return -1;
    }

    if (pkt->media == MediaType_Audio) {
        if (pkt->codec != CodecType_ALAW && pkt->codec != CodecType_MULAW)
            return -1;

        return flvenc_write_g711(enc, pkt->codec, pkt->data, pkt->size, pkt->time_ms);
        /* TBD */
    } else if (pkt->media == MediaType_Video) {
        if (pkt->codec != CodecType_H264) {
            printf("unsupported codec type %d\n", pkt->codec);
            return -1;
        }

        return flvenc_write_h264(enc, pkt->data, pkt->size, pkt->time_ms, pkt->keyframe);
    } else {
        printf("unsupported media type %d\n", pkt->media);
        return -1;
    }

    return 0;
}
