#ifndef __IPC_VBVIDEO_H__
#define __IPC_VBVIDEO_H__

#include "ipc_hk.h"
#include "hi_type.h"
#include "utils/HKCmdPacket.h"

extern int g_MotionDetectSensitivity;
extern int g_iCifOrD1;   //main stream channel index.
extern int g_isH264Open;  //main stream open flag.

extern struct HKVProperty video_properties_;

/*************************************
*fun:    the color turned grey
*author: wangshaoshu
**************************************/
int CheckAlarm(int iChannel, int iType, int nReserved, char *cFtpData);

/*************************************
*fun:    the color turned grey
*author: wangshaoshu
**************************************/
void AlarmVideoRecord(bool bAlarm);

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
*fun:   重置参数
*author: wangshaoshu
**************************************************************/
void OnMonSetResetParam();

/*************************************************************
*fun:   云台控制函数
*author: wangshaoshu
**************************************************************/
void OnMonPtz(const char *ev);

/*************************************************************
*fun:   To
*author: wangshaoshu
**************************************************************/
void OnAutoLptspeed(const char* ev);

/*************************************************************
*fun:   设置移动侦测的敏感
*author: wangshaoshu
**************************************************************/
void OnMonSenSitivity(const char *ev);

/*************************************************************
*fun:   获取视频信息
*author: wangshaoshu
**************************************************************/
void sccGetVideoInfo(char buf2[1024*8],int nCmd, int nSubCmd, unsigned int ulParam);

/*************************************************************
*fun:   设备重启是调用
*author: wangshaoshu
**************************************************************/
void OnDevPreset(HKFrameHead *pFrameHead );

/*************************************************************
*fun:   on alarm to do someting
*author: wangshaoshu
**************************************************************/
int sccOnAlarm( int pDvrInst, int nAlarmType, int  nReserved );

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

#endif
