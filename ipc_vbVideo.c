#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "rs.h"
#include "sys.h"
#include "utils/HKMonCmdDefine.h"
#include "utils/HKCmdPacket.h"

#include "osd_region.h"
#include "ipc_hk.h"
#include "ipc_email.h"
#include "ipc_file_sd.h"
#include "ptz.h"
#include "HISI_VDA.h"
#include "sample_comm.h"
#include "TXAudioVideo.h"
#include "ipc_vbAudio.h"
#include "scc_video.h"
#include "utils_biaobiao.h"

#if ENABLE_ONVIF
#include "IPCAM_Export.h"
#endif

#if 0
#define VIDEVID   0
#define VICHNID   0
#define VOCHNID   0
#define VENCCHNID 0
#endif

#define TVP2643_DEV   "/dev/misc/ov2643"
#define TVP99141_DEV  "/dev/misc/nt99141"
#define TVP7725_DEV   "/dev/misc/ov7725"
#define TVP5150_DEV   "/dev/tvp5150"

#define MPEG4           1
#define M_JPEG          2
//#define YUV420        3
//#define MPEG4_JPEG_SYNC 3
#define H263            4
#define H264            4
#define H264_TF         6
#define NULL 0

VENC_CHN_ATTR_S g_stVencChnAttrMain;
VENC_CHN_ATTR_S g_stVencChnAttrSlave;
static int g_Chan = -1;
int g_sccDevCode = -1;

extern VENC_CHN_ATTR_S stVencChnAttr;
extern VENC_ATTR_H264_S stH264Attr;
extern VENC_ATTR_H264_CBR_S stH264Cbr;
extern VENC_ATTR_H264_VBR_S stH264Vbr;

extern VideoDataRecord *hostVideoDataP;//master starem //*mVideoDataBuffer;
extern VideoDataRecord *slaveVideoDataP; //slave starem

extern int g_startCheckAlarm;
extern int g_IRCutCurState;

/********************* IO alarm & email ***********************/
#define PATH_SNAP "/mnt/mmc/snapshot"
HI_S32 g_vencSnapFd = -1; //picture snap fd.
short g_SavePic = 0;
static const HI_U8 g_SOI[2] = {0xFF, 0xD8};//start of image.
static const HI_U8 g_EOI[2] = {0xFF, 0xD9};//end of image.
int imgSize[MAXIMGNUM] = {0};
char g_JpegBuffer[MAXIMGNUM][ALLIMGBUF] = {0}; //picture buffer.
bool get_pic_flag = false; //enable snap pictures.
static int g_SnapCnt = 0; //picture snap counter.
static int g_OpenAlarmEmail = 0; //enable send alarm email.
static bool g_bIsOpenPict = false;
HKEMAIL_T hk_email; //email info struct.
int g_MotionDetectSensitivity = 0;

bool g_bPhCifOrD1=false;
bool g_bCifOrD1 = false;
extern short g_onePtz;
extern int  g_DevVer;
extern short g_DevIndex;
//extern int Ledstatus;
extern int g_irOpen;

extern int g_HK_SensorType;
extern int g_HK_VideoResoType;

extern int g_iFlip;
int g_iCifOrD1=0;  //main stream resolution index.
//static short g_sunCifOrD1=0;
int g_sunCifOrD1=0; //sub stream resolution index.

//HI_S32 SODTIME(HI_VOID);
int g_iMonitorStatus=0; 


HI_S32 s32VencFd=0;
HI_S32 s32JPEGFd=0;
HI_S32 s32TFH264Fd=0;
HI_U32 u32FrameIdx = 0;


VENC_CHN_STAT_S stStat;
VENC_CHN_STAT_S stJpegStat;
VENC_CHN_STAT_S stTFStat;


VENC_CHN_ATTR_S stAttr[5];
VENC_ATTR_H264_S stH264Attr;
VENC_ATTR_H264_S stCifH264Attr;
VENC_ATTR_H264_S st1CifH264Attr;
VENC_ATTR_H264_S stSunCifH264Attr;
VENC_ATTR_H264_S stTfCifH264Attr;
VENC_ATTR_JPEG_S stJpegAttr;


extern volatile int quit_;

static RSObject video_inst_;
static RSObjectIRQEvent ev_irq_;
//static pthread_rwlock_t rwlock_;

extern short g_sdIsOnline;
extern HK_SD_PARAM_ hkSdParam;

/********* Open & Read video stream parameters ***********/

HI_S32 g_s32venchn = 0; //main stream channel.
VENC_CHN VeChn = 2;   //Venc Channel.
VENC_CHN VeSunChn = 1; //sub Venc Channel.
VENC_CHN VeTfChn = 4;
VENC_CHN g_Venc_Chn_M_Cur = 0;  //current main stream VENC_CHN index.
VENC_CHN g_Venc_Chn_S_Cur = 0;  //current sub stream VENC_CHN index.

int g_isH264Open = 0;   //main stream open flag.
int g_isMjpegOpen = 0;  //sub stream open flag.
int g_isTFOpen = 0;
enum { videobuflen = 200*1024 }; //global stream buffer size.

static char g_ReadVideobuf[videobuflen] = {0};
static int g_ReadFlag_H264 = 1;   //main read flag.
static int g_ReadTimes = 3;
static int g_ReadSizes = 0;

static char g_MjpegVideobuf[videobuflen] = {0};
static int g_ReadFlag_Mjpeg = 1;  //sub read flag.
static int g_ReadTimesMjpeg = 3;
static int g_ReadSizesMjpeg = 0;


/***************** Venc Fd ******************/
HI_S32 g_VencFd_Main = 0;    //current main stream fd.
HI_S32 g_VencFd_Sub = 0;     //current sub stream fd.
//HI_S32 g_Resol_Cli_Main = 0; //main stream resolution index for client preview.
//HI_S32 g_Resol_Cli_Sub = 0;  //sub stream resolution index for client preview.

struct timeval TimeoutVal;
HI_S32 g_maxFd = 0;
HI_S32 s32ChnTotal = 1;

bool g_bAlarmThread = false;

static unsigned short garyGurps[6]={6,4,4,4,3,3};
static unsigned short garyPicLevel[6]={5,4,3,2,1,0};

unsigned int g_CurStreamFrameRate = 20;

#define VBR_MAXQP_33    33
#define VBR_MAXQP_38    38
unsigned int g_VbrMaxQq = VBR_MAXQP_33;
unsigned int g_VbrMaxQq_Sub = VBR_MAXQP_33;
static  unsigned int g_MainRataTime = 0;
static  unsigned int g_MainVideoSize = 0;

bool g_bMDXY = false;

int COMM_Get_VencStream_FD(int s32venchn);
void AlarmVideoRecord(bool bAlarm);

void OnCmdPtz( int ev )
{
	printf("this func is bing ptz\n");	
}

static  unsigned short GetMainVideoRate( unsigned int nSize )
{   
	unsigned short nRate = 0;
	g_MainVideoSize += nSize;
	if( g_MainRataTime == 0 )
	{   
		g_MainRataTime = HKMSGetTick();
	}   
	else if( HKMSGetTick() - g_MainRataTime >= 5000 ) //5s
	{
		nRate = g_MainVideoSize/5120;
		g_MainVideoSize = 0;
		g_MainRataTime = HKMSGetTick();
	}   
	return nRate;
}  

static int Set_VBR_Image_QP(int iChnNo, int nQP)
{
	printf("...%s...QP: %d...\n", __func__, nQP);

	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_CHN VencChn = iChnNo;

	s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}

	if (VENC_RC_MODE_H264CBR == stVencChnAttr.stRcAttr.enRcMode)
	{
		SAMPLE_PRT("GetChnAttr: chn[%d], enRcMode:%d, BitRate:%d\n", VencChn, stVencChnAttr.stRcAttr.enRcMode, stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate);
	}
	else if (VENC_RC_MODE_H264VBR == stVencChnAttr.stRcAttr.enRcMode)
	{
		SAMPLE_PRT("GetChnAttr: chn[%d], enRcMode:%d, BitRate:%d\n", VencChn, stVencChnAttr.stRcAttr.enRcMode, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate);
	}

	if (VENC_RC_MODE_H264VBR == stVencChnAttr.stRcAttr.enRcMode)
	{
		stH264Attr.u32Profile = 1;/*0: baseline; 1:MP; 2:HP   ? */
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop            = 90;
		//stVencChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime       = 1;
		//stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate      = 25;
		//stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp          = 10; //24;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp          = nQP; //38; //32;

		//if (VencChn >= 9)
		//    stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = 160; //512; /* average bit rate */
		//else
		//    stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = 32; //256*3; /* average bit rate */

		s32Ret = HI_MPI_VENC_SetChnAttr(VencChn, &stVencChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
			return HI_FAILURE;
		}
		SAMPLE_PRT("VBR, Set BitRate, chn:%d, Profile:%d, Gop:%d, u32MaxQp:%d...\n", VencChn, stH264Attr.u32Profile, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp);
	}

	return 0;
}


///////////////wangshaoshu add/////////////////////
#if 1
/*************************************
 *fun:   set video frame rate
 *author: wangshaoshu
 **************************************/
int HISI_SetFrameRate(int iChnNo, int nFrameRate)
{
	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_CHN VencChn = iChnNo;

	if ((nFrameRate <= 0) || (nFrameRate > 15))
	{
		nFrameRate = 15;
	}

	s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}	
	//SAMPLE_PRT("===> Get FrameRate, chn:%d, enRcMode:%d...\n", VencChn, stVencChnAttr.stRcAttr.enRcMode);

	if (VENC_RC_MODE_H264CBR == stVencChnAttr.stRcAttr.enRcMode)
	{
		g_CurStreamFrameRate = stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate;
		stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = nFrameRate;
		//SAMPLE_PRT("(CBR) Set FrameRate, chn:%d, CbrTargetFrmRate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate);
	}
	else if (VENC_RC_MODE_H264VBR == stVencChnAttr.stRcAttr.enRcMode)
	{
		g_CurStreamFrameRate = stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = nFrameRate;	
		//SAMPLE_PRT("(VBR) Set FrameRate, chn:%d, VbrTargetFrmRate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate);
	}

	s32Ret = HI_MPI_VENC_SetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


int HISI_SetBitRate(int iChnNo, int iBitRate)
{
	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_CHN VencChn = iChnNo;

	s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}
	SAMPLE_PRT("GetChnAttr: chn[%d] u32BitRate:%d, enRcMode:%d\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate, stVencChnAttr.stRcAttr.enRcMode);

	if (VENC_RC_MODE_H264CBR == stVencChnAttr.stRcAttr.enRcMode) //bitrate only set for CBR.
	{
		if (iBitRate > 1024)
		{
			if (g_iCifOrD1 >= 9)
				iBitRate = 1024; //Main stream.
			else
				iBitRate = 32; //48; //64; //sub stream.
		}

		stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = iBitRate;
		s32Ret = HI_MPI_VENC_SetChnAttr(VencChn, &stVencChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
			return HI_FAILURE;
		}
		SAMPLE_PRT("CBR, Set BitRate, chn:%d, BitRate:%d...\n", VencChn, iBitRate);
	}
	else if (VENC_RC_MODE_H264VBR == stVencChnAttr.stRcAttr.enRcMode)
	{
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop            = 90;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime       = 1;
		//stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate      = 25;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp          = 10; //24;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp          = 33; //32;

		if (g_iCifOrD1 >= 9)
			stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = 200; //512; /* average bit rate */
		else
			stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = 32; //256*3; /* average bit rate */


		s32Ret = HI_MPI_VENC_SetChnAttr(VencChn, &stVencChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
			return HI_FAILURE;
		}
		SAMPLE_PRT("VBR, Set BitRate, chn:%d, bitrate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate);
	}

	return HI_SUCCESS;
}


/**************************************
 * fun:    set vbr or cbr
 * author: wangshaoshu
 **************************************/
int HISI_SetRCType(int nChanel, VENC_RC_MODE_E emType)
{	
	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_CHN VencChn = nChanel;
	int nValue = 0;
	//VIDEO_NORM_E enNorm;

	nValue = emType;
	//printf("--------------> nValue:%d, emType:%d <---------------\n", nValue, emType);
	s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}

	if (stVencChnAttr.stRcAttr.enRcMode == nValue)
	{
		SAMPLE_PRT("...Current RCType is: %d, matched, do nothing...\n", nValue);
		return 0;
	}

	if (VENC_RC_MODE_H264CBR == nValue)
	{
		//SAMPLE_PRT("==> Get RCType:CBR, venChn:%d, u32ViFrmRate:%d, TargetFrmRate:%d, u32BitRate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate, stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate, stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate);
		stVencChnAttr.stRcAttr.enRcMode 					   = VENC_RC_MODE_H264CBR;
		stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop            = stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop;
		stVencChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime       = 1; /* stream rate statics time(s) */
		stVencChnAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate      = stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate;
		stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate;
		stVencChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
	}
	else if (VENC_RC_MODE_H264VBR == nValue)
	{
		//SAMPLE_PRT("==> Get RCType:VBR, venChn:%d, ViFrmRate:%d, TargetFrmRate:%d, u32MaxBitRate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate, stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate);
		stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop            = 90; //stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime       = 1;  // 1~16
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate      = stVencChnAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate;
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp          = 10; //24;  //0~51
		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp          = 33; //38; //32;  //1~51

		switch (nChanel)
		{
			case 0:
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = 200; //512; //2*1024;//pVideoInfo->ChannelInfo[0].nBitRate;
				break;
			case 1:
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = 32; //512; //pVideoInfo->ChannelInfo[1].nBitRate;
				break;
defaut:
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = 200; //512; //1*1024;
				break;
		}
	}

	s32Ret = HI_MPI_VENC_SetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


/***********************************************
 *fun : to turn Type video input formats
 *param:  0:PAL, 1:NTSC, 2:AUTO //auto mode not support
 *author:wangshaoshu
 *************************************************/
int  HISI_SetLocalDisplay(VIDEO_NORM_E enNorm)			
{
	int iChnNo = 0;
	HI_S32 s32Ret = 0;
	VENC_CHN VencChn;
	VENC_CHN_ATTR_S stVencChnAttr;

	//VIDEO_ENCODING_MODE_AUTO //auto mode not support now
	if ( (VIDEO_ENCODING_MODE_PAL == enNorm) || (VIDEO_ENCODING_MODE_NTSC == enNorm) )
	{
		//not to set jpeg stream param
		for (iChnNo = 0; iChnNo < 2; iChnNo++)
		{
			VencChn = iChnNo;
			s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
			if (s32Ret != HI_SUCCESS)
			{
				SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
				return HI_FAILURE;
			}	

			if (stVencChnAttr.stRcAttr.enRcMode == enNorm)
			{
				SAMPLE_PRT("...Current chn:%d encoding mode is:%d, matched, do nothing...\n", VencChn, enNorm);
				return 0; 
			}

			if (VENC_RC_MODE_H264CBR == stVencChnAttr.stRcAttr.enRcMode)
			{
				stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop       = (VIDEO_ENCODING_MODE_PAL == enNorm)?20:25;
				stVencChnAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == enNorm)?20:25;

				//if (stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate > stVencChnAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate)
				//	stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = stVencChnAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate;
				if (stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate > 15)
					stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = 15;

				SAMPLE_PRT("Set Encoding Mode: CBR, venChn:%d, enRcMode:%d, enNorm:%d, TargetFrmRate:%d\n", VencChn, stVencChnAttr.stRcAttr.enRcMode, enNorm, stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate);
			}
			else if (VENC_RC_MODE_H264VBR == stVencChnAttr.stRcAttr.enRcMode)
			{
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop       = (VIDEO_ENCODING_MODE_PAL == enNorm)?20:25;
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == enNorm)?20:25;

				//if (stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate > stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate)
				//	stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = stVencChnAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate;
				if (stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate > 15)
					stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = 15;

				SAMPLE_PRT("Set Encoding Mode: VBR, venChn:%d, enRcMode:%d, enNorm:%d, TargetFrmRate:%d\n", VencChn, stVencChnAttr.stRcAttr.enRcMode, enNorm, stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate);
			}

			s32Ret = HI_MPI_VENC_SetChnAttr(VencChn, &stVencChnAttr);
			if (s32Ret != HI_SUCCESS)
			{
				SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
				return HI_FAILURE;
			}
		}
	}
	return HI_SUCCESS;
}


/*************************************************************
 *fun:   To set Saturation,Brightness,Contrast,Hue
 *author: wangshaoshu
 **************************************************************/
int HISI_SetCSCAttr(int nSaturation, int nBrightness,int nContrast, int nHue)
{
	HI_S32 nRet = -1;
	VI_DEV ViDev = 0;
	VI_CSC_ATTR_S stCSCAttr;

	nRet = HI_MPI_VI_GetCSCAttr(ViDev, &stCSCAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_GetCSCAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stCSCAttr.enViCscType = VI_CSC_TYPE_709;//HD
	if (nSaturation != 0)
	{
		nSaturation = nSaturation*100/64; //client value turn to hisi value
		stCSCAttr.u32SatuVal  = nSaturation;
	}

	if (nBrightness != 0)
	{
		nBrightness = nBrightness*100/64;
		stCSCAttr.u32LumaVal  = nBrightness;
	}

	if (nContrast != 0)
	{
		nContrast = nContrast*100/64;
		stCSCAttr.u32ContrVal = nContrast;
	}

	if (nHue != 0)
	{
		nHue = nHue*100/64;
		stCSCAttr.u32HueVal   = nHue;
	}

#if 0
	stCSCAttr.u32LumaVal  = nBrightness;
	stCSCAttr.u32ContrVal = nContrast;
	stCSCAttr.u32HueVal   = nHue;
	stCSCAttr.u32SatuVal  = nSaturation;
#endif

	nRet = HI_MPI_VI_SetCSCAttr(ViDev, &stCSCAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_SetCSCAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	SAMPLE_PRT("Set CSC Attr, nSaturation:%d, nBrightness:%d, nContrast:%d, nHue:%d...\n", nSaturation, nBrightness, nContrast, nHue);
	return HI_SUCCESS;
}



/*************************************
 *fun:   To set SharpNess
 *author: wangshaoshu
 **************************************/
int HISI_SetSharpNess(int iVal)	
{
	HI_S32 s32Ret; 
	ISP_SHARPEN_ATTR_S  stSharpenAttr;

	s32Ret = HI_MPI_ISP_GetSharpenAttr(&stSharpenAttr); 
	if (HI_SUCCESS != s32Ret) 
	{ 
		SAMPLE_PRT("HI_MPI_ISP_GetSharpenAttr err:0x%x\n", s32Ret); 
		return HI_FAILURE; 
	}

	stSharpenAttr.bEnable = HI_TRUE;
	stSharpenAttr.bManualEnable = HI_TRUE;

	iVal = iVal*255/64; //client value turn to hisi value
	stSharpenAttr.u8StrengthTarget = iVal;
	s32Ret = HI_MPI_ISP_SetSharpenAttr(&stSharpenAttr); 
	if (HI_SUCCESS != s32Ret) 
	{ 
		SAMPLE_PRT("HI_MPI_ISP_SetSharpenAttr err:0x%x\n", s32Ret); 
		return HI_FAILURE; 
	}

	SAMPLE_PRT("Set SharpNess: %d \n", iVal);
	return HI_SUCCESS;
}

/*************************************
 *fun:    the color turned grey
 *author: wangshaoshu
 **************************************/
int	HISI_SetTurnColor(int bEnable, int chnSum)
{
	HI_U32 s32Ret;
	VENC_GRP VeGroup;
	GROUP_COLOR2GREY_CONF_S Color2GreyGrp;
	int nChn = 0;

	for (nChn = 0; nChn < chnSum; nChn++) //Including jgeg stream does not support the color turn grey
	{
		VeGroup = nChn;
		s32Ret = HI_MPI_VENC_GetColor2GreyConf(&Color2GreyGrp);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("__________GetColor2GreyConf failed!_________\n");
			return HI_FAILURE;
		}

		if (Color2GreyGrp.bEnable != HI_TRUE)
		{
			Color2GreyGrp.bEnable = bEnable;
			switch(nChn)
			{
				case 0:
					Color2GreyGrp.u32MaxHeight = 720;
					Color2GreyGrp.u32MaxWidth =  1280;
					break;
				case 1:
				case 2:
				case 4:
					Color2GreyGrp.u32MaxHeight = 480;
					Color2GreyGrp.u32MaxWidth =  640;	
					break;
				case 3:
					Color2GreyGrp.u32MaxHeight = 240;
					Color2GreyGrp.u32MaxWidth =  320;
					break;						
				default:
					break;
			}

			s32Ret = HI_MPI_VENC_SetColor2GreyConf(&Color2GreyGrp);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("__________SetColor2GreyConf failed!_______\n");
				return HI_FAILURE;
			}
		}
		Color2GreyGrp.bEnable = bEnable;

		switch(nChn)
		{
			case 0:
				Color2GreyGrp.u32MaxHeight = 720;
				Color2GreyGrp.u32MaxWidth =  1280;	
				break;
			case 1:
			case 2:
			case 4:
				Color2GreyGrp.u32MaxHeight = 480;
				Color2GreyGrp.u32MaxWidth =  640;
				break;
			case 3:
				Color2GreyGrp.u32MaxHeight = 240;
				Color2GreyGrp.u32MaxWidth =  320;
				break;						
			default:
				break;
		}

		s32Ret = HI_MPI_VENC_SetGrpColor2Grey(VeGroup,&Color2GreyGrp);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("__________SetGrpColor2Grey failed!_________\n");
			return HI_FAILURE;
		}			
	}

	SAMPLE_PRT("HISI_SetTurnColor OK!_________\n");
	return HI_SUCCESS;
}

/************************************
 *fun:   Set Mirror
 *author: wangshaoshu
 *************************************/
int HISI_SetMirror(int bFlipH)
{
	HI_S32 s32Ret; 
	VI_CHN ViChn = 0; 
	VI_CHN_ATTR_S stChnAttr;

	s32Ret = HI_MPI_VI_GetChnAttr(ViChn, &stChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_GetChnAttr failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	stChnAttr.bMirror = bFlipH;
	s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_SetChnAttr failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	SAMPLE_PRT("Set Mirror: %d\n", bFlipH);
	return HI_SUCCESS;
}

/************************************
 *fun:   Set Flip 
 *author: wangshaoshu
 *************************************/
int HISI_SetFlip(int bFlipV)
{
	HI_S32 s32Ret; 
	VI_CHN ViChn = 0; 
	VI_CHN_ATTR_S stChnAttr;

	s32Ret = HI_MPI_VI_GetChnAttr(ViChn, &stChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_GetChnAttr failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	stChnAttr.bFlip = bFlipV;
	s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_SetChnAttr failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	SAMPLE_PRT("Set Flip: %d\n", bFlipV);
	return HI_SUCCESS;
}


/************************************
 *fun:   Set quality level
 *author: wangshaoshu
 *************************************/
int  HISI_SetQLevel(int nChanel, int nQuality)
{	
	HI_S32 s32Ret;
	HI_S32 qualityLevel = 0;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_CHN 	VencChn = nChanel;

	unsigned int qp_max[4][2] =
	{
		{34, 51}, // Quality 1
		{33, 51}, // Quality 2
		{28, 50}, // Quality 3  default
		{24, 50}, // Quality 4  best
	};

	unsigned int qp_sub[4][2] =
	{
		{34, 51}, // Quality 1
		{33, 51}, // Quality 2
		{28, 50}, // Quality 3  default
		{24, 30}, // Quality 4  best
	};

	s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
		return HI_FAILURE;
	}	

	qualityLevel = nQuality;
	qualityLevel = (qualityLevel > 3) ? 3 : qualityLevel;
	qualityLevel = (qualityLevel < 0) ? 0 : qualityLevel;

	stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = (0 == nChanel) ? qp_max[qualityLevel][0] : qp_sub[qualityLevel][0];
	stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = (0 == nChanel) ? qp_max[qualityLevel][1] : qp_sub[qualityLevel][1];	
	//stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = (0 == nChanel) ? 2048:1024;	
	stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = (0 == nChanel) ? 1024:64;
	s32Ret = HI_MPI_VENC_SetChnAttr(VencChn,&stVencChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", \
				VencChn, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/*************************************
 *fun:   AntiFog
 *author: wangshaoshu
 **************************************/
int  HISI_SetAntiFog()
{
	HI_S32 s32Ret;
	ISP_ANTIFOG_S stAntiFog;

	s32Ret = HI_MPI_ISP_GetAntiFogAttr(&stAntiFog);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAntiFogAttr failed!\n");
		return FALSE;
	}	
	//printf("KKKKKKKKKKKKKKbEnable=%d,u8Strength =%d\n",stAntiFog.bEnable,stAntiFog.u8Strength);

	stAntiFog.bEnable = HI_TRUE;
	stAntiFog.u8Strength = 255; //0~255
	s32Ret = HI_MPI_ISP_SetAntiFogAttr(&stAntiFog);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAntiFogAttr failed!\n");
		return FALSE;
	}

	printf("--------------- Set AntiFog OK ---------------\n");
	return TRUE;
}


//////////Isp param init//////////////////////////////////////////////////////
#if 0
HI_S32 VISP_SetAE(int nExposureType, int nExposureValue, int nDigitalGain)
{
	int nRet = -1;
	ISP_OP_TYPE_E enExpType = OP_TYPE_AUTO;
	enExpType = (nExposureType == 0) ? OP_TYPE_AUTO : OP_TYPE_MANUAL;

	nRet = HI_MPI_ISP_SetExposureType(enExpType);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetExposureType failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	if (enExpType == OP_TYPE_MANUAL)
	{
		ISP_ME_ATTR_S stMEAttr;        
		nRet = HI_MPI_ISP_GetMEAttr(&stMEAttr);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_GetMEAttr failed with %#x!\n", nRet);
			return HI_FAILURE;
		}

		stMEAttr.u32ExpLine = nExposureValue;        
		nRet = HI_MPI_ISP_SetMEAttr(&stMEAttr);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetMEAttr failed with %#x!\n", nRet);
			return HI_FAILURE;
		}
	}

	ISP_AE_ATTR_EX_S stAEAttrEx = {0};
	nRet = HI_MPI_ISP_GetAEAttrEx(&stAEAttrEx);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAEAttrEx failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stAEAttrEx.u32SystemGainMax = 1024*15;//1024*20;
	stAEAttrEx.u32ExpTimeMax= 1644;
	//stAEAttrEx.u8ExpCompensation = 200;  //add ±³¹â²¹³¥Á¿
	//stAEAttrEx.enAEMode = AE_MODE_LOW_NOISE;
	stAEAttrEx.enAEMode = AE_MODE_FRAME_RATE;
	stAEAttrEx.u8ExpStep = 80;
	stAEAttrEx.enFrameEndUpdateMode = 2;
	HI_MPI_ISP_SetAEAttrEx(&stAEAttrEx);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetAEAttrEx failed with %#x!\n", nRet);
		return HI_FAILURE;
	}    

	return HI_SUCCESS;
}
#else
HI_S32 VISP_SetAE(int nAEMode)
{
	int nRet = -1;
	ISP_OP_TYPE_E enExpType = OP_TYPE_AUTO; //OP_TYPE_MANUAL;

	nRet = HI_MPI_ISP_SetExposureType(enExpType);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetExposureType failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	if (enExpType == OP_TYPE_MANUAL)
	{
		ISP_ME_ATTR_S stMEAttr;        
		nRet = HI_MPI_ISP_GetMEAttr(&stMEAttr);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_GetMEAttr failed with %#x!\n", nRet);
			return HI_FAILURE;
		}

		stMEAttr.u32ExpLine = 100;        
		nRet = HI_MPI_ISP_SetMEAttr(&stMEAttr);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetMEAttr failed with %#x!\n", nRet);
			return HI_FAILURE;
		}
	}

	ISP_AE_ATTR_EX_S stAEAttrEx = {0};
	nRet = HI_MPI_ISP_GetAEAttrEx(&stAEAttrEx);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAEAttrEx failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stAEAttrEx.u32SystemGainMax = 1024*15;//1024*20;
	stAEAttrEx.u32ExpTimeMax= 1644;
	//stAEAttrEx.u8ExpCompensation = 200;  //add ±³¹â²¹³¥Á¿
	stAEAttrEx.u8ExpStep = 80;
	stAEAttrEx.enFrameEndUpdateMode = 2;

	//printf("...%s..........g_IRCutCurState: %d...........\n", __func__, g_IRCutCurState);
	if (1 == nAEMode) //ircut night mode.
	{
		stAEAttrEx.enAEMode = AE_MODE_LOW_NOISE;
	}
	else if (0 == nAEMode) //ircut day mode.
	{
		stAEAttrEx.enAEMode = AE_MODE_FRAME_RATE;
	}

	HI_MPI_ISP_SetAEAttrEx(&stAEAttrEx);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetAEAttrEx failed with %#x!\n", nRet);
		return HI_FAILURE;
	}    

	return HI_SUCCESS;
}
#endif


HI_S32 VISP_SetAI(HI_S32 bIRISEnable)
{
	HI_S32 nRet = -1;

	ISP_AI_ATTR_S stAIAttr;
	nRet = HI_MPI_ISP_GetAIAttr(&stAIAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAIAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stAIAttr.bIrisCalEnable = (HI_TRUE == bIRISEnable) ? HI_TRUE : HI_FALSE;
	nRet = HI_MPI_ISP_SetAIAttr(&stAIAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetAIAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 VISP_SetAWB(int nAWBType, int nRGain, int nGGain, int nBGain)
{
	int nRet = -1;

	ISP_ADV_AWB_ATTR_S stAdvAWBAttr;    
	nRet = HI_MPI_ISP_GetAdvAWBAttr(&stAdvAWBAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAdvAWBAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	if (nAWBType == 1) //outdoor
	{
		nRet = HI_MPI_ISP_SetWBType(OP_TYPE_AUTO);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetWBType failed with %#x!\n", nRet);
			return HI_FAILURE;
		}

		nRet = HI_MPI_ISP_SetAWBAlgType(AWB_ALG_ADVANCE);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetAWBAlgType failed with %#x!\n", nRet);
			return HI_FAILURE;
		}        

		stAdvAWBAttr.bAccuPrior = 0;
		stAdvAWBAttr.u8Tolerance = 4;
		stAdvAWBAttr.u16CurveLLimit = 0xE0;
		stAdvAWBAttr.u16CurveRLimit = 0x120;
		nRet = HI_MPI_ISP_SetAdvAWBAttr(&stAdvAWBAttr);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetAdvAWBAttr failed with %#x!\n", nRet);
			return HI_FAILURE;
		}
	}
	else if (nAWBType == 2) //indoor
	{
		nRet = HI_MPI_ISP_SetWBType(OP_TYPE_AUTO);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetWBType failed with %#x!\n", nRet);
			return HI_FAILURE;
		}

		nRet = HI_MPI_ISP_SetAWBAlgType(AWB_ALG_ADVANCE);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetAWBAlgType failed with %#x!\n", nRet);
			return HI_FAILURE;
		}

		stAdvAWBAttr.bAccuPrior = 1;
		stAdvAWBAttr.u8Tolerance = 8;
		stAdvAWBAttr.u16CurveLLimit = 0xE0;
		stAdvAWBAttr.u16CurveRLimit = 0x130;
		nRet = HI_MPI_ISP_SetAdvAWBAttr(&stAdvAWBAttr);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetAdvAWBAttr failed with %#x!\n", nRet);
			return HI_FAILURE;
		}
	}    
	else
	{
		nRet = HI_MPI_ISP_SetWBType(OP_TYPE_AUTO);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetWBType failed with %#x!\n", nRet);
			return HI_FAILURE;
		}

		nRet = HI_MPI_ISP_SetAWBAlgType(AWB_ALG_ADVANCE);
		if (nRet != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_ISP_SetAWBAlgType failed with %#x!\n", nRet);
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}

HI_S32 VISP_SetDRC(int enable)
{
	int nRet = -1;
	int enable_bak = -1;

	if (enable == enable_bak)
	{
		return HI_SUCCESS;
	}

	enable_bak = enable;	
	ISP_DRC_ATTR_S stDRC;
	nRet = HI_MPI_ISP_GetDRCAttr(&stDRC);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetDRCAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stDRC.bDRCEnable = (enable != 0) ? HI_TRUE : HI_FALSE;
	stDRC.bDRCManualEnable = HI_TRUE;
	stDRC.u32StrengthTarget = 122;
	nRet = HI_MPI_ISP_SetDRCAttr(&stDRC);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetDRCAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 VISP_SetDenoise()
{
	int nRet = -1;

	ISP_DENOISE_ATTR_S stDenoiseAttr;
	nRet = HI_MPI_ISP_GetDenoiseAttr(&stDenoiseAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetDenoiseAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	} 

#if 1 // 1
	stDenoiseAttr.u8ThreshMax = 84;
	stDenoiseAttr.u8SnrThresh[0] = 35;
	stDenoiseAttr.u8SnrThresh[1] = 40;
	stDenoiseAttr.u8SnrThresh[2] = 43;
	stDenoiseAttr.u8SnrThresh[3] = 65;//60;//70;//53;
	stDenoiseAttr.u8SnrThresh[4] = 70;//65;//80;//72;//63;
	stDenoiseAttr.u8SnrThresh[5] = 72;//70;//85;//70;
	stDenoiseAttr.u8SnrThresh[6] = 74;//75;//90;//75;
	stDenoiseAttr.u8SnrThresh[7] = 80;//80;//95;//79;
#endif

	stDenoiseAttr.bEnable = HI_TRUE;
	stDenoiseAttr.bManualEnable = HI_FALSE;
	nRet = HI_MPI_ISP_SetDenoiseAttr(&stDenoiseAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetDenoiseAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 VISP_SetShading(int bEnable)
{
	static int ShadeEnable = -1;	
	HI_S32 nRet = HI_FAILURE;

	if(ShadeEnable == bEnable)
	{
		return 0;
	}
	ShadeEnable = bEnable;

	ISP_SHADING_ATTR_S stShadingAttr;
	nRet = HI_MPI_ISP_GetShadingAttr(&stShadingAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetShadingAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stShadingAttr.Enable = (HI_TRUE == bEnable) ? HI_TRUE : HI_FALSE;
	nRet = HI_MPI_ISP_SetShadingAttr(&stShadingAttr);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_SetShadingAttr failed with %#x!\n", nRet);
		return HI_FAILURE;
	}		

	return HI_SUCCESS;
}


HI_S32 VISP_SetAntiFog()
{
	HI_S32 s32Ret;
	ISP_ANTIFOG_S stAntiFog;
	s32Ret = HI_MPI_ISP_GetAntiFogAttr(&stAntiFog);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAntiFogAttr failed!\n");
		return HI_FAILURE;
	}

	stAntiFog.bEnable = HI_TRUE;
	stAntiFog.u8Strength = 255; //0~255
	s32Ret = HI_MPI_ISP_SetAntiFogAttr(&stAntiFog);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetAntiFogAttr failed!\n");
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

HI_S32 VISP_SetVPSSGrpParam(int AnalogGain)
{
	VPSS_GRP VpssGrp = 0;
	VPSS_GRP_PARAM_S stVpssParam;
	int level = 0;
	int nMode = 0;
	static int level_bak = -1;
	int SfStrength[4] 	= {32,40,64,96};//{32,40,64,80};
	int TfStrength[4] 	= {8,16,16,24};//{8,16,16,20};
	int ChromaRange[4] 	= {8,16,16,48};//{8,16,16,40};
	int nRet = -1;

	if(AnalogGain >= 1024*18)
	{
		level = 3;
	}
	else if(AnalogGain >= 1024*12)
	{
		level = 2;
	}
	else if(AnalogGain >= 1024*3)
	{
		level = 1;
	}
	else
	{
		level = 0;
	}
	//printf("...%s...level:%d, level_bak:%d, AnalogGain:%d...\n", __func__, level, level_bak, AnalogGain);

	if(level == level_bak)
	{
		return 0;
	}
	level_bak = level;

	//printf("8888888888888888888888888888-----level=%d,AnalogGain=%d\n",level,AnalogGain);
	nRet = HI_MPI_VPSS_GetGrpParam(VpssGrp, &stVpssParam);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	stVpssParam.u32SfStrength = SfStrength[level];
	stVpssParam.u32TfStrength = TfStrength[level];
	stVpssParam.u32ChromaRange = ChromaRange[level];
	nRet = HI_MPI_VPSS_SetGrpParam(VpssGrp, &stVpssParam);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 ISP_Ctrl_Sharpness()
{
	int nRet = 0;
	ISP_INNER_STATE_INFO_EX_S stInnerStateInfoEx = {0};    

	nRet = HI_MPI_ISP_QueryInnerStateInfoEx(&stInnerStateInfoEx);
	if (nRet != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_QueryInnerStateInfoEx failed with %#x!\n", nRet);
		return HI_FAILURE;
	}

	VISP_SetVPSSGrpParam(stInnerStateInfoEx.u32AnalogGain);

	return HI_SUCCESS;
}


int ISP_Params_Init()
{
	HI_S32 s32Ret = 0;
	HI_S32 bEnable = TRUE;

#if 0
	/////////AE
	int nExposureType = 0;
	int nExposureValue = 100;
	int nDigitalGain = 100;
	s32Ret = VISP_SetAE(nExposureType, nExposureValue, nDigitalGain);
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("VISP_SetAE failed with %#x!\n",  s32Ret);
		return FALSE;
	}
#else
	/////////AE
	//s32Ret = VISP_SetAE(0);
	s32Ret = VISP_SetAE(1);
	if (HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("VISP_SetAE failed with %#x!\n",  s32Ret);
		return FALSE;
	}
#endif

	////////AI
	HI_S32 bIRISEnable = FALSE;
	s32Ret = VISP_SetAI(bIRISEnable);
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("VISP_SetAI failed with %#x!\n",  s32Ret);
		return FALSE;		
	}

	///////AWB
	int nAWBType = 2; //2; //1;
	int nRGain, nGGain, nBGain;
	s32Ret = VISP_SetAWB(nAWBType, nRGain, nGGain, nBGain);
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("[VISP_SetAWB] failed with %#x!\n",  s32Ret);
		return FALSE;		
	}

	//////DRC
	int bDRC = FALSE;
	s32Ret = VISP_SetDRC(bDRC);
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("[VISP_SetDRC] failed with %#x!\n",  s32Ret);
		return FALSE;		
	}

	//////Denoise
	//int ThreshValue = 50;
	s32Ret = VISP_SetDenoise();
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("[VISP_SetDenoise] failed with %#x!\n",  s32Ret);
		return FALSE;				
	}

	//////Shading
	int bShading = TRUE;
	s32Ret = VISP_SetShading(bShading);
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("[VISP_SetShading] failed with %#x!\n",  s32Ret);
		return FALSE;		
	}

	//////AntiFog
	s32Ret = VISP_SetAntiFog();
	if(HI_FAILURE == s32Ret)
	{
		SAMPLE_PRT("[VISP_SetAntiFog] failed with %#x!\n",  s32Ret);
		return FALSE;		
	}

	printf("ISP------------------------------SET ok\n");
	return TRUE;
}

#if 0
int HISI_InitConfig()
{
	HISI_SetSharpNess(40);	
	HISI_SetAntiFog();

	return TRUE;
}

int HISI_GetMainChanNum()
{
	return g_s32venchn;
}

int HISI_GetSubChanNum()
{
	return VeSunChn;
}

int HISI_GetM_CurChanNum()
{
	return g_Venc_Chn_M_Cur;
}

int HISI_SetM_CurChanNum(int chanNum)
{
	g_Venc_Chn_M_Cur = chanNum;
	return g_Venc_Chn_M_Cur;
}

int HISI_GetM_VencFd()
{
	return g_VencFd_Main;
}

int HISI_SetM_VencFd(int fd)
{
	g_VencFd_Main = fd;
	return g_VencFd_Main;
}
///////////////////////////////////////////////////

static unsigned short Rate2Gurps( unsigned short nRate )
{
	unsigned short nInde = nRate/100;
	if( nInde>4 ) nInde = 4;
	return garyGurps[nInde]; 
}

static unsigned short Rate2PicLevel( unsigned short nRate )
{
	unsigned short nInde = nRate/200;
	if( nInde>5 ) nInde = 5;
	return garyPicLevel[nInde]; 
}

#endif
#endif
struct HKVProperty video_properties_;




#define ALARMTIME 6000*3
static int Getms()
{
	struct timeval tv;
	int ms = 0;

	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	return ms;
}

static void raise_alarm(const char *res, int vfmt)
{
	char prop[256] = {0};
	char cont[32] = {0};
	int len = sprintf(cont, HK_KEY_FROM"=%s;", getenv("USER"));
	unsigned int nLevel = 1 << 8; //(vfmt==MPEG4 ? 2 : 3) << 8;
	sprintf(prop, HK_KEY_EVENT"="HK_EVENT_ALARM";"HK_KEY_SUBRESOURCE"=%s;FD=%d;Flag=%u;", res, vfmt, nLevel);
	ev_irq_( &video_inst_, prop, cont, len );

	//hk_IOAlarm();
}

static void raise_alarm_server( int iType, int nReserved,char *cFtpData)
{
	char buf[256] = {0};
	int iLen = 0;
	Dict *DictPacket = DictCreate(0, 0);
	DictSetInt( DictPacket, HK_KEY_MAINCMD, HK_AS_NOTIFY );
	DictSetInt( DictPacket, HK_KEY_SUBTYPE, 1 );
	DictSetInt(DictPacket, HK_KEY_CHANNEL, nReserved );
	DictSetInt( DictPacket, HK_KEY_ALERT_TYPE, iType );
	DictSetStr( DictPacket, HK_KEY_EVENT, "alarm" );

	if( NULL !=cFtpData)
	{
		printf("Wan Alarm......=%s..\n", cFtpData);
		DictSetStr(DictPacket, HK_KEY_FTPSERVER, cFtpData );
	}

	time_t rawtime;
	struct tm *timeinfo;
	char buffer[80] = {0};
	time( &rawtime );
	timeinfo = localtime( &rawtime );
	strftime(buffer, 80, "%Y-%m-%d %X", timeinfo);

	DictSetStr( DictPacket, HK_KEY_TIME, buffer );
	DictSetStr( DictPacket, HK_KEY_FROM, getenv("USER") );

	iLen = DictEncode(buf, sizeof(buf), DictPacket);
	buf[iLen] = '\0';
	//NetSend( HK_KEY_MONSERVER, buf, iLen );
	DictDestroy(DictPacket);
}


/****************************************************************
 * func: save snapshot picture into SD card for test.
 ***************************************************************/
int SaveJPEGPicsOnSDCard(char *pJpegData, int JpegSize, int num)
{
	if ((NULL == pJpegData) || (JpegSize <= 0))
	{
		printf("Save JPEG Pictures On SDCard failed, exit.\n");
		return HI_FAILURE;
	}
	HK_DEBUG_PRT("######## JPEG size = %d......num = %d #########\n", JpegSize, num);

	FILE *pFile_Snap = NULL;
	char acFile[32]  = {0};

	//sprintf(acFile, "snap_%d.jpg", num);
	sprintf(acFile, PATH_SNAP"/snap_%d.jpg", num);

	pFile_Snap = fopen(acFile, "wb");
	if (NULL == pFile_Snap)
	{
		SAMPLE_PRT("open file for save snap failed with: %d, %s.\n", errno, strerror(errno));
		return HI_FAILURE;
	}

	if (fwrite(pJpegData, JpegSize, 1, pFile_Snap) != 1)//write image.
	{
		SAMPLE_PRT("save snapshort picture failed!\n");
		fclose(pFile_Snap);
		return HI_FAILURE;
	}

	fflush(stdout);
	fclose(pFile_Snap);
	return HI_SUCCESS;
}


/************************************************************
 * func: save picture data into g_JpegBuffer.
 ***********************************************************/
HI_S32 SampleSaveJpegStream(VENC_STREAM_S *pstStream, int iNum)
{
	VENC_PACK_S *pstData = NULL;
	HI_U32  i = 0;
	unsigned int nPos = 0;

	/*there is no need to add 'head' for JPEG image*/
	//memcpy( g_JpegBuffer[iNum], g_SOI, sizeof(g_SOI) );
	//nPos += sizeof(g_SOI);

	for (i = 0; i < pstStream->u32PackCount; i++)
	{
		pstData = &pstStream->pstPack[i];

		memcpy(g_JpegBuffer[iNum] + nPos, pstData->pu8Addr[0], pstData->u32Len[0]);
		nPos += pstData->u32Len[0];

		if (pstData->u32Len[1] > 0)
		{
			memcpy(g_JpegBuffer[iNum] + nPos, pstData->pu8Addr[1], pstData->u32Len[1]);
			nPos += pstData->u32Len[1];
		}
	}

	//memcpy( g_JpegBuffer[iNum] + nPos, g_EOI, sizeof(g_EOI) );
	//imgSize[iNum] = nPos + sizeof(g_EOI);
	imgSize[iNum] = nPos;
	//HK_DEBUG_PRT("imgSize[%d] = %d\n", iNum, imgSize[iNum]);

#if JPEG_SNAP
	SaveJPEGPicsOnSDCard( g_JpegBuffer[iNum], imgSize[iNum], iNum );
#endif

	return HI_SUCCESS;
}


/***************************************************
 * func: get JPEG stream from VENC channel,
 *       and save into global picture buffer.
 **************************************************/
HI_S32 Video_JPEG_SnapShort(VENC_CHN SnapChn)
{
	HI_S32 s32Ret;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;

	FD_ZERO(&read_fds);
	FD_SET(g_vencSnapFd, &read_fds);
	s32Ret = select(g_vencSnapFd+1, &read_fds, NULL, NULL, NULL);
	if (s32Ret < 0) 
	{
		SAMPLE_PRT("select failed!\n");
		return HI_FAILURE;
	}
	else if (0 == s32Ret) 
	{
		SAMPLE_PRT("get venc stream time out, exit !\n");
		return HI_FAILURE;
	}
	else
	{
		if (FD_ISSET(g_vencSnapFd, &read_fds))
		{
			s32Ret = HI_MPI_VENC_Query(SnapChn, &stStat);
			if (s32Ret != HI_SUCCESS) 
			{
				SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", SnapChn, s32Ret);
				fflush(stdout);
				return HI_FAILURE;
			}

			stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
			if (NULL == stStream.pstPack)  
			{
				SAMPLE_PRT("malloc stream pack failed!\n");
				return HI_FAILURE;
			}
			stStream.u32PackCount = stStat.u32CurPacks;

			s32Ret = HI_MPI_VENC_GetStream(SnapChn, &stStream, HI_IO_BLOCK);
			if (HI_SUCCESS != s32Ret) 
			{
				SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
				fflush(stdout);
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				return HI_FAILURE;
			}

			/**stream OSD (wangshaoshu)**/
			RGN_HANDLE RgnHandle = 0;
			RgnHandle = 3 + SnapChn;
			OSD_Overlay_RGN_Display_Time(RgnHandle, SnapChn); 
			/** stream OSD end **/

			if (g_SavePic < 3) //discard the first 3 pictures.
			{
				g_SavePic++;
				s32Ret = HI_MPI_VENC_ReleaseStream(SnapChn, &stStream);
				if (s32Ret) 
				{
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return HI_FAILURE;
				}
				//printf("......... aaaaaaa ........\n");
				return HI_SUCCESS;
			}
			//printf("......... bbbbbbb ........\n");

			VENC_PACK_S *pstData = NULL; //get snapshort pictures buffer start.
			HI_U32 i = 0;
			unsigned int streamSize = 0;
			for (i = 0; i < stStream.u32PackCount; i++)
			{
				pstData = &stStream.pstPack[i];
				streamSize += pstData->u32Len[0];
				streamSize += pstData->u32Len[1];
			}



			HK_DEBUG_PRT("SnapChn: %d, g_SavePic:%d, streamSize:%d, g_SnapCnt: %d\n", SnapChn, g_SavePic, streamSize, g_SnapCnt);
			if ((streamSize <= (ALLIMGBUF-40*1024)) && (g_SnapCnt < 4))//snapshot 4 pictures.
			{
				s32Ret = SampleSaveJpegStream(&stStream, g_SnapCnt);//save pic data into g_JpegBuffer.
				if (HI_SUCCESS != s32Ret) 
				{
					fflush(stdout);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return HI_FAILURE;
				}
				g_SnapCnt++;//picture counter.
			}

			s32Ret = HI_MPI_VENC_ReleaseStream(SnapChn, &stStream);
			if (s32Ret) 
			{
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				return HI_FAILURE;
			}
			free(stStream.pstPack);
			stStream.pstPack = NULL;
		}
	}
	return HI_SUCCESS;
}

/*******************************************************
 * func: thread for getting pictures.
 ******************************************************/
HI_S32 getPicture(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_GRP SnapGrp = 2; //3518e, JPEG enc grp:2
	VENC_CHN SnapChn = 2; //3518e, JPEG enc chn:2
	g_SavePic = 0;
	g_SnapCnt = 0;
	g_bIsOpenPict = false;

	printf("[%s, %d] get_pic_flag:%d, g_OpenAlarmEmail:%d, hk_email.mcount:%d....\n", __func__, __LINE__, get_pic_flag, g_OpenAlarmEmail, hk_email.mcount);

#if ENABLE_ONVIF
	IPCAM_setTskName("getPicture");  
#endif
	pthread_detach(pthread_self()); 
	while( get_pic_flag )
	{
		if (g_OpenAlarmEmail == 0)//email service switched off.
		{
			sleep(2);
			continue;
		}

		if ((1 == g_OpenAlarmEmail) && (hk_email.mcount > 0))//enable pictures snapshot.
		{
			if (g_bIsOpenPict == false)
			{
				g_bIsOpenPict = true;
				g_SnapCnt = 0;
				//memset(g_JpegBuffer, 0, sizeof(g_JpegBuffer)); //picture buffer.

				/*start JPEG venc channel for receiving pictures*/
				Video_DisableVencChn(SnapChn);
				usleep(1000);
				s32Ret = Video_EnableVencChn(SnapChn);
				if (s32Ret != HI_SUCCESS)  return -1;

				g_vencSnapFd = COMM_Get_VencStream_FD( SnapChn );
				if (g_vencSnapFd < 0)
					return HI_FAILURE;

				HK_DEBUG_PRT("snapshort, venc fd: %d, g_SnapCnt: %d\n", g_vencSnapFd, g_SnapCnt);
			}

			s32Ret = Video_JPEG_SnapShort(SnapChn);
			if (s32Ret != HI_SUCCESS)
			{
				HK_DEBUG("Video_JPEG_SnapShort err with: 0x%x\n",s32Ret);
				break;
			}
		}

		printf("[%s, %d]...g_SnapCnt:%d, hk_email.mcount:%d...\n", __func__, __LINE__, g_SnapCnt, hk_email.mcount);
		if (g_SnapCnt >= hk_email.mcount) //pictures snap success.
		{
			g_SavePic = 0;         //disable getting picture into global buffer.
			g_bIsOpenPict = false; //disable picture snapshort.
			g_OpenAlarmEmail = 0;  //switch off pictures snapshort.
			g_SnapCnt = 0;         //clear picture counter.

			if (hk_email.mcount > 0)
			{
				Video_DisableVencChn(SnapChn); //stop receive picturs from venc chn.
			}

			int j = 0;
			char emailMSG[512] = {0};
			char cEmailInfo[256] = {0};
			conf_get( HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_INFO, cEmailInfo, 256 );
			if (strlen(cEmailInfo) <= 1)
			{
				// sprintf(emailMSG, "%s ID:%s", MSG, getenv("USER"));
				sprintf(emailMSG, "Your camera(%s) has noticed movement!", getenv("USER"));
			}
			else
			{
				sprintf(emailMSG, "%s ID:%s", cEmailInfo, getenv("USER"));
			}

			if (hk_email.secType == 0) //normal sending.
			{
				j = send_mail_to_user(hk_email.mailserver, hk_email.passwd, hk_email.mailfrom, hk_email.mailto, hk_email.username, emailMSG, 1);
				printf("......send normal email, result: %d......\n", j);
			}
			else
			{ //openssl secure.
				j = send_ssl_email(hk_email.mailserver, hk_email.passwd, hk_email.mailfrom, hk_email.mailto, hk_email.username, emailMSG, 1, hk_email.port, hk_email.secType);
				printf("2222222......send ssl email, result: %d......\n", j);
			}

			if (j == 0)
			{
				printf("============> Email Send Success ! <============\n"); 
			}
		}
		usleep(40*1000);
	}

	//if (hk_email.mcount > 0)
	//{
	//    Video_DisableVencChn(SnapChn); //stop receive picturs from venc chn.
	//}

	get_pic_flag = false; //disable picture snaping.
	return 0;
}



static int CreatePICThread(void)
{
	printf("scc..CreatePICThread.......\n");
	pthread_t picid;
	int ret = 0;

	ret = pthread_create(&picid, NULL, (void *)getPicture, NULL);
	if (ret != 0)
	{
		HK_DEBUG("create pic thread error!...%d..\n", __LINE__);
		return -1;
	}
	return 0;
}


/***************************************************
 * func: check alarm type & snap picture.
 * iType:
 *      1:moveAlarm, 2:ioAlarm, 3:sdError.
 **************************************************/
int CheckAlarm(int iChannel, int iType, int nReserved, char *cFtpData)
{
	//printf("........iType:%d, g_startCheckAlarm:%d........\n", iType, g_startCheckAlarm);
	if (g_startCheckAlarm < 4)
		return 0;

	if ((1 == iType) && (0 == video_properties_.vv[HKV_MotionSensitivity]))
		return 0;

	AlarmVideoRecord( true );//check SD card for video recording.
	static unsigned int raise_time = 0;
	static unsigned int raise_email = 0;
	unsigned int cur = Getms(); 

	if ((iType == 4) || (iType == 6))// 4 i/o out dev alarm; 6 433 alarm.
	{
		raise_alarm_server(iType,nReserved, cFtpData);
		raise_alarm("video.vbVideo.MPEG4", MPEG4);
		//sccLocalAlarm(iChannel, iType, nReserved, cFtpData);
	}
	else if (cur - raise_time > ALARMTIME)
	{
		raise_alarm_server(iType ,nReserved, cFtpData);
		//sccLocalAlarm(iChannel, iType, nReserved, cFtpData);
		raise_alarm("video.vbVideo.MPEG4", MPEG4);

		raise_time = Getms();
	}

	//sun
	//if ( ((1 == iType) && (0 == hkSdParam.sdMoveOpen)) || ((2 == iType) && (1 == hkSdParam.sdIoOpen)) )
	{
		if( cur - raise_email > ((ALARMTIME)*3))
		{
			raise_email = Getms();
			//g_OpenAlarmEmail = 1; //enable picture snap for sending alarm email.
			if (1 == hk_email.isOpen)
			{
				if (!get_pic_flag)
				{ 
					g_OpenAlarmEmail = 1; //enable picture snap for sending alarm email.
					get_pic_flag = true; 
					CreatePICThread();
				}
			}
		}
	}
	return 0;
}

int sccOnAlarm( int pDvrInst, int nAlarmType, int  nReserved )
{
	if (g_startCheckAlarm < 4)
		return 0;

	printf("433 alarm nAlarmType=%d. nReserved=%d...\n", nAlarmType, nReserved);
	if( nAlarmType == 100 )
	{
		printf("scc Dev Code.=%d.............\n", nReserved);
		g_sccDevCode = nReserved;
		return;
	}
	if ( nAlarmType != 0 )//move
	{
		int iObj = 0; 
		CheckAlarm(iObj, nAlarmType,nReserved, NULL);
		//sccAlarmVideo(true );
	}
}


//int sd_record_start();
//void sd_record_stop();

#define HK_MHDR_VERSION 2
/*
#define NTOHL
#define HTONL
#define HK_MHDR_SET_FRAGX(hdr, val) do{long msk=NTOHL(hdr) & ~0x000f, x=val; (hdr)=HTONL(msk|(x&0x0f));}while(0)
#define HK_MHDR_SET_FLIP(hdr, val)        ((val) ? ((hdr) |= 0x0020) : ((hdr) &= (~0x0020)))
#define HK_MHDR_SET_RESOLUTION(hdr, val)  ((hdr) |= ((val) & 0x000f))
#define HK_MHDR_SET_ENCODE_TYPE(hdr, val) ((hdr) |= (((val) << 6) & 0x07ff))
#define HK_MHDR_SET_VERSION(hdr, val)     ((hdr) |= ((val) << 14))
#define HK_MHDR_SET_MEDIA_TYPE(hdr, val)  ((hdr) |= (((val) << 11) & 0x3fff))
 */
#define HK_MHDR_SET_VERSION(hdr, val)     ((hdr) |= ((val) << 14))
#define HK_MHDR_GET_VERSION(hdr)     (((hdr) >> 14) & 0x0003)
#if HK_MHDR_VERSION == 1
// version  mediatype  encodetype  flip  x   resolution
// // [][]     [][][]     [][][][][]  []    []  [][][][]
#define HK_MHDR_SET_RESOLUTION(hdr, val)  ((hdr) |= ((val) & 0x000f))
#define HK_MHDR_SET_FLIP(hdr, val)        ((val) ? ((hdr) |= 0x0020) : ((hdr) &= (~0x0020)))
#define HK_MHDR_SET_ENCODE_TYPE(hdr, val) ((hdr) |= (((val) << 6) & 0x07ff))
#define HK_MHDR_SET_MEDIA_TYPE(hdr, val)  ((hdr) |= (((val) << 11) & 0x3fff))

#define HK_MHDR_GET_RESOLUTION(hdr)  ((hdr) & 0x000f)
#define HK_MHDR_GET_FLIP(hdr)        ((hdr) & 0x0020)
#define HK_MHDR_GET_ENCODE_TYPE(hdr) (((hdr) >> 6) & 0x001f)
#define HK_MHDR_GET_MEDIA_TYPE(hdr)  (((hdr) >> 11) & 0x0007)

#elif HK_MHDR_VERSION == 2
// // version flip  encodetype  resolution fragment-seq  
// // [][]    [][    [][][][][]  [][][][]   [][][][]   
#define NTOHL
#define HTONL
#define HK_MHDR_SET_RESOLUTION(hdr, val) do{long msk=NTOHL(hdr) & ~0x00f0; (hdr)=HTONL(msk|(((val)&0x0f) << 4));}while(0)
#define HK_MHDR_GET_RESOLUTION(hdr)  ((NTOHL(hdr)>>4) & 0x0f)
#define HK_MHDR_SET_ENCODE_TYPE(hdr, val) do{long msk=NTOHL(hdr) & ~0x1f00; (hdr)=HTONL(msk|(((val)&0x1f) << 8));}while(0)
#define HK_MHDR_GET_ENCODE_TYPE(hdr) ((NTOHL(hdr)>>8) & 0x1f)
#define HK_MHDR_SET_FLIP(hdr, val) do{long msk=NTOHL(hdr) & ~0x2000; (hdr)=HTONL(msk|((!!(val)) << 13));}while(0)
#define HK_MHDR_GET_FLIP(hdr)        ((NTOHL(hdr)>>13) & 0x01)
//#define HK_MHDR_SET_FRAGX(hdr, val) do{long msk=NTOHL(hdr) & ~0x000f, x=val; (hdr)=HTONL(msk|(x&0x0f));}while(0)
#define HK_MHDR_GET_FRAGX(hdr) (NTOHL(hdr) & 0x0f)
#define HK_MHDR_SET_MEDIA_TYPE(x,y) (void)0

/*
#define HK_MHDR_SET_ENCODE_TYPE(hdr, val) do{long msk=NTOHL(hdr) & ~0xf00; (hdr)=HTONL(msk|(((val)&0xf) << 8));}while(0)
#define HK_MHDR_GET_ENCODE_TYPE(hdr) ((NTOHL(hdr)>>8) & 0x0f)
//#define HK_MHDR_SET_FLIP(hdr, val) do{long msk=NTOHL(hdr) & ~0x3000; (hdr)=HTONL(msk|((!!(val)) << 12));}while(0)
#define HK_MHDR_GET_FLIP(hdr)        ((NTOHL(hdr)>>12) & 0x03)
#define HK_MHDR_SET_FLIP(hdr, val)  ((hdr) |= ((val) << 12))
 */
#define HK_MHDR_SET_FLIPEX(hdr, val)  ((hdr) |= ((val) << 2))
#define HK_MHDR_SET_FRAGX(hdr, val)  ((hdr) |= (val) )
#endif

static void video_property_builtins(struct HKVProperty* vp)
{
	memset(vp, 0, sizeof(*vp));
	vp->vv[HKV_VEncCodec] = MPEG4; //encode type.
	vp->vv[HKV_VinFormat] = 128;//bit rate(kb)
	vp->vv[HKV_Cbr] = 0;
	vp->vv[HKV_Vbr] = 3;
	vp->vv[HKV_Checksum] = 1;
	vp->vv[HKV_AnalogEncodeMode] = 1;
	vp->vv[HKV_VinFrameRate] = 15;
	vp->vv[HKV_VEncIntraFrameRate] = 45;   //I
	vp->vv[HKV_HueLevel] = 0;
	vp->vv[HKV_CamContrastLevel] = 32;
	vp->vv[HKV_CamSaturationLevel] = 32;
	vp->vv[HKV_CamEffectLevel] = 4;
	vp->vv[HKV_BrightnessLevel] = 32;
	vp->vv[HKV_SharpnessLevel] = 38;
	vp->vv[HKV_CamExposureLevel] = 0;
	vp->vv[HKV_CamLightingModeLevel] = 0;
	vp->vv[HKV_DividedImageEncodeMode] = 0;
	vp->vv[HKV_MotionSensitivity] = 0;
	vp->vv[HKV_Flip] = 0;
	vp->vv[HKV_Mirror] = 0;
	vp->vv[HKV_Yuntai] = 5;
	vp->vv[HKV_Focus] = 0;
	vp->vv[HKV_FocuMax] = 1;
	vp->vv[HKV_BaudRate] = 0;
	vp->vv[HKV_FrequencyLevel] = 1;
	vp->vv[HKV_NightLight] = 2;
	vp->vv[HKV_Autolpt] = 0;
	vp->vv[HKV_BitRate] = 25; //15;//frame rate.
}


/**************************************************************************
 * func: initialize main stream params according to 
 *       subipc.conf for phone preview, (zqj).
 **************************************************************************/
int MainStreamConfigurate(void)
{
	int s32ret = 0;
	video_property_builtins(&video_properties_);
	video_properties_.vv[HKV_VinFormat]          = conf_get_int(HOME_DIR"/hkipc.conf", "VinFormat");
	video_properties_.vv[HKV_CamSaturationLevel] = conf_get_int(HOME_DIR"/hkipc.conf", "CamSaturationLevel");
	video_properties_.vv[HKV_SharpnessLevel]     = conf_get_int(HOME_DIR"/hkipc.conf", "SharpnessLevel");
	video_properties_.vv[HKV_BrightnessLevel]    = conf_get_int(HOME_DIR"/hkipc.conf", "BrightnessLevel");
	video_properties_.vv[HKV_CamContrastLevel]   = conf_get_int(HOME_DIR"/hkipc.conf", "CamContrastLevel");
	video_properties_.vv[HKV_HueLevel]           = conf_get_int(HOME_DIR"/hkipc.conf", "HueLevel");
	video_properties_.vv[HKV_Yuntai]             = conf_get_int(HOME_DIR"/hkipc.conf", "ptzspeed");
	video_properties_.vv[HKV_FrequencyLevel]     = conf_get_int(HOME_DIR"/hkipc.conf", "FrequencyLevel");
	video_properties_.vv[HKV_Cbr]                = conf_get_int(HOME_DIR"/hkipc.conf", "Cbr");
	video_properties_.vv[HKV_MotionSensitivity]  = conf_get_int(HOME_DIR"/hkipc.conf", "MotionSensitivity");;
	HK_DEBUG_PRT("BitRate:%d, Saturat:%d, Sharp:%d, Bright:%d, Contrast:%d, Hue:%d, Yuntai:%d, Freq:%d, Cbr:%d, MotionSens:%d\n",\
			video_properties_.vv[HKV_VinFormat], video_properties_.vv[HKV_CamSaturationLevel], \
			video_properties_.vv[HKV_SharpnessLevel], video_properties_.vv[HKV_BrightnessLevel], \
			video_properties_.vv[HKV_CamContrastLevel], video_properties_.vv[HKV_HueLevel], \
			video_properties_.vv[HKV_Yuntai], video_properties_.vv[HKV_FrequencyLevel], \
			video_properties_.vv[HKV_Cbr], video_properties_.vv[HKV_MotionSensitivity]);

	if (video_properties_.vv[HKV_Yuntai] <= 0)
		video_properties_.vv[HKV_Yuntai] = 5;

	/**Flip**/
	int iFlip = conf_get_int(HOME_DIR"/hkipc.conf", "Flip");
	video_properties_.vv[HKV_Flip] = iFlip;
	if (1 == iFlip)
	{
		s32ret = HISI_SetFlip(1);
		if (s32ret)
		{
			HK_DEBUG_PRT("set video flip failed !\n"); 
			return -1;
		}
	}

	/**Mirror**/
	int iMirror = conf_get_int(HOME_DIR"/hkipc.conf", "Mirror");
	video_properties_.vv[HKV_Mirror] = iMirror;
	if (1 == iMirror)
	{
		s32ret = HISI_SetMirror(1);
		if (s32ret)
		{
			HK_DEBUG_PRT("set video mirror failed !\n"); 
			return -1;
		}
	}

	/**Rate Control**/
	VENC_CHN_ATTR_S s_VencChnAttr; //video encode channel attribute.
	if (0 == video_properties_.vv[HKV_Cbr]) //hkipc.conf => 0:CBR, 1:VBR.
	{
		s_VencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR; //enRcMode:1.
	}
	else
	{
		s_VencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR; //enRcMode:2.
	}
	printf("--------> s_VencChnAttr.stRcAttr.enRcMode: %d <--------\n", s_VencChnAttr.stRcAttr.enRcMode);
	s32ret = HISI_SetRCType(g_s32venchn, s_VencChnAttr.stRcAttr.enRcMode);
	if (s32ret)
	{
		HK_DEBUG_PRT("set rate control failed !\n"); 
		return -1;
	}

	/**Frame Rate**/
	int iFrameRate = conf_get_int(HOME_DIR"/hkipc.conf", "BitRate");
	if (iFrameRate > 0)
	{
		if (iFrameRate > 15)
			iFrameRate = 15;

		//HK_DEBUG_PRT("......frame rate: %d......\n", iFrameRate);
		s32ret = HISI_SetFrameRate(g_s32venchn, iFrameRate);
		if (s32ret)
		{
			HK_DEBUG_PRT("set frame rate failed !\n"); 
			return -1;
		}
	}

	/**BitRate**/
	//if (0 == video_properties_.vv[HKV_Cbr]) //hkipc.conf => 0:CBR, 1:VBR.
	{
		if (video_properties_.vv[HKV_VinFormat] > 1024)
		{
			s32ret = HISI_SetBitRate(g_s32venchn, 1024);
		}
		else
		{
			s32ret = HISI_SetBitRate(g_s32venchn, video_properties_.vv[HKV_VinFormat]);
		}

		if (s32ret)
		{
			HK_DEBUG_PRT("set video bit rate failed !\n"); 
			return -1;
		}
	}

	//printf("..............111111111................\n");
	/**Color**/
	int staturation = video_properties_.vv[HKV_CamSaturationLevel];
	int brightness  = video_properties_.vv[HKV_BrightnessLevel];
	int huelevel    = 0; //video_properties_.vv[HKV_HueLevel]; //don't set huelevel.
	int contrast    = video_properties_.vv[HKV_CamContrastLevel];
	s32ret = HISI_SetCSCAttr(staturation, brightness, contrast, huelevel); //don't set huelevel.
	if (s32ret)
	{
		HK_DEBUG_PRT("set CSC Attribute failed !\n"); 
		return -1;
	}

	s32ret = HISI_SetSharpNess(video_properties_.vv[HKV_SharpnessLevel]);
	if (s32ret)
	{
		HK_DEBUG_PRT("set sharpness failed !\n"); 
		return -1;
	}
	//printf("..............22222222222................\n");

#if 0 //no need to set PAL & NTSC.
	/**Frequency**/
	VIDEO_NORM_E s_enNorm;
	if (0 == video_properties_.vv[HKV_FrequencyLevel]) //PAL(50Hz)
	{
		s_enNorm = VIDEO_ENCODING_MODE_PAL;
		s32ret = HISI_SetLocalDisplay(s_enNorm);
		if (s32ret)
		{
			HK_DEBUG_PRT("set video encoding mode failed !\n"); 
			return -1;
		}
	}
	else if (1 == video_properties_.vv[HKV_FrequencyLevel]) //NTSC(60Hz)
	{
		s_enNorm = VIDEO_ENCODING_MODE_NTSC;
		s32ret = HISI_SetLocalDisplay(s_enNorm);
		if (s32ret)
		{
			HK_DEBUG_PRT("set video encoding mode failed !\n"); 
			return -1;
		}
	}
	else
	{
		s_enNorm = VIDEO_ENCODING_MODE_AUTO; //AUTO
		s32ret = HISI_SetLocalDisplay(s_enNorm);
		if (s32ret)
		{
			HK_DEBUG_PRT("set video encoding mode failed !\n"); 
			return -1;
		}
	}
#endif

	return 0;
}


/**************************************************************************
 * func: initialize sub stream params according to 
 *       subipc.conf for phone preview (zqj).
 **************************************************************************/
int SubStreamConfigurate(void)
{
	int ret = 0;
	int s_EncResolution = conf_get_int(HOME_DIR"/subipc.conf", "enc");
	int s_BitRate       = conf_get_int(HOME_DIR"/subipc.conf", "stream");
	int s_FrameRate     = conf_get_int(HOME_DIR"/subipc.conf", "rate");
	int s_Smooth        = conf_get_int(HOME_DIR"/subipc.conf", "smooth");
	int s_Saturation    = conf_get_int(HOME_DIR"/subipc.conf", "sat");
	int s_Contrast      = conf_get_int(HOME_DIR"/subipc.conf", "con");
	int s_Brightness    = conf_get_int(HOME_DIR"/subipc.conf", "bri");
	int s_Sharpness     = conf_get_int(HOME_DIR"/subipc.conf", "sha");
	HK_DEBUG_PRT("...VeSunChn:%d, s_EncResolution:%d, s_BitRate:%d, s_FrameRate:%d, s_Smooth:%d, s_Saturation:%d, s_Contrast:%d, s_Brightness:%d, s_Sharpness:%d...\n", VeSunChn, s_EncResolution, s_BitRate, s_FrameRate, s_Smooth, s_Saturation, s_Contrast, s_Brightness, s_Sharpness);

	switch (s_EncResolution)
	{
		case 3:
			s_BitRate = 24; //48; //64;
			break;
		case 5:
			s_BitRate = 24; //48; //64; //128;
			break;
		default:
			s_BitRate = 24; //48; //64;
			break;
	}

	ret = HISI_SetBitRate(VeSunChn, s_BitRate);
	if (ret)
	{
		printf("[%s, %d] set bitrate failed !\n", __func__, __LINE__);
		return -1;
	}

	if ( (s_FrameRate <= 0) || (s_FrameRate > 15) )
	{
		s_FrameRate = 15;
	}
	ret = HISI_SetFrameRate(VeSunChn, s_FrameRate);
	if (ret)
	{
		printf("[%s, %d] set frame rate failed !\n", __func__, __LINE__);
		return -1;
	}

	ret = HISI_SetCSCAttr(s_Saturation, s_Brightness, s_Contrast, 0);
	if (ret)
	{
		printf("[%s, %d] set CSC attribute failed !\n", __func__, __LINE__);
		return -1;
	}

	ret = HISI_SetSharpNess(s_Sharpness);
	if (ret)
	{
		printf("[%s, %d] set CSC attribute failed !\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int hk_set_video_hdr(long int* flags, unsigned short* hdr, int enc, int fmt,int iChennl)
{
	*hdr = 0;
	HK_MHDR_SET_VERSION( *hdr, HK_MHDR_VERSION ); 
	HK_MHDR_SET_MEDIA_TYPE( *hdr, 1 );
	HK_MHDR_SET_ENCODE_TYPE( *hdr, enc );
	//HK_MHDR_SET_RESOLUTION(*hdr, VinFormat_from_ipc_2hk[fmt]);
	HK_MHDR_SET_RESOLUTION( *hdr, fmt );

	HK_MHDR_SET_FLIPEX( *hdr, 0 );// video_properties_.vv[HKV_Flip]); 

	//if (enc==H264)
	//{
	int nLevel = iChennl;
	*flags |= (nLevel<<8);
	//}
	//else
	//{
	//    int nLevel = 3;
	//    *flags |= (nLevel<<8);
	//}
	return 1;
}

TAlarmSet_ g_tAlarmSet[MAX_CHAN];//MAX_CHAN==3
unsigned char g_MdMask[MAX_CHAN][MAX_MACROCELL_NUM];//
bool g_bMDStartOK[MAX_CHAN];//

static bool bsMonut = false;
static int iSdRecord = 0;

/************************************************
 * func: check SD card for video recording.
 ***********************************************/
void AlarmVideoRecord(bool bAlarm)
{
	if ( bAlarm )
	{
		//HK_DEBUG_PRT("...hkSdParam.outMoveRec:%d, hkSdParam.moveRec:%d...\n", hkSdParam.outMoveRec, hkSdParam.moveRec);
		iSdRecord = 60;
		if ((hkSdParam.outMoveRec == 1) || (hkSdParam.moveRec == 1))
		{
			if ( (1 == g_sdIsOnline) && (1 != hkSdParam.autoRec) && (! bsMonut) )
			{
				bsMonut = true;
				sd_record_start();
			}
		}
	}
	else
	{
		if ( bsMonut )
		{
			iSdRecord--;
			if ( iSdRecord <= 0 )
			{
				bsMonut = false;
				sd_record_stop();
			}
		}
	}
}


static void Close(int obj)
{
	if( obj == MPEG4)
	{
		g_isH264Open = 0;

		fprintf(stderr, "...Close...current MPEG4 venc channel:%d\n", g_Venc_Chn_M_Cur);
		//Video_DisableVencChn( g_Venc_Chn_M_Cur );
	}
	else if ( obj == M_JPEG )
	{
		g_isMjpegOpen = 0;
		fprintf(stderr, "...Close...current M_JPEG venc channel:%d\n", g_Venc_Chn_S_Cur);
		//Video_DisableVencChn( g_Venc_Chn_S_Cur );
	}
	else if( obj == H264_TF )
	{
		g_isTFOpen = 0;
	}
	else
	{
		fprintf( stderr,"Close err\n" );
		exit(1);
	}
}


/******************************************************* 
 * func:  Get Venc Stream file descriptor according 
 *      the specified Venc Channel.
 * return: success on positive, and error on -1;
 ******************************************************/
int COMM_Get_VencStream_FD(int s32venchn)
{
	int i = 0;
	HI_S32 s32Ret = HI_FAILURE;
	HI_S32 s_vencFd = HI_FAILURE; //venc fd.
	VENC_CHN_ATTR_S stVencChnAttr;

	/*********************************************
	  step 1:  check & prepare save-file & venc-fd
	 **********************************************/
	if ((s32venchn < 0) || (s32venchn >= VENC_MAX_CHN_NUM))
	{
		SAMPLE_PRT("Venc Channel is out of range !\n");
		return HI_FAILURE;
	}

	/***** check if the channel created *****/
	//s32Ret = HI_MPI_VENC_GetChnAttr( s32venchn, &stVencChnAttr );
	if (g_Chan == 0)
	{	
		s32Ret = HI_MPI_VENC_GetChnAttr( s32venchn, &g_stVencChnAttrMain);
	}    
	else if ( g_Chan == 1)
	{
		s32Ret = HI_MPI_VENC_GetChnAttr( s32venchn, &g_stVencChnAttrSlave);
	}
	else
	{
		s32Ret = HI_MPI_VENC_GetChnAttr( s32venchn, &stVencChnAttr );
	}

	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", s32venchn, s32Ret);
		return HI_FAILURE;
	}

	/* Get Venc Fd. */
	s_vencFd = HI_MPI_VENC_GetFd( s32venchn );
	if ( s_vencFd < 0 )
	{
		SAMPLE_PRT("HI_MPI_VENC_GetFd failed with %#x!\n", s_vencFd);
		return HI_FAILURE;
	}

	return s_vencFd;
}


int s_fd7725 = -1;
int open_7725();
/**************************************************************
 * configurate sub stream params for phone client settings.
 **************************************************************/
static void hk_SetPhonePlaram(int ibit, int iEnc, int iRate)
{
	HK_DEBUG_PRT("...ibit:%d, iEnc:%d, iRate:%d...\n", ibit, iEnc, iRate);
	if ( (ibit <= 0) || (iRate <= 0) )
		return;

	if ( HISI_SetBitRate(g_Venc_Chn_S_Cur, ibit) )
	{
		printf("[%s, %d] phone set bit rate failed!\n", __func__, __LINE__); 
	}

	if ( HISI_SetFrameRate(g_Venc_Chn_S_Cur, iRate) )
	{
		printf("[%s, %d] phone set frame rate failed!\n", __func__, __LINE__); 
	}

	//g_bPhCifOrD1 = false;
	return;
}

static int sccOpen(const char* name, const char* args, int* threq)
{
	//HK_DEBUG_PRT("......sccOpen: %s..........\n", name);
	HI_S32 s32Ret = 0;
	if (strcmp(name, "video.vbVideo.MPEG4") == 0) //open for main stream.
	{
		*threq = 0;
		//if (g_isH264Open == 0)
		{
			/**main stream Venc Channel==> 0:960P; 1:VGA, if 3518e, only chn: 0**/
			Video_DisableVencChn( g_s32venchn );
			usleep(1000);
			Video_EnableVencChn( g_s32venchn );

			g_Chan = 0;
			g_VencFd_Main = COMM_Get_VencStream_FD( g_s32venchn );
			if ( HI_FAILURE == g_VencFd_Main )
			{
				printf("Get Venc Stream FD failed: %s, %d\n", __func__, __LINE__); 
				return -1;
			}

			//g_isH264Open    = 1;
			g_ReadFlag_H264 = 1;
			g_ReadTimes     = 3;
			*threq          = 1;
			g_Venc_Chn_M_Cur = g_s32venchn; //current VENC channel.
			HK_DEBUG_PRT("Open:%s, g_Venc_Chn_M_Cur:%d, g_VencFd_Main:%d.\n", name, g_Venc_Chn_M_Cur, g_VencFd_Main);
		}
		return MPEG4;
	}
	else if (strcmp(name, "video.vbVideo.M_JPEG") == 0) //open for sub stream.
	{
		/**sub stream Venc Chn==> 2:VGA; 3:QVGA, if 3518e, only chn: 1**/
		*threq = 0;
		//if ( g_isMjpegOpen == 0 )
		{
			/**config sub stream params**/
			if ( SubStreamConfigurate() )
				printf("[%s, %d] configurate sub stream failed !\n", __func__, __LINE__); 
			else
				printf("[%s, %d] configurate sub stream success !\n", __func__, __LINE__); 

			Video_DisableVencChn( VeSunChn );
			usleep(1000);
			Video_EnableVencChn( VeSunChn );

			g_Chan = 1;
			g_VencFd_Sub = COMM_Get_VencStream_FD( VeSunChn ); //get sub stream fd.
			if (HI_FAILURE == g_VencFd_Sub)
			{
				printf("Get Venc Stream FD failed: %s, %d\n", __func__, __LINE__); 
				return -1;
			}

			//g_isMjpegOpen    = 1;
			g_ReadFlag_Mjpeg = 1;
			*threq           = 1;
			g_Venc_Chn_S_Cur = VeSunChn; //current VENC channel.
			HK_DEBUG_PRT("Open:%s, g_sunCifOrD1:%d, g_Venc_Chn_S_Cur:%d, g_VencFd_Sub:%d.\n", name, g_sunCifOrD1, g_Venc_Chn_S_Cur, g_VencFd_Sub);
		}
		return M_JPEG;
	}

	/*
	   else if ( strcmp(name, "video.vbVideo.TF") == 0)
	   {
	 *threq = 0;
	 if ( g_isTFOpen == 0 )
	 {
	//SampleUNEnableEncode(VeTfChn, VeTfChn, VIDEVID, VICHNID);
	usleep(1000);
	s32Ret = 0;//SampleEnableEncode(VeTfChn, VeTfChn, VIDEVID, VICHNID);
	if( s32Ret != HI_SUCCESS )
	return -1;

	g_isTFOpen = 1;
	 *threq = 1;
	 s32TFH264Fd = HI_MPI_VENC_GetFd( VeTfChn );
	 }
	 return H264_TF;
	 }
	 */
	return 0;
}


//extern LPIPCAM_VIDEOBUFFER g_mH264VideoBuf;
//extern LPIPCAM_VIDEOBUFFER g_sH264VideoBuf;
int g_Video_Thread=0;

#if ENABLE_QQ
static int s_gopIndex = 0;
unsigned long _GetTickCount()
{        
        struct timeval current = {0};  
        gettimeofday(&current, NULL);  
        return (current.tv_sec*1000 + current.tv_usec/1000);                           
}
#endif

int sccGetVideoThread()
{
	int threq = 0;
	sccOpen("video.vbVideo.MPEG4", NULL, &threq);

	char videobuf[200*1024] = {0};
	HI_S32 s32Ret = 0;
	int iLen = 0; 		   //stream data size.
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.
	int iFrame = 0;
	fd_set read_fds;
	VENC_STREAM_S stStream;    //captured stream data struct.	
	VENC_CHN_STAT_S stStat;

	s_vencFd = g_VencFd_Main;
	s_maxFd = s_vencFd + 1;    //for select.
	s_vencChn = g_Venc_Chn_M_Cur; //current video encode channel.	
	int j = 0;
	RGN_HANDLE RgnHandle = 0;

	struct sched_param param;
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	

	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		HK_DEBUG_PRT("malloc failed, %d, %s\n", errno, strerror(errno));
		pstPack = NULL;
		return NULL;
	}
	while( g_Video_Thread )
	{
		FD_ZERO( &read_fds );
		FD_SET( s_vencFd, &read_fds );

		static int s_nFrameIndex = -1;
		static int s_dwTotalFrameIndex = 0;

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			SAMPLE_PRT("select failed!\n");
			usleep(1000);
			continue;
		}
		else if (s32Ret > 0)
		{
			if (FD_ISSET( s_vencFd, &read_fds ))
			{
				iLen = 0;
				s32Ret = HI_MPI_VENC_Query( s_vencChn, &stStat );
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
					usleep(1000);
					continue;
				}

				stStream.pstPack = pstPack;
				stStream.u32PackCount = stStat.u32CurPacks;
				s32Ret = HI_MPI_VENC_GetStream( s_vencChn, &stStream, HI_TRUE );
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
					usleep(1000);
					//break;
					continue;
				}

				for (j = 0; j < stStream.u32PackCount; j++)
				{
					memcpy( videobuf + iLen, stStream.pstPack[j].pu8Addr[0], stStream.pstPack[j].u32Len[0] );
					iLen += stStream.pstPack[j].u32Len[0];	       

					if (stStream.pstPack[j].u32Len[1] > 0)
					{
						memcpy( videobuf+iLen, stStream.pstPack[j].pu8Addr[1], stStream.pstPack[j].u32Len[1] );
						iLen += stStream.pstPack[j].u32Len[1];
					}

					switch (stStream.pstPack[j].DataType.enH264EType)
					{
						case H264E_NALU_PSLICE:
							iFrame = 1; //HK_BOAT_PFREAM; //P frame
#if ENABLE_QQ
							s_nFrameIndex++;
#endif
							break;
						case H264E_NALU_BUTT:
							HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
							stStream.pstPack = NULL;
							continue;
							break;
						default:
							iFrame = 0; //HK_BOAT_IFREAM; //I frame
#if ENABLE_QQ
							s_gopIndex++;
#endif
							break;
					}
				} //end for()

				/*****OSD: TIME*****/
				RgnHandle = 3 + s_vencChn;
				OSD_Overlay_RGN_Display_Time(RgnHandle, s_vencChn); 
				/*****OSD END*****/

#if ENABLE_P2P
                                //P2PNetServerChannelDataSndToLink(0,0,videobuf,iLen,iFrame,0);
#endif

#if ENABLE_QQ
				tx_set_video_data(videobuf, iLen, iFrame, _GetTickCount(), s_gopIndex, s_nFrameIndex, s_dwTotalFrameIndex++, 40);
				//tx_set_video_data(videobuf, iLen, iFrame, _GetTickCount(), 0, 0, 0, 40);
#endif

				s32Ret = HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
					stStream.pstPack = NULL;
					usleep(1000);
					continue;
				}
			}
		}
	}//end while
	HK_DEBUG_PRT("......video thread quit......\n");
	if(pstPack)
	{
		free(pstPack);
	}
	return 1;
}

int CreateVideoThread(void)
{
	if (0 == g_Video_Thread)
	{
		g_Video_Thread = 1;
		pthread_t tfid;
		int ret = 0;

		ret = pthread_create(&tfid, NULL, (void *)sccGetVideoThread, NULL);
		if (ret != 0)
		{
			HK_DEBUG_PRT("pthread_create failed, %d, %s\n", errno, strerror(errno));
			return -1;
		}
		//pthread_detach(tfid);
	}
	return 1;
}

static int g_SubVideo_Thread=0;

#if 0
static int sccRead_SubH264( );

int sccGetSubVideoThread()
{
	int threq=0;
	sccOpen("video.vbVideo.M_JPEG", NULL, &threq);

	sleep(1);
	while(g_SubVideo_Thread )
	{
		sccRead_SubH264( );
	}
	return 1;
}
#else
int sccGetSubVideoThread()
{
	int threq = 0;
	sccOpen("video.vbVideo.M_JPEG", NULL, &threq);

	//char videobuf[200*1024] = {0};
	char *videobuf = NULL; //2015-01-09.
	HI_S32 s32Ret = 0;
	int iLen = 0;  //stream data size.
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.
	int iFrame = 0;
	fd_set read_fds;
	VENC_STREAM_S stStream; //captured stream data struct.	
	VENC_CHN_STAT_S  stStat;

	s_vencFd = g_VencFd_Sub;
	s_maxFd = s_vencFd + 1; //for select.
	s_vencChn = g_Venc_Chn_S_Cur; //current video encode channel.	
	int j = 0;
	RGN_HANDLE RgnHandle = 0;

	struct sched_param param;
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	
#if ENABLE_ONVIF
	LPIPCAM_VIDEOBUFFER pH264VideoBuf = NULL;

	IPCAM_PTHREAD_DETACH;
	IPCAM_setTskName("SubVideoThread");
	param.sched_priority = ((sched_get_priority_min(SCHED_FIFO) + sched_get_priority_max(SCHED_FIFO)) / 3) * 2;
	if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0)
	{
		IPCAM_DEBUG("can not set thread prio\r\n");
	}

	pH264VideoBuf = (LPIPCAM_VIDEOBUFFER)malloc(sizeof(IPCAM_VIDEOBUFFER));
	if (NULL == pH264VideoBuf)
	{
		HK_DEBUG_PRT("malloc failed, %d, %s\n", errno, strerror(errno));
		return NULL;
	}
#endif

	videobuf = (char *)malloc(200*1024*sizeof(char)); //2015-01-09.
	if (NULL == videobuf)
	{
		HK_DEBUG_PRT("malloc failed, %d, %s\n", errno, strerror(errno));
		videobuf = NULL;
		return NULL;
	}

	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		HK_DEBUG_PRT("malloc failed, %d, %s\n", errno, strerror(errno));
		return NULL;
	}

	sleep(1);
	while( g_SubVideo_Thread )
	{
		FD_ZERO( &read_fds );
		FD_SET( s_vencFd, &read_fds );

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			SAMPLE_PRT("select failed!\n");
			usleep(1000);
			//break;
			continue;
		}
		else if(s32Ret > 0)
		{
			if (FD_ISSET( s_vencFd, &read_fds ))
			{
				iLen = 0;
				//memset(videobuf, 0, sizeof(videobuf)); //note: high CPU.
				s32Ret = HI_MPI_VENC_Query( s_vencChn, &stStat );
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
					//break;
					continue;
				}

				stStream.pstPack = pstPack;
				stStream.u32PackCount = stStat.u32CurPacks;
				s32Ret = HI_MPI_VENC_GetStream( s_vencChn, &stStream, HI_TRUE );
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
					//break;
					continue;
				}

				for (j = 0; j < stStream.u32PackCount; j++)
				{
					memcpy( videobuf + iLen, stStream.pstPack[j].pu8Addr[0], stStream.pstPack[j].u32Len[0] );
					iLen += stStream.pstPack[j].u32Len[0];			  

					if ( stStream.pstPack[j].u32Len[1] > 0 )
					{
						memcpy( videobuf+iLen, stStream.pstPack[j].pu8Addr[1], stStream.pstPack[j].u32Len[1] );
						iLen += stStream.pstPack[j].u32Len[1];
					}

					switch (stStream.pstPack[j].DataType.enH264EType)
					{
						case H264E_NALU_PSLICE:
							iFrame = 1;// HK_BOAT_PFREAM; //P frame
#if ENABLE_ONVIF
							pH264VideoBuf->dwFrameType = VIDEO_P_FRAME;
#endif
							break;
						default:
							iFrame = 0;// HK_BOAT_IFREAM; //I frame						  
#if ENABLE_ONVIF
							pH264VideoBuf->dwFrameType = VIDEO_I_FRAME;
#endif
							break;
					}
				} //end for()


				/*****OSD: TIME*****/
				RgnHandle = 3 + s_vencChn;
				OSD_Overlay_RGN_Display_Time(RgnHandle,s_vencChn); 
				/*****OSD END*****/
#if ENABLE_P2P
                                P2PNetServerChannelDataSndToLink(0,1,videobuf,iLen,iFrame,0);
#endif


#if ENABLE_ONVIF
				memcpy(pH264VideoBuf->VideoBuffer, videobuf, iLen );
				pH264VideoBuf->bzEncType	  = ENC_AVC;
				pH264VideoBuf->dwBufferType   = VIDEO_LOCAL_RECORD;
				pH264VideoBuf->dwFrameNumber  = stStream.u32Seq; 
				pH264VideoBuf->dwWidth		  = g_stVencChnAttrSlave.stVeAttr.stAttrH264e.u32PicWidth;
				pH264VideoBuf->dwHeight 	  = g_stVencChnAttrSlave.stVeAttr.stAttrH264e.u32PicHeight;
				pH264VideoBuf->dwSec		  = stStream.pstPack[0].u64PTS/1000;
				pH264VideoBuf->dwUsec		  = (stStream.pstPack[0].u64PTS%1000)*1000;
				pH264VideoBuf->dwFameSize	  = iLen;			
				IPCAM_PutStreamData(VIDEO_LOCAL_RECORD, 1, VOIDEO_MEDIATYPE_VIDEO, pH264VideoBuf);
#endif

				s32Ret = HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
					stStream.pstPack = NULL;
					//break;
					continue;
				}			

				/** push pool stream **/
				//if (1 == g_isMjpegOpen)
				if (2 == g_isMjpegOpen)
				{
					sccPushStream( 1234, PSTREAMTWO, videobuf, iLen, iFrame, g_iCifOrD1, H264 );

					if (1 == video_properties_.vv[HKV_Cbr]) //hkipc.conf => 0:CBR, 1:VBR.
					{
						int nRate = GetMainVideoRate( iLen );
						if ( (nRate > 40) && (g_VbrMaxQq_Sub == VBR_MAXQP_33) )
						{
							Set_VBR_Image_QP( VeSunChn, VBR_MAXQP_38 );
							g_VbrMaxQq_Sub = VBR_MAXQP_38;
							printf( "fxb ------------- sub change, nRate: %d, g_VbrMaxQq: %d \n", nRate, g_VbrMaxQq );
						}
						else if ( (nRate < 10) && (nRate > 0) && (g_VbrMaxQq_Sub == VBR_MAXQP_38) )
						{
							Set_VBR_Image_QP( VeSunChn, VBR_MAXQP_33 );
							g_VbrMaxQq_Sub = VBR_MAXQP_33;
							printf( "fxb --------------- sub low, nRate: %d, g_VbrMaxQq:%d \n", nRate, g_VbrMaxQq );
						}
					}
				}

				if (0 == hkSdParam.sdrecqc)
				{
					sccPushTfData( PSTREAMTWO, videobuf, iLen, iFrame, g_iCifOrD1, H264 );
				}
			}		
		}// end while
	}
	HK_DEBUG_PRT("......sub video thread end......\n" );
	g_SubVideo_Thread = 0;
	if (pstPack)  free(pstPack);
#if ENABLE_ONVIF
	if (pH264VideoBuf)	free(pH264VideoBuf);
	IPCAM_PTHREAD_EXIT; 
#endif

	if (videobuf)  //2015-01-09.
	{
		free(videobuf);
		videobuf = NULL;
	}

	return 1;
}
#endif


int CreateSubVideoThread() //create Get SubVideo Thread
{
	if (g_SubVideo_Thread == 0)
	{
		g_SubVideo_Thread = 1;
		pthread_t tfid;
		int ret = 0;

		ret = pthread_create(&tfid, NULL, (void *)sccGetSubVideoThread, NULL);
		if (ret != 0)
		{
			SAMPLE_PRT("pthread_create failed, %d, %s\n", errno, strerror(errno));
			return -1;
		}
		//pthread_detach(tfid);
	}
	return 1;
}

//start video thread
int sccStartVideoThread()
{
	CreateVideoThread();
	CreateSubVideoThread();
	CreateAudioThread();
}

static void GetInitAlarmParam(HKFrameHead *pFrameHead)
{
	int i = 0;
	if (g_iCifOrD1 >= 5)
	{
		int iCount = conf_get_int(HOME_DIR"/RngMD.conf", "D1Count");
		printf("[%s, %d]...g_iCifOrD1:%d, D1Count:%d...\n", __func__, __LINE__, g_iCifOrD1, iCount);

		SetParamUN( pFrameHead, HK_KEY_INDEX, iCount ); 
		for (i = 0; i < iCount; i++)
		{
			char cXYbuf[54] = {0};
			char cWHbuf[54] = {0};
			snprintf( cXYbuf, sizeof(cXYbuf), "D1XY%d", i );
			snprintf( cWHbuf, sizeof(cWHbuf), "D1WH%d", i );

			int iXY = conf_get_int(HOME_DIR"/RngMD.conf", cXYbuf);
			int iWh = conf_get_int(HOME_DIR"/RngMD.conf", cWHbuf);

			char cKey[HK_KEY_VALUES] = {0};
			char cYey[HK_KEY_VALUES] = {0};
			snprintf( cKey, sizeof(cKey), "%s%d", HK_KEY_POINTX, i );
			snprintf( cYey, sizeof(cYey), "%s%d", HK_KEY_POINTY, i );

			SetParamUN( pFrameHead, cKey, iXY ); 
			SetParamUN( pFrameHead, cYey, iWh ); 
			printf("...iXY:%d, iWh:%d...\n", iXY, iWh);
		}
	}
	else
	{
		int iCount = conf_get_int(HOME_DIR"/RngMD.conf", "CifCount");
		printf("[%s, %d]...g_iCifOrD1:%d, CifCount:%d...\n", __func__, __LINE__, g_iCifOrD1, iCount);

		SetParamUN( pFrameHead, HK_KEY_INDEX, iCount ); 
		for (i = 0; i < iCount; i++)
		{
			char cXYbuf[54] = {0};
			char cWHbuf[54] = {0};
			snprintf( cXYbuf, sizeof(cXYbuf), "CifXY%d", i );
			snprintf( cWHbuf, sizeof(cWHbuf), "CifWH%d", i );

			int iXY = conf_get_int(HOME_DIR"/RngMD.conf", cXYbuf);
			int iWh = conf_get_int(HOME_DIR"/RngMD.conf", cWHbuf);

			char cKey[HK_KEY_VALUES] = {0};
			char cYey[HK_KEY_VALUES] = {0};
			snprintf( cKey, sizeof(cKey), "%s%d", HK_KEY_POINTX, i );
			snprintf( cYey, sizeof(cYey), "%s%d", HK_KEY_POINTY, i );

			SetParamUN( pFrameHead, cKey, iXY ); 
			SetParamUN( pFrameHead, cYey, iWh ); 
			printf("...iXY:%d, iWh:%d...\n", iXY, iWh);
		}
	}
}
#if 0
void sccGetVideoInfo(char buf2[1024*8],int nCmd, int nSubCmd, unsigned int ulParam)
{
	char buf[1014*8]={0};
	HKFrameHead *pFrameHead= CreateFrameB();
	GetInitAlarmParam( pFrameHead);
	char bufMD[512]={0};
	int iLen = GetFramePacketBuf( pFrameHead, bufMD, sizeof(bufMD) );
	bufMD[iLen] = '\0';
	DestroyFrame(pFrameHead);

#define HK_STR_INBAND_DESC HK_KEY_FROM"=%s;" \
	HK_KEY_ALERT_INFO"=%s;"\
	HK_KEY_IRATE"=%hhd;"\
	HK_KEY_COMPRESS"=%hhd;" \
	HK_KEY_SENSITIVITY"=%hhd;" \
	HK_KEY_RESOLU"=%d;"
#define HK_STR_INBAND_DIGIT HK_KEY_RATE"=%hhd;" \
	HK_KEY_HUE"=%hhd;" \
	HK_KEY_SHARPNESS"=%hhd;" \
	HK_KEY_BITRATE"=%hhd;"\
	HK_KEY_BRIGHTNESS"=%hhd;" \
	HK_KEY_SATURATION"=%hhd;" \
	HK_KEY_CONTRAST"=%hhd;"\
	HK_KEY_FREQUENCY"=%hhd;" \
	HK_KEY_CBRORVBR"=%d;"\
	HK_KEY_VER"=%d;"\
	HK_KEY_PT"=%d;"\
	HK_KEY_IOIN"=%d;"\
	HK_KEY_MAINCMD"=%d;"\
	HK_KEY_SUBCMD"=%d;"\
	HK_KEY_UIPARAM"=%d;"\
	HK_KEY_EXPOSURE"=0;"\
	HK_KEY_QUALITE"=3;"\
	HK_KEY_FLAG"=2;"
	//unsigned int nLevel = (2<<8);
	//char prop[128];
	//char buf[sizeof(HK_STR_INBAND_DESC) + sizeof(HK_STR_INBAND_DIGIT) + 256];
	int len = 0; 
	char FrequencyLevel =60; 

	if(video_properties_.vv[HKV_FrequencyLevel]==0)
		FrequencyLevel = 50;
	else if(video_properties_.vv[HKV_FrequencyLevel]==1)
		FrequencyLevel = 60;
	else
		FrequencyLevel = 70;

	int iHue =0;// video_properties_.vv[HKV_HueLevel];///4;
	int iBrig = video_properties_.vv[HKV_BrightnessLevel];///4;
	int iSatur = video_properties_.vv[HKV_CamSaturationLevel];///4;
	int iContr = video_properties_.vv[HKV_CamContrastLevel];///4;

	len = snprintf(buf, sizeof(buf), HK_STR_INBAND_DESC HK_STR_INBAND_DIGIT,
			getenv("USER"),
			bufMD,
			video_properties_.vv[HKV_VEncIntraFrameRate],
			video_properties_.vv[HKV_VEncCodec],
			video_properties_.vv[HKV_MotionSensitivity],
			video_properties_.vv[HKV_VinFormat],
			video_properties_.vv[HKV_BitRate],
			iHue,
			video_properties_.vv[HKV_SharpnessLevel],
			g_iCifOrD1,
			iBrig,
			iSatur,
			iContr,
			FrequencyLevel,
			video_properties_.vv[HKV_Cbr],
			g_DevVer,
			video_properties_.vv[HKV_Yuntai],
			g_irOpen,
			nCmd,
			nSubCmd,
			ulParam);   

	strcpy(buf2, buf);
}
#endif

#if 0
/********************************************************************************
func:	open read and so on ¿¿¿
********************************************************************************/
static int Open(const char* name, const char* args, int* threq)
{
	HK_DEBUG_PRT("scc......Open: %s..........\n", name);
	HI_S32 s32Ret;
	if (strcmp(name, "video.vbVideo.MPEG4") == 0) //open for main stream.
	{
		/**Main stream Venc Chn==> 0:960P/720P; 1:VGA, if 3518e, only chn: 0**/
		*threq = 0;
		int iRet = sccInitVideoData( PSTREAMONE );
		if (-1 == iRet)
		{
			HK_DEBUG_PRT("...init video data failed !!!!\n");
			return 0;
		}

		if (0 == g_isH264Open)
		{
			g_isH264Open = 1;
			*threq       = 1;
			sccResetVideData( PSTREAMONE, hostVideoDataP );
			CreateVideoThread(); //create Get Video Thread
		}
		else
		{
			HI_MPI_VENC_RequestIDRInst(0);
		}
		return MPEG4;
	}
	else if (strcmp(name, "video.vbVideo.M_JPEG") == 0) //open for sub stream.
	{
		/**sub stream Venc Chn==> 2:VGA; 3:QVGA, if 3518e, only chn: 1**/
		*threq = 0;
		int iRet = sccInitVideoData( PSTREAMTWO );
		if (-1 == iRet)
		{
			HK_DEBUG_PRT("...init video data failed !!!!\n");
			return 0;
		}

		if (0 == g_isMjpegOpen)
		{
			g_isMjpegOpen = 1;
			*threq        = 1;
			sccResetVideData( PSTREAMTWO, slaveVideoDataP );
			CreateSubVideoThread(); //create Get SubVideo Thread
		}
		else
		{
			HI_MPI_VENC_RequestIDRInst(1);
		}
		return M_JPEG;
	}
	return 0;
}

static unsigned int nTimeTick_H264 = 0;
static int read_H264_2(char* buf, unsigned int bufsiz, long* flags)
{
	static int evenlen = 0;
	enum { times = 3 };

	int i = 0;
	HI_S32 s32Ret;
	int iLen = 0;  //stream data size.
	int iFrame = 0;
	int iCifOrD1 = 0;
	int iStreamType = 0;

	if (g_ReadFlag_H264 == 1)
	{
		//memset(g_ReadVideobuf, '\0', sizeof(g_ReadVideobuf)); //note: high CPU.
		int iGetData = sccGetVideoDataSlave(PSTREAMONE, g_ReadVideobuf, &iLen, &iFrame, &iCifOrD1, &iStreamType);
		if ((iGetData <= 0) || (iLen <= 0) )
		{
			usleep(5000);
			return 0;
		}
	}

	if( iFrame == 1)
	{
		*flags= HK_BOAT_IFREAM;
	}
	else
	{
		*flags=  HK_BOAT_PFREAM;
	}

	//printf( "scc--iLen - %d...bufsiz=%d,,\n",iLen, bufsiz );
	if( nTimeTick_H264 == 0 )
	{
		nTimeTick_H264 = HKMSGetTick();
	}

	if ((iLen < bufsiz) && g_ReadFlag_H264)
	{
		memcpy( buf+2+4, g_ReadVideobuf, iLen );
		nTimeTick_H264 += (1000 / g_CurStreamFrameRate);
		//printf("......nTimeTick_H264:%d, g_CurStreamFrameRate:%d\n", nTimeTick_H264, g_CurStreamFrameRate);

		memcpy( buf+2, &nTimeTick_H264, 4 );
		hk_set_video_hdr(flags, (unsigned short*)buf, H264, g_iCifOrD1, 2);
		HK_MHDR_SET_FRAGX(*buf, 0);
		return iLen+2+4;
	}

	if ( g_ReadFlag_H264 && (iLen >= bufsiz) )
	{
		g_ReadFlag_H264 = 0;
		g_ReadTimes = 3;	
		evenlen = iLen / times;
		if( evenlen >= bufsiz ) 
			evenlen = bufsiz - 10;
		g_ReadSizes = evenlen + iLen % times;	
		memcpy( buf+2+4, g_ReadVideobuf, g_ReadSizes );
		nTimeTick_H264 += (1000 / g_CurStreamFrameRate);
		memcpy( buf+2, &nTimeTick_H264, 4 );

		/*******************************************
		 * 9: HD720p.  5: VGA.  3:QVGA.
		 *******************************************/
		hk_set_video_hdr(flags, (unsigned short*)buf, H264,  g_iCifOrD1, 2);
		HK_MHDR_SET_FRAGX(*buf, g_ReadTimes);

		g_ReadTimes--;
		return g_ReadSizes+2+4;
	}

	if ( g_ReadFlag_H264 == 0 )
	{
		*flags = HK_BOAT_PFREAM; //P frame
		memcpy( buf+2, g_ReadVideobuf+g_ReadSizes+(evenlen*(times-g_ReadTimes-1)), evenlen );

		hk_set_video_hdr(flags, (unsigned short*)buf, H264,  g_iCifOrD1, 2);
		HK_MHDR_SET_FRAGX(*buf, g_ReadTimes);

		g_ReadTimes--;
		if (g_ReadTimes == 0)
		{
			g_ReadFlag_H264 = 1;
		}
		return evenlen+2;
	}
	return 0;
}
#if 0
static int sccRead_SubH264( )
{
	char videobuf[videobuflen] = {0};
	int iFrame=0;
	int i = 0;
	HI_S32 s32Ret = 0;
	fd_set read_fds;
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.
	int iLen = 0; //count stream data length.

	s_vencFd = g_VencFd_Sub;
	s_maxFd = s_vencFd + 1;
	s_vencChn = g_Venc_Chn_S_Cur; 

	VENC_STREAM_S stMjpegStream;
	//if ( g_ReadFlag_Mjpeg )
	{
		FD_ZERO( &read_fds );
		for (i = 0; i < s32ChnTotal; i++)
		{
			FD_SET( s_vencFd, &read_fds );
		}

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret < 0)
		{
			SAMPLE_PRT("select failed!\n");
			return -1;
		}
		else if (s32Ret == 0)
		{
			SAMPLE_PRT("get venc stream time out, exit thread\n");
			return 0;
		}
		else
		{
			i = 0;
			if (FD_ISSET( s_vencFd, &read_fds )) //wait for prepared VENC_FD.
			{
				/*******************************************************
				  step 2.1 : query how many packs in one-frame stream.
				 *******************************************************/
				memset(&stMjpegStream, 0, sizeof(stMjpegStream));
				s32Ret = HI_MPI_VENC_Query( s_vencChn, &stJpegStat );
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", i, s32Ret);
					return -1;
				}

				/*******************************************************
				  step 2.2 : malloc corresponding number of pack nodes.
				 *******************************************************/
				stMjpegStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stJpegStat.u32CurPacks);
				if (NULL == stMjpegStream.pstPack)  
				{
					SAMPLE_PRT("malloc jpeg stream pack filed!\n");
					return -1;
				}

				/*******************************************************
				  step 2.3 : call mpi to get one-frame stream
				 *******************************************************/
				stMjpegStream.u32PackCount = stJpegStat.u32CurPacks;

				s32Ret = HI_MPI_VENC_GetStream( s_vencChn, &stMjpegStream, HI_TRUE );
				if (HI_SUCCESS != s32Ret) 
				{
					SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n",s32Ret);
					free(stMjpegStream.pstPack);
					stMjpegStream.pstPack = NULL;
					return 0;
				}

				for( i=0; i< stMjpegStream.u32PackCount; i++)
				{
					memcpy( videobuf + iLen, stMjpegStream.pstPack[i].pu8Addr[0], stMjpegStream.pstPack[i].u32Len[0] );
					int enType = stMjpegStream.pstPack[i].DataType.enH264EType; 

					if( enType == H264E_NALU_PSLICE )
					{
						//*flags = HK_BOAT_PFREAM; //P frame
						iFrame=0;							
						//g_sH264VideoBuf->dwFrameType = VIDEO_P_FRAME;
					}
					else if(enType == H264E_NALU_BUTT)
					{
						HI_MPI_VENC_ReleaseStream( s_vencChn, &stMjpegStream);
						if( NULL!= stMjpegStream.pstPack)
						{
							free(stMjpegStream.pstPack);
							stMjpegStream.pstPack = NULL;
						}
						return 0;
					}
					else
					{
						//*flags = HK_BOAT_IFREAM; //I frame
						iFrame=1;						
						// g_sH264VideoBuf->dwFrameType = VIDEO_I_FRAME;
					}
					iLen += stMjpegStream.pstPack[i].u32Len[0];
					if( stMjpegStream.pstPack[i].u32Len[1] > 0 )
					{
						memcpy( videobuf+iLen, stMjpegStream.pstPack[i].pu8Addr[1], stMjpegStream.pstPack[i].u32Len[1] );
						iLen += stMjpegStream.pstPack[i].u32Len[1];
					}
				} //end for()
			} //end FD_ISSET
		}//end else
	}//end ReadFlag


	/**Sub stream OSD (wangshaoshu)**/
	RGN_HANDLE RgnHandle = 0;
	RgnHandle = 3 + s_vencChn;
	OSD_Overlay_RGN_Display_Time(RgnHandle,s_vencChn); 
	/** Sub stream OSD end **/

#if 0 //wss 20141015	 rtsp	
	memcpy(g_sH264VideoBuf->VideoBuffer, videobuf, iLen );
	g_sH264VideoBuf->bzEncType	  = ENC_AVC;
	g_sH264VideoBuf->dwBufferType   = VIDEO_LOCAL_RECORD;
	g_sH264VideoBuf->dwFrameNumber  = stMjpegStream.u32Seq; 
	g_sH264VideoBuf->dwWidth 	  = g_stVencChnAttrSlave.stVeAttr.stAttrH264e.u32PicWidth;
	g_sH264VideoBuf->dwHeight	  = g_stVencChnAttrSlave.stVeAttr.stAttrH264e.u32PicHeight;
	g_sH264VideoBuf->dwSec		  = stMjpegStream.pstPack[0].u64PTS/1000;
	g_sH264VideoBuf->dwUsec		  = (stMjpegStream.pstPack[0].u64PTS%1000)*1000;
	g_sH264VideoBuf->dwFameSize	  = iLen;	
	IPCAM_PutStreamData(VIDEO_LOCAL_RECORD, 1, VOIDEO_MEDIATYPE_VIDEO, g_sH264VideoBuf);
#endif

	if( g_isMjpegOpen  == 1 && iLen > 0 && iLen < videobuflen )
	{
		//printf("scc Push Video len=%d...\n", iLen);
		sccPushStream(1234,PSTREAMTWO, videobuf, iLen, iFrame,g_iCifOrD1, H264 );
	}
	if(hkSdParam.sdrecqc == 0)
	{
		sccPushTfData(PSTREAMTWO,videobuf, iLen,iFrame, g_iCifOrD1,H264 );
	}

	s32Ret = HI_MPI_VENC_ReleaseStream( s_vencChn, &stMjpegStream );
	if (s32Ret) 
	{
		//printf("HI_MPI_VENC_ReleaseStream:0x%x\n",s32Ret);
		free(stMjpegStream.pstPack);
		stMjpegStream.pstPack = NULL;
		return -1;
	}
	free(stMjpegStream.pstPack);
	stMjpegStream.pstPack = NULL;

	//usleep(1000*10);
	return 0;
}
#endif

static unsigned int nTimeTick_MJPEG = 0;
static int read_MJPEG_2( char *buf, unsigned int bufsiz, long *flags )
{
	int i = 0;
	HI_S32 s32Ret = 0;
	*(long long int*)buf = 0;  //init target buffer.
	static int evenlen = 0;
	int iLen = 0; //count stream data length.
	enum { times = 3 };
	int iFrame = 0;
	int iCifOrD1 = 0;
	int iStreamType = 0;

	if (g_ReadFlag_Mjpeg == 1)
	{
		//memset(g_MjpegVideobuf, '\0', sizeof(g_MjpegVideobuf)); //note: high CPU.
		int iGetData = sccGetVideoDataSlave(PSTREAMTWO, g_MjpegVideobuf, &iLen, &iFrame, &iCifOrD1, &iStreamType);
		if ((iGetData <= 0) || (iLen <= 0) )
		{
			//HK_DEBUG_PRT("get video data slave failed !!!!!\n");
			usleep(5000);
			return 0;
		}
	}

	if( iFrame == 1)
	{
		*flags= HK_BOAT_IFREAM;
	}
	else
	{
		*flags=  HK_BOAT_PFREAM;
	}

	if( nTimeTick_MJPEG == 0 ) 
	{
		nTimeTick_MJPEG = HKMSGetTick();
	}

	if ( iLen < bufsiz && g_ReadFlag_Mjpeg )
	{
		memcpy( buf+2+4, g_MjpegVideobuf, iLen );
		nTimeTick_MJPEG += (1000 / g_CurStreamFrameRate);
		memcpy( buf+2, &nTimeTick_MJPEG, 4 );

		hk_set_video_hdr(flags, (unsigned short*)buf, H264, g_sunCifOrD1, 3);
		HK_MHDR_SET_FRAGX(*buf, 0);
		return iLen+2+4;
	}

	if ( g_ReadFlag_Mjpeg && (iLen >= bufsiz) )
	{
		g_ReadFlag_Mjpeg = 0;
		g_ReadTimesMjpeg = 3;	

		evenlen = iLen / times;
		g_ReadSizesMjpeg = evenlen + iLen % times;	
		memcpy(buf+2+4, g_MjpegVideobuf, g_ReadSizesMjpeg );
		nTimeTick_MJPEG += (1000 / g_CurStreamFrameRate);
		memcpy( buf+2, &nTimeTick_MJPEG, 4 );

		hk_set_video_hdr(flags, (unsigned short*)buf, H264, g_sunCifOrD1, 3);
		HK_MHDR_SET_FRAGX(*buf, g_ReadTimesMjpeg);

		g_ReadTimesMjpeg--;

		return g_ReadSizesMjpeg+2+4;
	}

	if( g_ReadFlag_Mjpeg == 0 )
	{
		*flags = HK_BOAT_PFREAM; //P frame
		memcpy(buf+2,g_MjpegVideobuf+g_ReadSizesMjpeg+(evenlen*(times-g_ReadTimesMjpeg-1)),evenlen);

		hk_set_video_hdr(flags, (unsigned short*)buf, H264, g_sunCifOrD1, 3);
		HK_MHDR_SET_FRAGX(*buf, g_ReadTimesMjpeg);

		g_ReadTimesMjpeg--;
		if(g_ReadTimesMjpeg == 0)
		{
			g_ReadFlag_Mjpeg = 1;
		}
		return evenlen+2;
	}
	return 0;
}


static int Read(int obj, char* buf, unsigned int bufsiz, long* flags)
{
	if (obj == MPEG4) 
	{
		if ( g_bCifOrD1 )
			return 0;

		if (1 == g_isH264Open)
		{
			g_isH264Open = 2;
			HI_MPI_VENC_RequestIDRInst(0);
		}

		int iSize = read_H264_2(buf, bufsiz, flags);
		return iSize;
	}

	if (obj == M_JPEG)
	{
		if (g_bPhCifOrD1) 
			return 0;

		if (1 == g_isMjpegOpen)
		{
			g_isMjpegOpen = 2;
			HI_MPI_VENC_RequestIDRInst(1);
		}

		int iSize = read_MJPEG_2(buf, bufsiz, flags);
		return iSize;
	}
	return 0;
}


/**********************************************
 * func: update configuration parameters 
 *********************************************/
static int write_config(char *key, int value) 
{
	FILE *fd_r = NULL;
	FILE *fd_w = NULL;
	char line[64] = {0};
	int WriteFlag = 0; 

	if( (fd_r = fopen( "/mnt/sif/hkipc.conf", "r" )) == NULL )
		return -1;
	if( (fd_w = fopen( "/mnt/sif/hkipc.conf_", "w" )) == NULL )
	{    
		fclose(fd_r);
		return -1;
	}    

	while (fgets(line, 64, fd_r) != NULL)
	{    
		if (strstr(line, key) != NULL)
		{    
			WriteFlag = 1; 
			//if(strcmp("VinFormat",key)==0)
			//fprintf(fd_w,"%s=%s\n",key,video_format_names_[value]);
			//else 
			fprintf(fd_w, "%s=%d\n", key, value);
		}    
		else 
		{    
			fputs(line, fd_w);
		}    
	}    

	if(WriteFlag == 0)
	{    
		fprintf(fd_w, "%s=%d\n", key, value);
	}    

	fclose(fd_r);
	fclose(fd_w);

	remove("/mnt/sif/hkipc.conf");
	rename("/mnt/sif/hkipc.conf_", "/mnt/sif/hkipc.conf");
	system("sync");

	return 1;
}


/**********************************************************
 * func: set resolution according to client side.
 *********************************************************/
void OnMonsetDevResolutionOv2643( const char *ev )
{
	int s32Ret = 0;
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iEnv = GetParamUN( pFrameHead, HK_KEY_RESOLU );
	int iFlag = GetParamUN( pFrameHead, HK_KEY_FLAG );

	HK_DEBUG_PRT(" g_iCifOrD1:%d, iEnv:%d, iFlag:%d, gVenc_Chn_M_Cur:%d.\n", g_iCifOrD1, iEnv, iFlag, g_Venc_Chn_M_Cur);
	if (iFlag == 1)
	{
		int iStream = GetParamUN( pFrameHead, "phstream" );
		int iRate = GetParamUN( pFrameHead, HK_KEY_RATE );
		hk_SetPhonePlaram(iStream, iEnv, iRate); //phone set.
		DestroyFrame( pFrameHead );
		return;
	}
	DestroyFrame( pFrameHead );

	if (g_iCifOrD1 == iEnv)
	{
		return;
	}

	HK_DEBUG_PRT("---> g_Venc_Chn_M_Cur:%d, g_VencFd_Main:%d, g_iCifOrD1:%d \n<---", g_Venc_Chn_M_Cur, g_VencFd_Main, g_iCifOrD1);
	return;
}


void OnMonSetDevIRate( const char* ev )
{
	HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	int iEnv = GetParamUN( pFrameHead, HK_KEY_IRATE );
	if ((iEnv > 0) && (iEnv <=1000))
	{
		video_properties_.vv[HKV_VEncIntraFrameRate] = iEnv;
		write_config( "VEncIntraFrameRate", iEnv );

		if ( g_iCifOrD1 >= 5 )
		{
			//stH264Attr.u32Gop = iEnv;
			//video_properties_.vv[HKV_VEncIntraFrameRate] = iEnv;
			//write_config("VEncIntraFrameRate",iEnv );
			//memset(&stAttr[0], 0 ,sizeof(VENC_CHN_ATTR_S));
			//stAttr[0].enType = PT_H264;
			//stAttr[0].pValue = (HI_VOID *)&stH264Attr;
			int iDoReso = HI_MPI_VENC_SetChnAttr( VeChn, &stAttr[0] );
			if( iDoReso != 0 )
			{
				printf("...change I Fail..%d..\n", iDoReso );
				DestroyFrame( pFrameHead );
				return;
			}
			printf("...change  I success..%d..\n", iDoReso );
		}
		else
		{
			//stCifH264Attr.u32Gop = iEnv;
			//memset(&stAttr[2], 0 ,sizeof(VENC_CHN_ATTR_S));
			//stAttr[2].enType = PT_H264;
			//stAttr[2].pValue = (HI_VOID *)&stCifH264Attr;
			int iDoReso = HI_MPI_VENC_SetChnAttr( VeChn, &stAttr[2] );
			if( iDoReso != 0 )
			{
				printf("...change I Fail..%d..\n", iDoReso );
				DestroyFrame( pFrameHead );
				return;
			}
			//printf("...change  I success..%d..\n", iDoReso );
		}
	}

	DestroyFrame( pFrameHead );
}


/*************************************************
 * func: set FrameRate according to client side.
 *************************************************/
void OnMonSetDevRate( const char* ev )
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iEnv = GetParamUN( pFrameHead, HK_KEY_RATE );
	HK_DEBUG_PRT("...g_iCifOrD1:%d, iEnv:%d...\n", g_iCifOrD1, iEnv);

	if ((iEnv > 0) && (iEnv <= 30))
	{
		video_properties_.vv[HKV_BitRate] = iEnv;
		write_config( "BitRate", iEnv );


		/**set VENC channel attributes**/
		static int s_chn = 0;
		if (g_iCifOrD1 >= 9) //960P or 720P.
		{
			s_chn = 0; //channel 0.
		}
		else if (5 == g_iCifOrD1) //VGA.
		{
			s_chn = 1; //channel 1.
		}
		else if (3 == g_iCifOrD1) //QVGA.
		{
			s_chn = 2; //channel 2.
		}

		int iDoReso = HISI_SetFrameRate(s_chn, iEnv);
		if (iDoReso)
		{
			printf("[%s, %d] ...channel:%d, change Rate Failed...%d...\n", __func__, __LINE__, s_chn, iDoReso);
			DestroyFrame( pFrameHead );
		}
	}

	DestroyFrame( pFrameHead );
}

void OnMonChangeResolution( const char* ev )
{
	int iDoReso = -1;
	HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );

	int iEnv = GetParamUN( pFrameHead, HK_KEY_RESOLU );
	if( iEnv < 30 )
		iEnv = 30;

	HK_DEBUG_PRT("......set bit rate: %d......\n", iEnv);
	video_properties_.vv[HKV_VinFormat] = iEnv;
	write_config( "VinFormat", iEnv );
	iDoReso = HISI_SetBitRate(g_s32venchn, iEnv);
	if( iDoReso != 0 )
	{
		printf("[%s, %d] change resolution Failed..%d..\n", __func__, __LINE__, iDoReso);
	}

	DestroyFrame( pFrameHead );
}

void OnMonSenSitivity( const char *ev )
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );

	int iEnv = GetParamUN( pFrameHead, HK_KEY_SENSITIVITY );
	HK_DEBUG_PRT("...scc...set...SenSitivity: %d, g_bAlarmThread: %d ...\n", iEnv, g_bAlarmThread);
	if (iEnv > 0)//client switch on motion detect.
	{
		g_bAlarmThread = true;
	}
	else if (0 == iEnv)
	{
		g_bAlarmThread = false;
		get_pic_flag = false; //start to snap pictures.
	}

	g_MotionDetectSensitivity = iEnv;
	g_iMonitorStatus = iEnv;
	video_properties_.vv[HKV_MotionSensitivity] = iEnv; 
	write_config( "MotionSensitivity", iEnv );
	DestroyFrame( pFrameHead );
}



static void onMonSend_inband_desc(int direct)
{
	HKFrameHead *pFrameHead = CreateFrameB();
	GetInitAlarmParam( pFrameHead );
	char bufMD[512] = {0};
	int iLen = GetFramePacketBuf( pFrameHead, bufMD, sizeof(bufMD) );
	bufMD[iLen] = '\0';
	DestroyFrame( pFrameHead );

#define HK_STR_INBAND_DESC HK_KEY_FROM"=%s;" \
	HK_KEY_ALERT_INFO"=%s;"\
	HK_KEY_IRATE"=%hhd;"\
	HK_KEY_COMPRESS"=%hhd;" \
	HK_KEY_SENSITIVITY"=%hhd;" \
	HK_KEY_RESOLU"=%d;"
#if 0
#define HK_STR_INBAND_DIGIT HK_KEY_RATE"=%hhd;" \
	HK_KEY_HUE"=%hhd;" \
	HK_KEY_SHARPNESS"=%hhd;" \
	HK_KEY_BITRATE"=%hhd;"\
	HK_KEY_BRIGHTNESS"=%hhd;" \
	HK_KEY_SATURATION"=%hhd;" \
	HK_KEY_CONTRAST"=%hhd;"\
	HK_KEY_FREQUENCY"=%hhd;" \
	HK_KEY_CBRORVBR"=%d;"\
	HK_KEY_VER"=%d;"\
	HK_KEY_PT"=%d;"\
	HK_KEY_IOIN"=%d;"\
	HK_KEY_EXPOSURE"=0;"\
	HK_KEY_QUALITE"=3;"\
	HK_KEY_FLAG"=2;"
#endif

	unsigned int nLevel = (2<<8);
	char prop[128];
	char buf[sizeof(HK_STR_INBAND_DESC) + sizeof(HK_STR_INBAND_DIGIT) + 256];
	int len = 0; 
	char FrequencyLevel = 60; 

	if (video_properties_.vv[HKV_FrequencyLevel] == 0)
		FrequencyLevel = 50;
	else if (video_properties_.vv[HKV_FrequencyLevel] == 1)
		FrequencyLevel = 60;
	else
		FrequencyLevel = 70;

	int iHue = 0;// video_properties_.vv[HKV_HueLevel];///4;
	int iBrig = video_properties_.vv[HKV_BrightnessLevel];///4;
	int iSatur = video_properties_.vv[HKV_CamSaturationLevel];///4;
	int iContr = video_properties_.vv[HKV_CamContrastLevel];///4;

	len = snprintf(buf, sizeof(buf), HK_STR_INBAND_DESC HK_STR_INBAND_DIGIT,
			getenv("USER"),
			bufMD,
			video_properties_.vv[HKV_VEncIntraFrameRate],
			video_properties_.vv[HKV_VEncCodec],
			video_properties_.vv[HKV_MotionSensitivity],
			video_properties_.vv[HKV_VinFormat],
			video_properties_.vv[HKV_BitRate],
			iHue,
			video_properties_.vv[HKV_SharpnessLevel],
			g_iCifOrD1,
			iBrig,
			iSatur,
			iContr,
			FrequencyLevel,
			video_properties_.vv[HKV_Cbr],
			g_DevVer,
			video_properties_.vv[HKV_Yuntai],
			g_irOpen
		      );   

	snprintf(prop, sizeof(prop),
			HK_KEY_EVENT"="HK_EVENT_INBAND_DATA_DESC";FD=%d;"HK_KEY_SUBRESOURCE"=video.vbVideo%s;Flag=%u;",
			direct,
			direct==MPEG4?".MPEG4":(direct==M_JPEG?".M_JPEG":""),
			nLevel);

	ev_irq_(&video_inst_, prop, buf, len);

	//printf("Raise:EvtInband: %s,,,,=%d",  buf,video_properties_.vv[HKV_VinFormat]);
}


static void OnMonSetReversal( const char *ev )
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iFlag = GetParamUN(pFrameHead, HK_KEY_DEVPARAM);
	DestroyFrame( pFrameHead );

	printf("scc...flip=%d..\n", iFlag);
	if (2 == iFlag)//r or l
	{
		int iFlp = video_properties_.vv[HKV_Mirror];
		if (! iFlp) 
		{
			video_properties_.vv[HKV_Mirror] = 1;
			HISI_SetMirror(1);
		}
		else
		{
			video_properties_.vv[HKV_Mirror] = 0;
			HISI_SetMirror(0);
		}
		write_config("Mirror", video_properties_.vv[HKV_Mirror]);
	}
	if (1 == iFlag)//up or down
	{
		int iFlp = video_properties_.vv[HKV_Flip];
		if (! iFlp) 
		{
			video_properties_.vv[HKV_Flip] = 1;
			HISI_SetFlip(1);
		}
		else
		{
			video_properties_.vv[HKV_Flip] = 0;
			HISI_SetFlip(0);
		}
		write_config("Flip", video_properties_.vv[HKV_Flip]);
	}
}


static void OnMonSetFocus( const char *ev )
{
	HKFrameHead *pFrameHead;
	int i,ulpcam;
	int flag = 0;
	static unsigned int tm=0;
	unsigned int tm1;

	int SendVideoIrq(int flag,int uipcam,int i)
	{
		char prop[128];
		char buf[64];
		int len = snprintf(buf, sizeof(buf),HK_KEY_FROM"=%s;"HK_KEY_FLAG"=%d;"HK_KEY_UIPARAM"=%d;"HK_KEY_FOCUS"=%d;", getenv("USER"), flag,uipcam,i);

		snprintf(prop, sizeof(prop)
				, HK_KEY_EVENT"="HK_EVENT_FOCUS";FD=%d;"HK_KEY_SUBRESOURCE"=video.vbVideo.%s;"
				, MPEG4, "MPEG4");
		ev_irq_(&video_inst_, prop, buf, len);

		snprintf(prop, sizeof(prop)
				, HK_KEY_EVENT"="HK_EVENT_FOCUS";FD=%d;"HK_KEY_SUBRESOURCE"=video.vbVideo.%s;"
				, M_JPEG, "M_JPEG");
		ev_irq_(&video_inst_, prop, buf, len);

		return 1;
	}

	tm1 = Getms() - tm;
	if (tm1 < 1000)
		return;

	tm = Getms();

	pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	i = GetParamUN( pFrameHead, HK_KEY_FOCUS);
	ulpcam = GetParamUN( pFrameHead, HK_KEY_UIPARAM);
	DestroyFrame( pFrameHead );

	if( video_properties_.vv[HKV_FocuMax] == 1 ) 
	{
		SendVideoIrq(flag, ulpcam, i);
		return;
	}

	//video_properties_.vv[HKV_Autolpt] = 0;
	return;
}    

#define CLIENT_CONF_ "/mnt/sif/hkclient.conf"

static int iOnconf_get_vb_int(const char* cf, const char* nm)
{
	char v[64];
	if (conf_get(cf, nm, v, sizeof(v)))
		return atoi(v);
	return 0;
}

//1 open; 0 close;
static void OnDevIrOrSignal( HKFrameHead *pFrameHead )
{
	int iFlag = GetParamUN( pFrameHead, HK_KEY_RESTYPE );
	int iOpenOrClose = GetParamUN( pFrameHead, HK_KEY_DEVPARAM );
	HK_DEBUG_PRT("...iflag:%d...iropen:%d...\n", iFlag, iOpenOrClose);
	if (iFlag == 1)//signal
	{
		//if( iOpenOrClose == 1 )
		//{
		//Ledstatus = !Ledstatus;
		//}
		//else
		//{
		//    Ledstatus = 1;
		//}
		//write_config("ledstatus", Ledstatus);
	}
	else if (iFlag == 2)//IR
	{
		g_irOpen = iOpenOrClose;
		write_config("iropen", iOpenOrClose);
	}
}

static void OnDevNetProxy( HKFrameHead *pFrameHead )
{
	int iID = GetParamUN(pFrameHead,HK_KEY_REC_ID);
	int  iSerID = GetParamUN(pFrameHead, HK_KEY_DEVPARAM);
	char *cAddr = GetParamStr(pFrameHead, HK_KEY_ADDR);
	if ( iID<=0 || iSerID<=0  || cAddr==NULL )
		return;
	int iSrvID = iOnconf_get_vb_int( CLIENT_CONF_, "SrvID");
	char *pSrvIP = NULL;
	if ( iSrvID == 0 )
	{
		char bufValue[64]={0};
		conf_get( CLIENT_CONF_, "PROXY", bufValue, 64);

		if(strncmp( bufValue, "www.uipcam", 10 )==0 )
		{
			char cSrvIP[64]={0};

			conf_set_int( CLIENT_CONF_, "SrvID", 730 );

			pSrvIP = conf_get( CLIENT_CONF_, "SrvIP", cSrvIP, 64);
			if ( pSrvIP == NULL )
			{
				//printf("..730..srvIP=%s..\n",bufValue);
				conf_set( CLIENT_CONF_, "SrvIP", bufValue );
			}
			conf_set_int( CLIENT_CONF_, "IndexID", iSerID );
			g_DevIndex = iSerID;
			conf_set(CLIENT_CONF_, "PROXY", cAddr );
		}

		return;
	}
	else if ( iID == iSrvID )
	{
		char cSrvIP[64]={0};
		pSrvIP = conf_get( CLIENT_CONF_, "SrvIP", cSrvIP, 64);
		if ( pSrvIP == NULL )
		{
			char cValue[64]={0};
			conf_get( CLIENT_CONF_, "PROXY", cValue, 64);

			conf_set( CLIENT_CONF_, "SrvIP", cValue );
		}
		conf_set_int( CLIENT_CONF_, "IndexID", iSerID );
		g_DevIndex = iSerID;
		conf_set(CLIENT_CONF_, "PROXY", cAddr );
	}
}

/*******************************************
 * PTZ control by client
 *******************************************/
extern int g_PtzRotateEnable;//ptz either rotate
extern int g_PtzRotateType; //type ==> 1:leftright; 2:updown; 3:all direction auto rotate.
extern int g_PtzStepType;  //ptz step by step left or right or up or down
extern int g_PtzPresetPos; //preset position.
extern int g_RotateSpeed;   //rotate speed
extern int g_UD_StepCount; //calculate the whole length from top to bottom.
extern int g_LR_StepCount; //calculate the whole lenght from left to right.
extern unsigned long g_tmPTZStart;
#if (ENABLE_QQ || ENABLE_P2P)
void OnCmdPtz(int ev )
{

	if (1)
	{
		switch (ev) //ptz rotate step by step.
		{
			case 1: //left
				printf("*****************1\n");
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 2; //right.
				break;
			case 2: //right.
				printf("*****************2\n");
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 1; //left.
				break;
			case 3: //up.
				printf("*****************3\n");
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 4; //down.
				break;
			case 4: //down.
				printf("*****************4\n");
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 3; //up.
				break;
			default:
				printf("*****************0\n");
				g_PtzRotateEnable = 0;
				g_PtzStepType = 0;	
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				break;
		}
		sleep(1);
		g_PtzRotateEnable = 0;
		g_PtzStepType = 0;
		g_PtzRotateType = 0;
		g_PtzPresetPos = 0;
	}
	else
	{    
		switch (ev) //step by step.
		{ 
			case 1:		
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 2; //left.
				break;
			case 2:
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 1; //right.
				break;
			case 3:
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 3; //up.
				break;
			case 4:
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 4; //down.
				break;
			default:
				g_PtzRotateEnable = 0;
				g_PtzStepType = 0;	
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				break;
		}
	}
	return;
}

#endif

void OnMonPtz( const char *ev )
{
	HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	int iPt = GetParamUN( pFrameHead, HK_KEY_PT );
	DestroyFrame( pFrameHead );

	if ( (1 == video_properties_.vv[HKV_Flip]) && (1 == video_properties_.vv[HKV_Mirror]) )
	{
		switch (iPt) //ptz rotate step by step.
		{
			case 1: //left
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 2; //right.
				break;
			case 2: //right.
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 1; //left.
				break;
			case 3: //up.
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 4; //down.
				break;
			case 4: //down.
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 3; //up.
				break;
			default:
				g_PtzRotateEnable = 0;
				g_PtzStepType = 0;	
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				break;
		}
		printf("[%s, %d]...iPt:%d, g_PtzRotateEnable:%d, g_PtzStepType:%d, g_PtzRotateType:%d, g_PtzPresetPos:%d...\n", __func__, __LINE__, iPt, g_PtzRotateEnable, g_PtzStepType, g_PtzRotateType, g_PtzPresetPos);
	}
	else
	{    
		switch (iPt) //step by step.
		{ 
			case 1:		
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 1; //left.
				break;
			case 2:
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 2; //right.
				break;
			case 3:
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 3; //up.
				break;
			case 4:
				g_PtzRotateEnable = 1;
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				g_PtzStepType = 4; //down.
				break;
			default:
				g_PtzRotateEnable = 0;
				g_PtzStepType = 0;	
				g_PtzRotateType = 0;
				g_PtzPresetPos = 0;
				break;
		}
		printf("[%s, %d]...iPt:%d, g_PtzRotateEnable:%d, g_PtzStepType:%d, g_PtzRotateType:%d, g_PtzPresetPos:%d...\n", __func__, __LINE__, iPt, g_PtzRotateEnable, g_PtzStepType, g_PtzRotateType, g_PtzPresetPos);
	}
	return;
}

void OnAutoLptspeed(const char* ev )
{
	HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	int iPtAuto = GetParamUN( pFrameHead, HK_KEY_DEVPARAM );
	DestroyFrame( pFrameHead );

	video_properties_.vv[HKV_Autolpt]= !video_properties_.vv[HKV_Autolpt];

	printf("[%s, %d]...iPtAuto:%d, video_properties_.vv[HKV_Autolpt]:%d, g_PtzRotateEnable:%d, g_PtzRotateType:%d...\n", __func__, __LINE__, iPtAuto, video_properties_.vv[HKV_Autolpt], g_PtzRotateEnable, g_PtzRotateType);
	if (1 == video_properties_.vv[HKV_Autolpt])
	{
		video_properties_.vv[HKV_Autolpt] = 1;
		Set_PTZ_PresetParams(9, g_LR_StepCount, g_UD_StepCount);
		time(&g_tmPTZStart);
	}
	else
	{
		g_PtzRotateEnable = 0;
		g_PtzRotateType = 0;
		g_PtzStepType = 0;	
		g_PtzPresetPos = 0;
		return;
	}

	if (1 == iPtAuto)
	{
		g_PtzRotateEnable = 1;
		g_PtzStepType = 0;	
		g_PtzPresetPos = 0;
		g_PtzRotateType = 1; //left & right auto rotate.
	}
	else if (2 == iPtAuto)
	{
		g_PtzRotateEnable = 1;
		g_PtzStepType = 0;	
		g_PtzPresetPos = 0;
		g_PtzRotateType = 2; //up & down auto rotate.
	}
	else if (3 == iPtAuto)
	{
		g_PtzRotateEnable = 1;
		g_PtzStepType = 0;	
		g_PtzPresetPos = 0;
		g_PtzRotateType = 3; //all direction auto rotate.
	}
}

//lptspeed
static void OnMonLptspeed( const char *ev )
{
	HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	int iPt = GetParamUN( pFrameHead, HK_KEY_PT );
	DestroyFrame( pFrameHead );

	if ((iPt > 0) && (iPt < 11))
	{
		g_PtzRotateEnable = 0;
		g_PtzRotateType = 0;
		g_PtzStepType = 0;	
		g_PtzPresetPos = 0;
		Set_PTZ_RotateSpeed(iPt);	
		g_RotateSpeed = Get_PTZ_RotateSpeed();
	}
	printf("[%s, %d]...iPt:%d, g_PtzRotateEnable:%d, g_RotateSpeed:%d...\n", __func__, __LINE__, iPt, g_PtzRotateEnable, g_RotateSpeed);
	return;
}

void OnDevPreset( HKFrameHead *pFrameHead )
{
	int iFlag = GetParamUN(pFrameHead, HK_KEY_RESTYPE);
	int iPreset = GetParamUN(pFrameHead, HK_KEY_DEVPARAM);

	if (1 == iFlag)//set
	{
		g_PtzRotateEnable = 0;
		g_PtzRotateType = 0;
		g_PtzStepType = 0;	
		g_PtzPresetPos = 0;
		Set_PTZ_PresetParams(iPreset, g_LR_StepCount, g_UD_StepCount);
	}
	else if( iFlag == 2)//get
	{    
		g_PtzRotateEnable = 1;
		g_PtzRotateType = 0;
		g_PtzStepType = 0;	
		g_PtzPresetPos = iPreset;
	}
	printf("[%s, %d]...iPreset:%d, iFlag:%d, g_PtzRotateEnable:%d, g_PtzPresetPos:%d, g_LR_StepCount:%d, g_UD_StepCount:%d...\n", __func__, __LINE__, iPreset, iFlag, g_PtzRotateEnable, g_PtzPresetPos, g_LR_StepCount, g_UD_StepCount);
	return;
}


/**********************************************************
 * func: set video stream mode (cbr:0  vbr:1).
 *********************************************************/
static void OnMonSetVbrOrCbr( HKFrameHead *pFrameHead )
{
	int iPreset = GetParamUN( pFrameHead, HK_KEY_DEVPARAM );

	HK_DEBUG_PRT("g_iCifOrD1:%d, iPreset:%d, video_properties_.vv[HKV_Cbr]:%d\n", g_iCifOrD1, iPreset, video_properties_.vv[HKV_Cbr]);

	if (video_properties_.vv[HKV_Cbr] == iPreset )
		return;
	else
	{
		video_properties_.vv[HKV_Cbr] = iPreset;
		write_config( "Cbr", iPreset );
	}

	VENC_CHN_ATTR_S stVencChnAttr;
	if (iPreset == 0) //0: CBR
	{
		stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
		printf(".........change to CBR........\n");
	}
	else              //1: VBR
	{
		stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
		printf(".........change to VBR........\n");
	}

	if (g_iCifOrD1 >= 9) //HD720P.
	{
		HISI_SetRCType(0, stVencChnAttr.stRcAttr.enRcMode);
		/*
		   int s32ret = HI_MPI_VENC_SetChnAttr( 0, &stVencChnAttr );
		   if( s32ret != 0 )
		   {
		   printf("[%s, %d] ...change cbr vbr Failed...%d...\n", __func__, __LINE__, s32ret);
		   return;
		   }
		 */
	}
	else if (5 == g_iCifOrD1) //VGA.
	{
		HISI_SetRCType(1, stVencChnAttr.stRcAttr.enRcMode);
		/* 
		   int s32ret = HI_MPI_VENC_SetChnAttr( 1, &stVencChnAttr );
		   if( s32ret != 0 )
		   {
		   printf("[%s, %d] ...change cbr vbr Failed...%d...\n", __func__, __LINE__, s32ret);
		   return;
		   }
		 */
	}
	else if (3 == g_iCifOrD1) //QVGA.
	{
		HISI_SetRCType(2, stVencChnAttr.stRcAttr.enRcMode);
		/*
		   int s32ret = HI_MPI_VENC_SetChnAttr( 2, &stVencChnAttr );
		   if( s32ret != 0 )
		   {
		   printf("[%s, %d] ...change cbr vbr Failed...%d...\n", __func__, __LINE__, s32ret);
		   return;
		   }
		 */
	}
	//printf("......change CBR or VBR mode success......\n");

	return;
}


/******************************************************
 * restort camera params to default settings.
 ******************************************************/
void OnResetToDefaultSettings(void)
{
	HK_DEBUG_PRT("...reset camera video params to default settings...\n");

	/**reset hkipc.conf**/
	video_properties_.vv[HKV_CamSaturationLevel] = 32;
	write_config("CamSaturationLevel", 32);

	video_properties_.vv[HKV_SharpnessLevel] = 38;
	write_config("SharpnessLevel", 38);

	video_properties_.vv[HKV_HueLevel] = 0;
	write_config("HueLevel", 0); //no need to set this.

	video_properties_.vv[HKV_BrightnessLevel] = 32;
	write_config("BrightnessLevel", 32);

	video_properties_.vv[HKV_CamContrastLevel] = 32;
	write_config("CamContrastLevel", 32 );

	//video_properties_.vv[HKV_MotionSensitivity] = 0;
	//write_config("MotionSensitivity", 0);

	//video_properties_.vv[HKV_Flip] = 0;
	//write_config("Flip", 0);

	//video_properties_.vv[HKV_Mirror] = 0;
	//write_config("Mirror", 0);
	video_properties_.vv[HKV_VinFormat] = 1024;
	write_config("VinFormat", 1024);

	video_properties_.vv[HKV_BitRate] = 25;
	write_config("BitRate", 25 );

	video_properties_.vv[HKV_Cbr] = 1;
	write_config("Cbr", 1);

	write_config("CifOrD1", 9); //10:960P, 9:720P.
	//write_config("oneptz", 1 );

#if 0
	/**reset subipc.conf**/
	conf_set_int(HOME_DIR"/subipc.conf", "stream", 3);
	conf_set_int(HOME_DIR"/subipc.conf", "rate", 15); //frame rate.
	conf_set_int(HOME_DIR"/subipc.conf", "sat", 32); //saturation.
	conf_set_int(HOME_DIR"/subipc.conf", "con", 32); //contrast.
	conf_set_int(HOME_DIR"/subipc.conf", "bri", 32); //brightness.
	conf_set_int(HOME_DIR"/subipc.conf", "sha", 38); //sharpness.
	conf_set_int(HOME_DIR"/subipc.conf", "enc", 5); //encode resolution.
	conf_set_int(HOME_DIR"/subipc.conf", "smooth", 1);
#endif

	/**reset video stream encode**/
	if ( MainStreamConfigurate() )
		HK_DEBUG_PRT("reset video params for main stream failed!\n");
	//if (SubStreamConfigurate() )
	//    HK_DEBUG_PRT("reset video params for sub stream failed!\n");

	printf("...reset camera main stream params to default settings ok!...\n");
	return;
}


#if 0
void OnMonSet99141ResetParam()
{
	//tvp7725_SetImageColor(0, DC_SET_SATURATION, 32);
	video_properties_.vv[HKV_CamSaturationLevel] = 32;
	write_config("CamSaturationLevel", 32 );

	//tvp7725_SetImageColor(0,DC_SET_SHARPNESS, 32);
	video_properties_.vv[HKV_SharpnessLevel] = 32;
	write_config("SharpnessLevel",32);

	//tvp7725_SetImageColor(0, DC_SET_HUE, 1);
	//if( video_properties_.vv[HKV_HueLevel] != 1)
	//    write_config("HueLevel", 1 );

	//tvp7725_SetImageColor(0, DC_SET_BRIGHT, 32);
	video_properties_.vv[HKV_BrightnessLevel] = 32;
	write_config("BrightnessLevel", 32);

	//tvp7725_SetImageColor(0, DC_SET_CONTRACT, 32);
	video_properties_.vv[HKV_CamContrastLevel] = 32;
	write_config("CamContrastLevel", 32 );
}

//reset param
void OnMonSet2643ResetParam( )
{
	//tvp7725_SetImageColor(0, DC_SET_SATURATION, 40);
	video_properties_.vv[HKV_CamSaturationLevel] = 40;
	write_config("CamSaturationLevel", 40 );

	//tvp7725_SetImageColor(0,DC_SET_SHARPNESS, 24);
	video_properties_.vv[HKV_SharpnessLevel] = 24;
	write_config("SharpnessLevel",24);

	//tvp7725_SetImageColor(0, DC_SET_HUE, 1);
	//if( video_properties_.vv[HKV_HueLevel] != 1)
	//    write_config("HueLevel", 1 );

	//tvp7725_SetImageColor(0, DC_SET_BRIGHT, 9);
	video_properties_.vv[HKV_BrightnessLevel] = 9;
	write_config("BrightnessLevel", 9);

	//tvp7725_SetImageColor(0, DC_SET_CONTRACT, 32);
	video_properties_.vv[HKV_CamContrastLevel] = 32;
	write_config("CamContrastLevel", 32 );
}
#endif

//reset param
void OnMonSetResetParam( )
{
	video_properties_.vv[HKV_CamSaturationLevel] = 32;
	write_config("CamSaturationLevel", 32);

	video_properties_.vv[HKV_HueLevel] = 1;
	write_config("HueLevel", 1 );

	video_properties_.vv[HKV_SharpnessLevel] = 38;
	write_config("SharpnessLevel", 38);

	video_properties_.vv[HKV_BrightnessLevel] = 32;
	write_config("BrightnessLevel", 32);

	video_properties_.vv[HKV_CamContrastLevel] = 32;
	write_config("CamContrastLevel", 32);
}


/*
   static void  setdevparam()
   {
   if( video_properties_.vv[HKV_CamSaturationLevel]> 0 )
//tvp7725_SetImageColor(0, DC_SET_SATURATION, video_properties_.vv[HKV_CamSaturationLevel]);

if( video_properties_.vv[HKV_SharpnessLevel]>0)
//tvp7725_SetImageColor(0,DC_SET_SHARPNESS, video_properties_.vv[HKV_SharpnessLevel]);

if(video_properties_.vv[HKV_BrightnessLevel]>0 )
//tvp7725_SetImageColor(0, DC_SET_BRIGHT, video_properties_.vv[HKV_BrightnessLevel]);

if(video_properties_.vv[HKV_CamContrastLevel] >0)
//tvp7725_SetImageColor(0, DC_SET_CONTRACT, video_properties_.vv[HKV_CamContrastLevel]);
}
 */

//IO IN 0:NO 1:NC
static void OnMonSetIoIn(HKFrameHead *pFrameHead)
{
	int iValue = GetParamUN(pFrameHead,HK_KEY_IOHIGHORLOW );
	HK_DEBUG_PRT("...set IO In, iValue:%d...\n", iValue);
	//stDevInfo.ioin=iValue;
	//sccSetSysInfo( g_pDvrInst, 105, iValue );
}


static void InitAlarmParam(int iRest)
{
	int i = 0, m = 1, n = 1;
	//g_tAlarmSet[2].bMotionDetect = 0;
	memset( &g_tAlarmSet, 0, sizeof(g_tAlarmSet) );
	memset( &g_bMDStartOK, 0, sizeof(g_bMDStartOK) );

	g_bMDXY = false;
	if (iRest >= 5)
	{
		int iCount = conf_get_int(HOME_DIR"/RngMD.conf", "D1Count");
		m = 1;
		n = 1;

		for (i = 0; i < iCount; i++)
		{
			char cXYbuf[54] = {0};
			char cWHbuf[54] = {0};
			snprintf( cXYbuf, sizeof(cXYbuf), "D1XY%d", i );
			snprintf( cWHbuf, sizeof(cWHbuf), "D1WH%d", i );

			int iXY = conf_get_int(HOME_DIR"/RngMD.conf", cXYbuf);
			int iWh = conf_get_int(HOME_DIR"/RngMD.conf", cWHbuf);
			if (iWh > 0)
			{
				unsigned short ix = HKGetHPid( iXY );//a
				unsigned short iy = HKGetLPid( iXY );//a
				unsigned short ixW = HKGetHPid(iWh) - ix;
				unsigned short iyH = HKGetLPid(iWh) - iy;
				//printf("...W=%d,,,H=%d...x=%d...y=%d..\n", ixW, iyH, ix, iy );

				g_tAlarmSet[i].bMotionDetect = 1;
				g_tAlarmSet[i].RgnX = ix;
				g_tAlarmSet[i].RgnY = iy;
				g_tAlarmSet[i].RgnW = ixW;
				g_tAlarmSet[i].RgnH = iyH;
				g_tAlarmSet[i].Sensitivity = g_iMonitorStatus;
				g_tAlarmSet[i].Threshold = g_iMonitorStatus;

				TAlarmSet_ *pAlmSet = &g_tAlarmSet[i];
				if ((pAlmSet->bMotionDetect) && (pAlmSet->RgnW > 0) && (pAlmSet->RgnH > 0))
				{
					int x, y;
					int md_x1 = pAlmSet->RgnX * m * C_W / 16;
					int md_y1 = pAlmSet->RgnY * n * C_H /16;
					int md_x2 = (pAlmSet->RgnX + pAlmSet->RgnW) * m *C_W / 16;
					int md_y2 = (pAlmSet->RgnY + pAlmSet->RgnH) * n *C_H / 16;
					//int md_stride = CIF_WIDTH_PAL * m*C_W / 16;
					int md_stride;
					if (iRest == ENUM_720P)
						md_stride = 1280 * m * C_W / 16;
					else if (iRest == ENUM_VGA)
						md_stride = 640 * m * C_W / 16;
					else
						md_stride = 704 * m * C_W / 16;


					memset(&g_MdMask[i][0], 0, MAX_MACROCELL_NUM);
					for(y = md_y1; y < md_y2; y++)
					{
						for(x = md_x1; x < md_x2; x++)
							g_MdMask[i][y * md_stride + x] = 1;
					}
					//if(do_md_enable(i) == HI_SUCCESS)
					g_bMDStartOK[i] = true;
					g_bMDXY = true;
				}
			}
		}
	}
	else
	{
		int iCount = conf_get_int(HOME_DIR"/RngMD.conf", "CifCount");
		m = 1;
		n = 1;

		for (i = 0; i < iCount; i++)
		{
			char cXYbuf[54] = {0};
			char cWHbuf[54] = {0};
			snprintf( cXYbuf, sizeof(cXYbuf), "CifXY%d", i );
			snprintf( cWHbuf, sizeof(cWHbuf), "CifWH%d", i );

			int iXY = conf_get_int(HOME_DIR"/RngMD.conf", cXYbuf);
			int iWh = conf_get_int(HOME_DIR"/RngMD.conf", cWHbuf);
			if (iWh > 0)
			{
				unsigned short ix = HKGetHPid( iXY );//a
				unsigned short iy = HKGetLPid( iXY );//a

				unsigned short ixW = HKGetHPid(iWh) - ix;
				unsigned short iyH = HKGetLPid(iWh) - iy;
				//printf("...W=%d,,,H=%d...x=%d...y=%d..\n", ixW, iyH, ix, iy );

				g_tAlarmSet[i].bMotionDetect = 1;
				g_tAlarmSet[i].RgnX = ix;
				g_tAlarmSet[i].RgnY = iy;
				g_tAlarmSet[i].RgnW = ixW;
				g_tAlarmSet[i].RgnH = iyH;
				g_tAlarmSet[i].Sensitivity = g_iMonitorStatus;
				g_tAlarmSet[i].Threshold = g_iMonitorStatus;

				TAlarmSet_ *pAlmSet = &g_tAlarmSet[i];
				if ((pAlmSet->bMotionDetect) && (pAlmSet->RgnW > 0) && (pAlmSet->RgnH > 0))
				{
					int x, y;
					int md_x1 = pAlmSet->RgnX * m * C_W / 16;
					int md_y1 = pAlmSet->RgnY * n * C_H /16;
					int md_x2 = (pAlmSet->RgnX + pAlmSet->RgnW) * m * C_W / 16;
					int md_y2 = (pAlmSet->RgnY + pAlmSet->RgnH) * n*C_H / 16;
					//int md_stride = CIF_WIDTH_PAL * m*C_W / 16;
					int md_stride;
					if (iRest == ENUM_Q720P)
						md_stride = 640 * m * C_W / 16;

					memset(&g_MdMask[i][0], 0, MAX_MACROCELL_NUM);
					for (y = md_y1; y < md_y2; y++)
					{
						for (x = md_x1; x < md_x2; x++)
							g_MdMask[i][y * md_stride + x] = 1;
					}
					//if(do_md_enable(i) == HI_SUCCESS)
					g_bMDStartOK[i] = true;
					g_bMDXY = true;
				}
			}
		}
	}
}

static void iMonitorStatus( )
{
	if (video_properties_.vv[HKV_MotionSensitivity] > 0 )
		g_iMonitorStatus = 11 - video_properties_.vv[HKV_MotionSensitivity];
	else
		g_iMonitorStatus = 0;

	return;
}

static void OnMonSetAlarmRng(HKFrameHead *pFrameHead )
{
	int i = 0;
	int iCount = GetParamUN( pFrameHead, HK_KEY_INDEX );
	int iSensit = GetParamUN( pFrameHead, HK_KEY_SENSITIVITY);
	int iResole = GetParamUN( pFrameHead, HK_KEY_RESOLU );

	video_properties_.vv[HKV_MotionSensitivity] = iSensit; 
	write_config("MotionSensitivity", iSensit);
	g_MotionDetectSensitivity = iSensit;
	iMonitorStatus();
	HK_DEBUG_PRT("...iCount:%d, iSensit:%d, iResole:%d, g_iCifOrD1:%d, g_MotionDetectSensitivity:%d, g_iMonitorStatus:%d...\n", iCount, iSensit, iResole, g_iCifOrD1, g_MotionDetectSensitivity, g_iMonitorStatus);

	iResole = g_iCifOrD1;
	if (iResole >= 5)
	{
		int iNumber = iCount;
		if (iCount > 0)
		{
			for (i = 0; i < iCount; i++)
			{
				char cKey[64] = {0};
				snprintf(cKey, sizeof(cKey), "%s%d", HK_KEY_POINTX, i);
				char cYKey[64] = {0};
				snprintf(cYKey, sizeof(cYKey), "%s%d", HK_KEY_POINTY, i);

				int iRngXY = GetParamUN( pFrameHead, cKey);
				int iRngWH = GetParamUN( pFrameHead, cYKey);
				printf("xy:%d...wh:%d...\n", iRngXY, iRngWH);
				if (iRngWH > 0)
				{
					char cXYbuf[54]={0};
					char cWHbuf[54]={0};
					snprintf( cXYbuf, sizeof(cXYbuf), "D1XY%d", i );
					snprintf( cWHbuf, sizeof(cWHbuf), "D1WH%d", i );

					conf_set_int( HOME_DIR"/RngMD.conf", cXYbuf, iRngXY );
					conf_set_int( HOME_DIR"/RngMD.conf", cWHbuf, iRngWH );
				}
				else
				{
					iNumber--;
				}
			}
		}
		conf_set_int( HOME_DIR"/RngMD.conf", "D1Count", iNumber );
	}
	else
	{
		int iNumber = iCount;
		//conf_set_int( HOME_DIR"/RngMD.conf", "CifCount", iCount );
		if (iCount > 0)
		{
			for (i = 0; i < iCount; i++)
			{
				char cKey[64]={0};
				snprintf(cKey, sizeof(cKey), "%s%d", HK_KEY_POINTX, i);
				char cYKey[64]={0};
				snprintf(cYKey, sizeof(cYKey), "%s%d", HK_KEY_POINTY, i);

				int iRngXY = GetParamUN( pFrameHead, cKey);
				int iRngWH = GetParamUN( pFrameHead, cYKey);
				if( iRngWH > 0 )
				{
					char cXYbuf[54] = {0};
					char cWHbuf[54] = {0};
					snprintf( cXYbuf, sizeof(cXYbuf), "CifXY%d", i );
					snprintf( cWHbuf, sizeof(cWHbuf), "CifWH%d", i );

					conf_set_int( HOME_DIR"/RngMD.conf", cXYbuf, iRngXY );
					conf_set_int( HOME_DIR"/RngMD.conf", cWHbuf, iRngWH );
				}
				else
				{
					iNumber--;
				}
			}
		}
		conf_set_int( HOME_DIR"/RngMD.conf", "CifCount", iNumber );
	}

	if (g_iMonitorStatus > 0)
	{
		if ( !g_bAlarmThread )
		{
			g_bAlarmThread = true;
		}
		InitAlarmParam(iResole);
	}
	else if (0 == g_iMonitorStatus)
	{
		printf("scc...Close Alarm Rng..\n");
		g_bAlarmThread = false;
		get_pic_flag = false;
	}
}


static void OnMonSetDevTranslate( const char * ev )
{
	HKFrameHead *pFrameHead;
	int ec, mc;
	pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );

	mc = GetParamUN( pFrameHead,HK_KEY_SUBCMD );
	switch (mc)
	{
		case HK_MON_DEV_IRORSIGNAL:
			OnDevIrOrSignal( pFrameHead );//IR signal
			break;
		case HK_MON_DEV_NETPROXY:
			OnDevNetProxy( pFrameHead );
			break;
		case HK_MON_DEV_PRESET://ptz set / get preset.
			OnDevPreset( pFrameHead );
			break;
		case HK_MON_SET_DEV_APERTURE://+ -
			break;
		case HK_MON_SET_VBRORCBR://cbr:0  or vbr:1 
			OnMonSetVbrOrCbr( pFrameHead );
			break;
		case HK_MON_SET_RESET_PARAM: //reset param
			OnResetToDefaultSettings(); 
			break;
		case HK_MON_SET_IO_IN://IO IN 0:NO 1:NC
			OnMonSetIoIn( pFrameHead );
			break;
		case HK_MON_SET_ALARM_RNG://RngMD
			OnMonSetAlarmRng( pFrameHead );
			break;
		default:
			break;
	}
	DestroyFrame(pFrameHead);
}

//static int s_fd7725 = -1;
#if 0
int open_7725()
{
	if( s_fd7725 < 0 )
	{
		if( g_HK_SensorType==HK_99141 )
		{
			s_fd7725 = open(TVP99141_DEV,O_RDWR);
		}
		else if(g_HK_SensorType==HK_OV2643)
			s_fd7725 = open(TVP2643_DEV,O_RDWR);
		else if(g_HK_SensorType==HK_OV7725)
			s_fd7725 = open(TVP7725_DEV,O_RDWR);
		else
			s_fd7725 = open(TVP5150_DEV, O_RDWR);
		if (s_fd7725 < 0)
		{
			printf("can't open OV...\n");
			return -1;
		}
	}
	return s_fd7725;
}

void close_7725()
{
	if (s_fd7725 > 0)
	{
		close(s_fd7725);
	}
}
#endif
/*
   static void OnDevIoAram( const char  *ev )
   {
   HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
   int iAlarm = GetParamUN( pFrameHead, HK_KEY_IOHIGHORLOW );
   DestroyFrame( pFrameHead );
   if( iAlarm == 5 )
   {
   printf("scc IO Alarm.....%d....\n", iAlarm);
   Hi_SetGpio_SetBit( 2,3,1 );
   sleep(2);
   Hi_SetGpio_SetBit( 2,3,0 );
   sleep(1);
   Hi_SetGpio_SetBit( 2,3,1 );
   sleep(2);
   Hi_SetGpio_SetBit( 2,3,0 );
   }
   }
 */

//50  60
static void OnMonFrequency(const char *ev)
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iValue = GetParamUN(pFrameHead, HK_KEY_DEVPARAM );
	DestroyFrame( pFrameHead );

	VIDEO_NORM_E enNorm;
	printf("set Frequency, iValue:%d\n", iValue);
	if (50 == iValue) //PAL(50Hz)
	{
		enNorm = VIDEO_ENCODING_MODE_PAL;
		HISI_SetLocalDisplay(enNorm);
		video_properties_.vv[HKV_FrequencyLevel] = 0;
		write_config("FrequencyLevel", 0);
	}
	else if (60 == iValue) //NTSC(60Hz)
	{
		enNorm = VIDEO_ENCODING_MODE_NTSC;
		HISI_SetLocalDisplay(enNorm);
		video_properties_.vv[HKV_FrequencyLevel] = 1;
		write_config("FrequencyLevel", 1);
	}
	else
	{
		enNorm = VIDEO_ENCODING_MODE_AUTO; //AUTO
		HISI_SetLocalDisplay(enNorm);
		video_properties_.vv[HKV_FrequencyLevel] = 2;
		write_config("FrequencyLevel", 2);
	}
}

static void OnMonSaturation( const char* ev )
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iSaturation = GetParamUN( pFrameHead, HK_KEY_DEVPARAM );
	DestroyFrame( pFrameHead );

	if (iSaturation > 0)
	{
		HISI_SetCSCAttr(iSaturation, 0, 0, 0);
		video_properties_.vv[HKV_CamSaturationLevel] = iSaturation;
		write_config( "CamSaturationLevel", iSaturation );
	}
}

static void OnMonSetSharpness( const char *ev )
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int nSharpness = GetParamUN( pFrameHead, HK_KEY_DEVPARAM);
	DestroyFrame( pFrameHead );

	if (nSharpness > 0)
	{
		HISI_SetSharpNess(nSharpness);	
		video_properties_.vv[HKV_SharpnessLevel] = nSharpness;
		write_config("SharpnessLevel", nSharpness);
	}
}

/*
   static void OnMonExposure( const char *ev )
   {
   HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
   int iExpos = GetParamUN( pFrameHead, HK_KEY_DEVPARAM);
   DestroyFrame( pFrameHead );

   if( iExpos == 1 )
   {
//tvp7725_SetImageColor(0,DC_SET_SCENE,DC_VAL_OUTDOOR );
}
else if( iExpos ==2 )
{
//tvp7725_SetImageColor(0,DC_SET_SCENE,DC_VAL_INDOOR );
}
else
//tvp7725_SetImageColor(0,DC_SET_SCENE,DC_VAL_MANUAL );

}
 */

//color
static void OnMonColor(const char* ev )
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iColor = GetParamUN( pFrameHead, HK_KEY_DEVPARAM );
	DestroyFrame( pFrameHead );

	if( iColor > 0 )
	{
		//HISI_SetCSCAttr(0,0,0,iColor);
		video_properties_.vv[HKV_HueLevel] = iColor;
		write_config("HueLevel", iColor );
	}
}

static void OnMonBrightness(const char* ev )
{
	HKFrameHead* pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	int iBrightness = GetParamUN( pFrameHead,HK_KEY_DEVPARAM );
	DestroyFrame( pFrameHead );

	if (iBrightness > 0)
	{
		HISI_SetCSCAttr(0, iBrightness, 0, 0);
		video_properties_.vv[HKV_BrightnessLevel] = iBrightness;
		write_config("BrightnessLevel", iBrightness);
	}
}

//Contrast
static void OnMonContrast(const char* ev)
{
	HKFrameHead* pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	int iContrast = GetParamUN( pFrameHead, HK_KEY_DEVPARAM );
	DestroyFrame( pFrameHead );
	//iContrast *= 4;
	if (iContrast > 0)
	{
		HISI_SetCSCAttr(0, 0, iContrast, 0);
		video_properties_.vv[HKV_CamContrastLevel] = iContrast;
		write_config("CamContrastLevel", iContrast);
	}
}
#if 0
int32_t tvp7725_SetImageColor(int32_t nViChn, int32_t nType, int32_t nValue)
{
	int32_t fd;
	int32_t ret;

	fd = s_fd7725;
	if (fd < 0)
	{
		fd = open_7725();
		if ( fd < 0 )
		{
			return -1;
		}
	}
	ret = ioctl(fd, nType, nValue);

	return ret;
}
#endif

static int VDoEvent( const char* devname, int obj, const char* ev ) 
{
	HKFrameHead *pFrameHead;
	int ec, mc;

	if (ev == NULL)
	{
		return 1;
	}

	pFrameHead = CreateFrameA( (char*)ev, strlen(ev) );
	mc = GetParamUN( pFrameHead, HK_KEY_MAINCMD );
	DestroyFrame( pFrameHead );

	//HK_DEBUG_PRT("..........client set val: %d ...........\n", mc);
	switch(mc)
	{
		case HK_MON_DEVRESOLUTION:
			OnMonChangeResolution( ev ); //bit rate.
			break;
		case HK_RS_NEW_VISITOR://24.
			onMonSend_inband_desc(MPEG4);
			break;
		case HK_MON_SENSITIVITY:
			OnMonSenSitivity( ev ); //motion detect sensitivity.
			break;
		case HK_MON_DEVVIDEOREVERSAL: // 
			OnMonSetReversal( ev );
			break;
		case HK_MON_CONTROLFU: // Set Focus, Added by lingyb
			OnMonSetFocus(ev);
			break;
		case HK_MON_UPDATEDEVRATE: //set framerate.
			OnMonSetDevRate( ev );
			break;
		case HK_MON_DEV_IRATE:
			OnMonSetDevIRate( ev );//I frame interval.
			break;
		case HK_MON_DEV_RESOLUTION://video resolution.
			OnMonsetDevResolutionOv2643( ev );
			break;
		case HK_MON_DEV_TRANSLATE:
			OnMonSetDevTranslate( ev ); //translate.
			break;
		case HK_MON_CONTROLPT: //PTZ step by step.
			#if ENABLE_P2P
			OnMonPtz( ev);
			#endif
			break;
		case HK_MON_AUTOLPTSPEED:
			OnAutoLptspeed( ev ); //auto curise.
			break;
		case HK_MON_LPTSPEED://lptspeed
			OnMonLptspeed( ev );
			break;
		case HK_MON_CONTRAST://Contrast
			OnMonContrast( ev );
			break;
		case HK_MON_BRIGHTNESS:
			OnMonBrightness( ev );
			break;
		case HK_MON_COLOR://color
			//OnMonColor( ev );
			break;
		case HK_MON_SATURATION:
			OnMonSaturation( ev );
			break;
		case HK_MON_SHARPNESS:
			OnMonSetSharpness( ev );
			break;
		case HK_MON_EXPOSURE:
			//OnMonExposure( ev );
			break;
		case HK_MON_FREQUENCY://50  60
			OnMonFrequency(ev); //PAL or NTSC or AUTO
			break;
		case HK_MON_DEVIOARAM:
			//OnDevIoAram( ev );
		case HK_MON_RESTORATION_PARAM:
			break;
		default:
			break;
	}

	return 0;
}

static int WriteNull(int obj, const char* data, unsigned int len, long flags)
{
	return len;
}

/* This function is Added by lingyb, for Alarm test  */
static void SetIRQEventCallback(RSObjectIRQEvent cb) 
{
	ev_irq_ = cb;
}

static const char* Retrieve()
{
	return "video.vbVideo.MPEG4,video.vbVideo.M_JPEG,video.vbVideo.TF,video.vbVideo.NULL";
}

static const char* GetObjectInfo()
{
	return "Type=video.vbVideo;Outpin=MPEG4,M_JPEG,H263;";
	return HI_SUCCESS;
}

void video_RSLoadObjects(RegisterFunctType reg) 
{
	/**video resolution**/
	g_iCifOrD1   = conf_get_int(HOME_DIR"/hkipc.conf", "CifOrD1");//main stream.
	g_sunCifOrD1 = conf_get_int(HOME_DIR"/subipc.conf", "enc");   //sub stream.

	HK_DEBUG_PRT("......hk platform: hi3518E, g_iCifOrD1:%d, g_sunCifOrD1:%d......\n", g_iCifOrD1, g_sunCifOrD1);
	/**main stream**/
	if ((0 == g_iCifOrD1) || (g_iCifOrD1 > 5))
	{
		g_s32venchn = 0; //chn: 0.
		g_iCifOrD1 = ENUM_720P; //9: 1280*720.
	}

	/**sub stream**/
	//if (g_sunCifOrD1 >= 4) //5 ==> VGA:640*480.
	{
		VeSunChn = 1;   //Venc Channel 1: VGA (640*480)
		g_sunCifOrD1 = ENUM_VGA ; //5
	}

	/**config main stream params**/
	if ( MainStreamConfigurate() )
		printf("[%s, %d] configurate main stream failed !\n", __func__, __LINE__); 
	else
		printf("[%s, %d] configurate main stream success !\n", __func__, __LINE__); 

	/**video api for client callback**/
	video_inst_.SetIRQEventCallback = &SetIRQEventCallback;
	video_inst_.Retrieve            = &Retrieve;
	video_inst_.GetObjectInfo       = &GetObjectInfo;
	video_inst_.Open                = &Open;
	video_inst_.Read                = &Read;
	video_inst_.Write               = &WriteNull;
	video_inst_.DoEvent             = &VDoEvent;
	video_inst_.Close               = &Close;
	video_inst_.Convert             = NULL;
	ev_irq_                         = NULL;
	(*reg)(&video_inst_); //register callback function.

	iMonitorStatus();

	/**motion detect**/
	HK_DEBUG_PRT("motionsensitivity: %d, g_bAlarmThread: %d\n", video_properties_.vv[HKV_MotionSensitivity], g_bAlarmThread);
	g_MotionDetectSensitivity = video_properties_.vv[HKV_MotionSensitivity];
	if ( VDA_MotionDetect_Start() ) //enable motion detect.
	{
		HK_DEBUG_PRT("start motion detect failed !\n"); 
	}

}
#endif
