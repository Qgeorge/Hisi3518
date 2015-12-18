#ifndef __FLVDEC_H__
#define __FLVDEC_H__

#include "flv.h"

typedef struct FlvDec FlvDec;

FlvDec *flvdec_open(char *filename);
int flvdec_close(FlvDec *dec);

int flvdec_read(FlvDec *dec, av_packet_t *pkt);

#endif
