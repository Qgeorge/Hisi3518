#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>

#include "rs.h"
#include "sys.h"
#include "utils/HKCmdPacket.h"

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_sys.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_aenc.h"
#include "mpi_adec.h"

#include "gpiodriver.h"
#include "ipc_hk.h"
#include "g711codec.h"

#if ENABLE_P2P
#include "P2Pserver.h"
#endif

#if ENABLE_QQ
#include "TXAudioVideo.h"
#endif

#if ENABLE_ONVIF
#include "IPCAM_Export.h"
#endif
extern int PCM2AMR(char *pStream, int len, char *outbuf);
//extern LPIPCAM_AUDIOBUFFER g_AudioBuf;
//zqjun.
/************************ AUDIO params **************************/

//macro for debug:
#define AUDIO_WRITE 0 //if open file for write audio, 0:disable, 1:enable.
#define AIAO        0 //1: read form file.     0:write recived data into file.

//audio debug print.
#define SAMPLE_DBG(s32ret)      \
	do{                         \
		printf("s32ret=%#x,fuc:%s,line:%d\n", s32ret, __FUNCTION__, __LINE__);\
	}while(0)

#define SET_MICBIAS_LEVEL 0x35

//#define SIO0_WORK_MODE    AIO_MODE_I2S_MASTER
#define SIO0_WORK_MODE      AIO_MODE_I2S_SLAVE
#define SIO1_WORK_MODE  	AIO_MODE_I2S_SLAVE
#define AUDIO_PAYLOAD_TYPE  PT_G711A /*encoder type, PT_ADPCMA,PT_G711A,PT_AAC...*/
//#define AUDIO_PAYLOAD_TYPE  PT_ADPCMA /*encoder type, PT_ADPCMA,PT_G711A,PT_AAC...*/

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_IMA /* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define AUDIO_AAC_TYPE AAC_TYPE_AACLC   /* AAC_TYPE_AACLC, AAC_TYPE_EAAC, AAC_TYPE_EAACPLUS*/
#define G726_BPS MEDIA_G726_16K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */
#define AMR_FORMAT AMR_FORMAT_MMS       /* AMR_FORMAT_MMS, AMR_FORMAT_IF1, AMR_FORMAT_IF2*/
#define AMR_MODE AMR_MODE_MR122         /* AMR_MODE_MR122, AMR_MODE_MR102 ... */
#define AMR_DTX 1

#define AUDIO_POINT_NUM 320             /* point num of one frame 80,160,320,480*/
//#define AUDIO_POINT_NUM 160             /* point num of one frame 80,160,320,480*/

static PAYLOAD_TYPE_E gs_enPayloadType = PT_LPCM; //PT_G711A; PT_G711U; PT_ADPCMA;
static PAYLOAD_TYPE_E gs_decPayloadType = PT_G711A; //PT_G711U; PT_ADPCMA;
//static HI_BOOL gs_bMicIn = HI_TRUE; //MICIN (error).
static HI_BOOL gs_bMicIn = HI_FALSE;  //LINEIN.
static HI_BOOL gs_bAiAnr = HI_FALSE;
//static HI_BOOL gs_bAiAnr = HI_TRUE; //HI_FALSE;
static HI_BOOL gs_bAioReSample = HI_FALSE;
static HI_BOOL gs_bUserGetMode = HI_FALSE; //HI_TRUE;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAiReSmpAttr = NULL;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAoReSmpAttr = NULL;
extern HK_SD_PARAM_ hkSdParam;

HI_U32 g_AiChnCnt = 2;   //audio input channel number.
AUDIO_DEV g_AiDevId = 0; //audio input device (only 0).
AUDIO_DEV g_AiChn = 0;   //audio input channel.
AENC_CHN g_AencChn = 0;  //audio encode channel.
ADEC_CHN g_AdecChn = 0;  //audio decode channel.
AUDIO_DEV g_AoDevId = 0; //audio output device.
AO_CHN g_AoChn = 0;      //audio output channel.

HI_S32 g_AiFd = 0;       //audio input file descriptor.
HI_S32 g_AencFd = 0;     //audio encode file descriptor.

FILE *pfd_AEnc = NULL;   //audio encode file descriptor.
FILE *pfd_ADec = NULL;   //audio decode file descriptor.

static char g_testAudioBuf[320]={0};
/*
   typedef struct
   {
   HI_BOOL bStart;
   HI_BOOL bUseAEC;
   HI_S32 s32AiDev;
   HI_S32 s32AiChn;
   HI_S32 s32AencChn; 
   FILE *pfd;
   pthread_t stAencPid;
   } SAMPLE_AENC_S;
 */

typedef struct tagSAMPLE_AENC_S
{
	HI_BOOL bStart;
	pthread_t stAencPid;
	HI_S32  AeChn;
	HI_S32  AdChn;
	FILE    *pfd;
	HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;

SAMPLE_AENC_S *pstSampleAenc = NULL;
static SAMPLE_AENC_S s_stSampleAenc[8];
static SAMPLE_AENC_S gs_stSampleAenc[AENC_MAX_CHN_NUM];
SAMPLE_AENC_S *pstAenc = &gs_stSampleAenc[0];

/************************* end **********************************/


#define CFG_FILE "/mnt/sif/cfg.ini"

#define CHANNEL_NUM 0
#define CALLER_ID 0

typedef struct Date_Head
{
	char 	codec_type;
	char 	operation_type;
	short 	talker_id;
	unsigned int  	timestamp;
}Date_Head;

Date_Head bufhead;

//#define G729_len 20
//#define buf_count 20
//static char audio_buf[buf_count][4*G729_len+1];
//static int count_in=0;
//static int count_out=0;

#define PUTS(...) HKLOG(L_DBG, __VA_ARGS__)

typedef struct IM_Input_Object 
{
	RSObject super;
	RSObjectIRQEvent irq;

} IM_Input_Object;
extern IM_Input_Object vbAudio;
IM_Input_Object vbAudio;

int CreateAudioThread(void);


/******************************************************************************
 * function : PT Number to String
 ******************************************************************************/
static char* SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
	if (PT_G711A == enType)  return "g711a";
	else if (PT_G711U == enType)  return "g711u";
	else if (PT_ADPCMA == enType)  return "adpcm";
	else if (PT_G726 == enType)  return "g726";
	else if (PT_LPCM == enType)  return "pcm";
	else return "data";
}

/******************************************************************************
 * function : Open Aenc File
 ******************************************************************************/
static FILE * SAMPLE_AUDIO_OpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
	FILE *pfd = NULL;
	HI_CHAR aszFileName[128];

	/* create file for save stream*/
	sprintf(aszFileName, "audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
	pfd = fopen(aszFileName, "w+");
	if (NULL == pfd)
	{
		printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
		return NULL;
	}
	printf("open stream file:\"%s\" for aenc ok\n", aszFileName);

	return pfd;
}

/******************************************************************************
 * function : Open Adec File
 ******************************************************************************/
static FILE *SAMPLE_AUDIO_OpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
	FILE *pfd;
	HI_CHAR aszFileName[128];

	/* create file for save stream*/
	sprintf(aszFileName, "audioIn_chn%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
#if AIAO
	pfd = fopen(aszFileName, "rb");
#else
	pfd = fopen(aszFileName, "w+");
#endif
	if (NULL == pfd)
	{
		printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
		return NULL;
	}
	printf("open stream file:\"%s\" for adec ok\n", aszFileName);
	return pfd;
}


HI_S32 SAMPLE_Audio_AiAenc()
{
	HI_S32 s32ret = 0, i = 0;
	static AUDIO_DEV s_AiDev   = 0;
	static AI_CHN    s_AiChn   = 0;
	static AENC_CHN  s_AencChn = 0;
	AIO_ATTR_S stAioAttr;
	ADEC_CHN_ATTR_S stAdecAttr;    

	/*** audio input/output attrbutes ***/
	/* init stAio. all of cases will use it */
	stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
	stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16; //hi3518c only support 16.
	stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER; //must be MASTER for built-in codec. 
	stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO; // 0:mono(built-in)  1:stereo.
	stAioAttr.u32EXFlag      = 1;  /*should set 1, expand 8bit to 16bit*/
	stAioAttr.u32FrmNum      = 32;
	stAioAttr.u32PtNumPerFrm = AUDIO_POINT_NUM;
	stAioAttr.u32ChnCnt      = 2; //2;4;8;16; built-in only support 2.
	stAioAttr.u32ClkSel      = 1;

	/********************************************
	  step 1: config audio codec
	 ********************************************/
	s32ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr, gs_bMicIn);
	if (HI_SUCCESS != s32ret)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}

	/********************************************
	  step 2: start Ai
	 ********************************************/
	HI_S32 s32AiChnCnt = stAioAttr.u32ChnCnt;
	//s32ret = SAMPLE_COMM_AUDIO_StartAi(g_AiDevId, s32AiChnCnt, &stAioAttr, gs_bAiAnr, gs_pstAiReSmpAttr);
	s32ret = SAMPLE_COMM_AUDIO_StartAi(s_AiDev, s32AiChnCnt, &stAioAttr, gs_bAiAnr, gs_pstAiReSmpAttr);
	if (s32ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}

	/********************************************
	  step 3: start Aenc
	 ********************************************/
	HI_S32 s32AencChnCnt = 1;
	s32ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, gs_enPayloadType);
	if (s32ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}

	/********************************************
	  step 4: Aenc bind Ai Chn
	 ********************************************/
	for (i = 0; i < s32AencChnCnt; i++)
	{
		//s32ret = SAMPLE_COMM_AUDIO_AencBindAi(g_AiDevId, g_AiChn, g_AencChn);
		s32ret = SAMPLE_COMM_AUDIO_AencBindAi(s_AiDev, s_AiChn, s_AencChn);
		if (s32ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32ret);
			return s32ret;
		}
		printf("Ai(%d,%d) bind to AencChn:%d ok!\n", s_AiDev , s_AiChn, s_AencChn);
	}

	//g_AencFd = HI_MPI_AENC_GetFd(g_AencChn);
	g_AencFd = HI_MPI_AENC_GetFd(s_AencChn);
	if (g_AencFd <= 0)
	{
		printf("[%s--%d]  HI_MPI_AI_GetFd(%d, %d) failed\n", __FUNCTION__, __LINE__, s_AiDev, s_AiChn);
		return HI_FAILURE;
	}
	printf("------------------> g_AencFd = %d <-----------------\n", g_AencFd);

#if AUDIO_WRITE
	//pfd_AEnc = SAMPLE_AUDIO_OpenAencFile(g_AencChn, gs_enPayloadType);
	pfd_AEnc = SAMPLE_AUDIO_OpenAencFile(s_AencChn, gs_enPayloadType);
	if (!pfd_AEnc)
	{
		SAMPLE_DBG(HI_FAILURE);
		return HI_FAILURE;
	}
#endif

	return HI_SUCCESS;          
}


/************************************
 * func: init audio decoder & ao.
 ***********************************/
HI_S32 SAMPLE_Audio_AdecAo()
{
	HI_S32 s32ret = 0, i = 0;
	static AUDIO_DEV s_AoDev   = 0;
	static AO_CHN    s_AoChn   = 0;
	static ADEC_CHN  s_AdecChn = 0;
	AIO_ATTR_S stAioAttr;
	ADEC_CHN_ATTR_S stAdecAttr;    

	/*** audio input/output attrbutes ***/
	/* init stAio. all of cases will use it */
	stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
	stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16; //hi3518c only support 16.
	stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER; //must be MASTER for built-in codec. 
	stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO; // 0:mono(built-in)  1:stereo.
	stAioAttr.u32EXFlag      = 1;  /*should set 1, expand 8bit to 16bit*/
	stAioAttr.u32FrmNum      = 30;
	stAioAttr.u32PtNumPerFrm = AUDIO_POINT_NUM;
	stAioAttr.u32ChnCnt      = 2; //2;4;8;16; built-in only support 2.
	stAioAttr.u32ClkSel      = 1;

	/********************************************
	  step 1: config audio codec
	 ********************************************/
	s32ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr, gs_bMicIn);
	if (HI_SUCCESS != s32ret)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}

	/*********************************************
	 *  step 2: start Adec
	 *********************************************/
	//s32ret = SAMPLE_COMM_AUDIO_StartAdec(g_AdecChn, gs_enPayloadType);
	s32ret = SAMPLE_COMM_AUDIO_StartAdec(s_AdecChn, gs_decPayloadType);
	if (s32ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}

	/*********************************************
	 *  step 3: start Ao
	 *********************************************/
	//s32ret = SAMPLE_COMM_AUDIO_StartAo(g_AoDevId, g_AoChn, &stAioAttr, gs_pstAoReSmpAttr);
	s32ret = SAMPLE_COMM_AUDIO_StartAo(s_AoDev, s_AoChn, &stAioAttr, gs_pstAoReSmpAttr);
	if (s32ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}

	/*********************************************
	 * step 4: Ao bind Adec
	 *********************************************/
	//s32ret = SAMPLE_COMM_AUDIO_AoBindAdec(g_AoDevId, g_AoChn, g_AdecChn);
	s32ret = SAMPLE_COMM_AUDIO_AoBindAdec(s_AoDev, s_AoChn, s_AdecChn);
	if (s32ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32ret);
		return HI_FAILURE;
	}
	printf("bind adec:%d to ao(%d,%d) ok \n", s_AdecChn, s_AoDev, s_AoChn);

	//#if AUDIO_WRITE
	//    //pfd_ADec = SAMPLE_AUDIO_OpenAdecFile(g_AdecChn, gs_enPayloadType);
	//    pfd_ADec = SAMPLE_AUDIO_OpenAdecFile(s_AdecChn, gs_enPayloadType);
	//    if (!pfd_ADec)
	//    {
	//        SAMPLE_DBG(HI_FAILURE);
	//        return HI_FAILURE;
	//    }
	//    HK_DEBUG_PRT("open adec file ok!\n");
	//#endif

	return HI_SUCCESS;          
}


static int inOpen = 0;
static int nopen = 0;  //client listening index.

static int scc_AudioOpen(const char *name, const char *args, int *threadreq) 
{
	HK_DEBUG_PRT("...name:%s, .\n", name);
	if (strcmp(name, "audio.vbAudio.Out") == 0)
	{
		if(HI_SUCCESS != SAMPLE_Audio_AiAenc())
		{
			return -1;
		}
		*threadreq = 1;
		return 1;
	}
	else if( strcmp( name, "audio.vbAudio.In")==0)
	{
		if( inOpen == 0 )
		{
			if( HI_SUCCESS != SAMPLE_Audio_AdecAo())
			{
				return -1;
			}
			inOpen = 1;
			*threadreq = 1;
			return 2;
		}
		return 2;
	}
}

extern VideoDataRecord *slaveAudioDataP;
static int Open(const char *name, const char *args, int *threadreq) 
{
	HK_DEBUG_PRT("...name:%s, ..\n", name);
	if (strcmp(name, "audio.vbAudio.Out") == 0)
	{
		*threadreq=0;

		if( nopen == 0 )
		{
			int iRet = sccInitVideoData( PSTREAUDIO);
			if (iRet == -1)
				return 0;

			sccResetVideData( PSTREAUDIO, slaveAudioDataP );
			CreateAudioThread();

			nopen = 1;
			*threadreq = 1;
			return 1;
		}
		return 1;
	}
	else if( strcmp( name, "audio.vbAudio.In")==0)
	{
		if( inOpen == 0 )
		{
#if 1
			unsigned int groupnum = 5;
			unsigned int bitnum = 0; //GPIO:5_0 (audio out).
			unsigned int val_set = 1; //pull up.
			Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
			Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
			HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
#endif

			if( HI_SUCCESS != SAMPLE_Audio_AdecAo())
			{
				return -1;
			}
			inOpen = 1;
			*threadreq = 1;
			return 2;
		}
		return 2;
	}
}

static void SetAudioAttValue(char *att, int Value) 
{
	return;
	//static radio deviceAtt = { };
	//tDEVICE_INFO device;
	//tAUDIO_CODEC codec;

	//	if (ApiAudioIsOpen( CHANNEL_NUM)) 
	//	{
	//		PUTS(" Open Audio success ");
	//
	//		if (strcmp(att, "Volume") == 0)
	//			deviceAtt.volume = Value;
	//
	//		if (strcmp(att, "direction") == 0)
	//			deviceAtt.direction = Value;
	//
	//		if (strcmp(att, "channel") == 0)
	//			deviceAtt.channel = Value;
	//
	//		ApiAudioSetVolume(deviceAtt.channel, deviceAtt.direction, deviceAtt.volume);
	//	}
}


/***********************************************************
 * func: get stream from aenc channel,then send to client.
 ***********************************************************/
static int Read(int obj, char* buf, unsigned int bufsiz, long* flags) 
{
	int iFream=0;
	int iResType=0;
	int iStreamType=0;
	int iFlag=0;

	int iLen =0;
	int loop=1;
	char *p = buf;
	int iCount=0;
	while (loop-- > 0) 
	{
		int nSize=0;
		char cTmp[320*5]={0};
		iFlag=sccGetVideoDataSlave(PSTREAUDIO, cTmp, &nSize,&iFream,&iResType,&iStreamType);
		//printf("scc Get ...audio len=%d....\n",nSize);
		if (nSize <= 0 || iFlag <=0)
		{
			//if( iCount++ > 10 )
			//break;
			usleep(1000*200);
			loop++;
			continue;
		}
		memcpy( p, cTmp, nSize );
		p += nSize;
		iLen += nSize;
	}

	//printf(".........%d...........\n", iLen);
	int nLevel = 1;
	*flags = HK_BOAT_YLOST;
	//*flags = HK_BOAT_NLOST;
	*flags |= (nLevel<<8);
	return iLen;
}


static int Getms()
{
	struct timeval tv;
	int ms = 0;

	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	return ms;
}

AUDIO_STREAM_S stAudioStream;
AUDIO_FRAME_INFO_S stAudioFrameInfo;
AO_CHN_STATE_S pstStatus;

static char aryVoiceBuf[1280] = {0};
//static char aryVoiceBuf[4000] = {0};
static char aryHeard[4] = {0,1,160,0}; //hisi audio header.

/***********************************************************
 * func: receive audio from client,then decode and paly.
 **********************************************************/
static int Write(int obj, const char* buf, unsigned int bufsiz, long flags)
{
	char *p = (char *)buf;
	short iLen = 320;
	int iCont = 0;
	int i = 0;
	static AUDIO_DEV s_AoDev   = 0;
	static AO_CHN    s_AoChn   = 0;
	static ADEC_CHN  s_AdecChn = 0;

	if (bufsiz % 2 == 1)
	{
		p = p + 1;
		iCont = (bufsiz - 1)/iLen;
		for (i = 0; i < iCont; i++)
		{
			memcpy( aryVoiceBuf+i*4+i*iLen, aryHeard, 4 ); 
			memcpy( aryVoiceBuf+(i+1)*4+i*iLen, p+i*iLen, iLen );
		}
		p = aryVoiceBuf;
	}
	else
	{
		iCont = bufsiz/iLen;
		for (i = 0; i < iCont; i++)
		{
			memcpy( aryVoiceBuf+i*4+i*iLen, aryHeard, 4 );
			memcpy( aryVoiceBuf+(i+1)*4+i*iLen, p+i*iLen, iLen );
		}
		p = aryVoiceBuf;
	}

	iLen += 4;
	while( iCont-- )
	{
		HI_U8 s32ret;
		stAudioStream.pStream = p;
		stAudioStream.u32Len = iLen; 

		//HK_DEBUG_PRT("...bufsiz=%d, iCont=%d, p[0]=%d, p[1]=%d, p[2]=%d, p[3]=%d...\n", bufsiz, iCont, p[0], p[1],p[2],p[3] );

		p += iLen;

		//s32ret = HI_MPI_ADEC_SendStream(g_AdecChn, &stAudioStream, HI_TRUE);
		//s32ret = HI_MPI_ADEC_SendStream(g_AdecChn, &stAudioStream, HI_FALSE);
		s32ret = HI_MPI_ADEC_SendStream(s_AdecChn, &stAudioStream, HI_FALSE);
		if (s32ret)
		{
			printf("error: HI_MPI_ADEC_SendStream failed with:%#x\n", s32ret);
			return 0;
		}

		//#if AUDIO_WRITE
		//    //int AoFd = HI_MPI_AO_GetFd(g_AoDevId, g_AoChn);
		//    int AoFd = HI_MPI_AO_GetFd(s_AoDev, s_AoChn);
		//    HK_DEBUG_PRT("...... AoFd = %d ......\n");
		//#endif

#if 0
		//s32ret = HI_MPI_AO_QueryChnStat(g_AoDevId, g_AoChn, &pstStatus);
		s32ret = HI_MPI_AO_QueryChnStat(s_AoDev, s_AoChn, &pstStatus);
		if (s32ret)
		{
			printf("error: HI_MPI_AO_QueryChnStat failed with:%#x\n", s32ret);
		}
		else
			HK_DEBUG_PRT("-----> pstStatus: u32ChnTotalNum=%d, u32ChnFreeNum=%d, u32ChnBusyNum=%d <------\n", pstStatus.u32ChnTotalNum, pstStatus.u32ChnFreeNum, pstStatus.u32ChnBusyNum);
#endif

		//HI_MPI_ADEC_ClearChnBuf(g_AdecChn);
	}
	return bufsiz;
}

int IPCAM_AoStart()
{
	int threq=0;
	int ret = -1;
	ret = Open("audio.vbAudio.In", NULL, &threq);

	return ret;
}


int IPCAM_AudioDecode(char *pAudioBuf,unsigned int len)
{
	HI_S32 s32Ret;
	SAMPLE_AENC_S stAenc;
	AUDIO_STREAM_S stStream;

	stAenc.AdChn = 0;
	memset(&stStream,0,sizeof(AUDIO_STREAM_S));

	stStream.pStream = pAudioBuf;
	stStream.u32Len = len;

	//½âÂë²¥·Å
	s32Ret = HI_MPI_ADEC_SendStream(stAenc.AdChn, &stStream, HI_TRUE);
	if (s32Ret)
	{
		return 0;
	}

	return 1;
}

//static int Write2dev(char* buf, unsigned int bufsiz,int loop)
//{
//	int buf_len, wr_len;
//	int left;
//	int bytes=0;
//	char *write_buf;
//	static int atm = -1;
//	int btm;
//			
//	buf_len = bufsiz / loop;
//
//	write_buf = malloc(buf_len + 8);
//	if(write_buf == NULL)
//	{
//		PUTS("vbAudio Write can not malloc buf\n");
//		return -1;
//	}
//	
//	while (loop-- > 0)
//	{
//		btm = Getms() - atm;
//		if((btm < 19) && (btm >= 0))
//		{
//			usleep((19-btm)*1000);
//			PUTS("...Write Audio time is %ld %ld...\n",Getms(),atm);
//		}
//		memset(write_buf,0,buf_len + 8);
//			
//		bufhead.codec_type = 0;
//		bufhead.operation_type = 0;
//		bufhead.talker_id = 0;
//		bufhead.timestamp = HKMSGetTick();
//	
//		memcpy(write_buf,&bufhead,8);
//		memcpy(&write_buf[8],&buf[bytes],buf_len);
//
//		wr_len = ApiAudioWriteVoiceFrame(CHANNEL_NUM, write_buf, buf_len + 8);
//		atm = Getms();
//		if (wr_len <= 0) 
//		{
//			PUTS("Audio, Write error\n");
//			free(write_buf);
//			return -1;
//		}
//	
//		bytes += wr_len - 8;
//	
//	}
//	free(write_buf);
//	PUTS("...Write Audio bytes is %d....\n",bytes);
//	return bytes;
//}

int g_Audio_Thread=0;
void AudioThread(void)
{
	printf("scc Create AudioThread.......\n");
	int threq = 0;
	scc_AudioOpen("audio.vbAudio.Out", NULL, &threq);

	unsigned int bufsiz = 320*5;
	char buf[320*5]={0};
	char *p = buf;
	int i = 0;
	int left = 0, loop = 4; //build packet size:2560. 
	int buf_len = 0;        //valid audio size.
	int bytes = 0;          //read audio stream size.
	HI_S32 s32ret = 0;
	static AENC_CHN s_AencChn = 0; //audio encode channel.
	AUDIO_STREAM_S stStream;    
	fd_set read_fds;
	struct timeval TimeoutVal;
	int G711U_Len = 0, G711A_Len = 0;
#if 0
	char g711a_buf[320*5]={0};
#endif
	char amr_buf[32*2];
	int ret;

#if ENABLE_QQ
	tx_audio_encode_param audio_encode_param;
#endif

#if ENABLE_ONVIF
	LPIPCAM_AUDIOBUFFER pAudioBuf = NULL;
	pAudioBuf = (LPIPCAM_AUDIOBUFFER)malloc(sizeof(IPCAM_AUDIOBUFFER));
	if(NULL == pAudioBuf) return NULL;

	IPCAM_PTHREAD_DETACH;
	IPCAM_setTskName("AudioThread");
#endif

#if ENABLE_QQ
	audio_encode_param.head_length = 12;
	audio_encode_param.audio_format = 1;
	audio_encode_param.encode_param = 7;
	audio_encode_param.frame_per_pkg = 2;
	audio_encode_param.sampling_info = GET_SIMPLING_INFO(1, 8, 16);
	audio_encode_param.reserved = 0;
#endif

	FD_ZERO(&read_fds);    
	FD_SET(g_AencFd, &read_fds);
	sleep(1);
	while(g_Audio_Thread )
	{	
		loop = 4;
		bytes = 0;
		left = bufsiz;
		buf_len = 0;
		memset(buf,0,sizeof(buf));
		while (loop-- > 0) 
		{     
			TimeoutVal.tv_sec = 1;
			TimeoutVal.tv_usec = 0;

			FD_ZERO(&read_fds);
			FD_SET(g_AencFd, &read_fds);

			s32ret = select(g_AencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
			if (s32ret <= 0) 
			{
				printf("[%s, %d] get aenc stream select error or timeout!s32ret=%d\n", __FUNCTION__, __LINE__,s32ret);
				break;
			}

			if (FD_ISSET(g_AencFd, &read_fds))
			{
				s32ret = HI_MPI_AENC_GetStream(s_AencChn, &stStream, HI_FALSE);
				if (HI_SUCCESS != s32ret )
				{
					printf("%s: HI_MPI_AENC_GetStream(%d) failed with %#x!\n", __FUNCTION__, s_AencChn, s32ret);
					break;
				}
#if ENABLE_P2P
				P2PNetServerChannelDataSndToLink( 0, 0, stStream.pStream, stStream.u32Len, 1, DATA_AUDIO);
				P2PNetServerChannelDataSndToLink( 0, 1, stStream.pStream, stStream.u32Len, 1, DATA_AUDIO);
#endif

#if ENABLE_QQ
				//printf("the lenth is %d\n", stStream.u32Len);
				ret = PCM2AMR(stStream.pStream, stStream.u32Len, amr_buf);
				tx_set_audio_data(&audio_encode_param, amr_buf, ret);
#endif

				HI_MPI_AENC_ReleaseStream(s_AencChn, &stStream);
#if ENABLE_ONVIF
				//pAudioBuf->dwFameSize = stStream.u32Len;
				//pAudioBuf->dwFrameNumber= stStream.u32Seq;
				//pAudioBuf->bzEncType	= STREAM_ENCTYPE_AUDIO_G711UL;
				//pAudioBuf->dwSec		= stStream.u64TimeStamp/1000;//TimeoutVal.tv_sec;
				//pAudioBuf->dwUsec		= (stStream.u64TimeStamp%1000)*1000;//TimeoutVal.tv_usec;	
				//memcpy(pAudioBuf->AudioBuffer, stStream.pStream, stStream.u32Len);
				//IPCAM_PutStreamData(VIDEO_LOCAL_RECORD, 0, VOIDEO_MEDIATYPE_AUDIO, pAudioBuf);

				/**transfered to g711u for NVR**/
				G711U_Len = PCM2G711u(stStream.pStream, pAudioBuf->AudioBuffer, stStream.u32Len, 0);
				pAudioBuf->dwFameSize   = G711U_Len;
				pAudioBuf->dwFrameNumber = stStream.u32Seq;
				pAudioBuf->bzEncType	= STREAM_ENCTYPE_AUDIO_G711UL;
				pAudioBuf->dwSec		= stStream.u64TimeStamp/1000;//TimeoutVal.tv_sec;
				pAudioBuf->dwUsec		= (stStream.u64TimeStamp%1000)*1000;//TimeoutVal.tv_usec;	
				IPCAM_PutStreamData(VIDEO_LOCAL_RECORD, 0, VOIDEO_MEDIATYPE_AUDIO, pAudioBuf);
#endif
#if 0
				/**transfered to g711a for PC client**/
				G711A_Len = PCM2G711a(stStream.pStream, g711a_buf, stStream.u32Len, 0);
				buf_len = G711A_Len;
				//printf("-----> G711U_Len:%d, G711A_Len:%d <------\n", G711U_Len, G711A_Len);
				if ( buf_len > 8 )
				{
					memcpy(buf+bytes, g711a_buf, buf_len ); 
				}

				HI_MPI_AENC_ReleaseStream(s_AencChn, &stStream);

				if ( buf_len <= 8 ) 
				{
					usleep(1000);
					loop++;
					continue;
				}

				bytes += buf_len;
				left -= buf_len;
				if (left < 0)
					break;
#endif
			}    
		}
#if 0
		if ((nopen != 0) && (bytes > 0))
		{
			sccPushStream( 1234, PSTREAUDIO, buf, bytes, 0, 0, 10 );
		}

		if ((bytes > 0) && (1 == hkSdParam.audio))
		{			
			sccPushTfData( PSTREAMTWO, buf, bytes, 1, 0, 0 );
		}
#endif
		usleep(1000*100);
	}

#if ENABLE_ONVIF
	if (pAudioBuf) free(pAudioBuf);
	IPCAM_PTHREAD_EXIT;	
#endif
}

int CreateAudioThread(void)
{
	if ( g_Audio_Thread == 0 )
	{
		g_Audio_Thread = 1;
		pthread_t id;
		int ret = 0;

		ret = pthread_create(&id, NULL, (void *)AudioThread, NULL);
		if (ret != 0)
		{
			PUTS("CreateAudioThread error!\n");
			return -1;
		}
		//pthread_detach(id);
	}
	return 1;
}

static void SetIRQEventCallback(RSObjectIRQEvent irq) 
{
	irq = irq;
}

enum _HK_CONFIG_TYPE
{
	STR_STR = 0,
	STR_INT,
	STR_DOUBLE,
};

static int GetConfigStrValue(char* buff, void* pValue, int nType) 
{
	if ((buff = strstr(buff, "=")) == NULL)
		return -1;
	buff += 1;
	switch (nType) 
	{
		case STR_INT:
			*(int*) pValue = atoi(buff);
			break;
		case STR_DOUBLE:
			*(double*) pValue = atof(buff);
			break;
		case STR_STR:
			strcpy((char*) pValue, buff);
			break;
		default:
			return -1;
	}
	return 0;
}

static int _xGetConfigValue(char* szPath, char* szRoot, char* szName, void* pValue,
		int nType)
{
	FILE* pfile;
	int nFlag = 0;
	char buff[1024], *p;

	if ( (pfile = fopen(szPath, "r")) == NULL)
	{
		PUTS("fopen %s: error\n", szPath);;
		return -1;
	}

	while (fgets(buff, sizeof(buff), pfile))
	{
		// fprintf(stderr, buff);
		if (buff[0] == '#') 
		{
			continue;
		}
		if ( (p = strchr(buff, '\r')))
			*p = '\0';
		if ( (p = strstr(buff, "\n")))
			*p = '\0';
		if (nFlag == 0 && buff[0] != '[') 
		{
			continue;
		} 
		else if (nFlag == 0 && buff[0] == '[') 
		{
			if (strncmp(buff+1, szRoot, strlen(szRoot)) == 0)
				nFlag = 1;
		} else if (nFlag == 1 && buff[0] == '[') 
		{
			break;
		} 
		else 
		{
			if (strstr(buff, szName) == &buff[0]) 
			{
				GetConfigStrValue(buff, pValue, nType);
				fclose(pfile);
				return 0;
			}
		}
	}
	fclose(pfile);
	return -1;
}

static const char* Retrieve() 
{
	return "audio.vbAudio.Out,audio.vbAudio.In";

	//static char buf[128];
	//_xGetConfigValue( CFG_FILE, "system", "DEVID", buf, STR_STR);

	//return buf;
}


/********************************************
  step 6: exit the process
 ********************************************/
static void Close(int obj) 
{
	if( obj == 1)
	{
		if( nopen == 0 )
			return;
		nopen = 0;

		/*
		   static AUDIO_DEV s_AiDev = 0;
		   static AI_CHN    s_AiChn = 0;
		   static AENC_CHN  s_AencChn = 0;
		   int i = 0;
		   HI_S32 s32AencChnCnt = 1;

		   for (i = 0; i < s32AencChnCnt; i++)
		   {
		//SAMPLE_COMM_AUDIO_AencUnbindAi(g_AiDevId, g_AiChn, g_AencChn);
		SAMPLE_COMM_AUDIO_AencUnbindAi(s_AiDev, s_AiChn, s_AencChn);
		}

		SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
		//SAMPLE_COMM_AUDIO_StopAi(g_AiDevId, g_AiChnCnt, gs_bAiAnr, HI_FALSE);
		SAMPLE_COMM_AUDIO_StopAi(s_AiDev, g_AiChnCnt, gs_bAiAnr, HI_FALSE);
		 */
		printf("[%s, %d]......audio out closed !\n", __func__, __LINE__);
	}
	else if ( obj == 2 )
	{
		if ( inOpen == 0) 
			return;

#if 1
		unsigned int groupnum = 5;
		unsigned int bitnum = 0; //GPIO:5_0 (audio out).
		unsigned int val_set = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
#endif

		static AUDIO_DEV s_AoDev = 0;
		static AO_CHN    s_AoChn = 0;
		static ADEC_CHN  s_AdecChn = 0;
		inOpen = 0;

		//SAMPLE_COMM_AUDIO_AoUnbindAdec(g_AoDevId, g_AoChn, g_AdecChn);
		//SAMPLE_COMM_AUDIO_StopAdec(g_AdecChn);
		//SAMPLE_COMM_AUDIO_StopAo(g_AoDevId, g_AoChn, gs_bAioReSample);
		SAMPLE_COMM_AUDIO_AoUnbindAdec(s_AoDev, s_AoChn, s_AdecChn);
		SAMPLE_COMM_AUDIO_StopAdec(s_AdecChn);
		SAMPLE_COMM_AUDIO_StopAo(s_AoDev, s_AoChn, gs_bAioReSample);
		printf("[%s, %d]......audio in closed !\n", __func__, __LINE__);
	}
	else 
	{
		exit(1);
	}
}


static const char* GetObjectInfo() 
{
	return "Type=audio.vbAudio;Outpin=G723,G723_24,G726_24;";
}

static int DoEvent(const char* devname, int obj, const char* ev) 
{

	HKFrameHead *pFrameHead;
	char *cValue;
	int iValue;

	pFrameHead = CreateFrameA(ev, strlen(ev) );
	iValue = GetParamUN(pFrameHead, "Volume");
	SetAudioAttValue("Volume", iValue);

	iValue = GetParamUN(pFrameHead, "direction");
	SetAudioAttValue("direction", iValue);

	iValue = GetParamUN(pFrameHead, "Volume");
	SetAudioAttValue("Volume", iValue);

	return 0;
}

void audio_RSLoadObjects(RegisterFunctType reg) 
{
	vbAudio.super.SetIRQEventCallback = SetIRQEventCallback;
	vbAudio.super.Compare       = NULL;
	vbAudio.super.Retrieve      = &Retrieve;
	vbAudio.super.GetObjectInfo = &GetObjectInfo;
	vbAudio.super.Open          = &Open;
	vbAudio.super.Read          = &Read;
	vbAudio.super.Write         = &Write;
	vbAudio.super.DoEvent       = &DoEvent;
	vbAudio.super.Close         = &Close;

	vbAudio.super.Convert = NULL;
	vbAudio.irq = NULL;

	(*reg)(&vbAudio.super);
	assert((void*)&vbAudio.super == (void*)&vbAudio);
}

