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
#include "ipc_email.h"
#include "ipc_sd.h"
#include "ipc_hk.h"


/*报警检查*/
extern int g_startCheckAlarm;

extern HK_SD_PARAM_ hkSdParam;

#define ALARMTIME 6000*3
#define PATH_SNAP "/mnt/mmc/snapshot"
char g_JpegBuffer[MAXIMGNUM][ALLIMGBUF] = {0}; //picture buffer.
//截图的存储空间
int imgSize[MAXIMGNUM] = {0};

/********************* IO alarm & email ***********************/
HI_S32 g_vencSnapFd = -1; //picture snap fd.
short g_SavePic = 0;
bool get_pic_flag = false; //enable snap pictures.
static int g_SnapCnt = 0; //picture snap counter.
static bool g_bIsOpenPict = false;

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

#if 0
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


