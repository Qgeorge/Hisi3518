#ifndef __FLV_H__
#define __FLV_H__

#include <stdint.h>

typedef struct {
    uint8_t  Signature[3];
    uint8_t  Version;
    uint8_t  TypeFlags;
    uint32_t DataOffset;
} FlvHeader;

typedef struct {
    uint8_t  TagType;
    uint32_t DataSize;
    uint32_t Timestamp;
    uint32_t StreamID;
} FlvTag;

typedef enum {
    TagType_Audio  = 8,
    TagType_Video  = 9,
    TagType_Script = 18,
} TagType;

typedef enum {
    MediaType_Video,
    MediaType_Audio,
    MediaType_Data,
} MediaType;

typedef enum {
    CodecType_H264,
    CodecType_ALAW,
    CodecType_MULAW,
    CodecType_RawData,
} CodecType;

typedef struct {
    MediaType media;
    CodecType codec;
} MediaDesc;

typedef struct {
    MediaType media;
    CodecType codec;

    int keyframe;

    uint8_t data[1024 * 1024];
    int size;

    int64_t time_ms;
} av_packet_t;

typedef struct {
#ifdef BIG_ENDIAN
    uint8_t CodecID    :4;
    uint8_t FrameType  :4;
#else
    uint8_t FrameType  :4;
    uint8_t CodecID    :4;
#endif
} VideoTagHeader;

typedef struct {
#ifdef BIG_ENDIAN
    uint8_t Stereo     :1;
    uint8_t SampleBits :1;
    uint8_t SampleRate :2;
    uint8_t CodecID    :4;
#else
    uint8_t CodecID    :4;
    uint8_t SampleRate :2;
    uint8_t SampleBits :1;
    uint8_t Stereo     :1;
#endif
} AudioTagHeader;

typedef enum {
    FrameType_KeyFrame          = 1,
    FrameType_InterFrame        = 2,
    FrameType_DispInterFrame    = 3,
    FrameType_GeneratedKeyFrame = 4,
    FrameType_VideoInfoFrame    = 5,
} FrameType;

typedef struct {
    uint8_t configurationVersion;
    uint8_t AVCProfileIndication;
    uint8_t profile_compatibility;
    uint8_t AVCLevelIndication;
#ifdef BIGENDIAN
    uint8_t reserved1                 :6;
    uint8_t lengthSizeMinusOne        :2;
#else
    uint8_t lengthSizeMinusOne        :2;
    uint8_t reserved1                 :6;
#endif
} AVCConfiguration;

void av_dump_packet(av_packet_t *pkt);
void dump_tag(FlvTag *tag);
void dump_video_tag_header(VideoTagHeader *th);
void dump_audio_tag_header(AudioTagHeader *th);
void dump_avcc(AVCConfiguration *avcc);

#endif
