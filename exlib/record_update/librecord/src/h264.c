#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "h264.h"

/**
 * \file
 * This file defines functions, structures for handling streams of bits in vlc
 */

typedef struct bs_s
{
    uint8_t *p_start;
    uint8_t *p;
    uint8_t *p_end;

    ssize_t  i_left;    /* i_count number of available bits */
} bs_t;

static inline void bs_init( bs_t *s, const void *p_data, size_t i_data )
{
    s->p_start = (void *)p_data;
    s->p       = s->p_start;
    s->p_end   = s->p_start + i_data;
    s->i_left  = 8;
}

static inline int bs_pos( const bs_t *s )
{
    return( 8 * ( s->p - s->p_start ) + 8 - s->i_left );
}

static inline int bs_eof( const bs_t *s )
{
    return( s->p >= s->p_end ? 1: 0 );
}

static inline uint32_t bs_read( bs_t *s, int i_count )
{
     static const uint32_t i_mask[33] =
     {  0x00,
        0x01,      0x03,      0x07,      0x0f,
        0x1f,      0x3f,      0x7f,      0xff,
        0x1ff,     0x3ff,     0x7ff,     0xfff,
        0x1fff,    0x3fff,    0x7fff,    0xffff,
        0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
        0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
        0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
        0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};
    int      i_shr;
    uint32_t i_result = 0;

    while( i_count > 0 )
    {
        if( s->p >= s->p_end )
        {
            break;
        }

        if( ( i_shr = s->i_left - i_count ) >= 0 )
        {
            /* more in the buffer than requested */
            i_result |= ( *s->p >> i_shr )&i_mask[i_count];
            s->i_left -= i_count;
            if( s->i_left == 0 )
            {
                s->p++;
                s->i_left = 8;
            }
            return( i_result );
        }
        else
        {
            /* less in the buffer than requested */
           i_result |= (*s->p&i_mask[s->i_left]) << -i_shr;
           i_count  -= s->i_left;
           s->p++;
           s->i_left = 8;
        }
    }

    return( i_result );
}

static inline uint32_t bs_read1( bs_t *s )
{
    if( s->p < s->p_end )
    {
        unsigned int i_result;

        s->i_left--;
        i_result = ( *s->p >> s->i_left )&0x01;
        if( s->i_left == 0 )
        {
            s->p++;
            s->i_left = 8;
        }
        return i_result;
    }

    return 0;
}

static inline uint32_t bs_show( bs_t *s, int i_count )
{
    bs_t     s_tmp = *s;
    return bs_read( &s_tmp, i_count );
}

static inline void bs_skip( bs_t *s, ssize_t i_count )
{
    s->i_left -= i_count;

    if( s->i_left <= 0 )
    {
        const int i_bytes = ( -s->i_left + 8 ) / 8;

        s->p += i_bytes;
        s->i_left += 8 * i_bytes;
    }
}

static inline void bs_write( bs_t *s, int i_count, uint32_t i_bits )
{
    while( i_count > 0 )
    {
        if( s->p >= s->p_end )
        {
            break;
        }

        i_count--;

        if( ( i_bits >> i_count )&0x01 )
        {
            *s->p |= 1 << ( s->i_left - 1 );
        }
        else
        {
            *s->p &= ~( 1 << ( s->i_left - 1 ) );
        }
        s->i_left--;
        if( s->i_left == 0 )
        {
            s->p++;
            s->i_left = 8;
        }
    }
}

static inline void bs_align( bs_t *s )
{
    if( s->i_left != 8 )
    {
        s->i_left = 8;
        s->p++;
    }
}

static inline void bs_align_0( bs_t *s )
{
    if( s->i_left != 8 )
    {
        bs_write( s, s->i_left, 0 );
    }
}

static inline void bs_align_1( bs_t *s )
{
    while( s->i_left != 8 )
    {
        bs_write( s, 1, 1 );
    }
}

static ssize_t CreateDecodedNAL( uint8_t *dst, int i_dst, const uint8_t *src, int i_src )
{
    if (!dst || !src || i_dst < i_src)
        return -1;

    const uint8_t *end = &src[i_src];
    uint8_t *p = dst;

    if( p )
    {
        while( src < end )
        {
            if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
                src[2] == 0x03 )
            {
                *p++ = 0x00;
                *p++ = 0x00;

                src += 3;
                continue;
            }
            *p++ = *src++;
        }
    }
    return p - dst;
}

static inline int bs_read_ue( bs_t *s )
{
    int i = 0;

    while( bs_read1( s ) == 0 && s->p < s->p_end && i < 32 )
    {
        i++;
    }
    return( ( 1 << i) - 1 + bs_read( s, i ) );
}

static inline int bs_read_se( bs_t *s )
{
    int val = bs_read_ue( s );

    return val&0x01 ? (val+1)/2 : -(val/2);
}

static int h264_parse_slice(uint8_t *p_buffer, int len)
{
    int slice_type;
    int frame_type;
    bs_t s;

    bs_init(&s, &p_buffer[1], 5);

    /* first_mb_in_slice */
    /* int i_first_mb = */ bs_read_ue(&s);

    /* slice_type */
    slice_type = bs_read_ue(&s);
    switch (slice_type) {
    case 0:
    case 5:
        frame_type = BLOCK_FLAG_TYPE_P;
        break;
    case 1:
    case 6:
        frame_type = BLOCK_FLAG_TYPE_B;
        break;
    case 2:
    case 7:
        frame_type = BLOCK_FLAG_TYPE_I;
        break;
    case 3:
    case 8:        /* SP */
        frame_type = BLOCK_FLAG_TYPE_P;
        break;
    case 4:
    case 9:
        frame_type = BLOCK_FLAG_TYPE_I;
        break;
    default:
        frame_type = 0;
        break;
    }

    return frame_type;
}

static const uint8_t *h264_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
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

const uint8_t *h264_find_startcode(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *out = h264_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1]) out--;
    return out;
}

int h264_get_frame_type(uint8_t *buf, int len)
{
    int i;
    uint32_t code = -1;

    for (i = 0; i < len; i++) {
        code = (code << 8) + buf[i];
        if ((code & 0xffffff00) == 0x100) {
            int type = code & 0x1F;
            if (type >= 1 && type <= 5) {
                return h264_parse_slice(buf + i, 10);
            }
        }
    }

    return 0;
}

#define min(x, y)    ((x) < (y) ? (x) : (y))

ssize_t h264_get_sps(uint8_t *src, size_t src_len, uint8_t *sps, size_t sps_len)
{
    if (!src || !src_len || !sps || sps_len < 32)
        return -1;

    const uint8_t *p = src;
    const uint8_t *end = src + src_len;
    const uint8_t *r, *r1;

    r = h264_find_startcode(p, end);
    while (r < end) {
        while (!*(r++));
        r1 = h264_find_startcode(r, end);

        int type = r[0] & 0x1f;
        if (type == 0x7) {    /* sps */
            int size = min(r1 - r, sps_len);
            memcpy(sps, r, size);
            return size;
        }

        r = r1;
    }

    return 0;
}

ssize_t h264_get_pps(uint8_t *src, size_t src_len, uint8_t *pps, size_t pps_len)
{
    if (!src || !src_len || !pps || pps_len < 32)
        return -1;

    const uint8_t *p = src;
    const uint8_t *end = src + src_len;
    const uint8_t *r, *r1;

    r = h264_find_startcode(p, end);
    while (r < end) {
        while (!*(r++));
        r1 = h264_find_startcode(r, end);

        int type = r[0] & 0x1f;
        if (type == 0x8) {    /* pps */
            int size = min(r1 - r, pps_len);
            memcpy(pps, r, size);
            return size;
        }

        r = r1;
    }

    return 0;
}

#define SPS_MAX (32)
#define PPS_MAX (256)
typedef struct decoder_sys_t
{
    /* Useful values of the Sequence Parameter Set */
    int i_log2_max_frame_num;
    int b_frame_mbs_only;
    int i_pic_order_cnt_type;
    int i_delta_pic_order_always_zero_flag;
    int i_log2_max_pic_order_cnt_lsb;
} decoder_sys_t;

/**
 * video format description
 */
typedef struct video_format_t
{
    unsigned int i_width;                                 /**< picture width */
    unsigned int i_height;                               /**< picture height */
    unsigned int i_x_offset;               /**< start offset of visible area */
    unsigned int i_y_offset;               /**< start offset of visible area */
    unsigned int i_visible_width;                 /**< width of visible area */
    unsigned int i_visible_height;               /**< height of visible area */

    unsigned int i_bits_per_pixel;             /**< number of bits per pixel */

    unsigned int i_sar_num;                   /**< sample/pixel aspect ratio */
    unsigned int i_sar_den;

    unsigned int i_frame_rate;                     /**< frame rate numerator */
    unsigned int i_frame_rate_base;              /**< frame rate denominator */

    uint32_t i_rmask, i_gmask, i_bmask;          /**< color masks for RGB chroma */
    int i_rrshift, i_lrshift;
    int i_rgshift, i_lgshift;
    int i_rbshift, i_lbshift;
} video_format_t;

typedef struct es_format_t
{
    video_format_t video;     /**< description of video format */

    int      i_profile;       /**< codec specific information (like real audio flavor, mpeg audio layer, h264 profile ...) */
    int      i_level;         /**< codec specific information: indicates maximum restrictions on the stream (resolution, bitrate, codec features ...) */
} es_format_t;

typedef struct decoder_s {
    decoder_sys_t sys;
    es_format_t   fmt_out;
} decoder_t;

static void PutSPS(decoder_t *p_dec, uint8_t *buffer, size_t bufsiz)
{
    decoder_sys_t *p_sys = &p_dec->sys;

    uint8_t *pb_dec = NULL;
    int     i_dec = 0;
    bs_t s;
    int i_tmp;
    int i_sps_id;

    pb_dec = alloca(bufsiz - 5);

    i_dec = CreateDecodedNAL( pb_dec, bufsiz - 5, &buffer[5], bufsiz - 5 );
    if (i_dec < 0)
        return;

    bs_init( &s, pb_dec, i_dec );
    int i_profile_idc = bs_read( &s, 8 );
    p_dec->fmt_out.i_profile = i_profile_idc;
    /* Skip constraint_set0123, reserved(4) */
    bs_skip( &s, 1+1+1+1 + 4 );
    p_dec->fmt_out.i_level = bs_read( &s, 8 );
    /* sps id */
    i_sps_id = bs_read_ue( &s );
    if( i_sps_id >= SPS_MAX || i_sps_id < 0 )
    {
        return;
    }

    if( i_profile_idc == 100 || i_profile_idc == 110 ||
        i_profile_idc == 122 || i_profile_idc == 244 ||
        i_profile_idc ==  44 || i_profile_idc ==  83 ||
        i_profile_idc ==  86 )
    {
        /* chroma_format_idc */
        const int i_chroma_format_idc = bs_read_ue( &s );
        if( i_chroma_format_idc == 3 )
            bs_skip( &s, 1 ); /* separate_colour_plane_flag */
        /* bit_depth_luma_minus8 */
        bs_read_ue( &s );
        /* bit_depth_chroma_minus8 */
        bs_read_ue( &s );
        /* qpprime_y_zero_transform_bypass_flag */
        bs_skip( &s, 1 );
        /* seq_scaling_matrix_present_flag */
        i_tmp = bs_read( &s, 1 );
        if( i_tmp )
        {
            int i;
            for( i = 0; i < ((3 != i_chroma_format_idc) ? 8 : 12); i++ )
            {
                /* seq_scaling_list_present_flag[i] */
                i_tmp = bs_read( &s, 1 );
                if( !i_tmp )
                    continue;
                const int i_size_of_scaling_list = (i < 6 ) ? 16 : 64;
                /* scaling_list (...) */
                int i_lastscale = 8;
                int i_nextscale = 8;
                int j;
                for( j = 0; j < i_size_of_scaling_list; j++ )
                {
                    if( i_nextscale != 0 )
                    {
                        /* delta_scale */
                        i_tmp = bs_read_se( &s );
                        i_nextscale = ( i_lastscale + i_tmp + 256 ) % 256;
                        /* useDefaultScalingMatrixFlag = ... */
                    }
                    /* scalinglist[j] */
                    i_lastscale = ( i_nextscale == 0 ) ? i_lastscale : i_nextscale;
                }
            }
        }
    }

    /* Skip i_log2_max_frame_num */
    p_sys->i_log2_max_frame_num = bs_read_ue( &s );
    if( p_sys->i_log2_max_frame_num > 12)
        p_sys->i_log2_max_frame_num = 12;
    /* Read poc_type */
    p_sys->i_pic_order_cnt_type = bs_read_ue( &s );
    if( p_sys->i_pic_order_cnt_type == 0 )
    {
        /* skip i_log2_max_poc_lsb */
        p_sys->i_log2_max_pic_order_cnt_lsb = bs_read_ue( &s );
        if( p_sys->i_log2_max_pic_order_cnt_lsb > 12 )
            p_sys->i_log2_max_pic_order_cnt_lsb = 12;
    }
    else if( p_sys->i_pic_order_cnt_type == 1 )
    {
        int i_cycle;
        /* skip b_delta_pic_order_always_zero */
        p_sys->i_delta_pic_order_always_zero_flag = bs_read( &s, 1 );
        /* skip i_offset_for_non_ref_pic */
        bs_read_se( &s );
        /* skip i_offset_for_top_to_bottom_field */
        bs_read_se( &s );
        /* read i_num_ref_frames_in_poc_cycle */
        i_cycle = bs_read_ue( &s );
        if( i_cycle > 256 ) i_cycle = 256;
        while( i_cycle > 0 )
        {
            /* skip i_offset_for_ref_frame */
            bs_read_se(&s );
            i_cycle--;
        }
    }
    /* i_num_ref_frames */
    bs_read_ue( &s );
    /* b_gaps_in_frame_num_value_allowed */
    bs_skip( &s, 1 );

    /* Read size */
    p_dec->fmt_out.video.i_width  = 16 * ( bs_read_ue( &s ) + 1 );
    p_dec->fmt_out.video.i_height = 16 * ( bs_read_ue( &s ) + 1 );

    /* b_frame_mbs_only */
    p_sys->b_frame_mbs_only = bs_read( &s, 1 );
    p_dec->fmt_out.video.i_height *=  ( 2 - p_sys->b_frame_mbs_only );
    if( p_sys->b_frame_mbs_only == 0 )
    {
        bs_skip( &s, 1 );
    }
    /* b_direct8x8_inference */
    bs_skip( &s, 1 );

    /* crop */
    i_tmp = bs_read( &s, 1 );
    if( i_tmp )
    {
        /* left */
        int crop_left = bs_read_ue( &s );
        /* right */
        int crop_right = bs_read_ue( &s );
        /* top */
        int crop_top = bs_read_ue( &s );
        /* bottom */
        int crop_bottom = bs_read_ue( &s );

        p_dec->fmt_out.video.i_width  -= crop_left * 2 + crop_right  * 2;
        p_dec->fmt_out.video.i_height -= crop_top  * 2 + crop_bottom * 2;
    }

    /* vui */
    i_tmp = bs_read( &s, 1 );
    if( i_tmp )
    {
        /* read the aspect ratio part if any */
        i_tmp = bs_read( &s, 1 );
        if( i_tmp )
        {
            static const struct { int w, h; } sar[17] =
            {
                { 0,   0 }, { 1,   1 }, { 12, 11 }, { 10, 11 },
                { 16, 11 }, { 40, 33 }, { 24, 11 }, { 20, 11 },
                { 32, 11 }, { 80, 33 }, { 18, 11 }, { 15, 11 },
                { 64, 33 }, { 160,99 }, {  4,  3 }, {  3,  2 },
                {  2,  1 },
            };
            int i_sar = bs_read( &s, 8 );
            int w, h;

            if( i_sar < 17 )
            {
                w = sar[i_sar].w;
                h = sar[i_sar].h;
            }
            else if( i_sar == 255 )
            {
                w = bs_read( &s, 16 );
                h = bs_read( &s, 16 );
            }
            else
            {
                w = 0;
                h = 0;
            }

            if( w != 0 && h != 0 )
            {
                p_dec->fmt_out.video.i_sar_num = w;
                p_dec->fmt_out.video.i_sar_den = h;
            }
            else
            {
                p_dec->fmt_out.video.i_sar_num = 1;
                p_dec->fmt_out.video.i_sar_den = 1;
            }
        }
    }
}

int h264_sps_get_dimension(uint8_t *sps, size_t size, int *width, int *height)
{
    decoder_t dec;
    memset(&dec, 0, sizeof(decoder_t));

    PutSPS(&dec, sps, size);

    *width  = dec.fmt_out.video.i_width;
    *height = dec.fmt_out.video.i_height;

    return 0;
}

#if 0
int main(void)
{
    decoder_t dec;
    memset(&dec, 0, sizeof(decoder_t));

    //    uint8_t sps[] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x2a, 0x95, 0xa8, 0x1e, 0x00, 0x89, 0xf9, 0x50 };
    uint8_t sps[] = {
#if 0
#if 1
        0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x1f, 0xad, 0x84, 0x01, 0x0c, 0x20, 0x08, 0x61, 0x00,
        0x43, 0x08, 0x02, 0x18, 0x40, 0x10, 0xc2, 0x00, 0x84, 0x2b, 0x50, 0x28, 0x02, 0xdc, 0x80
#else
        0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x15,
        0xac, 0xc8, 0x60, 0x20, 0x09, 0x6c, 0x04, 0x40,
        0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x07,
        0xa3, 0xc5, 0x8b, 0x67, 0x80,
#endif
#endif
#if 0
        0x00, 0x00, 0x00, 0x01, 0x67, 0x4d, 0x00, 0x29,
        0x9a, 0x66, 0x03, 0xc0, 0x11, 0x3f, 0x2e, 0x02,
        0xd4, 0x04, 0x04, 0x05, 0x00, 0x00, 0x03, 0x03,
        0xe8, 0x00, 0x00, 0xea, 0x60, 0xe8, 0x60, 0x00,
        0x4c, 0x4c, 0x00, 0x00,
#endif
#if 1
        0x00, 0x00, 0x00, 0x01, 0x67, 0x4d, 0x00, 0x29, 0x9a, 0x66, 0x03, 0xc0, 0x11, 0x3f, 0x2e, 0x02,
        0xd4, 0x04, 0x04, 0x05, 0x00, 0x00, 0x03, 0x03, 0xe8, 0x00, 0x00, 0xc3, 0x50, 0xe8, 0x60, 0x00,
        0xfe, 0x38, 0x00, 0x03,
#endif
    };

    PutSPS(&dec, sps, sizeof(sps));

    printf("dec.fmt_out.video.i_width  = %d\n", dec.fmt_out.video.i_width);
    printf("dec.fmt_out.video.i_height = %d\n", dec.fmt_out.video.i_height);

    return 0;
}
#endif
