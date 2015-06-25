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

#include "sample_comm.h"
#include "ipc_hk.h"
#include "ipc_vbVideo.h"

/*defined in ipc_vbVideo.h*/
extern int g_MotionDetectSensitivity;
extern int g_iCifOrD1;   //main stream channel index.
extern int g_isH264Open;  //main stream open flag.
extern struct HKVProperty video_properties_;

#ifndef bool
#define bool int
#define false 0
#define true 1
#endif

/**********************************
 * motion detect level:
 * 0: close;
 * 1: low sensitivity; 
 * 2: middle sensitivity; 
 * 3: high sensitivity;
 *********************************/
#define MD_LEVEL_LOW     8000
#define MD_LEVEL_MIDDLE  2000
#define MD_LEVEL_HIGH    800

typedef struct hiVDA_MD_PARAM_S
{
	HI_BOOL bThreadStart;
	VDA_CHN VdaChn;
}VDA_MD_PARAM_S;

typedef struct hiVDA_OD_PARAM_S
{
	HI_BOOL bThreadStart;
	VDA_CHN VdaChn;
}VDA_OD_PARAM_S;

#define VDA_MD_CHN 0
#define VDA_OD_CHN 1

static pthread_t gs_VdaPid[2];
static VDA_MD_PARAM_S gs_stMdParam;
static VDA_OD_PARAM_S gs_stOdParam;

extern int HK_Check_PIR_State();

/******************************************************************************
* funciton : vda MD mode print -- SAD
******************************************************************************/
static HI_S32 VDA_MdPrtSad(FILE *fp, VDA_DATA_S *pstVdaData)
{
	HI_S32 i, j;
	HI_VOID *pAddr;
	
	fprintf(fp, "===== %s =====\n", __FUNCTION__);
	if (HI_TRUE != pstVdaData->unData.stMdData.bMbSadValid)
	{
		fprintf(fp, "bMbSadValid = FALSE.\n");
		return HI_SUCCESS;
	}

	for(i=0; i<pstVdaData->u32MbHeight; i++)
	{
		pAddr = (HI_VOID *)((HI_U32)pstVdaData->unData.stMdData.stMbSadData.pAddr
					+ i * pstVdaData->unData.stMdData.stMbSadData.u32Stride);
	
		for(j=0; j<pstVdaData->u32MbWidth; j++)
		{
			HI_U8  *pu8Addr;
			HI_U16 *pu16Addr;
		
			if(VDA_MB_SAD_8BIT == pstVdaData->unData.stMdData.stMbSadData.enMbSadBits)
			{
				pu8Addr = (HI_U8 *)pAddr + j;

				fprintf(fp, "%-2x",*pu8Addr);

			}
			else
			{
				pu16Addr = (HI_U16 *)pAddr + j;

				fprintf(fp, "%-4x",*pu16Addr);
			}
		}
		
		printf("\n");
	}
	
	fflush(fp);
	return HI_SUCCESS;
}


/******************************************************************************
* funciton : vda MD mode print -- Md OBJ
******************************************************************************/
static HI_S32 VDA_MdPrtObj(FILE *fp, VDA_DATA_S *pstVdaData)
{
	VDA_OBJ_S *pstVdaObj;
	HI_S32 i;
	
	fprintf(fp, "===== %s =====\n", __FUNCTION__);
	
	if (HI_TRUE != pstVdaData->unData.stMdData.bObjValid)
	{
		fprintf(fp, "bMbObjValid = FALSE.\n");
		return HI_SUCCESS;
	}
	
	fprintf(fp, "ObjNum=%d, IndexOfMaxObj=%d, SizeOfMaxObj=%d, SizeOfTotalObj=%d\n", \
			 pstVdaData->unData.stMdData.stObjData.u32ObjNum, \
			 pstVdaData->unData.stMdData.stObjData.u32IndexOfMaxObj, \
			 pstVdaData->unData.stMdData.stObjData.u32SizeOfMaxObj,\
			 pstVdaData->unData.stMdData.stObjData.u32SizeOfTotalObj);
	for (i=0; i<pstVdaData->unData.stMdData.stObjData.u32ObjNum; i++)
	{
		pstVdaObj = pstVdaData->unData.stMdData.stObjData.pstAddr + i;
		fprintf(fp, "[%d]\t left=%d, top=%d, right=%d, bottom=%d\n", i, \
			  pstVdaObj->u16Left, pstVdaObj->u16Top, \
			  pstVdaObj->u16Right, pstVdaObj->u16Bottom);
	}
	fflush(fp);
	return HI_SUCCESS;
}


/******************************************************************************
* funciton : vda MD mode print -- Alarm Pixel Count
******************************************************************************/
static HI_S32 VDA_MdPrtAp(FILE *fp, VDA_DATA_S *pstVdaData)
{
    //printf("[%s, %d] g_MotionDetectLevel:%d, bPelsNumValid:%d, AlarmPixCnt:%d\n", __func__, __LINE__, g_MotionDetectLevel, pstVdaData->unData.stMdData.bPelsNumValid, pstVdaData->unData.stMdData.u32AlarmPixCnt);

	if (HI_TRUE != pstVdaData->unData.stMdData.bPelsNumValid)
	{
		fprintf(fp, "bMbObjValid = FALSE.\n");
		return HI_SUCCESS;
	}

	//fprintf(fp, "AlarmPixelCount=%d______\n", pstVdaData->unData.stMdData.u32AlarmPixCnt);
	//fflush(fp);

    //printf(".....motion detect sensitivity level: %d ......\n", g_MotionDetectSensitivity);
    if (g_MotionDetectSensitivity > 0)
    {
        int s_MDAlarm_Pix_Cnt = 0;
        switch (g_MotionDetectSensitivity)
        {
            case 3:
                s_MDAlarm_Pix_Cnt = MD_LEVEL_HIGH;
                break;
            case 2:
                s_MDAlarm_Pix_Cnt = MD_LEVEL_MIDDLE;
                break;
            case 1:
                s_MDAlarm_Pix_Cnt = MD_LEVEL_LOW;
                break;
            default:
                s_MDAlarm_Pix_Cnt = MD_LEVEL_LOW;
                break;
        }

        //printf("-------------> AlarmPixelCount:%d <----------------\n", pstVdaData->unData.stMdData.u32AlarmPixCnt);
        if (pstVdaData->unData.stMdData.u32AlarmPixCnt > s_MDAlarm_Pix_Cnt)
        {
            printf("...MD: %d...\n", pstVdaData->unData.stMdData.u32AlarmPixCnt);
        #if DEV_DOORBELL
            if ( 1 == HK_Check_PIR_State() )
            {
                printf("...PIR Alarming...\n");
        #else
            {
        #endif

                CheckAlarm(1, 1, 0, NULL); //alarm notification.
                AlarmVideoRecord(true); //alarm video record.
            }
        }
        else
        {
            AlarmVideoRecord(false); //alarm video record.
        }
    }

#if 0 //add by ali.
    static int nVideoBR = VENC_RC_MODE_H264CBR;
    static int nNAlarmPix = 0;
    VENC_CHN_ATTR_S stVencChnAttr;
    //printf("[%s] nVideoBR:%d, video_properties_.vv[HKV_Cbr]:%d, g_iCifOrD1:%d, g_isH264Open:%d, nNAlarmPix:%d, PixCnt:%d....\n", __func__, nVideoBR, video_properties_.vv[HKV_Cbr], g_iCifOrD1, g_isH264Open, nNAlarmPix, pstVdaData->unData.stMdData.u32AlarmPixCnt);

    if (0 == video_properties_.vv[HKV_Cbr]) //return while current is CBR.
        return;

    //if (1 == g_isH264Open) //main stream open.
    if (2 == g_isH264Open) //main stream open.
    {
        //if( pstVdaData->unData.stMdData.u32AlarmPixCnt > MD_LEVEL_MIDDLE )
        if (pstVdaData->unData.stMdData.u32AlarmPixCnt > 4000)
        {
            if (nVideoBR != VENC_RC_MODE_H264CBR)  //Cbr
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                nVideoBR = VENC_RC_MODE_H264CBR;

                if (g_iCifOrD1 >= 9)
                {
                    HISI_SetRCType(0, stVencChnAttr.stRcAttr.enRcMode);
                    //HISI_SetFrameRate(0, video_properties_.vv[HKV_BitRate]);
                    HISI_SetBitRate(0, video_properties_.vv[HKV_VinFormat]);
                }
                else if (5 == g_iCifOrD1)
                {
                    HISI_SetRCType(1, stVencChnAttr.stRcAttr.enRcMode);
                    //HISI_SetFrameRate(1, video_properties_.vv[HKV_BitRate]);
                    HISI_SetBitRate(1, video_properties_.vv[HKV_VinFormat]);
                }
            }
            nNAlarmPix = 0;
        }
        else if (nVideoBR != VENC_RC_MODE_H264VBR)//VBR
        {
            nNAlarmPix ++;
            if( nNAlarmPix > 5*10 )// 5s
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                nVideoBR = VENC_RC_MODE_H264VBR;

                if (g_iCifOrD1 >= 9)
                {
                    HISI_SetRCType(0, stVencChnAttr.stRcAttr.enRcMode);
                    //HISI_SetFrameRate(0, 15);
                    HISI_SetBitRate(0, 64);
                }
                else if (5 == g_iCifOrD1)
                {
                    HISI_SetRCType(1, stVencChnAttr.stRcAttr.enRcMode);
                    //HISI_SetFrameRate(1, 15);
                    HISI_SetBitRate(1, 64);
                }
            }
        }
    }
    else
    {
        nNAlarmPix = 0;
    }
#endif

	return HI_SUCCESS;
}

/******************************************************************************
* funciton : vda MD mode thread process
******************************************************************************/
static HI_VOID *VDA_MdGetResult(HI_VOID *pdata)
{
	HI_S32 s32Ret;
	VDA_CHN VdaChn;
	VDA_DATA_S stVdaData;
	VDA_MD_PARAM_S *pgs_stMdParam;
	HI_S32 maxfd = 0;
	FILE *fp = stdout;
	HI_S32 VdaFd;
	fd_set read_fds;
	struct timeval TimeoutVal;

	pgs_stMdParam = (VDA_MD_PARAM_S *)pdata;

	VdaChn	 = pgs_stMdParam->VdaChn;

	/* decide the stream file name, and open file to save stream */
	/* Set Venc Fd. */
	VdaFd = HI_MPI_VDA_GetFd(VdaChn);
	if (VdaFd < 0)
	{
		SAMPLE_PRT("HI_MPI_VDA_GetFd failed with %#x!\n", VdaFd);
		return NULL;
	}
	if (maxfd <= VdaFd)
	{
		maxfd = VdaFd;
	}
	while (HI_TRUE == pgs_stMdParam->bThreadStart)
	{
		FD_ZERO(&read_fds);
		FD_SET(VdaFd, &read_fds);

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
		if (s32Ret < 0)
		{
			SAMPLE_PRT("select failed!\n");
			break;
		}
		else if (s32Ret == 0)
		{
			SAMPLE_PRT("get venc stream time out, exit thread\n");
			break;
		}
		else
		{
			if (FD_ISSET(VdaFd, &read_fds))
			{
                /*******************************************************
                  step 2.3 : call mpi to get one-frame stream
                 *******************************************************/
                s32Ret = HI_MPI_VDA_GetData(VdaChn, &stVdaData, HI_TRUE);
				if(s32Ret != HI_SUCCESS)
				{
					SAMPLE_PRT("HI_MPI_VDA_GetData failed with %#x!\n", s32Ret);
					return NULL;
				}

                /*******************************************************
                 * step 2.4 : save frame to file
                 *******************************************************/
                #if 1
				//printf("\033[0;0H");/*move cursor*/
				//VDA_MdPrtSad(fp, &stVdaData);
				//VDA_MdPrtObj(fp, &stVdaData);
				//if(1 == pSyscontext->ipcam_SysConfig->ConfigAlarm.MotiDetection[3].mdIsOpen)
				if (1)  //here must be get vda enable
				{
					VDA_MdPrtAp(fp, &stVdaData);
				}
                #endif

                /*******************************************************
                 * step 2.5 : release stream
                 *******************************************************/
                s32Ret = HI_MPI_VDA_ReleaseData(VdaChn,&stVdaData);
				if(s32Ret != HI_SUCCESS)
				{
					SAMPLE_PRT("HI_MPI_VDA_ReleaseData failed with %#x!\n", s32Ret);
					return NULL;
				}
			}
		}
		usleep(100*1000);
	}

	return HI_NULL;
}


/******************************************************************************
* funciton : start vda MD mode
******************************************************************************/
static HI_S32 VDA_MdStart(VDA_CHN VdaChn, VI_CHN ViChn, SIZE_S *pstSize)
{
	HI_S32 s32Ret = HI_SUCCESS;
	VDA_CHN_ATTR_S stVdaChnAttr;
	MPP_CHN_S stSrcChn, stDestChn;
	
	if (VDA_MAX_WIDTH < pstSize->u32Width || VDA_MAX_HEIGHT < pstSize->u32Height)
	{
		SAMPLE_PRT("Picture size invaild!\n");
		return HI_FAILURE;
	}
	
	/* step 1 create vda channel */
	stVdaChnAttr.enWorkMode = VDA_WORK_MODE_MD;
	stVdaChnAttr.u32Width	= pstSize->u32Width;  //要求16对齐
	stVdaChnAttr.u32Height	= pstSize->u32Height; //要求16对齐

	stVdaChnAttr.unAttr.stMdAttr.enVdaAlg	   = VDA_ALG_REF;
	stVdaChnAttr.unAttr.stMdAttr.enMbSize	   = VDA_MB_16PIXEL;
	stVdaChnAttr.unAttr.stMdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
	stVdaChnAttr.unAttr.stMdAttr.enRefMode	   = VDA_REF_MODE_DYNAMIC;
	stVdaChnAttr.unAttr.stMdAttr.u32MdBufNum   = 8;
	stVdaChnAttr.unAttr.stMdAttr.u32VdaIntvl   = 4;  
	stVdaChnAttr.unAttr.stMdAttr.u32BgUpSrcWgt = 128;
	stVdaChnAttr.unAttr.stMdAttr.u32SadTh	   = 240;
	stVdaChnAttr.unAttr.stMdAttr.u32ObjNumMax  = 128;

	s32Ret = HI_MPI_VDA_CreateChn(VdaChn, &stVdaChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err!\n");
		return s32Ret;
	}

	/* step 2: vda chn bind vi chn */
	stSrcChn.enModId = HI_ID_VIU;
	stSrcChn.s32ChnId = ViChn;
	stSrcChn.s32DevId = 0;

	stDestChn.enModId = HI_ID_VDA;
	stDestChn.s32ChnId = VdaChn;
	stDestChn.s32DevId = 0;

	s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err!\n");
		return s32Ret;
	}

	/* step 3: vda chn start recv picture */
	s32Ret = HI_MPI_VDA_StartRecvPic(VdaChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err!\n");
		return s32Ret;
	}

	/* step 4: create thread to get result */
	gs_stMdParam.bThreadStart = HI_TRUE;
	gs_stMdParam.VdaChn   = VdaChn;

	pthread_create(&gs_VdaPid[VDA_MD_CHN], 0, VDA_MdGetResult, (HI_VOID *)&gs_stMdParam);

	return HI_SUCCESS;
}

/******************************************************************************
* funciton : stop vda, and stop vda thread -- MD
******************************************************************************/
static HI_VOID VDA_MdStop(VDA_CHN VdaChn, VI_CHN ViChn)
{
	HI_S32 s32Ret = HI_SUCCESS;
	
	MPP_CHN_S stSrcChn, stDestChn;

	/* join thread */
	if (HI_TRUE == gs_stMdParam.bThreadStart)
	{
		gs_stMdParam.bThreadStart = HI_FALSE;
		pthread_join(gs_VdaPid[VDA_MD_CHN], 0);
	}
	
	/* vda stop recv picture */
	s32Ret = HI_MPI_VDA_StopRecvPic(VdaChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err(0x%x)!!!!\n",s32Ret);
	}

	/* unbind vda chn & vi chn */

	stSrcChn.enModId = HI_ID_VIU;
	stSrcChn.s32ChnId = ViChn;
	stSrcChn.s32DevId = 0;
	stDestChn.enModId = HI_ID_VDA;
	stDestChn.s32ChnId = VdaChn;
	stDestChn.s32DevId = 0;
	
	s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err(0x%x)!!!!\n", s32Ret);
	}
	
	/* destroy vda chn */
	s32Ret = HI_MPI_VDA_DestroyChn(VdaChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err(0x%x)!!!!\n", s32Ret);
	}
	
	return;
}


/******************************************************************************
* funciton : vda OD mode thread process
******************************************************************************/
static HI_VOID *VDA_OdGetResult(HI_VOID *pdata)
{
	HI_S32 i;
	HI_S32 s32Ret;
	VDA_CHN VdaChn;
	VDA_DATA_S stVdaData;
	HI_U32 u32RgnNum;
	VDA_OD_PARAM_S *pgs_stOdParam;
	//FILE *fp=stdout;

	pgs_stOdParam = (VDA_OD_PARAM_S *)pdata;

	VdaChn	  = pgs_stOdParam->VdaChn;
	
	while(HI_TRUE == pgs_stOdParam->bThreadStart)
	{
		s32Ret = HI_MPI_VDA_GetData(VdaChn,&stVdaData,HI_TRUE);
		if(s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VDA_GetData failed with %#x!\n", s32Ret);
			return NULL;
		}

		//SAMPLE_COMM_VDA_OdPrt(fp, &stVdaData);

		u32RgnNum = stVdaData.unData.stOdData.u32RgnNum;
		
		for(i=0; i<u32RgnNum; i++)
		{
			if(HI_TRUE == stVdaData.unData.stOdData.abRgnAlarm[i])
			{ 
				printf("################VdaChn--%d,Rgn--%d,Occ!\n",VdaChn,i);
				s32Ret = HI_MPI_VDA_ResetOdRegion(VdaChn,i);
				if(s32Ret != HI_SUCCESS)
				{
					SAMPLE_PRT("HI_MPI_VDA_ResetOdRegion failed with %#x!\n", s32Ret);
					return NULL;
				}
			}
		}

		s32Ret = HI_MPI_VDA_ReleaseData(VdaChn,&stVdaData);
		if(s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VDA_ReleaseData failed with %#x!\n", s32Ret);
			return NULL;
		}

		usleep(200*1000);
	}

	return HI_NULL;
}


/******************************************************************************
* funciton : start vda OD mode
******************************************************************************/
static HI_S32 VDA_OdStart(VDA_CHN VdaChn, VI_CHN ViChn, SIZE_S *pstSize)
{
	VDA_CHN_ATTR_S stVdaChnAttr;
	MPP_CHN_S stSrcChn, stDestChn;
	HI_S32 s32Ret = HI_SUCCESS;
	
	if (VDA_MAX_WIDTH < pstSize->u32Width || VDA_MAX_HEIGHT < pstSize->u32Height)
	{
		SAMPLE_PRT("Picture size invaild!\n");
		return HI_FAILURE;
	}
	
	/********************************************
	 step 1 : create vda channel
	********************************************/
	stVdaChnAttr.enWorkMode = VDA_WORK_MODE_OD;
	stVdaChnAttr.u32Width	= pstSize->u32Width;
	stVdaChnAttr.u32Height	= pstSize->u32Height;

	stVdaChnAttr.unAttr.stOdAttr.enVdaAlg	   = VDA_ALG_REF;
	stVdaChnAttr.unAttr.stOdAttr.enMbSize	   = VDA_MB_8PIXEL;
	stVdaChnAttr.unAttr.stOdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
	stVdaChnAttr.unAttr.stOdAttr.enRefMode	   = VDA_REF_MODE_DYNAMIC;
	stVdaChnAttr.unAttr.stOdAttr.u32VdaIntvl   = 4;
	stVdaChnAttr.unAttr.stOdAttr.u32BgUpSrcWgt = 128;
	
	stVdaChnAttr.unAttr.stOdAttr.u32RgnNum = 1;
	
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.s32X = 0;
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.s32Y = 0;
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.u32Width  = pstSize->u32Width;
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.u32Height = pstSize->u32Height;

	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].u32SadTh	   = 240;
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh	   = 80;
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].u32OccCntTh   = 6;
	stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[0].u32UnOccCntTh = 1;
	
	s32Ret = HI_MPI_VDA_CreateChn(VdaChn, &stVdaChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err!\n");
		return(s32Ret);
	}

	/********************************************
	 step 2 : bind vda channel to vi channel
	********************************************/
	stSrcChn.enModId = HI_ID_VIU;
	stSrcChn.s32ChnId = ViChn;

	stDestChn.enModId = HI_ID_VDA;
	stDestChn.s32ChnId = VdaChn;
	stDestChn.s32DevId = 0;

	s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err!\n");
		return s32Ret;
	}

	/* vda start rcv picture */
	s32Ret = HI_MPI_VDA_StartRecvPic(VdaChn);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("err!\n");
		return(s32Ret);
	}

	/*........*/
	gs_stOdParam.bThreadStart = HI_TRUE;
	gs_stOdParam.VdaChn   = VdaChn;

	pthread_create(&gs_VdaPid[VDA_OD_CHN], 0, VDA_OdGetResult, (HI_VOID *)&gs_stOdParam);

	return HI_SUCCESS;
}


int VDA_MotionDetect_Start(int md_level)
{
	HI_S32 s32Ret;
	VI_CHN ViExtChn = 1;
	VI_EXT_CHN_ATTR_S stViExtChnAttr;
	
	stViExtChnAttr.s32BindChn			= 0;
	stViExtChnAttr.stDestSize.u32Width	= 320;
	stViExtChnAttr.stDestSize.u32Height = 240;
	stViExtChnAttr.s32SrcFrameRate		= 25; //30;
	stViExtChnAttr.s32FrameRate 		= 25; //30;
	stViExtChnAttr.enPixFormat			= SAMPLE_PIXEL_FORMAT;

	s32Ret = HI_MPI_VI_SetExtChnAttr(ViExtChn, &stViExtChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("set vi	SetExtChnAttr failed!\n");
		return HI_FAILURE;
	}		 

	s32Ret = HI_MPI_VI_EnableChn(ViExtChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("set vi	EnableChn failed!\n");
		return HI_FAILURE;
	} 

	VIDEO_NORM_E enNorm = VIDEO_ENCODING_MODE_NTSC;
	PIC_SIZE_E enSize_Md = PIC_QVGA;
	SIZE_S stSize;
	VI_CHN ViChn_Md;
	VDA_CHN VdaChn_Md = 0;

	stSize.u32Width = 320;
	stSize.u32Height = 240;
	ViChn_Md = ViExtChn;
	s32Ret = VDA_MdStart(VdaChn_Md, ViChn_Md, &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("VDA Md Start failed!\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


static int VDA_MoveStop()
{
	VDA_CHN VdaChn = 0;
	VI_CHN ViChn = 1;
	VDA_MdStop(VdaChn,ViChn);
		
	return 0;
}

static int VDA_OverlayStart()
{
	HI_S32 s32Ret;
	VI_CHN ViExtChn = 2;
	VI_EXT_CHN_ATTR_S stViExtChnAttr;
	
	stViExtChnAttr.s32BindChn			= 0;
	stViExtChnAttr.stDestSize.u32Width	= 320;
	stViExtChnAttr.stDestSize.u32Height = 240;
	stViExtChnAttr.s32SrcFrameRate		= 25; //30;
	stViExtChnAttr.s32FrameRate 		= 25; //30;
	stViExtChnAttr.enPixFormat			= SAMPLE_PIXEL_FORMAT;

	s32Ret = HI_MPI_VI_SetExtChnAttr(ViExtChn, &stViExtChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("set vi	SetExtChnAttr failed!\n");
		return HI_FAILURE;
	}		 

	s32Ret = HI_MPI_VI_EnableChn(ViExtChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("set vi	EnableChn failed!\n");
		return HI_FAILURE;
	} 

	VIDEO_NORM_E enNorm = VIDEO_ENCODING_MODE_NTSC;
	PIC_SIZE_E enSize_Md = PIC_QVGA;
	SIZE_S stSize;
	VI_CHN ViChn_Md;
	VDA_CHN VdaChn_Md = 0;

	stSize.u32Width = 320;
	stSize.u32Height = 240;

	ViChn_Md = ViExtChn;
	s32Ret = VDA_OdStart(VdaChn_Md, ViChn_Md, &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("VDA Md Start failed!\n");
		return HI_FAILURE;
	}

	SAMPLE_PRT("222222222222222222222222VDA_OdStart___________\n");
    
	return 0;
}

