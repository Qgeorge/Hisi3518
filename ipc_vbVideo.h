#ifndef __IPC_VBVIDEO_H__
#define __IPC_VBVIDEO_H__

#include "ipc_hk.h"
#include "hi_type.h"

extern struct HKVProperty video_properties_;

/*************************************
*fun:    the color turned grey
*author: wangshaoshu
**************************************/
int sccStartVideoThread();

/*************************************
*fun:    the color turned grey
*author: wangshaoshu
**************************************/
int ISP_Params_Init();

/*************************************
*fun:    the color turned grey
*author: wangshaoshu
**************************************/
int HISI_SetTurnColor(int bEnable, int chnSum);

/*************************************
*fun:    ISP控制清晰度
*author: wangshaoshu
**************************************/
HI_S32 ISP_Ctrl_Sharpness();

/*************************************
*fun:   设置清晰度
*author: wangshaoshu
**************************************/
int HISI_SetSharpNess(int iVal);

/*************************************************************
*fun:   To set Saturation,Brightness,Contrast,Hue
*author: wangshaoshu
**************************************************************/
int HISI_SetCSCAttr(int nSaturation, int nBrightness,int nContrast, int nHue);

/*************************************************************
*fun:	创建获取主码流的线程
*author: wangshaoshu
**************************************************************/
int CreateVideoThread(void);

/*************************************************************
*fun:	创建获取子码流的线程
*author: wangshaoshu
**************************************************************/

int CreateSubVideoThread();
HI_S32 ISP_SET_AE();

void video_RSLoadObjects();

#endif
