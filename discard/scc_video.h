#ifndef __SCC_VIDEO_H__
#define __SCC_VIDEO_H__
//-------------------Init stream list ------------------------
#include "ipc_hk.h"

int sccInitVideoData(int pStreamType);

void sccResetVideData(int pStreamType, VideoDataRecord *mVideoDataBuffer);

int sccGetVideoDataSlave(int pStreamType, char *pVideoData, unsigned int *nSize, unsigned int *iFream, int *iResType, int *iStreamType);

int sccPushStream(int pIPCInst, int pStreamInst, char *poDataBuf, int poDataLen, int iPofType, int iResType, int iStreamType);

#endif
