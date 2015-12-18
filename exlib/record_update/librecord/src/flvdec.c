#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "flvdec.h"

#define min(x, y)    ((x) < (y) ? (x) : (y))

static int flvdec_read_sequence_parameter(FlvDec *dec);

struct FlvDec {
    char filename[512];
    int fd;

    int64_t timestamp;

    uint8_t sps[2048];
    uint16_t sps_len;

    uint8_t pps[1024];
    uint16_t pps_len;

    av_packet_t pkt1;
    int have_saved;

    int first_keyframe;
};

inline uint32_t AV_RB32(void *data)
{
    uint8_t *p = data;
    return (p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]);
}

inline uint32_t AV_RB24(void *data)
{
    uint8_t *p = data;
    return p[0] << 16 | p[1] << 8 | p[2] << 0;
}

inline uint32_t AV_RB16(void *data)
{
    uint8_t *p = data;
    return p[0] << 8 | p[1];
}

inline uint32_t AV_RB8(void *data)
{
    uint8_t *p = data;
    return p[0];
}

static int read_flvheader(FlvDec *dec, FlvHeader *header)
{
    uint8_t data[9];
    if (read(dec->fd, data, sizeof(data)) != sizeof(data))
        return -1;

    header->Signature[0] = data[0];
    header->Signature[1] = data[1];
    header->Signature[2] = data[2];
    header->Version      = data[3];
    header->TypeFlags    = data[4];
    header->DataOffset   = AV_RB32(data + 5);

    return 0;
}

static int valid_header(FlvHeader *header)
{
    if (header->Signature[0] != 'F'
            || header->Signature[1] != 'L'
            || header->Signature[2] != 'V')
        return 0;

    if (header->Version != 1)
        return 0;

    if (header->DataOffset != 9)
        return 0;

    return 1;
}

FlvDec *flvdec_open(char *filename)
{
    FlvDec *dec = (FlvDec *)malloc(sizeof(FlvDec));
    if (!dec)
        return NULL;
    memset(dec, 0, sizeof(FlvDec));

    dec->first_keyframe = 1;

    strncpy(dec->filename, filename, sizeof(dec->filename) - 1);

    dec->fd = open(filename, O_RDONLY);
    if (!dec->fd)
        goto fail;

    FlvHeader header;
    if (read_flvheader(dec, &header) < 0)
        goto fail;

    if (!valid_header(&header))
        goto fail;

    if (lseek(dec->fd, 4, SEEK_CUR) < 0)    /* skip PreviousTagSize0 */
        goto fail;

    dec->timestamp = -1;

    if (flvdec_read_sequence_parameter(dec) < 0)
        goto fail;

    return dec;

fail:
    flvdec_close(dec);
    return NULL;
}

int flvdec_close(FlvDec *dec)
{
    if (dec->fd)
        close(dec->fd);

    free(dec);

    return 0;
}

static int read_tag(FlvDec *dec, FlvTag *tag)
{
    uint8_t data[11];
    if (read(dec->fd, data, sizeof(data)) != sizeof(data))
        return -1;

    tag->TagType   = AV_RB8(data) & 0x1f;
    tag->DataSize  = AV_RB24(data + 1);
    tag->Timestamp = AV_RB24(data + 4) | (AV_RB8(data + 7) << 24);
    tag->StreamID  = AV_RB24(data + 8);

    return 0;
}

static int read_flags(FlvDec *dec, uint8_t *flags)
{
    if (read(dec->fd, flags, sizeof(uint8_t)) != sizeof(uint8_t))
        return -1;
    return 0;
}

static int _flvdec_read(FlvDec *dec, av_packet_t *pkt)
{
    memset(pkt, 0, sizeof(av_packet_t));

    FlvTag tag;
    if (read_tag(dec, &tag) < 0)
        return -1;

    if (dec->timestamp == -1)
        dec->timestamp = tag.Timestamp;

    switch (tag.TagType) {
    case TagType_Audio:     /* audio */
        pkt->media = MediaType_Audio;
        break;
    case TagType_Video:    /* video */
        pkt->media = MediaType_Video;
        break;
    case TagType_Script:     /* data */
        pkt->media = MediaType_Data;
        break;
    default:
        printf("unknown TagType %02x\n", tag.TagType);
        return -1;
    }

    uint8_t flags;
    if (read_flags(dec, &flags) < 0)
        return -1;

    if (pkt->media == MediaType_Video) {
        VideoTagHeader *h = (VideoTagHeader *)&flags;

        if (h->CodecID == 7)
            pkt->codec = CodecType_H264;
        else
            printf("unsupport video CodecID %d\n", h->CodecID);

//        dump_video_tag_header(h);

        switch (h->FrameType) {
        case FrameType_KeyFrame:
            pkt->keyframe = 1;
            break;
        case FrameType_InterFrame:
            pkt->keyframe = 0;
            break;
        }
    } else if (pkt->media == MediaType_Audio) {
        AudioTagHeader *h = (AudioTagHeader *)&flags;
        switch (h->CodecID) {
        case 7:
            pkt->codec = CodecType_ALAW;
            break;
        case 8:
            pkt->codec = CodecType_MULAW;
            break;
        default:
            printf("unsupport audio codec type %d\n", pkt->codec);
        }
    } else if (pkt->media == MediaType_Data) {
        pkt->codec = CodecType_RawData;
    }

    if (read(dec->fd, pkt->data, tag.DataSize - 1) < 0)
        return -1;

    uint32_t PreviousTagSize;

    if (read(dec->fd, &PreviousTagSize, sizeof(PreviousTagSize)) < 0)
        return -1;

    pkt->size    = tag.DataSize - 1;
    pkt->time_ms = tag.Timestamp;

    PreviousTagSize = AV_RB32(&PreviousTagSize);
    if (PreviousTagSize != tag.DataSize + 11)
        return -1;

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

static uint8_t start_code[] = { 0x00, 0x00, 0x00, 0x01 };

int flvdec_read(FlvDec *dec, av_packet_t *pkt)
{
    memset(pkt, 0, sizeof(av_packet_t ));

    while (1) {
        if (!dec->have_saved) {
            if (_flvdec_read(dec, &dec->pkt1) < 0)
                return -1;
        }

        if (!pkt->size) {
            if (dec->pkt1.media == MediaType_Video) {
                if (dec->pkt1.data[0] == 0x02)  /* EOF */
                    return -1;

                pkt->media    = dec->pkt1.media;
                pkt->codec    = dec->pkt1.codec;
                pkt->keyframe = dec->pkt1.keyframe;
                pkt->time_ms  = dec->pkt1.time_ms;

                pkt->size = sizeof(start_code) + dec->pkt1.size - 8;
#if 1
                if (pkt->keyframe && dec->first_keyframe)
                    pkt->size += sizeof(start_code) + dec->sps_len + sizeof(start_code) + dec->pps_len;
#endif

                uint8_t *ptr = pkt->data;

#if 1
                if (pkt->keyframe && dec->first_keyframe) {
                    /* write sps */
                    memcpy(ptr, start_code, sizeof(start_code));
                    ptr += sizeof(start_code);
                    memcpy(ptr, dec->sps, dec->sps_len);
                    ptr += dec->sps_len;

                    /* write pps */
                    memcpy(ptr, start_code, sizeof(start_code));
                    ptr += sizeof(start_code);
                    memcpy(ptr, dec->pps, dec->pps_len);
                    ptr += dec->pps_len;
                    dec->first_keyframe = 0;
                }
#endif

                memcpy(ptr, start_code, sizeof(start_code));
                ptr += sizeof(start_code);

                memcpy(ptr, dec->pkt1.data + 8, dec->pkt1.size - 8);
                ptr += dec->pkt1.size - 8;
            } else if (dec->pkt1.media == MediaType_Audio || dec->pkt1.media == MediaType_Data) {
                memcpy(pkt, &dec->pkt1, sizeof(av_packet_t));
                pkt->size = dec->pkt1.size;
                memcpy(pkt->data, dec->pkt1.data, min(sizeof(pkt->data), dec->pkt1.size));
            }
        } else {
            if (dec->pkt1.media != pkt->media || dec->pkt1.codec != pkt->codec || dec->pkt1.time_ms != pkt->time_ms) {
                dec->have_saved = 1;
                return 0;
            }

            if (dec->pkt1.media == MediaType_Video) {
                if (dec->pkt1.data[0] == 0x02) {
                    return -1;  /* EOF */
                }

                int size = pkt->size + sizeof(start_code) + dec->pkt1.size - 8;
                uint8_t *ptr = pkt->data + pkt->size;

                memcpy(ptr, start_code, sizeof(start_code));
                memcpy(ptr + sizeof(start_code), dec->pkt1.data + 8, dec->pkt1.size - 8);

                pkt->size = size;
            } else if (dec->pkt1.media == MediaType_Audio) {
                int size = pkt->size + dec->pkt1.size;

                uint8_t *ptr = pkt->data + pkt->size;
                memcpy(ptr, dec->pkt1.data, dec->pkt1.size);
                pkt->size = size;
            } else if (dec->pkt1.media == MediaType_Data) {
                int size = pkt->size + dec->pkt1.size;

                uint8_t *ptr = pkt->data + pkt->size;
                memcpy(ptr, dec->pkt1.data, dec->pkt1.size);
                pkt->size = size;
            }
        }

        dec->have_saved = 0;
    }

    return 0;
}

static int flvdec_read_sequence_parameter(FlvDec *dec)
{
    while (1) {
        av_packet_t pkt;

        if (_flvdec_read(dec, &pkt) < 0)
            return -1;

        if (pkt.media != MediaType_Video)
            continue;

        uint8_t *ptr = pkt.data;
        if (*ptr != 0x00)
            continue;

        ptr += 4;    /* skip CompositionTime */
        ptr += sizeof(AVCConfiguration);

        ptr += 1;    /* skip numOfSequenceParameterSets */
        dec->sps_len = AV_RB16(ptr);
        ptr += 2;
        memcpy(dec->sps, ptr, dec->sps_len);
        ptr += dec->sps_len;

        print_hex("sps: ", dec->sps, dec->sps_len);

        ptr += 1;    /* skip numOfPictureParameterSets */
        dec->pps_len = AV_RB16(ptr);
        ptr += 2;
        memcpy(dec->pps, ptr, dec->pps_len);

        print_hex("pps: ", dec->pps, dec->pps_len);

        break;
    }

    return 0;
}
