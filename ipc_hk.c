#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <ctype.h>

#include "utils/HKUtilAPI.h"

#include "ipc_hk.h"
#include <sys/vfs.h>
#include "hi_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "sample_comm.h"
#include "osd_region.h"
#include "ptz.h"
#include "osd_region.h"
#include "ipc_vbVideo.h"
#include "ipc_vbAudio.h"
#include "gpiodriver.h"
#include "Wdt_hi3518.h"
#include "HISI_VDA.h"

/*add by qjq*/
//#include "log.h"
#include "ipc_param.h"
#include "zlog.h"
//#include "smartconfig.h"
/*add by biaobiao*/
#include "ipc_alias.h"
#if ENABLE_P2P
#include "P2Pserver.h"
#include "BaseType.h"
#include "record.h"
#include "wifi_conf.h"
#include "gpio_dect.h"
extern INT32 p2p_server_f(); 
#include "net_http.h"
#include "ipc_sd.h"
#endif

//存储的锁 add by biaobiao
pthread_mutex_t record_mutex ;

#if ENABLE_QQ
#include "qq_server.h"
#endif

#if ENABLE_ONVIF
#include "IPCAM_Export.h"
#endif
/********************* Video ************************/
#define VIDEVID     0
#define VICHNID     0
#define VOCHNID     0
#define VENCCHNID   0
#define SNAPCHN     1
/*用户ID*/
char g_userid[50] = "17727532515";
/*视频模式*/
VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
/*视频的属性*/
extern struct HKVProperty video_properties_;
/***************** GPIO params ******************/
#define GPIO_READ  0
#define GPIO_WRITE 1

#if DEV_KELIV
unsigned int g_AlarmIn_grp  = 5;
unsigned int g_AlarmIn_bit  = 2; //alarm in:5_2.
unsigned int g_AlarmOut_grp = 5;
unsigned int g_AlarmOut_bit = 3; //alarm out:5_3.
#else //hk platform: 3518e.
unsigned int g_AlarmIn_grp  = 7;
unsigned int g_AlarmIn_bit  = 6; //alarm in:7_6.
unsigned int g_AlarmOut_grp = 7;
unsigned int g_AlarmOut_bit = 7; //alarm out:7_7.
unsigned int g_RUN_grp      = 5;
unsigned int g_RUN_bit      = 3; //RUN light:5_3.
#endif

/***************** Key Reset ******************/
//unsigned int g_KeyResetCount = 0;
//unsigned int g_KeyReset_grp = 5;
//unsigned int g_KeyReset_bit = 2; //GPIO:5_2 ==> reset key.


/***************** zlog ******************/
zlog_category_t *zc;

/************** Eable Print *****************/
#define PRINT_ENABLE    1
#if PRINT_ENABLE
#define HK_DEBUG_PRT(fmt...)  \
	do {                 \
		printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
		printf(fmt);     \
	}while(0)
#else
#define HK_DEBUG_PRT(fmt...)  \
	do { ; } while(0)
#endif

short g_DevPTZ = 0; //0: without PTZ; 1: PTZ device.
short g_onePtz = 0;
short g_DevIndex = 0;

int g_startCheckAlarm = 0;
int g_irOpen = 0;

enum Excode
{
	Excode_Exit = 11,
	Excode_Stop,
	Excode_Reset,
	Excode_Dead,
	Excode_Update,
	Excode_NetConfig,
	Excode_Daily_Reboot,
	Excode_Time_Change,
	Excode_MAC_Change,
	Excode_Complete_Init,
};

static pid_t watcher_pid_ = 0;

HK_SD_MSG_T hk_net_msg;
volatile int quit_ = 0;

extern void OnRestorationParam();
static const char* getEnv(const char* x, const char* defs) { return ((x = getenv(x)) ? x : defs); }
#if 0
typedef struct _HKIPAddres
{
	short bStatus;
	char  ipMode[64];//dhcp fixeip
	char  ip[64];
	char  netmask[64];
	char  gateway[64];
	char  dns1[64];
	char  dns2[64];
	char  mac[64];
}HKIPAddres;
HKIPAddres eth0Addr;
#endif

void wrap_sys_restart( )
{
	printf("...zzzzzzzzzzzzzzzzzzzzzzz reboot 222222 zzzzzzzzzzzzzzzzzzzzzz...\n");
	g_sdIsOnline=0;
	//be_present(0);
	//system( "ifconfig ra0 down" );
	//sd_record_stop();
	sleep(1);
	system("umount /mnt/mmc/");
	//usleep(50*1000);
	quit_ = 1;
	system("sync");
	system("reboot -f");
}
#if 0
static void* insert0(const char* fn, const char* line)
{
	if (fn)
	{
		FILE *f, *f2;
		char fn2[128], buf[512];
		snprintf(fn2,sizeof(fn2), "%s_%d%d", fn, (int)time(0), (int)getpid());
		if ( (f = fopen(fn, "r")) && (f2 = fopen(fn2, "w")))
		{
			fputs(line, f2);
			while (fgets(buf, sizeof(buf), f))
			  if (strcmp(line, buf) != 0)
				fputs(buf, f2);
			fclose(f);
			fclose(f2);
			remove(fn);
			rename(fn2, fn);
		}
		else if (f)
		  fclose(f);
	}
	return (void*)fn;
}

#define SPAWN(pid, cf, ...) do { pid_t p_p_=pid=getpid(); if ((pid=fork())==0){ cf(p_p_, __VA_ARGS__); exit(99); } } while (0)
#define CMD_PPAD(...) do{ char line[1024]; \
	int n = snprintf(line,sizeof(line)-2, __VA_ARGS__); \
	line[n] = '\n', line[n+1] = '\0'; \
	if (!insert0(getenv("CMDLINE"), line)) \
	system(line); \
}while(0)
#endif

#define Printf(fmt...)	\
	do{\
		printf("\n [%s]: %d ", __FUNCTION__, __LINE__);\
		printf(fmt);\
	} while(0)

static void Daemonize( void )
{
	pid_t pid = fork();
	if( pid )
	{
		printf("\n [%s]: %d ", __FUNCTION__, __LINE__);
		exit( 0 );
	}
	else if( pid< 0 )
	{
		printf("\n [%s]: %d ", __FUNCTION__, __LINE__);
		exit( 1 );
	}
	setsid();
	umask(0);
	signal(SIGHUP, SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN); 
}

static void create_detached_thread(void* (*func)(void*), void* arg)
{
	pthread_t tid;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tid, &a, func, arg);
	pthread_attr_destroy(&a);
}

/************************************************************
 * func: disable specified VENC channel.
 ***********************************************************/
HI_S32 Video_DisableVencChn(VENC_CHN venchn)
{
	HK_DEBUG_PRT("==> disable venc chn:%d \n", venchn);
	HI_S32 s32Ret = -1;

	s32Ret = HI_MPI_VENC_StopRecvPic(venchn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StopRecvPic err 0x%x\n", s32Ret);
		return HI_FAILURE;
	}

#if 0  //there is no need to call bellow interface.
	s32Ret = HI_MPI_VENC_UnRegisterChn(venchn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
#endif

	return HI_SUCCESS;
}

/**************************************************************************
 * func: enable specified VENC channel for resolution exchange.
 *************************************************************************/
HI_S32 Video_EnableVencChn(VENC_CHN venchn)
{
	HK_DEBUG_PRT("==> enable venc chn:%d \n", venchn);
	HI_S32 s32Ret = -1;

	s32Ret = HI_MPI_VENC_StartRecvPic(venchn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleEnableVencChn( VENC_GRP vencgroup, VENC_CHN venchn)
{
	HI_S32 s32Ret;
	HK_DEBUG_PRT("venc group:%d, venc chn:%d \n", vencgroup, venchn);

#if 0  //there is no need to call bellow interface.
	s32Ret = HI_MPI_VENC_RegisterChn(vencgroup, venchn);
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
#endif

	s32Ret = HI_MPI_VENC_StartRecvPic(venchn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


/*******************************************************************************
 * func: init media processing platform for 4-way H264 stream 
 *       for preview, and 1-way JPEG for snapshot. (2014-04-22)
 ******************************************************************************/
HI_S32 Video_SubSystem_Init(HI_VOID)
{
	HK_DEBUG_PRT("++++++ hk platform: hi3518E ++++++\n");
	PAYLOAD_TYPE_E enPayLoad[3] = {PT_H264, PT_H264, PT_JPEG};
	PIC_SIZE_E enSize[3]        = {PIC_HD720, PIC_Q720, PIC_VGA};

	HK_DEBUG_PRT("++++++ SENSOR_TYPE: %d ++++++\n", SENSOR_TYPE);
	if (APTINA_AR0130_DC_720P_30FPS == SENSOR_TYPE) //ar0130 ==>960P; ov9712/0712d ==>720P.
	{
		enSize[0] = PIC_HD960;
		HK_DEBUG_PRT("......AR0130: 960P ......\n");
	}
	else if (OMNI_OV9712_DC_720P_30FPS == SENSOR_TYPE)
	{
		enSize[0] = PIC_HD720;
		HK_DEBUG_PRT("......OV9712: 720P ......\n");
	}

	VB_CONF_S stVbConf;                 //1: video buffer.
	SAMPLE_VI_CONFIG_S stViConfig;      //2: video input.

	VPSS_GRP VpssGrp;                   //3: video process sub system.
	VPSS_CHN VpssChn;
	VPSS_GRP_ATTR_S stVpssGrpAttr;
	VPSS_CHN_ATTR_S stVpssChnAttr;
	VPSS_CHN_MODE_S stVpssChnMode;
	VPSS_EXT_CHN_ATTR_S stVpssExtChnAttr;

	VENC_GRP VencGrp;                   //4: video encode.
	VENC_CHN VencChn;

	//SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
	SAMPLE_RC_E enRcMode = SAMPLE_RC_VBR;

	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32BlkSize;
	SIZE_S stSize;

	/******************************************
	  step  1: init sys variable 
	 ******************************************/
	memset(&stVbConf, 0, sizeof(VB_CONF_S));
	stVbConf.u32MaxPoolCnt = 64;

	/*** video buffer ***/   
	/*HD720P*/
	u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt  = 6; //10;

	/*VGA*/
	u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	stVbConf.astCommPool[1].u32BlkCnt  = 1;

	/*VGA for JPEG*/
	u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
	stVbConf.astCommPool[2].u32BlkCnt  = 1;//6;

	/* hist buf*/
	stVbConf.astCommPool[3].u32BlkSize = (196*4);
	stVbConf.astCommPool[3].u32BlkCnt  = 1;//6;

	/******************************************
	  step 2: mpp system init. 
	 ******************************************/
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("system init failed with %d!\n", s32Ret);
		SAMPLE_COMM_SYS_Exit();
		return -1;
	}

	/******************************************
	  step 3: start vi dev & chn to capture
	 ******************************************/
	stViConfig.enViMode   = SENSOR_TYPE;
	stViConfig.enRotate   = ROTATE_NONE;
	stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
	stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
	s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vi failed!\n");
		SAMPLE_COMM_VI_StopVi(&stViConfig);
		return -1;
	}

	/******************************************
	  step 4: start vpss and vi bind vpss
	 ******************************************/
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		SAMPLE_COMM_VI_StopVi(&stViConfig);
		return -1;
	}

	VpssGrp = 0;
	stVpssGrpAttr.u32MaxW   = stSize.u32Width;
	stVpssGrpAttr.u32MaxH   = stSize.u32Height;
	stVpssGrpAttr.bDrEn     = HI_FALSE;
	stVpssGrpAttr.bDbEn     = HI_FALSE;
	stVpssGrpAttr.bIeEn     = HI_TRUE;
	stVpssGrpAttr.bNrEn     = HI_TRUE;
	stVpssGrpAttr.bHistEn   = HI_TRUE;
	stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
	stVpssGrpAttr.enPixFmt  = SAMPLE_PIXEL_FORMAT;
	s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Vpss failed!\n");
		SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
		return -1;
	}

	s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Vi bind Vpss failed!\n");
		SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
		return -1;
	}

	VpssChn = 0; //PHY channel. (HD960P or HD720P for Main stream)
	memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	stVpssChnAttr.bFrameEn = HI_FALSE;
	stVpssChnAttr.bSpEn    = HI_TRUE;    
	s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, HI_NULL, HI_NULL); //VPSS: group 0 channel 0.
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Enable vpss chn failed!\n");
		SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		return -1;
	}

	VpssChn = 1; //PHY channel. (VGA for Main stream)
	stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
	//stVpssChnMode.enChnMode     = VPSS_CHN_MODE_AUTO;
	stVpssChnMode.bDouble       = HI_FALSE;
	stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	stVpssChnMode.u32Width      = 640;
	stVpssChnMode.u32Height     = 360;
	s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL); //VPSS: group 0 channel 1.
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Enable vpss chn failed!\n");
		SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		return -1;
	}

	VpssChn = 3; //EXT channel 3: JPEG for snap.
	stVpssExtChnAttr.s32BindChn      = 1;
	stVpssExtChnAttr.s32SrcFrameRate = 25;
	stVpssExtChnAttr.s32DstFrameRate = 3;
	stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	stVpssExtChnAttr.u32Width        = 640;
	stVpssExtChnAttr.u32Height       = 480;
	s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, &stVpssChnMode, &stVpssExtChnAttr); //VPSS: group 0 channel X(3 or 5).
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Enable vpss chn failed!\n");
		SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		return -1;
	}

	/******************************************
	  step 5: start stream venc
	 ******************************************/
	/*** HD960P or HD720P **/
	VpssGrp = 0;
	VpssChn = 0;
	VencGrp = 0;
	VencChn = 0;
	s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[0], gs_enNorm, enSize[0], enRcMode);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
		return -1;
	}

	s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VpssChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
		return -1;
	}
	HK_DEBUG_PRT("Main stream, venc channel: 0 ==> 720P(1280*720).\n");

	/*** VGA **/
	VpssChn = 1;
	VencGrp = 1;
	VencChn = 1;
	s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[1], gs_enNorm, enSize[1], enRcMode);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
		return -1;
	}

	s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
		return -1;
	}
	HK_DEBUG_PRT("Sub stream, venc channel: 1 ==> VGA(640*480).\n");

	VpssChn = 3; /*platform: 3518e*/
	VencGrp = 2;
	VencChn = 2;
	s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[2], gs_enNorm, enSize[2], enRcMode);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
		return -1;
	}

	s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
		return -1;
	}
	HK_DEBUG_PRT("JPEG stream for SnapShort, venc chn: 2 ==> VGA(640*480).\n");

	/**overlay region init**/
	VOSD_Init();

	/**isp 3A init**/
	ISP_Params_Init();

	return s32Ret; 
}


static void setpidfile(const char* pidfile, pid_t pid)
{
	if (pidfile)
	{
		FILE* f = fopen(pidfile, "w");
		if (f)
		{
			fprintf(f, "%d", getpid());
			fclose(f);
		}
	}
}
#if 0
static void childproc(pid_t ppid, char* cmdline)
{
	int x;
	// setsid();
	for (x = 3; x < 65535; ++x)
	  close(x);
	for (x = 3; (x=sleep(x)); )
	  ;
	for (x = 16; ppid == getppid() && x > 0; --x)
	{
		//printf("ppid=%d still running\n", ppid);
		sleep(1);
	}
	if (ppid == getppid())
	{
		kill(ppid, SIGKILL);
		sleep(1);
		if (ppid == getppid())
		  perror("xxx ppid dead-locked xxx"); // CMD_PPAD("killall -9 hkipc && sleep 2");
	}
	if (cmdline)
	{
		char sh[] = "sh";
		char* args[] = { sh, cmdline, NULL };
		execve("/bin/sh", args, environ);
		perror("childproc:execve");
	}
	execl("/bin/pwd", "pwd", NULL);
}
static void exiting_progress()
{
	FILE* cf;
	char* cmdline = getenv("CMDLINE");
	if (cmdline && (cf = fopen(cmdline, "r")))
	{
		pid_t pid;
		const char* w;
		fclose(cf);
		if ((w = getenv("WATCH")))
		  remove(w);
		//HKLOG(L_DBG,"<><><><><><><><><><><>\n");
		fflush(stdout);
		SPAWN(pid, childproc, cmdline); // sleep(3);
		if (pid > 0)
		  setpidfile(getenv("PIDFILE"), pid);
		CMD_PPAD("[ $$ -ne %d ] && exit 0", (int)pid); //CMD_PPAD("echo $0: $@");
	}
}
#endif

/*******************************************************
 * func: get configuration params & init HKEMAIL_T.
 ******************************************************/
#if 0
static void GetAlarmEmailInfo()
{
	char sendEmail[128] = {0};
	conf_get( HOME_DIR"/emalarm.conf", HK_KEY_ALERT_EMAIL, sendEmail, 128);

	char recvEmail[128] = {0};
	conf_get( HOME_DIR"/emalarm.conf", HK_KEY_REEMAIL, recvEmail, 128);

	char smtpServer[128] = {0};
	conf_get( HOME_DIR"/emalarm.conf", HK_KEY_SMTPSERVER, smtpServer, 128);

	char smtpUser[128] = {0};
	conf_get( HOME_DIR"/emalarm.conf", HK_KEY_SMTPUSER, smtpUser, 128);

	char smtpPswd[128] = {0};
	conf_get( HOME_DIR"/emalarm.conf", HK_KEY_PASSWORD, smtpPswd, 128);

	int isOpen  = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_ISALERT_SENSE);
	int iPort   = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_PORT);
	int iCount  = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_COUNT);
	int secType = conf_get_int(HOME_DIR"/emalarm.conf", HK_WIFI_ENCRYTYPE );

	HK_DEBUG_PRT("......isOpen:%d, smtpServer:%s, sendEmail:%s, recvEmail:%s, smtpUser:%s, smtpPswd:%s, iPort:%d, iCount:%d, secType:%d......\n", isOpen, smtpServer, sendEmail, recvEmail, smtpUser, smtpPswd, iPort, iCount, secType);

	InitEmailInfo(isOpen, smtpServer, sendEmail, recvEmail, smtpUser, smtpPswd, iPort, iCount, secType);
}
/*
 *获取SD卡的参数信息
 */
void GetSdAlarmParam()
{
	hkSdParam.moveRec      = conf_get_int( HOME_DIR"/sdvideo.conf", SD_REC_MOVE );
	hkSdParam.outMoveRec   = conf_get_int( HOME_DIR"/sdvideo.conf", SD_REC_OUT );
	hkSdParam.autoRec      = conf_get_int( HOME_DIR"/sdvideo.conf", SD_REC_AUTO );
	hkSdParam.loopWrite    = conf_get_int( HOME_DIR"/sdvideo.conf", SD_REC_LOOP );
	hkSdParam.splite       = conf_get_int( HOME_DIR"/sdvideo.conf", SD_REC_SPLIT );
	hkSdParam.audio        = conf_get_int( HOME_DIR"/sdvideo.conf", SD_REC_AUDIO );
	hkSdParam.sdrecqc      = conf_get_int( HOME_DIR"/sdvideo.conf", HK_KEY_SDRCQC ); 

	hkSdParam.sdIoOpen     = conf_get_int( HOME_DIR"/emalarm.conf", HK_KEY_IOIN );
	hkSdParam.sdError      = conf_get_int( HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_SD_ERROR );
	//hkSdParam.sdMoveOpen   = conf_get_int( HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM );
	hkSdParam.sdMoveOpen   = 1;

	HK_DEBUG_PRT("---> moveRec:%d, outMoveRec:%d, autoRec:%d, loopWrite:%d, splite:%d, audio:%d, sdrecqc:%d, sdIoOpen:%d, sdError:%d, sdMoveOpen:%d <---\n", hkSdParam.moveRec, hkSdParam.outMoveRec, hkSdParam.autoRec, hkSdParam.loopWrite, hkSdParam.splite, hkSdParam.audio, hkSdParam.sdrecqc, hkSdParam.sdIoOpen, hkSdParam.sdError, hkSdParam.sdMoveOpen);
}
#endif


#if 0
/*******************************************************************
 * func: check ftp configuration & enable FTP bakup for SD data.
 ******************************************************************/
void hk_start_ftp_server()
{
	char ftpDevid[64] = {0};
	char sdFilePatch[64] = {0};
	char sdConf[64] = {0};

	conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_FROM, ftpDevid, sizeof(ftpDevid) );
	if (NULL != user_)
	{
		if (strcmp(user_, ftpDevid) != 0)
		  conf_set( HOME_DIR"/ftpbakup.conf", HK_KEY_FROM, user_ );
	}

	strcpy(sdFilePatch, "/mnt/mmc/snapshot");
	conf_get(HOME_DIR"/ftpbakup.conf", HK_KEY_FROM, sdConf, sizeof(sdConf));
	if (strcmp(sdFilePatch, sdConf) != 0)
	{
		conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_REC_FILE, sdFilePatch);
	}

	system("/mnt/sif/bin/ftp_server &");
	g_nFtpIsOpen = 1;
}
#endif


/*
   static void SdWrite()
   {
   int iGpioOpen = HI_SetGpio_Open();
   if( iGpioOpen == 0 )
   {
   int ulBitValue=1;
   Hi_SetGpio_GetBit( 7, 5, &ulBitValue );
//printf("...value-SD=%d...\n", ulBitValue );

Hi_SetGpio_SetBit( 7, 5, 0 );
}
Hi_SetGpio_Close();
}
*/

/*
   int sdISOnline()
   {
   int iGpioOpen = HI_SetGpio_Open();
   if( iGpioOpen == 0 )
   {
   int ulBitValue=1;
   Hi_SetGpio_GetBit( 7, 6, &ulBitValue );
   if( ulBitValue==0 )
   {
   Hi_SetGpio_Close();
   return 1;
   }
   }
   Hi_SetGpio_Close();
   return 0;
   }
   */



/***********************************************
 * func: Pull down GPIO group_bit register.
 **********************************************/
static void initGPIO()
{
	unsigned int val_read = 0;
	unsigned int val_set = 0; //pull down.
	unsigned int groupnum = 0;
	unsigned int bitnum = 0;
	short nRet = 0;

	/**init ircut**/ //platform: 3518e.
	groupnum = 1;
	bitnum   = 7; //GPIO:1_7 ==> IRCUT IN.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );
	HK_DEBUG_PRT("...Get GPIO %d_%d  read Value: %d...\n", groupnum, bitnum, val_read);

	groupnum = 2;
	bitnum   = 2; //GPIO:2_2 ==> IRCUT+.
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

	groupnum = 2;
	bitnum   = 4; //GPIO:2_4 ==> IRCUT-.
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);


	/**init IO alarm**/
	groupnum = 7;
	bitnum   = 6; //GPIO:7_6 ==> IO alarm In.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );
	HK_DEBUG_PRT("...Get GPIO %d_%d  read Value: %d...\n", groupnum, bitnum, val_read);
#if 0
	groupnum = 7;
	bitnum   = 7;  //GPIO:7_7 ==> IO alarm Out.
	val_set  = 0;  //pull down.
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
#endif

	/**read Wireless Card state**/
	groupnum = 5;
	bitnum   = 1; //GPIO:5_1.
	val_read  = 0; //pull down.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );
	HK_DEBUG_PRT("....Get GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);


	/**read Reset Key state**/
	groupnum = 5;
	bitnum   = 2; //GPIO:5_2.
	val_read  = 0; //pull down.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );
	HK_DEBUG_PRT("....Get GPIO %d_%d  read Value: %d....\n", groupnum, bitnum, val_read);


	/**init Audio Out**/
	groupnum = 5;
	bitnum   = 0; //GPIO:5_0 (audio out).
	val_set  = 0; //pull down.
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);


#if (DEV_INFRARED)
	/**init NetMode light**/
	groupnum = 1;
	bitnum   = 5; //GPIO:1_5.
	val_set  = 1; //pull up, blue light.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	nRet = Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

	groupnum = 1;
	bitnum   = 6; //GPIO:1_6.
	val_set  = 0; //pull down.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	nRet = Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
#endif


	/**init RUN light**/
	groupnum = 5;
	bitnum   = 3; //GPIO:5_3.
	val_set  = 0; //pull down.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	nRet = Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

	/**init RUN light**/
	groupnum = 7;
	bitnum   = 5; //GPIO:7_5.
	val_set  = 0; //pull down.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	nRet = Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
	
	/***init PTZ gpio***/
	if (1 == g_DevPTZ) //0:device without PTZ motor; 1:PTZ device.
	{
		groupnum = 9;
		bitnum   = 0; //GPIO:9_0.
		val_set  = 0; //pull down. 
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 1; //GPIO:9_1.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 2; //GPIO:9_2.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 3; //GPIO:9_3.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 4; //GPIO:9_4.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 5; //GPIO:9_5.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 6; //GPIO:9_6.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 9;
		bitnum   = 7; //GPIO:9_7.
		val_set  = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 


		/***init PTZ Switch Limit gpio***/
		groupnum = 5;
		bitnum   = 4; //GPIO5_4: MOTOR_UP.
		val_set  = 1; //pull up.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 5;
		bitnum   = 5; //GPIO5_5: MOTOR_DOWN.
		val_set  = 1; //pull up.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 5;
		bitnum   = 6; //GPIO5_6: MOTOR_RIGHT.
		val_set  = 1; //pull up.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 

		groupnum = 5;
		bitnum   = 7; //GPIO5_7: MOTOR_LEFT.
		val_set  = 1; //pull up.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set); 
	}

	return;
}

/*************************************
 * check IrCut and change mode.
 ************************************/
static int g_IRCutStateDayCnt = 0; //count day mode state.
static int g_IRCutStateNightCnt = 0; //count night mode state.
static int g_IRCutCurState = -1; //current state mode, 0:day mode;  1:night mode.
static int hk_IrcutCtrl(int nboardtype)
{
	unsigned int val_read = 0, curState = 0;
	unsigned int val_set = 0;
	unsigned int groupnum = 0;
	unsigned int bitnum = 0;
	short nRet = 0;

	/**read ircut status**/
#if DEV_KELIV
	groupnum = 0;
	bitnum   = 0; //GPIO:0_0.
#else //hk 3518e.
	groupnum = 1;
	bitnum   = 7; //GPIO:1_7(IRCUT IN).
#endif
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );//ircut:0-2.
	//HK_DEBUG_PRT("...nboardtype:%d, Get GPIO %d_%d  read Value: %d...\n", nboardtype, groupnum, bitnum, val_read);

	if (1 == g_irOpen) //disable IRCut ctrol, stay in day mode.
	{
#if DEV_KELIV
		HISI_SetTurnColor(0, 2);//wangshaoshu
		groupnum = 0;
		bitnum   = 2; //GPIO:0_2.
#else //hk 3518e.
		HISI_SetTurnColor(0, 2);//wangshaoshu
		groupnum = 2;
		bitnum   = 4; //GPIO:2_4 (ircut-).
#endif
		val_set  = 1; //pull up.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
		usleep(1000*200);

		val_set = 0; //pull down.
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
		//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n\n", groupnum, bitnum, val_set);
	}

	if (0 == g_irOpen) //enable IRCut change status.
	{
		if (0 == nboardtype) //ircut light board type: level 0.
		{
			/**change ircut mode**/
			if (1 == val_read) //day mode.
			{
				g_IRCutStateNightCnt = 0;
				g_IRCutStateDayCnt++;

				if (2 == g_IRCutStateDayCnt) 
				{
					g_IRCutStateDayCnt = 0;
					if (0 == g_IRCutCurState)
					  return 0;

					g_IRCutCurState = 0;

#if DEV_KELIV
					HISI_SetTurnColor(0, 2);//wangshaoshu
					groupnum = 0;
					bitnum   = 1; //GPIO:0_1.
#else //hk 3518e.
					HISI_SetTurnColor(0, 2);//wangshaoshu
					groupnum = 2;
					bitnum   = 4; //GPIO:2_4 (ircut-).
#endif
					val_set  = 1; //pull up.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

					usleep(1000*200);

					val_set = 0; //pull down.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n\n", groupnum, bitnum, val_set);
				}
			}

			if (0 == val_read) //night mode.
			{
				g_IRCutStateDayCnt = 0;
				g_IRCutStateNightCnt++;

				if (2 == g_IRCutStateNightCnt)
				{
					g_IRCutStateNightCnt = 0;
					if (1 == g_IRCutCurState)
					  return 0;

					g_IRCutCurState = 1;

#if DEV_KELIV
					HISI_SetTurnColor(1, 2);//wangshaoshu
					groupnum = 0;
					bitnum   = 2; //GPIO:0_2.
#else //hk 3518e.
					HISI_SetTurnColor(1, 2);//wangshaoshu
					groupnum = 2;
					bitnum   = 2; //GPIO:2_2 (ircut+).
#endif
					val_set  = 1; //pull up.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

					usleep(1000*200);

					val_set = 0; //pull down.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n\n", groupnum, bitnum, val_set);
				}
			}
		}
		else if (1 == nboardtype) //ircut light board type: level 1.
		{
			/**change ircut mode**/
			if (0 == val_read) //day mode.
			{
				g_IRCutStateNightCnt = 0;
				g_IRCutStateDayCnt++;

				if (2 == g_IRCutStateDayCnt) 
				{
					g_IRCutStateDayCnt = 0;
					if (0 == g_IRCutCurState)
					  return 0;

					g_IRCutCurState = 0;

#if DEV_KELIV
					HISI_SetTurnColor(0, 2);//wangshaoshu
					groupnum = 0;
					bitnum   = 1; //GPIO:0_1.
#else //hk 3518e.
					HISI_SetTurnColor(0, 2);//wangshaoshu
					groupnum = 2;
					bitnum   = 4; //GPIO:2_4 (ircut-).
#endif
					val_set  = 1; //pull up.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

					usleep(1000*200);

					val_set = 0; //pull down.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n\n", groupnum, bitnum, val_set);
				}
			}

			if (1 == val_read) //night mode.
			{
				g_IRCutStateDayCnt = 0;
				g_IRCutStateNightCnt++;

				if (2 == g_IRCutStateNightCnt)
				{
					g_IRCutStateNightCnt = 0;
					if (1 == g_IRCutCurState)
					  return 0;

					g_IRCutCurState = 1;

#if DEV_KELIV
					HISI_SetTurnColor(1, 2);//wangshaoshu
					groupnum = 0;
					bitnum   = 2; //GPIO:0_2.
#else //hk 3518e.
					HISI_SetTurnColor(1, 2);//wangshaoshu
					groupnum = 2;
					bitnum   = 2; //GPIO:2_2 (ircut+).
#endif
					val_set  = 1; //pull up.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

					usleep(1000*200);

					val_set = 0; //pull down.
					Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
					Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
					//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n\n", groupnum, bitnum, val_set);
				}
			}       
		}
	}
	return nRet;
}

#define HK_SYS_TIMEOUT  60*15
static time_t  gSysTime = 0;
static short   gbStartTime = 0;

/*****************************************************************
 * func: check GPIO status & generate IO alarm notification.
 ****************************************************************/
static void CheckIOAlarm()
{
	unsigned int val_read = 0, val_set = 0;

	Hi_SetGpio_SetDir( g_AlarmIn_grp, g_AlarmIn_bit, GPIO_READ );
	Hi_SetGpio_GetBit( g_AlarmIn_grp, g_AlarmIn_bit, &val_read ); //Alarm In:0_0.
	//HK_DEBUG_PRT("......IO Alarm In: val_read = %d......\n", val_read);
	if (0 == val_read) //IO is Alarming.
	{
		AlarmVideoRecord( true ); //sd alarm record.
		//CheckAlarm(0,2,0,NULL); //2:IO Alarm.

		val_set = 1;
		Hi_SetGpio_SetDir( g_AlarmOut_grp, g_AlarmOut_bit, GPIO_WRITE );
		Hi_SetGpio_SetBit( g_AlarmOut_grp, g_AlarmOut_bit, val_set ); //pull up.
		sleep(5);
		val_set = 0;
		Hi_SetGpio_SetDir( g_AlarmOut_grp, g_AlarmOut_bit, GPIO_WRITE );
		Hi_SetGpio_SetBit( g_AlarmOut_grp, g_AlarmOut_bit, val_set ); //pull down.
	}
	else
	{
		AlarmVideoRecord( false );
	}
}


/*******************************************
 * func: check reset key gpio in state.
 * return: 
 *      1: default state with no pressed key.
 *      0: reset key pressed.
 ******************************************/
#if 0
int HK_Check_KeyReset(void)
{
	unsigned int val_read = 1;
	short nRet = 0;

	nRet = Hi_SetGpio_SetDir( g_KeyReset_grp, g_KeyReset_bit, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( g_KeyReset_grp, g_KeyReset_bit, &val_read );
	if (nRet < 0)
	{
		HK_DEBUG_PRT("Set GPIO %d_%d error!\n", g_KeyReset_grp, g_KeyReset_bit);
		g_KeyResetCount = 0; 
		return -1; 
	}
	//HK_DEBUG_PRT("...g_KeyResetCount: %d, Get GPIO %d_%d  read Value: %d...\n", g_KeyResetCount, g_KeyReset_grp, g_KeyReset_bit, val_read);

	if (0 == val_read)
	{ //reset key pressed.
		g_KeyResetCount++; 

		if (4 < g_KeyResetCount) //10s, system restart after reset to factory settings.
		{
			g_KeyResetCount = 0; 
			OnRestorationParam(); //reset to factory settings.
			return 0;
		}
	}
	else if ((1 == val_read) && (0 < g_KeyResetCount) && (g_KeyResetCount < 3))
	{ //less than 4s, system restart immediately.
		g_KeyResetCount = 0; 
		wrap_sys_restart(); 
		return 0;
	}

	return 0;
}
#endif

static void init_conf()
{
	system("touch /mnt/sif/sdvideo.conf");
	system("touch /mnt/sif/subipc.conf");
	system("touch /mnt/sif/time.conf");
	system("touch /mnt/sif/RngMD.conf");

	system("touch /mnt/sif/sidlist.conf");
	system("touch /mnt/sif/emalarm.conf");
	system("touch /mnt/sif/ftpbakup.conf");
	system("touch /mnt/sif/hkpppoe.conf");
}

//#if ENABLE_ONVIF
#if 0
extern VideoDataRecord *hostVideoDataP;//master starem //*mVideoDataBuffer;
extern VideoDataRecord *slaveVideoDataP; //slave starem
extern VideoDataRecord *slaveAudioDataP;
void HK_Onvif_Init(void)
{
	int EnableOnvif = 0;
	Context_InitSysteBuffer();

	sccInitVideoData( PSTREAUDIO );
	sccResetVideData( PSTREAUDIO, slaveAudioDataP );	
	sccInitVideoData( PSTREAMONE);	
	sccResetVideData( PSTREAMONE, hostVideoDataP );
	sccInitVideoData( PSTREAMTWO);	
	sccResetVideData( PSTREAMTWO, slaveVideoDataP );

	CreateAudioThread();
	CreateVideoThread(); 
	CreateSubVideoThread(); 

	//ONVIF
	EnableOnvif = conf_get_int("/mnt/sif/web.conf","bOnvifEnable");
	if(EnableOnvif)
	{	    	
		IPCAM_RtspInit(8554, 1);
		onvif_start(); 
	}

	return;
}
#endif

/*add by biaobiao*/
#if ENABLE_P2P
void IPC_Video_Audio_Thread_Init(void)
{
	CreateAudioThread();
	CreateVideoThread();
	CreateSubVideoThread();
}
#endif
#if 0
int CheckWifi()
{
	//检查wifi的联通性
	if(Check_WPACLI_Status(1) != 1)
	{
		system("/usr/bin/pkill wpa_supplicant");
		system("/usr/bin/pkill udhcpc");
		sleep(1);
		system("wpa_supplicant -Dwext -ira0 -c/etc/wifiConf/wpa_supplicant.conf &");
	}
}
#endif
/*wifi模式*/
int g_wifimod = 1;
int main(int argc, char* argv[])
{
	/*IRCUT的类型,调节IRCUT的灵敏度*/
	int IRCutBoardType = 0;
	/*Sensor的类型*/
	char cSensorType[32]={0};
	int counter = 0;
	char usrid[32] = {0};
	char device_id[20] = {0};
	int f_wifi_connenct = 0;
#if 1
	int rc;


	rc = zlog_init("/mnt/sif/zlog.conf");
	if (rc) {
		printf("init failed\n");
		return -1;
	}

	zc = zlog_get_category("my_cat");
	if (!zc) {
		printf("get cat fail\n");
		zlog_fini();
		return -2;
	}
/*
	ZLOG_INFO(zc, "hello, zlog");
	ZLOG_ERROR(zc, "hello, zlog");
	zlog_put_mdc("myname","qjq");
	ZLOG_WARN(zc, "hello, zlog");
	ZLOG_NOTICE(zc, "hello, zlog");
	ZLOG_FATAL(zc, "hello, zlog");
*/
#endif

	/*获取设备ID*/
	get_device_id(device_id);

#if HTTP_DEBUG
	printf("Create the device id*********************");
#endif

	init_conf(); 
	//add by biaobiao
	pthread_mutex_init(&record_mutex,NULL);
	Daemonize();
	//init_sighandler();
	/*配置sensor的类型*/
	conf_get( HOME_DIR"/sensor.conf", "sensortype", cSensorType, 32 );
	if (strcmp(cSensorType, "ar0130") == 0)
	{

		printf("...scc...ar0130......\n");
	}
	else if (strcmp(cSensorType, "ov9712d") == 0)
	{
		printf("...scc...ov9712d......\n");
	}
	else
	{
		printf("...scc...unknown sensor type, use default: ov9712d lib......\n");  
	}
#if 1
	g_DevIndex         = conf_get_int(HOME_DIR"/hkclient.conf", "IndexID"); 
	g_irOpen           = conf_get_int(HOME_DIR"/hkipc.conf", "iropen");
	g_onePtz           = conf_get_int(HOME_DIR"/hkipc.conf", "oneptz");
	g_DevPTZ           = conf_get_int("/etc/device/ptz.conf", "HKDEVPTZ");
	IRCutBoardType     = conf_get_int(HOME_DIR"/hkipc.conf", "IRCutBoardType");
	g_wifimod		   = conf_get_int(HOME_DIR"/hkipc.conf", "WIFIMODE");
	//g_LOGIN		   = conf_get_int(HOME_DIR"/hkipc.conf", "LOGIN");
#endif

	hk_load_sd();
	init_param_conf();  //主要读取厂商号和设备号,若是测试模式还将读取ip号和gateway等信息

	if(!get_isTestMode())
	{
		if(g_wifimod == 0)
		{
			/*设为ap模式*/
			set_ap_mode();
		}
		else if(g_wifimod == 1)
		{	
			/*设为sta模式*/
			set_sta_mode();
			if(connect_the_ap() == 0);
			{
				net_modify_device(device_id);
#if 0
				if(g_LOGIN == 1)
				{
					net_create_device(device_id);
					net_bind_device( g_userid, device_id );
				}
				//设别绑定
#endif
			}
		}
	}else{
		connect_ap_for_test();
	}

//	hk_set_system_time();
	video_RSLoadObjects();
	/**** init video Sub System. ****/
	if( HI_SUCCESS != Video_SubSystem_Init() )
	{
		printf("[%s, %d] video sub system init failed !\n", __func__, __LINE__); 
	}
	/**** init video VDA ****/
	if ( VDA_MotionDetect_Start() ) //enable motion detect.
	{
		HK_DEBUG_PRT("start motion detect failed !\n"); 
	}
	HK_DEBUG_PRT("video sub system init OK!\n");

	/**GPIO init**/
	HI_SetGpio_Open();
	initGPIO();

	setpidfile(getenv("PIDFILE"), getpid());
	if ( getenv("wppid") )
	{
		watcher_pid_ = atoi(getenv("wppid"));
	}
#if (HK_PLATFORM_HI3518E)
	/*****neck Cruise*****/
	   if (1 == g_DevPTZ) //0:device without PTZ motor; 1:PTZ device.
	   {
	   HK_PtzMotor();
	   }
#endif

	/*hong wai 红外*/
#if (DEV_INFRARED)
	HK_Infrared_Decode();
#endif

#if ENABLE_QQ
	printf("####################################################\n");
	initDevice();
#endif

	/*add by biaobiao*/
#if ENABLE_P2P
	printf("############################p2p###################\n");
	create_detached_thread(p2p_server_f, (void *)device_id);
	//p2p_server_f();
#endif

	/*add by biaobiao*/
#if ENABLE_P2P
	IPC_Video_Audio_Thread_Init();
#endif

	/*add by biaobiao*/
#if ENABLE_P2P
	//RECORDSDK_Start();
#endif

#if ENABLE_ONVIF
	HK_Onvif_Init();
#endif
#if 0
#if (DEV_KELIV == 0) //init m433  by yy
	int m433enable = conf_get_int("/mnt/sif/hkipc.conf", "m433enable");
	printf("...m433enable: %d...\n", m433enable);
	if (m433enable == 1)
	{
		system("insmod /mnt/sif/bin/hi_m433.ko");
		sleep(1);
		if (init_m433(getpid()) == 0)
		{
			printf("m433 insmod success\n");
		}
		else
		{
			m433enable = 0;
		}
	}
#endif //end by yy
#endif

	//初始化看门狗
	HK_WtdInit(60*2); //watchdog.

	unsigned int groupnum = 0, bitnum = 0, val_set = 0;
	unsigned int valSetRun = 0;
	int ret = 0;

	system("echo 3 > /proc/sys/vm/drop_caches");
	for ( ; !quit_; counter++)
	{
		/*ISP控制*/
		ISP_Ctrl_Sharpness();
		ret = key_scan();
		/*add by biaobiao 检测按键 若按键长按重启进入ap模式，短按则进入smartconfig模式*/
		if(ret == 0)
		{
			printf("*********smart config begin******************\n");
#if 1
			system("/usr/bin/pkill wpa_supplicant");
			system("/usr/bin/pkill udhcpc");
			set_sta_mode();
			if(smart_config( g_userid ) == 0)
			{	
				//设置为sta模式
				conf_set_int(HOME_DIR"/hkipc.conf", "WIFIMODE", 1);
				f_wifi_connenct = 1;
				system("echo 3 > /proc/sys/vm/drop_caches");
			}
			printf("*********smart config complete******************\n");
#endif
		}else if(ret == 1)
		{
			conf_set_int(HOME_DIR"/hkipc.conf", "WIFIMODE", 0);
			printf("*********recovery******************\n");
			wrap_sys_restart();
		}
		/*smart_config结束，则连接wifi*/
		if(f_wifi_connenct)
		{
#if TEMP
			conf_set_int(HOME_DIR"/hkipc.conf", "LOGIN", 1);
			system("reboot -f");
#endif
			if(connect_smt_ap() == 0)
			{
				//创建设备
				printf("the deviceid is %s\n", device_id);
				net_create_device(device_id);
				//设备绑定
				printf("the useid :%s\n",usrid);
				sleep(5);
				net_bind_device( g_userid, device_id );
				f_wifi_connenct = 0;
				printf("*********connect the ap******************\n");
				PlaySound("/mnt/sif/audio/success.pcm");
			};
			sleep(5);
		}
#if 0 
		if (b_hkSaveSd)
		{
			printf("[%s, %d] scc stop sd....\n", __func__, __LINE__);
			//sd_record_stop();
			//GetSdAlarmParam();
			b_hkSaveSd = false;
		}
		if ((1 == hkSdParam.autoRec) && (b_OpenSd) && (1 == g_sdIsOnline))
		{
			g_OpenSd++;
			//if (g_OpenSd == 30)
			if (g_OpenSd == 15)
			{
				g_OpenSd = 0;
				b_OpenSd = false;

				//if (sd_record_start() < 0)  
				{
					printf("start sd record failed !\n");
				}
			}
		}
#endif
#if 0
#if (!DEV_KELIV)
		if ( m433enable == 0 )
		{
			CheckIOAlarm();//check AlarmIn & AlarmOut.
		}
#endif
#endif
		hk_IrcutCtrl( IRCutBoardType );//check & control Ircut mode.

#if (HK_PLATFORM_HI3518E | DEV_INFRARED)
		//HK_Check_KeyReset(); //system restart or reset to factory settings.
#endif
		if (gbStartTime)
		{
			time_t nowt = time(0);
			if ((nowt - gSysTime) > HK_SYS_TIMEOUT)
			{
				fprintf(stderr, "timeout quit !!!\n");
				wrap_sys_restart();
			}
		}
#if (HK_PLATFORM_HI3518E)
		/**RUN light**/
		valSetRun = 1;
		Hi_SetGpio_SetDir( g_RUN_grp, g_RUN_bit, GPIO_WRITE );
		Hi_SetGpio_SetBit( g_RUN_grp, g_RUN_bit, valSetRun ); //pull up.
		sleep(1);
		valSetRun = 0;
		Hi_SetGpio_SetDir( g_RUN_grp, g_RUN_bit, GPIO_WRITE );
		Hi_SetGpio_SetBit( g_RUN_grp, g_RUN_bit, valSetRun ); //pull down.
		sleep(1);
#else
		sleep(2);
#endif

#if (HK_PLATFORM_HI3518E)
		if (1 == g_DevPTZ) //0:device without PTZ motor; 1:PTZ device.
		{
			HK_PTZ_AutoRotate_Stop( 9 );
		}
#endif
		if (g_startCheckAlarm <= 4)
		{
			//printf("...startcheckalarm=%d.....\n",g_startCheckAlarm);
			//g_startCheckAlarm++;
		}
		/*挂载sd卡*/
		hk_load_sd();
	}
	//sd_record_stop();
	gSysTime = time(0);
	gbStartTime = 1;
	//if (quit_ != Excode_Stop)
	//exiting_progress();
	printf("\n [%s]: %d ", __FUNCTION__, __LINE__);
	exit (0);
}
