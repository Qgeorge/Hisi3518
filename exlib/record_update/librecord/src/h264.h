#ifndef __H264_H_
#define __H264_H_

/** Intra frame */
#define BLOCK_FLAG_TYPE_I        0x0001
/** Inter frame with backward reference only */
#define BLOCK_FLAG_TYPE_P        0x0002
/** Inter frame with backward and forward reference */
#define BLOCK_FLAG_TYPE_B        0x0004

int h264_get_frame_type(uint8_t *buf, int len);

ssize_t h264_get_sps(uint8_t *src, size_t src_len, uint8_t *sps, size_t sps_len);
ssize_t h264_get_pps(uint8_t *src, size_t src_len, uint8_t *pps, size_t pps_len);

const uint8_t *h264_find_startcode(const uint8_t *p, const uint8_t *end);
int h264_sps_get_dimension(uint8_t *sps, size_t size, int *width, int *height);

#endif /* __H264_H_ */
