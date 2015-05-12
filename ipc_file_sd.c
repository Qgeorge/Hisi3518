#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <time.h>

#include <sys/times.h>

#include "ipc_hk.h"

#include "rs.h"
#include "sys.h"
#include "utils/HKMonCmdDefine.h"
#include "utils/HKCmdPacket.h"
//#include "utils/HKDataBase.h"

extern bool b_hkSaveSd;
static RSObject file_sd_inst;
static RSObjectIRQEvent ev_irq = NULL;
//static pthread_rwlock_t fd_rwlock;
//static pthread_mutex_t plock_mutex;

static pthread_mutex_t g_MutexLock_TF = PTHREAD_MUTEX_INITIALIZER;
static short g_TF_start=0;
//char g_cDataBuf[200*1024]={0};
char *g_cDataBuf = NULL;

#define max_count	5
static int g_FileReadSum = 0;
#pragma pack(1)

typedef struct HK_VHeader_v2
{
	unsigned int fragx:2;
	unsigned int flipex:2;
	unsigned int resolution:4;
	unsigned int encode:5;
	unsigned int flip:1;
	unsigned int version:2;
} __attribute__ ((packed))  HK_VHeader;

HK_VHeader g_hkHeader;
int g_delcount=0;
int g_allcount=0;
HKFrameHead *frameRec=NULL;
extern HK_SD_PARAM_ hkSdParam;
extern int GetStorageInfo();
extern short g_sdIsOnline;


typedef struct FrameHead
{   // 
	unsigned char cFrameMark[3];    
	unsigned char cFrameType;       //DER_MASK

	unsigned int nDataType;         // 
	unsigned int nID;                   // 
	unsigned int nLength;               // 
	unsigned int nDurationTicks;        // 
	unsigned int nFrameCount;           // 
} __attribute__ ((packed)) FrameHead_;

typedef struct FileHead
{   // 
	unsigned char    cFileMark[3];       //'HEK',
	unsigned short  nFileHeadVer;       // 
	unsigned long long lStartDateTime;  //
	unsigned int    nDurationTicks; // 
	unsigned int    nFrameCount;        //
	unsigned int    nIndexSize;         //
	unsigned int    nUserInfoSize;      //
	char            szUserName[50];     // 

	unsigned char   bIsFinish;          // 
	unsigned int    iFileSize;          //
	char            szOther[100];       // 
}__attribute__ ((packed)) FileHead_;

typedef struct FrameIndex
{   //
	unsigned long nLength;              //
	unsigned int nDurationTicks;        // 
	unsigned int nDurationFrame;        //
}__attribute__ ((packed)) FrameIndex_;

enum HK_MEDIA_FILE_FRAME_DATA_TYPE
{
	HK_MFF_VIDEO = 1,
	HK_MFF_AUDIO = 2,
	HK_MFF_MPEG4 = 4,
	HK_MFF_MJPEG = 8,
	HK_MFF_H264 = 16,
	HK_MFF_G723 = 32,
	HK_MFF_G729 = 64,
	HK_MFF_G711 = 128,
	HK_MFF_G726 = 256,
	HK_MFF_PCMA = 512,
	HK_MFF_PCMU = 1024
};

#pragma pack()

FrameIndex_ m_frameIndex;
FileHead_ m_fileHead;
FrameHead_ m_frameHead;

static void sd_deleteRec();
static void SetIRQEventCallback(RSObjectIRQEvent cb) 
{
	ev_irq = cb;
}

static const char* Retrieve()
{
	return "File.SDStorage.in,File.SDStorage.out";
}

static const char* GetObjectInfo()
{
	return "Type=File;";
}

FILE *g_fpRead=NULL;
typedef struct File_list
{
	FILE *fpRead;
	short rflag;

}file_rlist;

file_rlist file_r[max_count] = {0};

static int Read(int obj, char *buf, unsigned int bufsiz, long *flags)
{
	//HK_DEBUG_PRT("...SD Read, g_sdIsOnline:%d, g_FileReadSum:%d, *flags:%d, obj:%d ...\n", g_sdIsOnline, g_FileReadSum, *flags, obj);
	if (0 == g_sdIsOnline)
		return -1;

	obj -= 1;
	usleep(40*1000 + g_FileReadSum*2000);
	unsigned int nRead=0;
	*flags = 0;
	if( file_r[obj].fpRead == NULL )
	{
		file_r[obj].rflag = 0;
		return -1;
	}

	int nReadSize = 4096;
	if (g_FileReadSum > 3)
	{
		g_FileReadSum = 1024;
	}

	nRead = fread( buf, 1, nReadSize, file_r[obj].fpRead );  //read ..
	if( nRead <= 0 )
	{
		//printf("scc Read TF=%d....\n", obj);
		fclose( file_r[obj].fpRead );
		file_r[obj].fpRead = NULL;
		file_r[obj].rflag = 0;
		return -1;
	}
	*flags = HK_BOAT_NLOST;
	int iLeve = 3;
	*flags |= (iLeve<<8);
	return nRead;
}

static VideoDataRecord *hostVideoDataPTF = NULL;
static int sccPushVideoData_TF(int pStreamType,char *pData,unsigned int nSize,short IFrame,int iResType,int iStreamType,VideoDataRecord  *mVideoDataBuffer);

static void sccResetVideDataTF(int pStreamType, VideoDataRecord  *mVideoDataBuffer);

static int TFsccInitVideoData(int pStreamType)
{
	if ((0 == pStreamType) && (NULL == hostVideoDataPTF))
	{
		hostVideoDataPTF = calloc(1, sizeof(VideoDataRecord));
		if (NULL == hostVideoDataPTF)
		{
			HK_DEBUG_PRT("calloc error with:%d, %s\n", errno, strerror(errno));
			return -1;
		}

		hostVideoDataPTF->g_videoBuf = calloc(1, MAX_VIDEODATA_SLAVETREAM);
		if (NULL == hostVideoDataPTF->g_videoBuf)
		{
			HK_DEBUG_PRT("calloc error with:%d, %s\n", errno, strerror(errno));
			free(hostVideoDataPTF);
			hostVideoDataPTF = NULL;
			return -1;
		}
		return 0;
	}
	return 0;
}


static char* GetTimeChar(char* buf)
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = gmtime(&timep);
	sprintf(buf, "%d%02d%02d%02d%02d%02d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	unsigned long time_Sec = (unsigned long)timep;
	conf_set_int("/mnt/sif/time.conf", "time_", time_Sec);
	printf("... update sys time, time_Sec:%u ...\n", time_Sec);

	return buf;
}

#define FILE_HEADER_MASK   "HEK"
#define FILE_INDEX_MASK    "IDEX"
#define FILE_USERINFO_AREA 1024*10
#define FILE_HEAD_VER 1

#define MIN_USEFUL_BYTES 1
#define HK_FRAME_I 1
#define HK_FRAME_OTHERS 0

static int sd_open( char *cTime );
FILE *g_fp=NULL;
static int Open(const char* name, const char* args, int* threq)
{
	int ret = 0;
	if( g_sdIsOnline == 0 )
		return 0;
	if (strcmp(name,"File.SDStorage.in")==0 )
	{
		return 0;
		if (args != NULL)
		{
			if(strcmp(args, "audioin")==0 )
			{
				//printf("....OPEN.audio=%s...\n",args);
				ret = max_count + 2;
				return ret;
			}

			int isdOpen;
			memset(&m_fileHead, 0, sizeof(m_fileHead));
			m_fileHead.nFileHeadVer = FILE_HEAD_VER;
			memcpy(m_fileHead.cFileMark, FILE_HEADER_MASK, 3);

			isdOpen = sd_open( args );//time
			if ( isdOpen == -1 )
				return 0;
			ret = max_count+1;
		}
	}
	else if (strcmp(name, "File.SDStorage.out")==0)
	{
		//printf("....args=%s...\n", args );
		if (args == NULL)
			return 0;

		char cPatch[64]={0};
		HKFrameHead* frameOpenRec = CreateFrameA(args, strlen(args));
		char *recfile = GetParamStr( frameOpenRec, HK_KEY_REC_FILE );
		if( NULL==recfile)
		{
			DestroyFrame(frameOpenRec);
			//printf("scc Open SD file recfile=NULL.....\n");
			return 0;
		}

		int i = 0;
		snprintf(cPatch, sizeof(cPatch), "/mnt/mmc/snapshot/%s", recfile);
		for (i = 0; i < max_count; i++)
		{
			if (file_r[i].rflag == 0)
			{
				FILE *fpRead = NULL;
				if((fpRead=fopen(cPatch, "rb"))==NULL )
				{
					printf("scc.SD.open fail...cPatch=NULL...\n");
					return 0;
				}
				file_r[i].rflag =1;
				file_r[i].fpRead = fpRead;
				*threq=1;
				g_FileReadSum = i+1;
				DestroyFrame(frameOpenRec);
				int iObj = i+1;
				printf("scc Open SD success obj=%d...\n",iObj);

				return iObj;
			}
		}
		printf("scc obj=0........\n");
		DestroyFrame(frameOpenRec);
		return 0;
	}
	return ret;
}

//unsigned long long g_ulDataLen = 0;
unsigned int g_ulDataLen = 0;
static void Close(int obj)
{
	printf("scc sd file Close obj=%d...\n", obj);
	if(obj > max_count)
	{
		g_ulDataLen = 0;
		if( NULL != g_fp )
		{
			fclose(g_fp);
			g_fp = NULL;
		}
		return;
	}
	else if(obj > 0 )
	{
		obj -= 1;
		if( g_FileReadSum>0 )
		{
			g_FileReadSum--;
		}
		if(NULL!=file_r[obj].fpRead)
		{
			fclose(file_r[obj].fpRead);
			file_r[obj].fpRead=NULL;
			file_r[obj].rflag=0;
		}
	}
}

unsigned long m_lPreTick=0;
bool m_bCurIsIVOP=true;

unsigned long GetTickCount()
{
	struct tms tm_s;
	return times(&tm_s)*10;
	/*struct timeval now;
	  gettimeofday( &now,NULL );
	  return now.tv_sec*1000+now.tv_usec/1000; */
}

void WriteFrameHead(FILE* pFile, long nID, int nLength, unsigned int nDataType/* = HK_MFF_VIDEO | HK_MFF_MPEG4*/,int iStreamType)
{
	int nRelativeTime = 0;

	unsigned long lNowTick = GetTickCount();
	if(m_lPreTick != 0)
		nRelativeTime = lNowTick - m_lPreTick;
	m_lPreTick = lNowTick;

	m_fileHead.nDurationTicks += nRelativeTime;
	m_fileHead.iFileSize += sizeof(FrameHead_);
	++m_fileHead.nFrameCount;

	memset(m_frameHead.cFrameMark,0x0,3);
	m_frameHead.cFrameMark[2] = 0x01;

	m_frameHead.cFrameType = m_bCurIsIVOP ? HK_FRAME_I : HK_FRAME_OTHERS;
	m_frameHead.nDataType = nDataType;
	m_frameHead.nID = nID;
	if(4== iStreamType)
	{
		m_frameHead.nLength = nLength+2;
	}
	else
	{
		m_frameHead.nLength = nLength;
	}
	m_frameHead.nDurationTicks = m_fileHead.nDurationTicks;
	m_frameHead.nFrameCount = m_fileHead.nFrameCount;
	fwrite(&m_frameHead, 1, sizeof(m_frameHead), pFile);
}

int Write_(long nID, const void* pBuffer, int nLength,int iFream, unsigned int nDataType/* = HK_MFF_VIDEO | HK_MFF_MPEG4*/,int iStreamType)
{
	time_t temp;
	time(&temp);
	m_fileHead.lStartDateTime = (unsigned long long)temp;

	if(iFream==1)
	{
		m_bCurIsIVOP = true;
	}
	else
	{
		m_bCurIsIVOP = false;           
	}

	WriteFrameHead(g_fp, nID, nLength, nDataType, iStreamType);
	if( iStreamType == 4 )//add head video
		m_fileHead.iFileSize += m_fileHead.iFileSize+nLength+2;
	else
		m_fileHead.iFileSize += nLength;
	if(iStreamType == 4 )//video
	{
		fwrite(&g_hkHeader, 1, 2, g_fp);
	}
	if( fwrite(pBuffer, 1, nLength, g_fp) <= 0 )
	{
		hk_SdInfoEmail(1);//email sd error
		hkSdParam.allSize = 14;
		g_sdIsOnline = 0;
		return -1;
	}
	WriteFileHeader(&m_fileHead);
	fflush(g_fp);
	return 1;
}

int WriteFileHeader(FileHead_* pFileHead)
{
	if( NULL == pFileHead || NULL == g_fp )
	{
		return -1;
	}

	fpos_t posCur;
	fgetpos(g_fp, &posCur);
	fseek(g_fp, 0, SEEK_SET);
	if( fwrite((void*)pFileHead, 1, sizeof(FileHead_), g_fp) <= 0 )
	{
		return -1;
	}
	fsetpos(g_fp, &posCur);
	return 1;
}

static int sccISIframe(const char *buffer, int length)
{
	int i = 0;
	for( i=0; i<length-100; i++)
	{
		if( (*( buffer+ i  ) == 0x00 )
				&& (*(buffer+ i + 1) == 0x00 )
				&& (*(buffer + i + 2 ) == 0x00 )
				&& ( *(buffer + i + 3 ) == 0x01) )
		{
			if( *(buffer+i+4)==0x6||*(buffer+i+4)==0x68||*(buffer+i+4)==0x67||*(buffer+i+4)==0x65|| ( *(buffer+i+4)==0x27 )||( *(buffer+i+4)==0x28 ) )
			{
				return 1;
			}
			else if( *(buffer+i+4)==0x61||*(buffer+i+4)==0x41|| *(buffer+ i + 4) == 0x21 ||  *(buffer+ i + 4) == 0x25 )
			{
				return 0;
			}
		}
	}
	return 1;
}



static unsigned int g_nLen=0;
static int g_nFlgX=0;

static char *g_cDataBufc=NULL;
static int OnDataBuf( int iIndex, const char *data, unsigned int iLen )
{
	if( iIndex==3)
	{
		memset(g_cDataBufc, 0, sizeof(g_cDataBufc));
		memcpy(g_cDataBufc, data, iLen);
		g_nFlgX= iIndex;
		g_nLen = iLen;
		return 1;
	}
	if( iIndex==2)
	{
		if( g_nFlgX!=3)
		{
			memset(g_cDataBufc, 0, sizeof(g_cDataBufc));
			g_nFlgX= 0;
			g_nLen = 0;
			return 0;
		}
		iLen -= 2;
		memcpy(g_cDataBufc+g_nLen, data+2, iLen);
		g_nLen += iLen;
		g_nFlgX= iIndex;
		return 1;
	}
	if( iIndex==1)
	{
		if( g_nFlgX!=2)
		{
			memset(g_cDataBufc, 0, sizeof(g_cDataBufc));
			g_nFlgX= 0;
			g_nLen = 0;
			return 0;
		}
		iLen -= 2;
		memcpy(g_cDataBufc+g_nLen, data+2, iLen);
		g_nLen += iLen;

		int flag = sccISIframe( g_cDataBufc, g_nLen);
		//flags=1;

		sccPushVideoData_TF(0,g_cDataBufc,g_nLen, flag, 4, 4, hostVideoDataPTF);
		g_nLen = 0;
		g_nFlgX= 0;
		return 1;
	}
	return 0;
}


static int Write(int obj, const char* buf, unsigned int len, long flags)
{
	printf("hkSdParam.leftSize:%d, hkSdParam.audio:%d, hkSdParam.loopWrite:%d, obj:%d, flags:%d...\n", hkSdParam.leftSize, hkSdParam.audio, hkSdParam.loopWrite, obj, flags);
	if( g_sdIsOnline == 0 )
	{
		return -1;
	}

	/*
	   if(hkSdParam.leftSize <= HK_SD_SPLITE)//MB
	   {
	   RSCHangup(HK_AS_MONS, "MotionAlarmSDRecord");

	   if (hkSdParam.audio == 1)
	   RSCHangup( HK_AS_MONS, "MotionAlarmSDAudio" );

	   g_ulDataLen = 0;
	   if (hkSdParam.loopWrite == 1)
	   b_hkSaveSd=true;

	   return -1;
	   }
	 */

	if( obj == max_count+2)//audio
	{
		sccPushVideoData_TF(0, buf, len, 1, 4, 10, hostVideoDataPTF);
		return len;
	}

	HK_VHeader hkHeader;
	memcpy( &hkHeader, buf, 2 );
	if( hkHeader.fragx != 0 )
	{
		if( g_cDataBufc ==NULL)
		{
			g_cDataBufc = malloc( 200*1024);
		}
		if( g_cDataBufc != NULL )
		{
			OnDataBuf( hkHeader.fragx, buf, len );
		}
		return len;
	}
	int iIsframe = sccISIframe( buf, len);
	//iIsframe=1;
	sccPushVideoData_TF(0, buf, len, iIsframe, 4, 4, hostVideoDataPTF);
	return len;
}

static int DoEvent( const char* devname, int obj, const char* ev ) 
{
#if 0
	HKFrameHead *pFrameHead;
	int ec, mc;
	// char* ev;
	HKLG_DEBUG("DoEvent(%s, %d, %s)", devname, obj, ev);

	//	printf("File_sd DoEvent in..........\n");
	if (ev == NULL)
		return 1;

	pFrameHead = CreateFrameA((char*)ev, strlen(ev) );
	ev = GetParamStr( pFrameHead, HK_KEY_EVENT );
	pthread_rwlock_wrlock(&fd_rwlock);

	if (strcmp(ev, HK_EVENT_ALARM) == 0)
	{
		alarm_on = 1;
	}

	pthread_rwlock_unlock(&fd_rwlock);
	DestroyFrame(pFrameHead);
#endif
	return 0;
}

static void updateSdFileListCount( char *cTime )
{
	FILE *fpWrite=NULL;
	fpWrite = fopen("/mnt/mmc/snapshot/rec_file.txt", "a+");
	if( fpWrite != NULL )
	{
		unsigned long iFleSzie=0;
		char cPatbuf[125]={0};
		char keyBuf[64]={0};
		sprintf(keyBuf, "%s%d", HK_KEY_VALUEN,g_allcount-1 );
		char *cFile = DictGetStr(frameRec, keyBuf );
		if( cFile != NULL )
		{
			snprintf(cPatbuf,sizeof(cPatbuf), "/mnt/mmc/snapshot/%s.hkv", cFile);
			iFleSzie = get_file_size( cPatbuf );
			//printf("scc get file size=%d....\n", iFleSzie);
		}
		if( iFleSzie > 0 )
		{
			sprintf( cPatbuf, "ValueN%d", g_allcount );
			SetParamStr(frameRec, cPatbuf,cTime);

			g_allcount += 1;
			char cRec[125]={0};
			sprintf( cRec, "%s=%s;",cPatbuf,cTime);
			if( fwrite(cRec,strlen(cRec), 1, fpWrite) <=0 )//time
			{
				g_sdIsOnline = 0;
			}
			fclose( fpWrite );
		}
		else
		{
			sprintf( cPatbuf, "ValueN%d", g_allcount-1 );
			SetParamStr(frameRec, cPatbuf,cTime);
			//sprintf( cRec, "%s=%s;",cPatbuf,cTime);
			//if( fwrite(cRec,strlen(cRec), 1, fpWrite) <=0 )//time
			//{
			//    g_sdIsOnline = 0;
			//}
			file_SdDeleteFile("123456");
			fclose( fpWrite );
		}

		FILE *fpCount=NULL;
		fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");
		if ( fpCount != NULL )
		{
			char cCount[64]={0};
			sprintf( cCount,"delcount=%d;allcount=%d;",g_delcount, g_allcount );
			fwrite( cCount, strlen(cCount), 1, fpCount);
			fclose( fpCount );
		}
	}
	return; 
}

static int sd_open( char *cTime )
{
	if (0 == g_sdIsOnline)
	{
		return -1;
	}

	char cPatch[64] = {0};
	if (NULL == g_fp)
	{
		snprintf(cPatch, sizeof(cPatch), "/mnt/mmc/snapshot/%s.hkv", cTime);
		if ( (g_fp = fopen(cPatch, "w+b")) == NULL )
		{
			printf( "open file fail\n" );
			g_sdIsOnline = 0;
			return -1;
		}
	}
	return 0;

	/*
	   if( g_allcount >=1 )
	   {
	   updateSdFileListCount( cTime );
	   return 1;;
	   }

	   FILE *fpWrite=NULL;
	   fpWrite = fopen("/mnt/mmc/snapshot/rec_file.txt", "a+");
	   if( fpWrite != NULL )
	   {
	   char cPatbuf[125]={0};

	   sprintf( cPatbuf, "ValueN%d",g_allcount );
	   SetParamStr(frameRec, cPatbuf,cTime);
	   char cRec[125]={0};
	   sprintf( cRec, "%s=%s;",cPatbuf,cTime);
	   if( fwrite(cRec,strlen(cRec), 1, fpWrite) <=0 )//time
	   {
	   g_sdIsOnline = 0;
	   }
	   fclose( fpWrite );
	   g_allcount += 1;

	   FILE *fpCount=NULL;
	   fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");
	   if ( fpCount != NULL )
	   {
	   char cCount[64]={0};
	   sprintf( cCount,"delcount=%d;allcount=%d;",g_delcount, g_allcount );
	   fwrite( cCount, strlen(cCount), 1, fpCount);
	   fclose( fpCount );
	   }
	   }
	   return 0;
	 */
}

static void sd_initSd()
{
	if( g_sdIsOnline == 0 )
	{
		return;
	}
	FILE *fpCount =NULL;
	fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "r");
	if ( fpCount==NULL )
	{
		char *pBuf = "delcount=0;allcount=0;";
		fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");

		if( fwrite( pBuf, strlen(pBuf), 1, fpCount)<=0)
		{
			hk_SdInfoEmail(1);//email sd error
			hkSdParam.allSize = 14;
			g_sdIsOnline = 0;
		}
		fclose( fpCount );
	}
	else
	{
		char buf[64]={0};
		fread(buf, sizeof(buf), 1, fpCount );

		HKFrameHead *framePacket=CreateFrameA(buf, strlen(buf));
		g_delcount = GetParamUN( framePacket, "delcount" );
		g_allcount = GetParamUN( framePacket, "allcount" );
		//g_allcount -= 1;
		DestroyFrame(framePacket);
		fclose( fpCount );
	}

	FILE *fpRec = NULL;
	fpRec = fopen( "/mnt/mmc/snapshot/rec_file.txt", "r" );
	if ( fpRec==NULL )
	{
		//fclose(fpRec);
		frameRec = CreateFrameB();
	}
	else
	{
		//char cBuf[84096]={0};
		char sdIndexData[1024]={0};
		int nFileSize = HKGetFileSize( "/mnt/mmc/snapshot/rec_file.txt" );
		char *pSDCBuf = (char *)malloc( nFileSize + 256 );
		if( pSDCBuf == NULL )
		{
			fclose(fpRec);
			printf( "SD malloc fail!\n" );
			return;
		}
		memset( pSDCBuf,0,nFileSize + 256 );
		int nRLen = 0;
		int nPos = 0;
		do
		{
			nRLen = fread( sdIndexData,1,sizeof(sdIndexData),fpRec );
			if( nRLen > 0 )
			{
				memcpy( pSDCBuf + nPos,sdIndexData,nRLen );
				nPos += nRLen;
			}
		}while( nRLen );
		*(pSDCBuf+nPos) = '\0';
		//printf( "fxb=== %s\n",pSDCBuf );
		frameRec = CreateFrameA( pSDCBuf, strlen(pSDCBuf) );
		free( pSDCBuf );
		pSDCBuf = NULL;
		fclose(fpRec);
	}
}

static void sd_deleteRec()
{

	char cPatch[64]={0};
	strcpy(cPatch,"/mnt/mmc/snapshot");
	char cDel[128]={0}; 
	snprintf(cDel,sizeof(cDel),"rm -r %s/$(ls %s -rt | sed -n '1p')",cPatch, cPatch );
	system(cDel);
	//printf("scc..delTF=%s.......\n", cDel);

	snprintf(cDel,sizeof(cDel),"rm -r %s/$(ls %s -rt | sed -n '2p')",cPatch, cPatch );
	system(cDel);
	//printf("scc..delTF=%s.......\n", cDel);

	snprintf(cDel,sizeof(cDel),"rm -r %s/$(ls %s -rt | sed -n '3p')",cPatch, cPatch );
	system(cDel);
	//printf("scc..delTF=%s.......\n", cDel);

	snprintf(cDel,sizeof(cDel),"rm -r %s/$(ls %s -rt | sed -n '4p')",cPatch, cPatch );
	system(cDel);
	//printf("scc..delTF=%s.......\n", cDel);

	snprintf(cDel,sizeof(cDel),"rm -r %s/$(ls %s -rt | sed -n '5p')",cPatch, cPatch );
	system(cDel);
	printf("scc..delTF=.......\n");


#if 0
	bool b_del=false;
	char delKey[64]={0};
	int i=1;
	if( g_delcount ==0 || g_allcount < g_delcount )
		g_delcount = 1;
	for ( i=1; i<=5; i++ )
	{
		sprintf(delKey, "%s%d",HK_KEY_VALUEN, g_delcount );
		char *cDelFile = GetParamStr( frameRec, delKey );
		if ( cDelFile != NULL )
		{
			char cPatch[64]={0};
			snprintf(cPatch,sizeof(cPatch), "/mnt/mmc/snapshot/%s.hkv", cDelFile);

			b_del = true;
			char delBuf[64]={0};
			sprintf(delBuf,"rm -rf %s",cPatch);// cDelFile );
			g_delcount +=1;
			//printf("...delbuf=%s...\n",delBuf);
			system( delBuf );
			DeleteFrameKey( frameRec, delKey );
		}
		else
		{
			if(g_delcount - g_allcount >= 10 )
				return;
			g_delcount +=1;
			FILE *fpCount=NULL;
			fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");
			if ( fpCount != NULL )
			{
				//b_del = true;
				char cCount[64]={0};
				sprintf( cCount,"delcount=%d;allcount=%d;",g_delcount, g_allcount );
				fwrite( cCount, strlen(cCount), 1, fpCount);
				fclose( fpCount );
			}
			i--;
		}
	}
	if(!b_del)
		return;

	FILE *fpRecFile=NULL;
	fpRecFile = fopen("/mnt/mmc/snapshot/rec_file.txt", "w");
	if ( fpRecFile != NULL )
	{
		char inserKey[64]={0};
		int iCount = (g_allcount-g_delcount)+1;
		int iKey = g_delcount;
		int i=0;
		for ( i=0; i<iCount; i++ )
		{
			sprintf(inserKey, "%s%d",HK_KEY_VALUEN, iKey++ );
			char *cDelFile = GetParamStr( frameRec, inserKey );
			if ( cDelFile != NULL )
			{
				char buf[128]={0};
				sprintf(buf, "%s=%s;", inserKey, cDelFile );
				fwrite( buf,strlen(buf), 1, fpRecFile );
			}
		}
		fclose( fpRecFile );
	}

	FILE *fpCount=NULL;
	fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");
	if ( fpCount != NULL )
	{
		char cCount[64]={0};
		sprintf( cCount,"delcount=%d;allcount=%d;",g_delcount, g_allcount );
		fwrite( cCount, strlen(cCount), 1, fpCount);
		fclose( fpCount );
	}
#endif
}

int file_SdDeleteAllFile()
{
	FILE *fpCount=NULL;
	fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");
	if ( fpCount != NULL )
	{
		char cCount[64]={0};
		sprintf( cCount,"delcount=%d;allcount=%d;",g_delcount, g_allcount );
		fwrite( cCount, strlen(cCount), 1, fpCount);
		fclose( fpCount );
	}

	if( NULL != frameRec )
	{
		DestroyFrame( frameRec );
		frameRec = CreateFrameB();
	}
	return 1;
}

static void updateFrameData()
{
	FILE *fpRec = NULL;
	fpRec = fopen( "/mnt/mmc/snapshot/rec_file.txt", "r" );
	if ( fpRec==NULL )
	{
		//fclose(fpRec);
		frameRec = CreateFrameB();
	}

	char sdIndexData[1024]={0};
	int nFileSize = HKGetFileSize( "/mnt/mmc/snapshot/rec_file.txt" );
	char *pSDCBuf = (char *)malloc( nFileSize + 256 );
	if( pSDCBuf == NULL )
	{
		fclose(fpRec);
		printf( "SD malloc fail!\n" );
		return;
	}
	memset( pSDCBuf,0,nFileSize + 256 );
	int nRLen = 0;
	int nPos = 0;
	do
	{
		nRLen = fread( sdIndexData,1,sizeof(sdIndexData),fpRec );
		if( nRLen > 0 )
		{
			memcpy( pSDCBuf + nPos,sdIndexData,nRLen );
			nPos += nRLen;
		}
	}while( nRLen );
	*(pSDCBuf+nPos) = '\0';
	//printf( "fxb=== %s\n",pSDCBuf );
	frameRec = CreateFrameA( pSDCBuf, strlen(pSDCBuf) );
	free( pSDCBuf );
	pSDCBuf = NULL;
	fclose(fpRec);
}


int file_SdDeleteFile( const char *cFileName )
{
	FILE *fpRecFile=NULL;
	fpRecFile = fopen("/mnt/mmc/snapshot/rec_file.txt", "w");
	if ( fpRecFile != NULL )
	{
		char inserKey[64]={0};
		int iCount = (g_allcount-g_delcount);
		int iKey = g_delcount;
		int delKey=g_delcount;
		int i=0;
		for ( i=0; i<iCount; i++ )
		{
			sprintf(inserKey, "%s%d",HK_KEY_VALUEN, iKey++ );
			char *cDelFile = GetParamStr( frameRec, inserKey );
			if ( cDelFile != NULL )
			{
				sprintf(inserKey, "%s%d",HK_KEY_VALUEN, delKey++ );
				char cSdFile[128]={0};
				sprintf( cSdFile,"%s.hkv", cDelFile );
				if( strcmp(cFileName, cSdFile)!=0 )
				{
					char buf[128]={0};
					sprintf(buf, "%s=%s;", inserKey, cDelFile );
					fwrite( buf,strlen(buf), 1, fpRecFile );
				}
				else
				{
					delKey--;
					//iKey -= 2;
					g_allcount--;
					DeleteFrameKey( frameRec, inserKey );

					FILE *fpCount=NULL;
					fpCount = fopen("/mnt/mmc/snapshot/rec_count.txt", "w");
					if ( fpCount != NULL )
					{
						char cCount[64]={0};
						sprintf( cCount,"delcount=%d;allcount=%d;",g_delcount, g_allcount );
						fwrite( cCount, strlen(cCount), 1, fpCount);
						fclose( fpCount );
					}
				}
			}
		}
		fclose( fpRecFile );

		updateFrameData();
	}
	return 0;
}

static int Sd_check()
{
	struct stat st;
	if (stat("/mnt/mmc/snapshot", &st) == 0)
	{
		return 1;
	}
	return 0;
}

void SD_RSLoadObjects(RegisterFunctType reg) 
{
	memset(&file_r, 0,sizeof(file_r));
	struct stat a, b;
	stat("/mnt", &a);
	if ( ((stat("/mnt/mmc", &b) == 0) && (a.st_dev != b.st_dev)) )
	{
		file_sd_inst.SetIRQEventCallback = &SetIRQEventCallback;
		file_sd_inst.Retrieve      = &Retrieve;
		file_sd_inst.GetObjectInfo = &GetObjectInfo;
		file_sd_inst.Open          = &Open;
		file_sd_inst.Read          = &Read;
		file_sd_inst.Write         = &Write;
		file_sd_inst.DoEvent       = &DoEvent;
		file_sd_inst.Close         = &Close;
		file_sd_inst.Convert       = NULL;
		ev_irq = NULL;

		(*reg)(&file_sd_inst);

		mkdir("/mnt/mmc/snapshot", 0755);
		if( Sd_check()==1 )
		{
			//sd_initSd();
		}
		else
		{
			g_sdIsOnline = 0;
		}
	}
}

static int sccGetVideoDataSlaveTF(int pStreamType, char *pVideoData, unsigned int *nSize, unsigned int *iFream, int *iResType, int *iStreamType);

#if 0
static int GetTfDataWrite()
{
	char g_cDataBuf[200*1024]={0};
	int len = 0;
	int iFream = 0;
	int iResType = 0;
	int iStreamType = 0;

	int iflag = sccGetVideoDataSlaveTF( 0, g_cDataBuf, &len, &iFream, &iResType, &iStreamType );
	//printf("Get flag=%d,,fream=%d..len=%d.streamType=%d.\n", iflag, iFream, len, iStreamType);
	if ( (len <= 0) || (iflag==0) )
	{
		return 0;
	}

	//unsigned long long LenTemp = g_ulDataLen/1024;
	unsigned int LenTemp = g_ulDataLen/1048576;//1024*1024 //count write data size.

	if (hkSdParam.splite <= 0)
		hkSdParam.splite = 50;

	if ( LenTemp >= (hkSdParam.splite) )
	{
		if (g_fp != NULL)
		{
			fclose(g_fp);
			g_fp = NULL;
		}

		int isdOpen;
		char nm[64]={0};
		g_ulDataLen = 0;
		GetTimeChar(nm);

		isdOpen = sd_open( nm );
		HK_DEBUG_PRT("LenTemp:%d, hkSdParam.splite:%d, nm:%s, isdOpen:%d\n", LenTemp, hkSdParam.splite, nm, isdOpen);
		if( isdOpen == -1 )
			return 0;
	}
	if(g_fp != NULL)
	{
		if (4 != iStreamType)
		{
			//printf("scc........audio........\n");
			g_ulDataLen += len;
			Write_( 0, g_cDataBuf, len, 0, HK_MFF_AUDIO|HK_MFF_G711, 0 );
			return len;
		}

		g_hkHeader.resolution = iResType;
		g_ulDataLen += len;
		//printf("scc.iFream=%d...streamType=%d....\n", iFream, iStreamType);
		Write_( 0, g_cDataBuf, len, iFream, HK_MFF_VIDEO|HK_MFF_MPEG4, iStreamType );
		return len;
	}
	return len;
}
#endif

static int GetTfDataWrite()
{
	//char g_cDataBuf[200*1024]={0};
	int len = 0;
	int iFream = 0;
	int iResType = 0;
	int iStreamType = 0;

	int iflag = sccGetVideoDataSlaveTF( 0, g_cDataBuf, &len, &iFream, &iResType, &iStreamType );
	//printf("Get flag=%d,,fream=%d..len=%d.streamType=%d.\n", iflag, iFream, len, iStreamType);
	if ( (len <= 0) || (iflag==0) )
	{
		return 0;
	}

	//unsigned long long LenTemp = g_ulDataLen/1024;
	unsigned int LenTemp = g_ulDataLen/1048576;//1024*1024 //count write data size.

	/**record split**/
	if (hkSdParam.splite <= 0)
		hkSdParam.splite = 50;

	if ( LenTemp >= (hkSdParam.splite) )
	{
		if (g_fp != NULL)
		{
			fclose(g_fp);
			g_fp = NULL;
		}

		int isdOpen;
		char nm[64]={0};
		g_ulDataLen = 0;
		GetTimeChar(nm);

		isdOpen = sd_open( nm );
		HK_DEBUG_PRT("LenTemp:%d, hkSdParam.splite:%d, nm:%s, isdOpen:%d\n", LenTemp, hkSdParam.splite, nm, isdOpen);
		if( isdOpen == -1 )
			return 0;
	}
	if(g_fp != NULL)
	{
		if (4 != iStreamType)
		{
			//printf("scc........audio........\n");
			g_ulDataLen += len;
			Write_( 0, g_cDataBuf, len, 0, HK_MFF_AUDIO|HK_MFF_G711, 0 );
			return len;
		}

		g_hkHeader.resolution = iResType;
		g_ulDataLen += len;
		//printf("scc.iFream=%d...streamType=%d....\n", iFream, iStreamType);
		Write_( 0, g_cDataBuf, len, iFream, HK_MFF_VIDEO|HK_MFF_MPEG4, iStreamType );
		return len;
	}
	return len;
}



static short g_TF_flag = 0;
int TFTIME(HI_VOID)
{
	int iCount = 0;
	int iret = 0;

#if ENABLE_ONVIF
	IPCAM_setTskName("TF_Thread");  
#endif

	g_cDataBuf = (char *)malloc(200*1024*sizeof(char));;
	if (NULL == g_cDataBuf)
	{
		printf("...%s...malloc error, %s\n", __func__, strerror(errno)); 
		return 0;
	}

	pthread_detach(pthread_self()); 
	while (g_TF_flag)
	{
		if (g_TF_start == 0)//close
		{
			sleep(2);
			continue;
		}

		iret = GetTfDataWrite();
		//usleep(1000);
		usleep(10000);
		iCount++;

		if ( (iCount > 3000) && (g_sdIsOnline != 0) )  
		{
			iCount=0;
			GetStorageInfo();
			if (hkSdParam.leftSize <= HK_SD_SPLITE+600)//MB
			{
				if( hkSdParam.loopWrite == 1 )
				{
					sd_deleteRec();
				}
			}
		}
	}

	if (g_cDataBuf) 
	{
		free(g_cDataBuf);
		g_cDataBuf = NULL;
	}
	return 1;
}

static int CreateTFThread(void)
{
	if (0 == g_TF_flag)
	{
		int ret = -1;
		pthread_t tfid;
		g_TF_flag = 1;

		ret = pthread_create(&tfid, NULL, (void *)TFTIME, NULL);
		if (0 != ret)
		{
			HK_DEBUG_PRT("create TF record thread failed!\n");
			return -1;
		}
	}
	return 1;
}

static int sd_video_start()
{
	char nm[64] = {0};
	int result = TFsccInitVideoData(0);
	if (-1 == result)
	{
		HK_DEBUG_PRT("......TFsccInitVideoData failed !\n");
		return 0;
	}
	sccResetVideDataTF(0, hostVideoDataPTF);
	CreateTFThread();

	GetTimeChar(nm);
	memset(&m_fileHead, 0, sizeof(m_fileHead));
	m_fileHead.nFileHeadVer = FILE_HEAD_VER;
	memcpy(m_fileHead.cFileMark, FILE_HEADER_MASK, 3);


	int isdOpen = sd_open( nm );//time
	if (isdOpen == -1)
		return 0;

	g_hkHeader.fragx = 0;
	g_hkHeader.flipex = 0;
	g_hkHeader.resolution = 0;
	g_hkHeader.encode = 4;
	g_hkHeader.flip = 0;
	g_hkHeader.version = 2;

	//start video thread
	sccStartVideoThread();
}

void sd_record_stop()
{
	printf(".........sd_record_stop...\n");
	if (g_TF_start == 0)
		return;

	g_TF_start = 0;
	sleep(1);
	g_ulDataLen = 0;
	if( NULL != g_fp )
	{
		fclose(g_fp);
		g_fp = NULL;
	}
	return;

	//RSCHangup(HK_AS_MONS, "MotionAlarmSDRecord" );
	//if(hkSdParam.audio==1)
	//{
	//    RSCHangup( HK_AS_MONS, "MotionAlarmSDAudio" );
	//}
}

int sd_record_start()
{
	printf("[%s, %d]..........sd_record_start, g_TF_Start:%d... \n", __func__, __LINE__, g_TF_start);
	if (1 == g_TF_start)
	{
		HK_DEBUG_PRT("......SD card is already in recording thread !\n");
		return 1;
	}

	sd_video_start();
	g_TF_start = 1;

	return 0;

	/*
	   int result = -1;
	   char a[256] = {0}, nm[64] = {0};

	   if( ((hkSdParam.splite <= 0) || (hkSdParam.leftSize <= HK_SD_SPLITE)) && (hkSdParam.loopWrite == 0) )
	   return -1;

	   result = TFsccInitVideoData(0);
	   if (-1 == result)
	   {
	   HK_DEBUG_PRT("......TFsccInitVideoData failed !\n");
	   return 0;
	   }

	   sccResetVideDataTF(0, hostVideoDataPTF);

	   GetTimeChar(nm);
	   int iSdrcqc = conf_get_int(HOME_DIR"/sdvideo.conf", HK_KEY_SDRCQC);
	   if( iSdrcqc == 1 )
	   {
	   snprintf(a,sizeof(a), "ftN0=video.vbVideo.MPEG4;ftN1=File.SDStorage.in;opN1=%s;", nm);
	   }
	   else
	   {
	   snprintf(a,sizeof(a), "ftN0=video.vbVideo.M_JPEG;ftN1=File.SDStorage.in;opN1=%s;", nm);
	   }

	   result = ExecDial(HK_AS_MONS, "MotionAlarmSDRecord", a);
	   if( result >= 0 && hkSdParam.audio==1 )
	   {
	   char cAudio[256]={0};
	   snprintf(cAudio,sizeof(cAudio), "ftN0=audio.vbAudio.Out;ftN1=File.SDStorage.in;opN1=%s;", "audioin");
	   result = ExecDial( HK_AS_MONS, "MotionAlarmSDAudio", cAudio);
	   }

	   return 1;
	 */
}

int sccPushTfData(int pStreamType, char *pData, unsigned int nSize, short iFrame, int iResType, int iStreamType )
{
	if ( g_TF_start == 0)
		return 0;

	if ((hkSdParam.audio != 1) && (iStreamType == 0))
		return 0;

	pthread_mutex_lock( &g_MutexLock_TF );
	sccPushVideoData_TF( pStreamType, pData, nSize, iFrame, iResType, iStreamType, hostVideoDataPTF );
	pthread_mutex_unlock( &g_MutexLock_TF );

	return 0;
}


static int sccPushVideoData_TF(int pStreamType,char *pData,unsigned int nSize,short IFrame,int iResType,int iStreamType,VideoDataRecord  *mVideoDataBuffer)
{
	//printf("...nSize: %d...\n", nSize);

	if(mVideoDataBuffer == NULL || nSize <= 0)
		return 0;

	//fprintf(stderr,"nSize = %d\r\n",nSize);
	if ( (mVideoDataBuffer->g_allDataSize + nSize) > (pStreamType ? MAX_VIDEODATA_SLAVETREAM : MAX_VIDEODATA_HOSTSTREAM) \
			|| (nSize > 199600) \
			|| (abs(mVideoDataBuffer->g_writeIndex - mVideoDataBuffer->g_readIndex) > (POOLSIZE-1)) )
	{
		mVideoDataBuffer->g_bLost = 1;
		fprintf(stderr, "SD.......again again \r\n");
		usleep(100);
		return 0;
	}

	if( IFrame < 0 )
	{
		mVideoDataBuffer->g_bLost = true;
		printf("SD..no I P Frame**********************\n");
		return 0;
	}

	if( mVideoDataBuffer->g_bLost && IFrame != 1 )
	{
		printf( "sd..lost--ccccccc\n" );
		return 0;
	}


	if( mVideoDataBuffer->g_writeIndex == POOLSIZE ) mVideoDataBuffer->g_writeIndex = 0;

	mVideoDataBuffer->g_bLost = 0;
	if( mVideoDataBuffer->g_bAgain )
	{
		if( mVideoDataBuffer->g_CurPos + nSize < mVideoDataBuffer->g_readPos )
		{
			memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_CurPos ,pData,nSize );
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos = mVideoDataBuffer->g_CurPos;
			//printf( "again[%d][%d-%d]\n",g_writeIndex,g_CurPos,nSize );
			mVideoDataBuffer->g_CurPos += nSize;
			mVideoDataBuffer->g_allDataSize += nSize;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
			mVideoDataBuffer->g_writeIndex ++ ;
			mVideoDataBuffer->g_haveFrameCnt++;
		}
		else
		{
			mVideoDataBuffer->g_bLost = 1;
			printf( "lost--a\n" );
			usleep(10);
			return 0;
		}
	}
	else
	{
		if( mVideoDataBuffer->g_writePos + nSize < (pStreamType?MAX_VIDEODATA_SLAVETREAM:MAX_VIDEODATA_HOSTSTREAM) )
		{
			//printf( "push1[%d][%d-%d]\n",g_writeIndex,g_writePos,nSize );
			memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_writePos,pData,nSize );
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = mVideoDataBuffer->g_writePos;
			mVideoDataBuffer->g_allDataSize += nSize;
			mVideoDataBuffer->g_writePos += nSize;
			mVideoDataBuffer->g_CurPos += nSize;//mVideoDataBuffer->g_writePos;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
			mVideoDataBuffer->g_writeIndex ++ ;
			mVideoDataBuffer->g_haveFrameCnt ++;
		}
		else if( nSize < mVideoDataBuffer->g_readPos )
		{
			mVideoDataBuffer->g_CurPos = 0;
			mVideoDataBuffer->g_bAgain = 1;
			memcpy( mVideoDataBuffer->g_videoBuf,pData,nSize );
			//printf( "push2[%d][%d-%d]\n",g_writeIndex,g_writePos,nSize );
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = 0;
			mVideoDataBuffer->g_CurPos += nSize;
			mVideoDataBuffer->g_allDataSize += nSize;
			mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
			mVideoDataBuffer->g_writeIndex ++ ;
			mVideoDataBuffer->g_haveFrameCnt ++;
		}
		else
		{
			mVideoDataBuffer->g_bLost = 1;
			printf( "SD...lost--b\n" );
			usleep(10);
			return 0;
		}
	}

	if( mVideoDataBuffer->g_haveFrameCnt >= POOLSIZE )
	{
		mVideoDataBuffer->g_bLost = 1;
		printf("SD.....lost--dd\n");
		usleep(10);
	}
	return 0;
}


static int sccGetVideoData_TF(int pStreamType, char *pVideoData, unsigned int *nSize, unsigned int *iFream, int *iResType, int *iStreamType, VideoDataRecord *mVideoDataBuffer);

static int sccGetVideoDataSlaveTF(int pStreamType, char *pVideoData, unsigned int *nSize, unsigned int *iFream, int *iResType, int *iStreamType)
{
	int ret = 0;
	if((pStreamType == 0) && (hostVideoDataPTF != NULL))
	{
		pthread_mutex_lock( &g_MutexLock_TF );
		ret = sccGetVideoData_TF( pStreamType, pVideoData, nSize, iFream, iResType, iStreamType, hostVideoDataPTF );
		pthread_mutex_unlock( &g_MutexLock_TF );
	}
	return ret;
}

static int sccGetVideoData_TF(int pStreamType, char *pVideoData, unsigned int *nSize, unsigned int *iFream, int *iResType, int *iStreamType, VideoDataRecord *mVideoDataBuffer)
{
	*nSize = 0;
	if(mVideoDataBuffer->g_haveFrameCnt < 3 )
	{
		return 0;
	}

	if ((mVideoDataBuffer->g_allDataSize == 0) || (mVideoDataBuffer->g_haveFrameCnt == 0) || (mVideoDataBuffer->g_readIndex == mVideoDataBuffer->g_writeIndex))
	{
		mVideoDataBuffer->g_bAgain = 0;
		mVideoDataBuffer->g_writePos = 0;
		mVideoDataBuffer->g_readPos = 0;
		mVideoDataBuffer->g_CurPos = 0;
		mVideoDataBuffer->g_allDataSize= 0;
		mVideoDataBuffer->g_haveFrameCnt = 0;
		mVideoDataBuffer->g_writeIndex = 0;
		mVideoDataBuffer->g_readIndex = 0;
		//fprintf(stderr,"====so=====GET 1 NO DATA \r\n");

		return 0;
	}

	if( mVideoDataBuffer->g_readIndex == POOLSIZE ) mVideoDataBuffer->g_readIndex = 0;

	if (mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize == 0)
	{
		printf( "SD.......have some err!\n" );
		//mVideoDataBuffer->g_readIndex ++;
		return 0;
	}

	memcpy( pVideoData, mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos, mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize  );
	//*pVideoData =   mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
	*nSize = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize;
	*iFream = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame;
	*iResType =mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc; 
	*iStreamType = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType;
	//if(mVideoDataBuffer->g_readPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
	if(mVideoDataBuffer->g_bAgain == 1 && mVideoDataBuffer->g_CurPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
	{
		mVideoDataBuffer->g_writePos = mVideoDataBuffer->g_CurPos;
		mVideoDataBuffer->g_bAgain = 0;
	}

	mVideoDataBuffer->g_haveFrameCnt --;
	mVideoDataBuffer->g_allDataSize -= mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize ;
	mVideoDataBuffer->g_readPos = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
	mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize = 0;
	mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos = 0;
	mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame = 0;
	mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc = 0;
	mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType = 0;

	mVideoDataBuffer->g_readIndex ++ ;

	return *nSize;
}

static void sccResetVideDataTF(int pStreamType, VideoDataRecord *mVideoDataBuffer)
{
	HK_DEBUG_PRT("...... sd reset video data, pStreamType: %d ......\n", pStreamType);
	pthread_mutex_lock( &g_MutexLock_TF );

	mVideoDataBuffer->g_writePos     = 0;
	mVideoDataBuffer->g_readPos      = 0;
	mVideoDataBuffer->g_allDataSize  = 0;
	mVideoDataBuffer->g_bLost        = 0;
	mVideoDataBuffer->g_bAgain       = 0;
	mVideoDataBuffer->g_CurPos       = 0;
	mVideoDataBuffer->g_readIndex    = 0;
	mVideoDataBuffer->g_writeIndex   = 0;
	mVideoDataBuffer->g_haveFrameCnt = 0;
	memset(mVideoDataBuffer->g_videoBuf, 0, sizeof(mVideoDataBuffer->g_videoBuf));

	pthread_mutex_unlock( &g_MutexLock_TF );
}

