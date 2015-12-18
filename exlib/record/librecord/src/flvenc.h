#ifndef __FLVENC_H__
#define __FLVENC_H__

#include "flv.h"

typedef struct FlvEnc FlvEnc;

FlvEnc *flvenc_open(char *filename, MediaDesc *descs, int nmemb);
int flvenc_close(FlvEnc *enc);

int flvenc_write(FlvEnc *enc, av_packet_t *pkt);

#endif
