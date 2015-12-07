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
#include "utils/HKMonCmdDefine.h"
#include "utils/HKCmdPacket.h"

#include "osd_region.h"
#include "ipc_hk.h"
#include "ipc_email.h"
//#include "ipc_file_sd.h"
#include "ptz.h"
#include "HISI_VDA.h"
#include "sample_comm.h"
#include "ipc_vbAudio.h"
//#include "scc_video.h"
#include "utils_biaobiao.h"
//add by biaobiao
#define RECORD 0
#define NEW_RECORD 1
#include "record.h"
#include "ipc_sd.h"
extern pthread_mutex_t record_mutex; 
#if RECORD
#include "recordStruct.h"
#include "recordSDK.h"
#endif


#if ENABLE_ONVIF
#include "IPCAM_Export.h"
#endif

#if ENABLE_QQ
#include "TXAudioVideo.h"
#endif

#define MPEG4           1
#define M_JPEG          2

#define H263            4
#define H264            4
#define H264_TF         6
#define NULL 0


/*报警检查*/
extern int g_startCheckAlarm;
//extern int g_IRCutCurState;

/********************* IO alarm & email ***********************/
#define PATH_SNAP "/mnt/mmc/snapshot"
HI_S32 g_vencSnapFd = -1; //picture snap fd.
short g_SavePic = 0;
bool get_pic_flag = false; //enable snap pictures.
static int g_SnapCnt = 0; //picture snap counter.
static bool g_bIsOpenPict = false;


int imgSize[MAXIMGNUM] = {0};
//截图的存储空间
char g_JpegBuffer[MAXIMGNUM][ALLIMGBUF] = {0}; //picture buffer.

//enable picture snap for sending alarm email.
//static int g_OpenAlarmEmail = 0; //enable send alarm email.

//HKEMAIL_T hk_email; //email info struct.

/*移动侦测灵敏度*/
int g_MotionDetectSensitivity = 0;

/*主码流分辨率索引*/
enumVGAMode g_iCifOrD1;

/*次码流分辨率索引*/
enumVGAMode g_sunCifOrD1;

/*编码器属性*/
VENC_ATTR_H264_S stH264Attr;

extern short g_sdIsOnline;

extern HK_SD_PARAM_ hkSdParam;

/********* Open & Read video stream parameters ***********/

/*通道索引*/
HI_S32 g_s32venchn = 0; //main stream channel.
VENC_CHN g_s32sunvenchn = 1; //sub Venc Channel.

VENC_CHN g_Venc_Chn_M_Cur = 0;  //current main stream VENC_CHN index.
VENC_CHN g_Venc_Chn_S_Cur = 1;  //current sub stream VENC_CHN index.


/*打开的标记*/
int g_isH264Open = 0;   //main stream open flag.
int g_isMjpegOpen = 0;  //sub stream open flag.
int g_isTFOpen = 0;

/*读取码流相关*/
static int g_ReadFlag_H264 = 1;   //main read flag.
static int g_ReadFlag_Mjpeg = 1;  //sub read flag.
static int g_ReadTimes = 3;

/***************** Venc Fd ******************/
HI_S32 g_VencFd_Main = 0;    //current main stream fd.
HI_S32 g_VencFd_Sub = 0;     //current sub stream fd.


/*设置当前帧率*/
unsigned int g_CurStreamFrameRate = 20;

/*图像质量相关*/
#define VBR_MAXQP_33    33
#define VBR_MAXQP_38    38
unsigned int g_VbrMaxQq = VBR_MAXQP_33;
unsigned int g_VbrMaxQq_Sub = VBR_MAXQP_33;

/*码率计算相关*/
static  unsigned int g_MainRataTime = 0;
static  unsigned int g_MainVideoSize = 0;


#if ENABLE_P2P
void OnCmdPtz( int ev )
{
	switch (ev) //ptz rotate step by step.
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
	return;	
}
#endif

/*
获取主码流的码率
*/
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
 *fun: 设置帧率
 *author: wangshaoshu
 *biaobiao checked 
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

/*
 *设置码率
*/
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

		stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = iBitRate; //512; /* average bit rate */
#if 0
		if (g_iCifOrD1 >= 9)
			stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = 200; //512; /* average bit rate */
		else
			stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate     = 32; //256*3; /* average bit rate */
#endif

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

#endif

//////////Isp param init//////////////////////////////////////////////////////
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
	//stAEAttrEx.u8ExpCompensation = 200;  //add ????????
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

#if 0
static void raise_alarm(const char *res, int vfmt)
{
	char prop[256] = {0};
	char cont[32] = {0};
	int len = sprintf(cont, HK_KEY_FROM"=%s;", getenv("USER"));
	unsigned int nLevel = 1 << 8; //(vfmt==MPEG4 ? 2 : 3) << 8;
	sprintf(prop, HK_KEY_EVENT"="HK_EVENT_ALARM";"HK_KEY_SUBRESOURCE"=%s;FD=%d;Flag=%u;", res, vfmt, nLevel);
	//ev_irq_( &video_inst_, prop, cont, len );

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
#endif

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
	char acFile[64]  = {0};
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	//sprintf(acFile, "snap_%d.jpg", num);
	strftime(acFile, sizeof(acFile),PATH_SNAP"/snap_%Y%m%d%H%M%S.jpg", timeinfo);
	printf("the file name  is %s\n", acFile);

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
	printf("........................................save the jpeg\n");
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
				printf("......... aaaaaaa ........\n");
				return HI_SUCCESS;
			}
			printf("......... bbbbbbb ........\n");

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
			if ((streamSize <= (ALLIMGBUF-40*1024)) && (g_SnapCnt < 1))//snapshot 4 pictures.
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
			if(g_SnapCnt >= 1)
			{
				get_pic_flag = 0;
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

	//printf("[%s, %d] get_pic_flag:%d, hk_email.mcount:%d....\n", __func__, __LINE__, get_pic_flag, hk_email.mcount);
	pthread_detach(pthread_self()); 
	while(get_pic_flag)
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
		//return 0;
#if 0
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
#endif
		usleep(40*1000);
	}
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
extern short g_sdIsOnline_f;
int CheckAlarm(int iChannel, int iType, int nReserved, char *cFtpData)
{
	static unsigned int raise_time = 0;
	unsigned int cur = Getms();

	printf("........iType:%d, g_startCheckAlarm:%d........\n", iType, g_startCheckAlarm);

	if( cur - raise_time > ALARMTIME )
	{
		raise_time = Getms();
		printf("biaobiao-------$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		//return 0;
		if((1 == iType) && (1 == hkSdParam.sdMoveOpen))
		{
			printf("%s %d",__FILE__,__LINE__);
			get_pic_flag = 0;
			if (!get_pic_flag)
			{ 
				printf("%s %d",__FILE__,__LINE__);
				get_pic_flag = true;
				//如果有sd卡，则进行拍照
				if(g_sdIsOnline_f)
				{
					printf("%s %d",__FILE__,__LINE__);
					CreatePICThread();
				}
			}
		}
	}

#if 0
	if (g_startCheckAlarm < 4)
		return 0;

	if ((1 == iType) && (0 == video_properties_.vv[HKV_MotionSensitivity]))
		return 0;

	AlarmVideoRecord( true );//check SD card for video recording.

	if((1 == iType) && (1 == hkSdParam.sdMoveOpen))
	{
			if (!get_pic_flag)
			{ 
				get_pic_flag = true; 
				CreatePICThread();
			}
	}
#endif
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
		return;
	}
	if ( nAlarmType != 0 )//move
	{
		int iObj = 0; 
		CheckAlarm(iObj, nAlarmType,nReserved, NULL);
		//sccAlarmVideo(true );
	}
}


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
 *       主码流设置参数
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
	video_properties_.vv[HKV_MotionSensitivity]  = conf_get_int(HOME_DIR"/hkipc.conf", "MotionSensitivity");
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
			//s32ret = HISI_SetBitRate(g_s32venchn, video_properties_.vv[HKV_VinFormat]);
			s32ret = HISI_SetBitRate(g_s32venchn, 200);
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
	HK_DEBUG_PRT("...g_s32sunvenchn:%d, s_EncResolution:%d, s_BitRate:%d, s_FrameRate:%d, s_Smooth:%d, s_Saturation:%d, s_Contrast:%d, s_Brightness:%d, s_Sharpness:%d...\n", g_s32sunvenchn, s_EncResolution, s_BitRate, s_FrameRate, s_Smooth, s_Saturation, s_Contrast, s_Brightness, s_Sharpness);

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

	ret = HISI_SetBitRate(g_s32sunvenchn, s_BitRate);
	if (ret)
	{
		printf("[%s, %d] set bitrate failed !\n", __func__, __LINE__);
		return -1;
	}

	if ( (s_FrameRate <= 0) || (s_FrameRate > 15) )
	{
		s_FrameRate = 15;
	}
	ret = HISI_SetFrameRate(g_s32sunvenchn, s_FrameRate);
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

#if 0
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
#endif

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
				//sd_record_start();
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
				//sd_record_stop();
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
	//VENC_CHN_ATTR_S stVencChnAttr;

	/*********************************************
	  step 1:  check & prepare save-file & venc-fd
	 **********************************************/
	if ((s32venchn < 0) || (s32venchn >= VENC_MAX_CHN_NUM))
	{
		SAMPLE_PRT("Venc Channel is out of range !\n");
		return HI_FAILURE;
	}
#if 0
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
#endif
	/* Get Venc Fd. */
	s_vencFd = HI_MPI_VENC_GetFd( s32venchn );
	if ( s_vencFd < 0 )
	{
		SAMPLE_PRT("HI_MPI_VENC_GetFd failed with %#x!\n", s_vencFd);
		return HI_FAILURE;
	}

	return s_vencFd;
}

#if 0
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
#endif

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

			//g_Chan = 0;
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

			Video_DisableVencChn( g_s32sunvenchn );
			usleep(1000);
			Video_EnableVencChn( g_s32sunvenchn );

			//g_Chan = 1;
			g_VencFd_Sub = COMM_Get_VencStream_FD( g_s32sunvenchn ); //get sub stream fd.
			if (HI_FAILURE == g_VencFd_Sub)
			{
				printf("Get Venc Stream FD failed: %s, %d\n", __func__, __LINE__); 
				return -1;
			}

			//g_isMjpegOpen    = 1;
			g_ReadFlag_Mjpeg = 1;
			*threq           = 1;
			g_Venc_Chn_S_Cur = g_s32sunvenchn; //current VENC channel.
			HK_DEBUG_PRT("Open:%s, g_sunCifOrD1:%d, g_Venc_Chn_S_Cur:%d, g_VencFd_Sub:%d.\n", name, g_sunCifOrD1, g_Venc_Chn_S_Cur, g_VencFd_Sub);
		}
		return M_JPEG;
	}
	return 0;
}

extern int g_start_video;
#if ENABLE_QQ
static int s_gopIndex = 0;
unsigned long _GetTickCount()
{        
	struct timeval current = {0};  
	gettimeofday(&current, NULL);  
	return (current.tv_sec*1000 + current.tv_usec/1000);                           
}
#endif

/*获取主码流视频的主线程 */
int g_Video_Thread=0;
int sccGetVideoThread()
{
#if NEW_RECORD
	//av_record_init("/mnt/mmc");
#endif
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

	//time_t time_ms;
	int sFrame = 0;

	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		HK_DEBUG_PRT("malloc failed, %d, %s\n", errno, strerror(errno));
		pstPack = NULL;
		return NULL;
	}
#if 1
	{

		//HISI_SetBitRate(g_SubVideo_Thread, 500);
		HI_S32 s32Ret;
		VENC_CHN_ATTR_S stVencChnAttr;
		VENC_CHN VencChn = 1;

		s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
			return HI_FAILURE;
		}
		SAMPLE_PRT("Main  BR, Set BitRate, chn:%d, bitrate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate);

	}
#endif 
	while( g_Video_Thread )
	{
		FD_ZERO( &read_fds );
		FD_SET( s_vencFd, &read_fds );

		static int s_nFrameIndex = -1;
		static int s_dwTotalFrameIndex = 0;

		//等待获取码流
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
							sFrame = 0;
#if ENABLE_QQ
							s_nFrameIndex++;
#endif
							break;
						case H264E_NALU_BUTT:
							HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
							//stStream.pstPack = NULL;
							continue;
							break;
						default:
							iFrame = 0; //HK_BOAT_IFREAM; //I frame
							sFrame = 1;
#if ENABLE_QQ
							s_gopIndex++;
#endif
							break;
					}
				} //end for()

				/*****OSD: TIME*****/
				//RgnHandle = 3 + s_vencChn;
				//OSD_Overlay_RGN_Display_Time(RgnHandle, s_vencChn); 
				/*****OSD END*****/

#if ENABLE_P2P
				#if 1
				int i = 0;
				if(i == 50)
				{
					i = 0;
					printf("***the videobuf's len is %d\n", iLen);
				}
				i++;
				#endif
				P2PNetServerChannelDataSndToLink(0,0,videobuf,iLen,iFrame,0);
#endif

#if ENABLE_QQ
				tx_set_video_data(videobuf, iLen, iFrame, _GetTickCount(), s_gopIndex, s_nFrameIndex, s_dwTotalFrameIndex++, 40);
#endif

#if NEW_RECORD
				if(g_start_video)
				{
					struct timeval tv;
					gettimeofday(&tv, NULL);
					int64_t time_ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
					//time_ms = time(NULL)*1000;
					pthread_mutex_lock(&record_mutex);
					av_record_write(0, videobuf, iLen, time_ms, sFrame);
					pthread_mutex_unlock(&record_mutex);
				}
				//printf("###########################record##########\n");
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
#if NEW_RECORD
	av_record_quit();
#endif
	HK_DEBUG_PRT("......video thread quit......\n");
	if(pstPack)
	{
		free(pstPack);
	}
	return 1;
}
/*创建主码流视频的主线程*/
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
/*子码流线程 */
int g_SubVideo_Thread=0;
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

#if 1
	{
		//HISI_SetBitRate(g_SubVideo_Thread, 500);
		HI_S32 s32Ret;
		VENC_CHN_ATTR_S stVencChnAttr;
		VENC_CHN VencChn = 0;

		s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
			return HI_FAILURE;
		}
		SAMPLE_PRT("***********sub BR, Set BitRate, chn:%d, bitrate:%d...\n", VencChn, stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate);

	}
#endif 

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
					#if 0
					sccPushStream( 1234, PSTREAMTWO, videobuf, iLen, iFrame, g_iCifOrD1, H264 );
					#endif

					if (1 == video_properties_.vv[HKV_Cbr]) //hkipc.conf => 0:CBR, 1:VBR.
					{
						int nRate = GetMainVideoRate( iLen );
						if ( (nRate > 40) && (g_VbrMaxQq_Sub == VBR_MAXQP_33) )
						{
							Set_VBR_Image_QP( g_s32sunvenchn, VBR_MAXQP_38 );
							g_VbrMaxQq_Sub = VBR_MAXQP_38;
							printf( "fxb ------------- sub change, nRate: %d, g_VbrMaxQq: %d \n", nRate, g_VbrMaxQq );
						}
						else if ( (nRate < 10) && (nRate > 0) && (g_VbrMaxQq_Sub == VBR_MAXQP_38) )
						{
							Set_VBR_Image_QP( g_s32sunvenchn, VBR_MAXQP_33 );
							g_VbrMaxQq_Sub = VBR_MAXQP_33;
							printf( "fxb --------------- sub low, nRate: %d, g_VbrMaxQq:%d \n", nRate, g_VbrMaxQq );
						}
					}
				}
				//是否进行录像
				if (0 == hkSdParam.sdrecqc)
				{
					//sccPushTfData( PSTREAMTWO, videobuf, iLen, iFrame, g_iCifOrD1, H264 );
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

/* 创建子码流线程 */
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
/* 创建媒体线程 */
int sccStartVideoThread()
{
	CreateVideoThread();
	CreateSubVideoThread();
	CreateAudioThread();
}
#if 0
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
#endif
void video_RSLoadObjects() 
{
    /**video resolution**/
    g_iCifOrD1   = conf_get_int(HOME_DIR"/hkipc.conf", "CifOrD1");//main stream. 9
    g_sunCifOrD1 = conf_get_int(HOME_DIR"/subipc.conf", "enc");   //sub stream.  5

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
        g_s32sunvenchn = 1;   //Venc Channel 1: VGA (640*480)
        g_sunCifOrD1 = ENUM_VGA ; //5
        //g_sunCifOrD1 = ENUM_CIF ; //5
    }

    /**config main stream params**/
    if ( MainStreamConfigurate() )
        printf("[%s, %d] configurate main stream failed !\n", __func__, __LINE__); 
    else
        printf("[%s, %d] configurate main stream success !\n", __func__, __LINE__); 
    
    /**motion detect**/
    g_MotionDetectSensitivity = video_properties_.vv[HKV_MotionSensitivity];

}

