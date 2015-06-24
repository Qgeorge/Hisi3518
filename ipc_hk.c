#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/prctl.h>


#include "utils/HKMonCmdDefine.h"
#include "utils/HKCmdPacket.h"
#include "utils/HKUtilAPI.h"
#include "sys.h"

#include "ipc_hk.h"
#include <sys/vfs.h>
#include "hi_common.h"
#include "utils/HKLog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "sample_comm.h"
#include "m433.h"
#include "osd_region.h"
#include "ptz.h"

/*add by biaobiao*/
#if ENABLE_P2P
#include "P2Pserver.h"
#include "BaseType.h"
extern INT32 p2p_server_f(); 
#endif

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

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
extern struct HKVProperty video_properties_;

int g_HK_SensorType = 0; //sensor type: 1(ar0130); 2(ov9712d).

/*
 * video resolution type: 0(960P); 1(720P).
 * only valid for ar0130 sensor.
 */
int g_HK_VideoResoType = 0;

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


/*** Key Reset ***/
unsigned int g_KeyResetCount = 0;
unsigned int g_KeyReset_grp = 5;
unsigned int g_KeyReset_bit = 2; //GPIO:5_2 ==> reset key.


/************* SD Card *************/
short g_sdIsOnline = 0;
bool b_hkSaveSd = false;
short g_nFtpIsOpen = 0;
HK_SD_PARAM_ hkSdParam;

/**************audio   alarm*****************/
int audio_alarm = 0;

/***************** WIFI Params *****************/
#define PATH_PUBLIC      "/mnt/sif/bin"
#define WIFIPASS_LEN     50
#define BSSID_LEN        20
#define CONF_WIFILIST    "/mnt/sif/wifilist.conf"
#define IPADDR_APMODE    "192.168.100.2"

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

char authMode[4][8]={"none","wep","wpa","wpa2"};
char encType[4][8]={"none","","TKIP","AES"};
static psk_flag = 0;

#if 0
typedef struct hk_remote_wffi_info
{
	int  iSignal;
	//int frequency;
	char nettype;
	char enctype;
	char authmode;
	char ssid[SSID_LEN];
	char bssid[BSSID_LEN];
}REMOTE_INFO;

typedef struct hk_remote_wifi_find
{
	int count;
	REMOTE_INFO wifi_info[HK_WIFI_FIND_LEN];
}REMOTE_WIFI_FIND;
#endif

//add by ali
short gTWifiStatus = 0;
HI_S32 g_s32Fd2815a = -1;
HI_S32 g_s32Fd2815b = -1;
char  gInterface[32]={0};

struct ethtool_value
{
	unsigned int cmd;
	unsigned int data;
};

short g_DevPTZ = 0; //0: without PTZ; 1: PTZ device.
short g_onePtz = 0;
int  g_DevVer = 507;
short g_DevIndex = 0;
int g_startCheckAlarm = 0;
int g_irOpen = 0;

#define HK_IPC_INITIAL "IPCInitial"
#define HK_SYNC_TIME "SyncTime"
#define HK_KEY_HWADDR HK_KEY_MACIP
//#define UNDEF_MAC_ADDRESS "0:10:10:10:10:10"

static void on_resp_update_id(Dict* d);
static void on_resp_update_connect(Dict* d);
static void on_resp_reqinit1(Dict* hf);
static void on_resp_sync_time(Dict* hf);

#define LOCAL_ASC 88
static char *GetInterIP( char *iter );
static char sysLocalIP[255]={0};

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

unsigned int gResetTime = 0;
static const char* user_ = NULL;
static const char* passwd_ = "123456";
static const char* host_ = "www.uipcam.com";
static const char* port_ = "8080";
static struct timedq *tq_ = 0;
static pid_t watcher_pid_ = 0;

#define HK_SYS_TIMEOUT  60*15
static time_t  gSysTime = 0;
static short   gbStartTime = 0;
int g_isWifiInit = 0;
volatile int quit_ = 0;
void* volatile req_ = 0;
static short g_isWanEnable=0;
static int g_lanPort=0;
short b_localTime=0;
volatile int g_nSignLogin=-2;
static volatile int signcode_ = -2;

enum { Sid_Size = 12 };
enum { Sched_D_Size = 3 };

struct SchedDial
{
	const char* dias;
	char sid[Sid_Size];
	// int n_atq;
	int atq[Sched_D_Size][2];
	struct timedq *tq;
};
static struct SchedDial mpeg_ = { "ftN0=video.vbVideo.MPEG4;ftN1=File_SDAV.SDStorage.in;" };

extern int is_free_time();
extern void video_RSLoadObjects(RegisterFunctType);
extern void free_number(); //rgb1555numbers.c
extern void OnRestorationParam();

static const char* getEnv(const char* x, const char* defs) { return ((x = getenv(x)) ? x : defs); }
static const char* getPx(HKFrameHead* hf, const char* x, const char* defs) { return ((x = GetParamStr(hf,x)) ? x : defs); }


void TrimRight(char *psz);

HKIPAddres eth0Addr,wifiAddr;
WIFICfg   mywifiCfg;


/*
 *  enable wifi search, parse wifi node, 
 *  and save the scan result into wifi list file.
 */
//static int ScanWifiInfo( REMOTE_WIFI_FIND *wifi )
int ScanWifiInfo( REMOTE_WIFI_FIND *pWifi )
{
	HK_DEBUG_PRT("...Searching remote wifi node ......\n");
#if 0
	HK_DEBUG_PRT("...... search wifi node ......\n");
	int  ret = 0;
	FILE *fp = NULL;
	char buf[4096]={0}, *ptr = NULL;
	char bssid[32]={0}, ssid[32]={0}, signal_level[8]={0};
	char flags[64]={0}, security[23]={0}, mode[7]={0};
	char frequency[8]={0}; //, nettype[3]={0};

	wifi->count = 0; //counter the wifi signal searched.
	memset(buf, 0, sizeof(buf));    

	system("wpa_cli scan"); //select interface 'ra0' to search wifi signal.
	system("wpa_cli scan_result > /dev/null");
	sleep(1);
	system("wpa_cli scan"); //select interface 'ra0' to search wifi signal.
	/*open a process by creating a pipe, forking and invoking the shell*/
	if (! (fp = popen("wpa_cli scan_result", "r")) ) 
	{
		fprintf(stderr, "[%s, %d] execute 'wpa_cli scan_result' failed with:%d, %s !\n", \
				__func__, __LINE__, errno, strerror(errno));
		return errno;
	}

	/*
	 * search result as follows:
	 *
	 * Selected interface 'ra0'
	 * bssid            / frequency /signal_level / flags                                   / ssid
	 * 00:0e:e8:b3:8c:44       2462    197     [WPA-PSK-TKIP+CCMP][WPA2-PSK-TKIP+CCMP][ESS]    TOTOLINK_B38C44
	 * 28:2c:b2:5b:a0:3a       2412    195     [WPA-PSK-CCMP][WPA2-PSK-CCMP][WPS][ESS] gyr12M
	 */

	fgets(buf, sizeof(buf), fp); //first line, invalid.
	//HK_DEBUG_PRT("--> buf:%s\n", buf);
	fgets(buf, sizeof(buf), fp); //second line, invalid.
	//HK_DEBUG_PRT("--> buf:%s\n", buf);

	while( fgets(buf, sizeof(buf), fp) ) //valid data begin.
	{
		//if (strlen(buf) < 4)    break;
		if (wifi->count >= 20)   break;

		ptr = buf;
		//formated scan from a line input.
		sscanf(ptr, "%s %s %s %s %s", bssid, frequency, signal_level, flags, ssid);
		//HK_DEBUG_PRT("count:%d, bssid:%s frequency:%s signal_level:%s flags:%s ssid:%s\n", wifi->count, bssid, frequency, signal_level, flags, ssid);

		bssid[17] = '\0';
		strcpy(wifi->wifi_info[wifi->count].bssid, bssid); //wifi mac address.
		strcpy(wifi->wifi_info[wifi->count].ssid, ssid); //wifi node name.
		//wifi->wifi_info[ wifi->count ].frequency = atoi(frequency);

		/******************* signal ***************************/
		ret = atoi(signal_level) - 120;
		wifi->wifi_info[wifi->count].iSignal = (ret > 100) ? 100 : ret;

		/******************* enctype + authmode ***************************/
		//if (strstr(flags, "AES") != NULL)
		if (strstr(flags, "CCMP") != NULL) //AES.
			wifi->wifi_info[wifi->count].enctype  = 3;//1-none,2-TKIP,3-AES
		else if (strstr(flags, "TKIP") != NULL)
			wifi->wifi_info[wifi->count].enctype  = 2;//TKIP
		else
			wifi->wifi_info[wifi->count].enctype  = 1;//NONE

		if (strstr(flags, "WPA2") != NULL)
			wifi->wifi_info[wifi->count].authmode  = 3;//0-none,1-wep,2-wpa,3-wpa2
		else if (strstr(flags, "WPA") != NULL)
			wifi->wifi_info[wifi->count].authmode  = 2;   //WPA
		else if (strstr(flags, "WEP") != NULL)
			wifi->wifi_info[wifi->count].authmode  = 1;   //WEP
		else
			wifi->wifi_info[wifi->count].authmode  = 0;   //NONE

		/******************* nettype ***************************/
		//if(strstr(nettype, "In") != NULL)
		//    strcpy( wifi->wifi_info[wifi->count].nettype, "0" );  //Infra.
		//else
		//    strcpy( wifi->wifi_info[wifi->count].nettype, "1" );  //Adhoc.

		wifi->count++;  //next line.
	}

	if (fp)  pclose(fp);
#endif 

#if 1
	int ret = 0;
	int i = 0, space_start = 0;
	FILE *fp = NULL;
	char buf[4096]={0}, *ptr = NULL;
	char channel[8]={0}, ssid[32]={0}, bssid[32]={0}, security[32]={0};
	char signal[8]={0}, w_mode[8]={0}, ext_ch[8]={0}, net_type[8]={0};

	pWifi->count = 0; //counter the wifi number searched.
	memset(buf, 0, sizeof(buf));    

	system("iwpriv ra0 set SiteSurvey=1");

	if (!(fp = popen("iwpriv ra0 get_site_survey", "r"))) 
	{
		printf("popen failed with:%d, %s\n", errno, strerror(errno));
		return errno;
	}

	/*****************************************************************************************************************
	 * wifi search result are as follows:
	 * # iwpriv ra0 get_site_survey
	 * ra0       get_site_survey:
	 * Ch  SSID                             BSSID               Security               Siganl(%)W-Mode  ExtCH  NT
	 * 2   ASUS_MS001                       ac:22:0b:54:8b:b4   WPA2PSK/AES            94       11b/g/n NONE   In
	 * 4   ChinaNet-x53c                    70:a8:e3:60:aa:78   WPA1PSKWPA2PSK/TKIPAES 63       11b/g/n NONE   In
	 * 6   IPC-1941000400                   00:03:7f:00:00:27   WPA2PSK/TKIPAES        94       11b/g/n NONE   In
	 ******************************************************************************************************************/

	pWifi->count = 0;
	memset(buf, 0, sizeof(buf));
	fgets(buf, sizeof(buf), fp); //first line, invalid
	fgets(buf, sizeof(buf), fp); //second line, invalid
	int ii = 0;
	while (fgets(buf, sizeof(buf), fp) && (ii ++ < 100)) 
	{
		//if (pWifi->count >= 20)
		//    break;
		if (strlen(buf) < 4)
			break;
		//fprintf(stderr, "[%d]: %s\n", pWifi->count, buf);

		ptr = buf;
		sscanf(ptr, "%s", channel); //Ch.
		ptr += 37; //begin with "BSSID".
		sscanf(ptr, "%s %s %s %s %s %s", bssid, security, signal, w_mode, ext_ch, net_type); //seperate string.
		//strncpy(pWifi->wifi_info[pWifi->count].bssid,bssid,11); 

		ptr = buf + 4; //begin with "SSID".
		space_start = 0;
		i = 0;
		while (i < 33) 
		{
			if ((0x20 == ptr[i]) && (0 == space_start))
			{
				space_start = i;
				break;
			}
			else
			{
				space_start = 0;
				i++;
			}
		}
		//ptr[space_start] = '\0'; //ssid end
		ptr[33] = '\0'; //ssid end
		strcpy(ssid, buf+4);
		TrimRight(ssid); //remove the ' ' space.

		if (strlen(ssid) == 0 || (strlen(ssid) == 1 && ssid[0] == ' '))
		{
			fprintf(stderr, "=== space ssid ===\n");
			continue;
		}
		//HK_DEBUG_PRT("count:%d, ssid:%s, bssid:%s, security:%s, w_mode:%s, net_type:%s, iSignal:%s\n", pWifi->count, ssid, bssid, security, w_mode, net_type, signal);

		strcpy(pWifi->wifi_info[pWifi->count].ssid, ssid); 
		pWifi->wifi_info[pWifi->count].iSignal = atoi(signal);//wifi signal level.

		/**Security Type ==> 1:none; 2:TKIP; 3:AES**/
		if (NULL != strstr(security, "AES"))
			pWifi->wifi_info[pWifi->count].enctype = 3;
		else if (NULL != strstr(security, "TKIP"))
			pWifi->wifi_info[pWifi->count].enctype = 2;
		else 
			pWifi->wifi_info[pWifi->count].enctype = 1;

		/**Authentication Type ==> 0:none; 1:wep; 2:wpa; 3:wpa2**/
		if (NULL != strstr(security, "WPA2"))
			pWifi->wifi_info[pWifi->count].authmode = 3;
		else if (NULL != strstr(security, "WPA"))
			pWifi->wifi_info[pWifi->count].authmode = 2;
		else if (NULL != strstr(security, "WEP")) 
			pWifi->wifi_info[pWifi->count].authmode = 1;
		else
			pWifi->wifi_info[pWifi->count].authmode = 0;

		/**Net Type ==> 0:Infra; 1:Adhoc**/
		if(strstr(w_mode, "11b/g/n") != NULL)
			pWifi->wifi_info[pWifi->count].nettype = 0;
		else if(strstr(w_mode, "11b/g") != NULL)
			pWifi->wifi_info[pWifi->count].nettype = 1;
		else
			pWifi->wifi_info[pWifi->count].nettype = 0;

		memset(buf, 0, sizeof(buf));
		pWifi->count++;
	}

	if (fp)  pclose(fp);
#endif

	return 0;
}

void WifiSearch(REMOTE_WIFI_FIND *pwifiFind)
{		
	//static REMOTE_WIFI_FIND wifiFind;
	ScanWifiInfo(pwifiFind);
}

/*************************************************************
 * func: sort wifi info according to wifi signal level.
 *       return 0 on success.
 *************************************************************/
int Sort_WifiInfo(REMOTE_WIFI_FIND *pWifiInfo)
{
	if (NULL == pWifiInfo)
	{
		printf("[%s, %d] wifi info is empty!!!\n", __func__, __LINE__); 
		return -1;
	}
	printf("[%s, %d] ...pWifiInfo->count:%d...\n", __func__, __LINE__, pWifiInfo->count);

	if (1 == pWifiInfo->count) 
		return 0;

	int i = 0, j = 0;
	REMOTE_INFO TmpWifiInfo;
	memset(&TmpWifiInfo, '\0', sizeof(TmpWifiInfo));

	for (i = 0; i < pWifiInfo->count; i++)
	{
		for (j = 0; j < pWifiInfo->count - i; j++) 
		{
			if (pWifiInfo->wifi_info[j].iSignal < pWifiInfo->wifi_info[j+1].iSignal)    
			{
				memset(&TmpWifiInfo, '\0', sizeof(TmpWifiInfo));

				strcpy( TmpWifiInfo.ssid, pWifiInfo->wifi_info[j].ssid );
				//strcpy( TmpWifiInfo.bssid, pWifiInfo->wifi_info[j].bssid );
				TmpWifiInfo.authmode = pWifiInfo->wifi_info[j].authmode;
				TmpWifiInfo.enctype = pWifiInfo->wifi_info[j].enctype;
				TmpWifiInfo.iSignal = pWifiInfo->wifi_info[j].iSignal;

				strcpy( pWifiInfo->wifi_info[j].ssid, pWifiInfo->wifi_info[j+1].ssid );
				//strcpy( pWifiInfo->wifi_info[j].bssid, pWifiInfo->wifi_info[j+1].bssid);
				pWifiInfo->wifi_info[j].authmode = pWifiInfo->wifi_info[j+1].authmode;
				pWifiInfo->wifi_info[j].enctype = pWifiInfo->wifi_info[j+1].enctype;
				pWifiInfo->wifi_info[j].iSignal = pWifiInfo->wifi_info[j+1].iSignal;

				strcpy( pWifiInfo->wifi_info[j+1].ssid, TmpWifiInfo.ssid );       
				//strcpy( pWifiInfo->wifi_info[j+1].bssid, TmpWifiInfo.bssid );
				pWifiInfo->wifi_info[j+1].authmode = TmpWifiInfo.authmode;
				pWifiInfo->wifi_info[j+1].enctype = TmpWifiInfo.enctype;
				pWifiInfo->wifi_info[j+1].iSignal = TmpWifiInfo.iSignal;
			}
		}
	} 

#if 0
	printf("\n==============> After Wifi Info Sort: <===============\n");
	for (i = 0; i < pWifiInfo->count; i++)
	{
		HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%d\t enctype=%d\t iSignal=%d\n", i, pWifiInfo->wifi_info[i].ssid, pWifiInfo->wifi_info[i].authmode, pWifiInfo->wifi_info[i].enctype, pWifiInfo->wifi_info[i].iSignal);

		//fprintf( fpwt, "%s %s %d %d %d\n", pWifiInfo->wifi_info[i].ssid, pWifiInfo->wifi_info[i].bssid, pWifiInfo->wifi_info[i].iSignal, pWifiInfo->wifi_info[i].authmode, pWifiInfo->wifi_info[i].enctype );
	}
#endif

	return 0;
}


void TrimRight(char *psz)
{
	int i;
	for(i=strlen(psz)-1;i>0;i--)
	{
		if(psz[i]==' ' || psz[i]==0x09)
			psz[i]=0;
		else
		{
			return;
		}
	}
}

void GetLocalMacEX( char *pMac )
{
	char macaddr[32] = {0};
	conf_get(HOME_DIR"/net.cfg", "MacAddress", macaddr, 32);
	//printf("[%s, %d] macaddr:%s\n", __func__, __LINE__, macaddr);
	macaddr[strlen(macaddr)] = '\0';
	strcpy(pMac, macaddr);

	return;
}

static int getLocalAddress( char *interface, char *pIP )
{
	//printf("...%s...interface: %s...\n", __func__, interface);

	int sfd = 0;
	struct ifreq ifr;
	struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;

	memset(&ifr, 0, sizeof( struct ifreq) );
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == sfd)
	{
		perror("socket error");
		return -1;
	}   

	strcpy(ifr.ifr_name, interface);
	sin->sin_family = AF_INET;
	//strncpy(ifr.ifr_name,interface, sizeof(ifr.ifr_name));
	if (0 <= ioctl(sfd, SIOCGIFADDR, &ifr))
	{
		strcpy( pIP, inet_ntoa(sin->sin_addr) );
		HKLOG(L_ERR, "%s: %s\n", ifr.ifr_name, inet_ntoa(sin->sin_addr));
		close(sfd);
		return 0;
	}

	close(sfd);
	return -1;
}

#if 0 //delete
void InitWifiCfg()
{
	char aryMode[64]={0};
	wifiAddr.bStatus = conf_get_int(HOME_DIR"/wifinet.cfg", "isopen");
	conf_get_space( HOME_DIR"/wifisid.conf", HK_WIFI_SID, mywifiCfg.ssid, 64);
	if( strcmp(mywifiCfg.ssid,"")==0 )
	{
		strcpy( mywifiCfg.ssid, " ");
	}
	conf_get_space( HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD, mywifiCfg.password, 64 );
	if( strcmp(mywifiCfg.password,"")==0 )
	{
		strcpy( mywifiCfg.password, " ");
	}

	conf_get( HOME_DIR"/wifinet.cfg", HK_WIFI_SAFETYPE, mywifiCfg.authmode, 64 );
	conf_get( HOME_DIR"/wifinet.cfg", HK_WIFI_ENCRYTYPE, mywifiCfg.enctype, 64 );

	conf_get( HOME_DIR"/wifinet.cfg", "IPAddressMode", aryMode, 64 );
	conf_get( HOME_DIR"/wifinet.cfg", "IPAddress", wifiAddr.ip, 64 );
	conf_get( HOME_DIR"/wifinet.cfg", "SubnetMask", wifiAddr.netmask, 64 );
	conf_get( HOME_DIR"/wifinet.cfg", "DNSIPAddress1", wifiAddr.dns1, 64 );
	conf_get( HOME_DIR"/wifinet.cfg", "DNSIPAddress2", wifiAddr.dns2, 64 );
	conf_get( HOME_DIR"/wifinet.cfg", "Gateway", wifiAddr.gateway, 64 );
	if( strcmp( aryMode,"FixedIP" ) == 0 )
	{
		strcpy( wifiAddr.ipMode, HK_NET_BOOTP_STATIC );
	}
	else
	{
		strcpy( wifiAddr.ipMode, HK_NET_BOOTP_DHCP );
	}
	strcpy( eth0Addr.ip,wifiAddr.ip ); 
	strcpy( eth0Addr.gateway,wifiAddr.gateway );
	strcpy( eth0Addr.netmask,wifiAddr.netmask );
	strcpy( eth0Addr.dns1,wifiAddr.dns1 );
	strcpy( eth0Addr.dns2,wifiAddr.dns2 );
	strcpy( eth0Addr.ipMode,wifiAddr.ipMode );
	if( strlen( eth0Addr.gateway ) == 0 || strcmp( eth0Addr.gateway,"0.0.0.0" ) == 0 )
	{
		strcmp( eth0Addr.gateway,"192.168.0.1" );
		conf_set( HOME_DIR"/wifinet.cfg", "Gateway", eth0Addr.gateway );
	}
}
#endif

const char* is_mac_well_format(const char* mac)
{
	int x;
	if (!mac || strlen(mac) != 17)
		return NULL;
	for (x = 0; x < 17; ++x)
	{
		if ((x+1) % 3)
		{
			if (!isxdigit(mac[x]))
				return NULL;
		}
		else if (mac[x] != ':')
			return NULL;
	}
	return mac;
}

static char* mac_generate(char mac[18])
{
	unsigned char v[6];
	int x;     
	unsigned int seed;
	struct timespec ts;
	clock_gettime(0, &ts);
	seed = ts.tv_nsec + ts.tv_sec;
	v[0] = 0;
	for (x = 1; x < 6; ++x)
	{
		srand(seed);
		seed = (seed << 8) | rand();
		v[x] = (unsigned char)(seed & 0xff);
	}
	snprintf(mac,18, "%02x:%02x:%02x:%02x:%02x:%02x", v[0], v[1], v[2], v[3], v[4], v[5]);
	if (!is_mac_well_format(mac))
	{
		HKLOG(L_DBG, "%s", mac);
		abort();
	}
	return mac;
}

static int g_bPresentCnt = 0;
void InitIPAddres(short nInterface, HKIPAddres *ipaddr)
{
	//rintf("...... %s ......\n", __func__);
	char aryMode[64]={0};
	conf_get( HOME_DIR"/net.cfg", "MacAddress", eth0Addr.mac, 64 );
	if (strlen(eth0Addr.mac) <= 0)
	{
		char newMac[18] = {0};

		printf("...%s...mac is empty, generate new mac...\n", __func__); 
		mac_generate(newMac);

		if (strlen(newMac) > 0)
		{
			printf("...%s...new Mac: %s...\n", __func__, newMac); 
			conf_set( HOME_DIR"/net.cfg", "MacAddress", newMac );
		}
	}

	if ( (g_bPresentCnt ++) >= 10 )
	{
		return;
	}

	if (0 == nInterface) // eth0
	{
		conf_get( HOME_DIR"/net.cfg", "IPAddress", eth0Addr.ip, 64 );
		conf_get( HOME_DIR"/net.cfg", "Gateway", eth0Addr.gateway, 64 );
		conf_get( HOME_DIR"/net.cfg", "DNSIPAddress1", eth0Addr.dns1, 64 );
		conf_get( HOME_DIR"/net.cfg", "DNSIPAddress2", eth0Addr.dns2, 64 );
		conf_get( HOME_DIR"/net.cfg", "SubnetMask", eth0Addr.netmask, 64 );
		conf_get( HOME_DIR"/net.cfg", "IPAddressMode", aryMode, 64 );

		if ( strcmp( aryMode, "FixedIP" ) == 0 )
		{
			strcpy( eth0Addr.ipMode, HK_NET_BOOTP_STATIC );
		}
		else
		{
			strcpy( eth0Addr.ipMode, HK_NET_BOOTP_DHCP );
		}
		if ( (strlen(eth0Addr.gateway) == 0) || (strcmp(eth0Addr.gateway, "0.0.0.0") == 0) )
		{
			strcpy( eth0Addr.gateway, "192.168.0.1" );
			conf_set( HOME_DIR"/net.cfg", "Gateway", eth0Addr.gateway );
		}
	}
	else if (1 == nInterface) //ra0.
	{    	
		wifiAddr.bStatus = conf_get_int(HOME_DIR"/wifinet.cfg", "isopen");
		conf_get( HOME_DIR"/wifinet.cfg", "IPAddressMode", aryMode, 64 );
		conf_get( HOME_DIR"/wifinet.cfg", "IPAddress", wifiAddr.ip, 64 );
		conf_get( HOME_DIR"/wifinet.cfg", "SubnetMask", wifiAddr.netmask, 64 );
		conf_get( HOME_DIR"/wifinet.cfg", "DNSIPAddress1", wifiAddr.dns1, 64 );
		conf_get( HOME_DIR"/wifinet.cfg", "DNSIPAddress2", wifiAddr.dns2, 64 );
		conf_get( HOME_DIR"/wifinet.cfg", "Gateway", wifiAddr.gateway, 64 );

		if( strcmp( aryMode, "FixedIP" ) == 0 )
		{
			strcpy( wifiAddr.ipMode, HK_NET_BOOTP_STATIC );
		}
		else
		{
			strcpy( wifiAddr.ipMode, HK_NET_BOOTP_DHCP );
		}
		conf_get_space( HOME_DIR"/wifisid.conf", HK_WIFI_SID, mywifiCfg.ssid, 64);
		if( strcmp(mywifiCfg.ssid,"")==0 )
		{
			strcpy( mywifiCfg.ssid, " ");
		}
		conf_get_space( HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD, mywifiCfg.password, 64 );
		if( strcmp(mywifiCfg.password,"")==0 )
		{
			strcpy( mywifiCfg.password, " ");
		}

		conf_get( HOME_DIR"/wifinet.cfg", HK_WIFI_SAFETYPE, mywifiCfg.authmode, 64 );
		conf_get( HOME_DIR"/wifinet.cfg", HK_WIFI_ENCRYTYPE, mywifiCfg.enctype, 64 );

		strcpy( eth0Addr.ip,wifiAddr.ip ); 
		strcpy( eth0Addr.gateway,wifiAddr.gateway );
		strcpy( eth0Addr.netmask,wifiAddr.netmask );
		strcpy( eth0Addr.dns1,wifiAddr.dns1 );
		strcpy( eth0Addr.dns2,wifiAddr.dns2 );
		strcpy( eth0Addr.ipMode,wifiAddr.ipMode );
		if( strlen( eth0Addr.gateway ) == 0 || strcmp( eth0Addr.gateway,"0.0.0.0" ) == 0 )
		{
			strcmp( eth0Addr.gateway,"192.168.0.1" );
			conf_set( HOME_DIR"/wifinet.cfg", "Gateway", eth0Addr.gateway );
		}
	}
}

#if 0
void be_present2(int iLen, char *cWifiInfo, unsigned int ulParam )
{
#define PRESENT_STR2 HK_KEY_MAINCMD"=LocalWifi;" \
	HK_KEY_NETWET"=%s;" \
	HK_KEY_USERTYPE"=323;"\
	HK_KEY_UIPARAM"=%d;"\
	HK_KEY_STATE"=%d;"

	//HK_KEY_FLAG"=1099;"
	char *cBuf = malloc(256+iLen+64);
	if ( NULL != cBuf )
	{
		int n = sprintf(cBuf, PRESENT_STR2, cWifiInfo, ulParam, 2 );
		LanPresent( cBuf, n);
		free( cBuf );
		cBuf = NULL;
	}
}
#endif

void be_present2(int iLen, char *cWifiInfo, char *cSend, unsigned int ulParam )
{
#define PRESENT_STR2 HK_KEY_MAINCMD"=%s;" \
	HK_KEY_NETWET"=%s;" \
	HK_KEY_USERTYPE"=323;"\
	HK_KEY_UIPARAM"=%d;"\
	HK_KEY_STATE"=%d;"

	//HK_KEY_FLAG"=1099;"
	char *cBuf = malloc(256+iLen+64);
	if ( NULL != cBuf )
	{
		int n = sprintf(cBuf, PRESENT_STR2,cSend, cWifiInfo, ulParam, 2 );
		LanPresent( cBuf, n);
		free( cBuf );
		cBuf = NULL;
	}
}



static char wifiInfo[1024] = {0};
static char presenBuf[1024*3]={0};
void be_present(int x)
{
#define PRESENT_STR HK_KEY_MAINCMD"=LocalData;" \
	HK_NET_BOOTPROTO"=%s;" \
	HK_NET_DNS0"=%s;" \
	HK_NET_DNS1"=%s;" \
	HK_KEY_NETMASK"=%s;" \
	HK_KEY_NETWET"=%s;" \
	HK_KEY_USERTYPE"=323;" \
	HK_KEY_STATE"=%d;" \
	HK_KEY_VER"=%d;" \
	HK_KEY_INDEX"=%d;"\
	HK_KEY_LANPORT"=%d;"\
	HK_KEY_WANENABLE"=%d;"\
	HK_KEY_ERRORCOD"=%d;"\
	HK_KEY_SDENABLESD"=%d;"\
	HK_KEY_SDMOVEREC"=%d;"\
	HK_KEY_SDOUTMOVEREC"=%d;"\
	HK_KEY_SDAUTOREC"=%d;"\
	HK_EKY_SDLOOPREC"=%d;"\
	HK_EKY_SDSPILTE"=%d;"\
	HK_EKY_SDALLSIZE"=%d;"\
	HK_KEY_SDHAVEUSE"=%d;"\
	HK_KEY_SDLEFTUSE"=%d;"\
	HK_KEY_SDAUDIOMUX"=%d;"\
	HK_KEY_SDRCQC"=%d;"\
	HK_KEY_DEVID"=%s;" \
	HK_KEY_PASSWORD"=%s;"\
	HK_KEY_ALLARRIBUTE"=%s;"

	if (!quit_)
	{
		int n;
		HKFrameHead *framePacket = CreateFrameB();

		if (0 == g_isWifiInit) //eth0.
		{
			InitIPAddres( 0, &eth0Addr );
		}

		if (1 == g_isWifiInit) //ra0.
		{
			InitIPAddres( 1, &wifiAddr );
			SetParamUN( framePacket, HK_WIFI_OPENORCLOSE, wifiAddr.bStatus );
			SetParamStr( framePacket, HK_NET_BOOTPROTO, wifiAddr.ipMode );
			gTWifiStatus = 1;// GetWifiStatus();
			SetParamUN( framePacket,HK_KEY_WIFISTATE,gTWifiStatus  );
			SetParamStr( framePacket,HK_KEY_IP, wifiAddr.ip );
			SetParamStr( framePacket,HK_KEY_NETMASK, wifiAddr.netmask );
			SetParamStr( framePacket,HK_KEY_NETWET, wifiAddr.gateway );
			SetParamStr( framePacket,HK_NET_DNS0, wifiAddr.dns1 );
			SetParamStr( framePacket,HK_NET_DNS1, wifiAddr.dns2 );

			SetParamStr( framePacket, HK_WIFI_SID,mywifiCfg.ssid );
			SetParamStr( framePacket, HK_WIFI_SAFETYPE, mywifiCfg.authmode );
			{
				char aryPSW[64]={0};
				strcpy( aryPSW,mywifiCfg.password );
				char* cEncPswd =  EncodeStr( aryPSW );
				SetParamStr( framePacket, HK_WIFI_PASSWORD, cEncPswd );
			}
			SetParamStr( framePacket, HK_WIFI_ENCRYTYPE, mywifiCfg.enctype );
		}
		SetParamUN( framePacket,HK_WIFI_DEVICE, g_isWifiInit );
		GetFramePacketBuf( framePacket, wifiInfo, sizeof(wifiInfo) );
		DestroyFrame( framePacket );

		if(g_sdIsOnline == 1)
		{
			GetStorageInfo();
		}

		const char* version = getenv("VERSION");
		n = sprintf(presenBuf, PRESENT_STR, eth0Addr.ipMode, eth0Addr.dns1, eth0Addr.dns2, eth0Addr.netmask, eth0Addr.gateway, x, g_DevVer,g_DevIndex, g_lanPort,g_isWanEnable,g_nSignLogin,\
				g_sdIsOnline,hkSdParam.moveRec,hkSdParam.outMoveRec,hkSdParam.autoRec,hkSdParam.loopWrite,hkSdParam.splite,hkSdParam.allSize,\
				hkSdParam.haveUse,hkSdParam.leftSize,hkSdParam.audio,hkSdParam.sdrecqc,getenv("USER"), getEnv("LAN_PASSWD","123456"),wifiInfo);
		LanPresent( presenBuf, n);
	}
}

void wrap_sys_restart( )
{
	//printf("...zzzzzzzzzzzzzzzzzzzzzzz reboot 222222 zzzzzzzzzzzzzzzzzzzzzz...\n");
	g_sdIsOnline=0;
	be_present(0);
	//system( "ifconfig ra0 down" );
	sd_record_stop();
	sleep(1);
	system("umount /mnt/mmc/");
	//usleep(50*1000);
	hk_WirelessCard_Reset();
	quit_ = 1;
	system("sync");
	system("reboot");
}

#if 0
void SetSTCRst()
{
	ApiSetStcRst();
	ApiSendCmd();
	ApiClearCmd(1);
}
#endif

static char *GetInterIP( char *iter )
{
	int sock;   
	struct sockaddr_in sin;   
	struct ifreq ifr; 

	sock = socket(AF_INET, SOCK_DGRAM, 0);   
	if( sock == -1 )   
	{   
		//printf( "neterr!!!" );
		return 0;   
	}   

	strncpy(ifr.ifr_name,iter,IFNAMSIZ);   
	ifr.ifr_name[IFNAMSIZ-1] = 0;   
	if( ioctl( sock,SIOCGIFADDR,&ifr) <0)   
	{   
		close( sock );
		return   0;   
	}   
	close( sock );
	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));   
	strcpy( sysLocalIP,inet_ntoa(sin.sin_addr) );   
	if(strcmp( sysLocalIP,"0.0.0.0")==0 )
		return 0;
	return sysLocalIP;
}

char *GetLocalIP( char *aryInerface )
{
	if( GetInterIP("eth0") != NULL  )
	{
		strcpy( aryInerface,"eth0" );
		return sysLocalIP;
	}
	if( GetInterIP("rausb0") != NULL  )
	{
		strcpy( aryInerface,"rausb0" );
		return sysLocalIP;
	}
	if( GetInterIP("ra0") != NULL  )
	{
		strcpy( aryInerface,"ra0" );
		return sysLocalIP;
	}
	return NULL;
}

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
	//signal(SIGPIPE, SIG_IGN);
	//signal(SIGALRM, SIG_IGN);
}

static void create_my_detached_thread(int (*func)())
{
        pthread_t tid;
        pthread_attr_t a;
        pthread_attr_init(&a);
        pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &a, (void *)func, NULL);
        pthread_attr_destroy(&a);
}
static void create_detached_thread(void *(*func)(void*), void* arg)
{
	pthread_t tid;
	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_create(&tid, &a, func, arg);
	pthread_attr_destroy(&a);
}


/**************************************************************************
 * func: disable specified VENC channel for resolution exchange.
 *************************************************************************/
//HI_S32 SampleUNEnableEncode(VENC_GRP VeGroup, VENC_CHN VeChn, VI_DEV ViDev, VI_CHN ViChn )
//{
//    HI_S32 s32Ret;
//    s32Ret = HI_MPI_VENC_StopRecvPic(VeChn);
//    if (s32Ret != HI_SUCCESS)
//    {
//        printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
//        return HI_FAILURE;
//    }
//    //HI_MPI_VENC_UnRegisterChn( VeChn );
//    s32Ret = HI_MPI_VENC_UnbindInput(VeGroup);
//    if (s32Ret != HI_SUCCESS)
//    {
//        printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
//        return HI_FAILURE;
//    }
//
//    return HI_SUCCESS;
//}


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


/**************************************************************************
 * func: enable specified VENC channel for resolution exchange.
 *************************************************************************/
//HI_S32 SampleEnableEncode(VENC_GRP VeGroup, VENC_CHN VeChn, VI_DEV ViDev, VI_CHN ViChn, VENC_CHN_ATTR_S *pstAttr)
//{
//    HI_S32 s32Ret;
//    
//    //HI_MPI_VENC_RegisterChn(VeGroup,VeChn);
//    s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
//    if (s32Ret != HI_SUCCESS)
//    {
//        printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
//        return HI_FAILURE;
//    }
//    s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
//   if (s32Ret != HI_SUCCESS)
//   {
//        printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
//        return HI_FAILURE;
//    }
//    return HI_SUCCESS;
//
//}

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



/*
   HI_S32 SampleEnableEncode_sun(VENC_GRP VeGroup, VENC_CHN VeChn, VENC_CHN_ATTR_S *pstAttr)
   {
   HI_S32 s32Ret;

   s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
   if (s32Ret != HI_SUCCESS)
   {
   printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
   return HI_FAILURE;
   }

   s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
   if (s32Ret != HI_SUCCESS)
   {
   printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
   return HI_FAILURE;
   }

   return HI_SUCCESS;
   }

   HI_S32 InitEncode(VENC_GRP VeGroup, VENC_CHN VeChn, VI_DEV ViDev, VI_CHN ViChn, VENC_CHN_ATTR_S *pstAttr)
   {
   HI_S32 s32Ret;

   s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
   if (s32Ret != HI_SUCCESS)
   {
   printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
   return HI_FAILURE;
   }

   s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
   if (s32Ret != HI_SUCCESS)
   {
   printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
   return HI_FAILURE;
   }

   s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
   if (s32Ret != HI_SUCCESS)
   {
   printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
   return HI_FAILURE;
   }

   return HI_SUCCESS;
   }

   HI_S32 SampleEnableMd(int iVeChn)
   {
   HI_S32 s32Ret;
   VENC_CHN VeChn = iVeChn;
   MD_CHN_ATTR_S stMdAttr;
   MD_REF_ATTR_S  stRefAttr;

   stMdAttr.stMBMode.bMBSADMode =HI_TRUE;
   stMdAttr.stMBMode.bMBMVMode = HI_FALSE;
   stMdAttr.stMBMode.bMBPelNumMode = HI_FALSE;
   stMdAttr.stMBMode.bMBALARMMode = HI_FALSE;
   stMdAttr.u16MBALSADTh = 1000;
   stMdAttr.u8MBPelALTh = 20;
   stMdAttr.u8MBPerALNumTh = 20;
   stMdAttr.enSADBits = MD_SAD_8BIT;
   stMdAttr.stDlight.bEnable = HI_FALSE;
   stMdAttr.u32MDInternal = 5;
   stMdAttr.u32MDBufNum = 16;

   stRefAttr.enRefFrameMode = MD_REF_AUTO;
   stRefAttr.enRefFrameStat = MD_REF_DYNAMIC;
//stRefAttr.enRefFrameStat = MD_REF_STATIC;

s32Ret =  HI_MPI_MD_SetChnAttr(VeChn, &stMdAttr);
if(s32Ret != HI_SUCCESS)
{
	printf("HI_MPI_MD_SetChnAttr Err 0x%x\n", s32Ret);
	return HI_FAILURE;
}

s32Ret = HI_MPI_MD_SetRefFrame(VeChn, &stRefAttr);
if(s32Ret != HI_SUCCESS)
{
	printf("HI_MPI_MD_SetRefFrame Err 0x%x\n", s32Ret);
	return HI_FAILURE;
}

s32Ret = HI_MPI_MD_EnableChn(VeChn);
if(s32Ret != HI_SUCCESS)
{
	printf("HI_MPI_MD_EnableChn Err 0x%x\n", s32Ret);
	return HI_FAILURE;
}

return HI_SUCCESS;
}
*/

//REGION_CTRL_PARAM_U unParam_;
//REGION_HANDLE handle[5];

//static byte *bmp_buffer_ =NULL;
//extern int m_fw;
//extern int m_fh;
/*
   void change_letter(int i, char c)
   {
   if(bmp_buffer_ == NULL)
   {
//printf("...bmp_buffer_ == NULL..\n");
return;
}
byte * dat = char2data(c);
if(dat == 0)
return;

int w = m_fw;
int h = m_fh;
//get_w_h(w, h);
pcopy(dat, w * 2, h, &bmp_buffer_[w * 2 * i], w * 20 * 2);
}

bool region_setbmp()
{
REGION_CRTL_CODE_E enCtrl = REGION_SET_BITMAP;
HI_S32 ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam_);
if(ret != HI_SUCCESS)
{
//printf("set region bitmap faild 0x%x!\n", ret);
return false;
}
return HI_SUCCESS;
}

HI_S32 SampleCtrl4OverlayRegion(HI_VOID)
{
HI_S32 i = 0;
HI_S32 s32Ret;
HI_S32 s32Cnt = 0;

int alpha_=100;
int sx_=4;
int sy_=4;

int w = m_fw;
int h = m_fh;
//get_w_h(w, h);

REGION_ATTR_S stRgnAttr;
stRgnAttr.enType = OVERLAY_REGION;
stRgnAttr.unAttr.stOverlay.bPublic =HI_TRUE;
stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
stRgnAttr.unAttr.stOverlay.stRect.s32X = (sx_ + 2) / 4 * 4;
stRgnAttr.unAttr.stOverlay.stRect.s32Y = sy_;
stRgnAttr.unAttr.stOverlay.stRect.u32Width = w * 20;
stRgnAttr.unAttr.stOverlay.stRect.u32Height = h;
stRgnAttr.unAttr.stOverlay.u32BgAlpha = 50;
stRgnAttr.unAttr.stOverlay.u32FgAlpha = alpha_;
stRgnAttr.unAttr.stOverlay.u32BgColor = 0;
stRgnAttr.unAttr.stOverlay.VeGroup = 0;

s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[0]);
if(s32Ret != HI_SUCCESS)
{
printf("HI_MPI_VPP_CreateRegion err 0x%x!\n",s32Ret);
return 0;
}

REGION_CRTL_CODE_E enCtrl = REGION_SHOW;
s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam_);
if(s32Ret != HI_SUCCESS)
{
printf("show faild 0x%x!\n",s32Ret);
return 0;
}
memset(&unParam_, 0, sizeof(REGION_CTRL_PARAM_U));

if( NULL!=bmp_buffer_)
{
	free(bmp_buffer_);
}
bmp_buffer_ =(byte*)malloc(w*20*h*2);
memset(bmp_buffer_, 0, w*20*h*2);

unParam_.stBitmap.pData = bmp_buffer_;
unParam_.stBitmap.u32Width = w * 20;
unParam_.stBitmap.u32Height = h;
unParam_.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;

{
	int i;
	for( i = 0; i < 20; ++i)
	{
		//if(i==10||i==11)
		//    continue;
		change_letter(i, ' ');
	}
}
return region_setbmp();
}
*/



/*******************************************************************************
 * func: init media processing platform for 4-way H264 stream 
 *       for preview, and 1-way JPEG for snapshot. (2014-04-22)
 ******************************************************************************/
HI_S32 Video_SubSystem_Init(HI_VOID)
{
	HK_DEBUG_PRT("++++++ hk platform: hi3518E ++++++\n");
	PAYLOAD_TYPE_E enPayLoad[3] = {PT_H264, PT_H264, PT_JPEG};
	PIC_SIZE_E enSize[3]        = {PIC_HD720, PIC_VGA, PIC_VGA};

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
	stVpssChnMode.bDouble       = HI_FALSE;
	stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	stVpssChnMode.u32Width      = 640;
	stVpssChnMode.u32Height     = 480;
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

static int gReLogIn = 0;
static int g_APNonLogin = 0;
static void* thd_nonblock_login(void* a)
{
	int x;

#if ENABLE_ONVIF
	IPCAM_setTskName("thd_nonblock_login");
#endif
	while (!quit_)
	{
		//printf("......%s, network_mode: %s......\n", __func__, getenv("NETWORK_MODE"));
		//printf("[%s, %d]...g_APNonLogin:%d, gReLogIn: %d...\n", __func__, __LINE__, g_APNonLogin, gReLogIn);
		//if ( (signcode_ != HK_LOGIN_SUCCESS) && (signcode_ != -1) && (0 == g_APNonLogin))
		//if ( (signcode_ != HK_LOGIN_SUCCESS) && (signcode_ != -1) )
		if ( signcode_ != HK_LOGIN_SUCCESS )
		{
			char cBuf[64] = {0};
			HKFrameHead *framePacket = CreateFrameB();
			//printf( "login in\n" );
			gSysTime     = time(0);
			gbStartTime  = 1;

#if 0
			if ( gReLogIn ++ > 10 ) //modyfied:2015-01-07.
			{
				wrap_sys_restart( );
			}
#else
			gReLogIn ++;
			//printf("...g_APNonLogin:%d, gReLogIn: %d...\n", g_APNonLogin, gReLogIn);
			//if ((12 == gReLogIn) || (13 == gReLogIn))
			if ((4 == gReLogIn) || (5 == gReLogIn))
			{
				char s_LocalIPAddr[16] = {0};
				if ((!getLocalAddress("ra0", s_LocalIPAddr)) && (strlen(s_LocalIPAddr) > 0) && (0 == strcmp(s_LocalIPAddr, IPADDR_APMODE)))
				{
					HK_DEBUG_PRT("...current status is AP mode...local ip address: %s, len:%d\n", s_LocalIPAddr, strlen(s_LocalIPAddr)); 
					gReLogIn = 0; //do not restart system while AP mode.
					//g_APNonLogin = 1;
				}
			}
#endif

			//if ( gReLogIn > 13 ) //modyfied:2015-02-05.
			if ( gReLogIn > 5 ) //modyfied:2015-03-06.
			{ //restart system while server login failed after 12 minutes.
				wrap_sys_restart( );
			}

			if ( signcode_ != -2 )
			{
				//printf( "login inlougout\n" );
				SysLogout();
			}
			//signcode_ = -1;

			SetParamUN( framePacket,HK_KEY_VERTYPE, HK_DEV_TYPE );//502
			GetFramePacketBuf( framePacket, cBuf, sizeof(cBuf) );
			DestroyFrame( framePacket );
			SysLogin_dm(user_, passwd_, cBuf, host_, atoi(port_), 0);
			gbStartTime = 0;
		}

		if ( signcode_ == HK_LOGIN_SUCCESS )
		{
			gReLogIn = 0;
			//g_APNonLogin = 0;
		}

		//for (x = 0; (x < 60) && !quit_; ++x) //1 minutes.
		for (x = 0; (x < 60*3) && !quit_; ++x) //3 minutes.
		{
			sleep(1);
		}
	}
	return (a=0);
}

static void start_nonblock_login()
{
	//printf(".......zzzzzzzzzz, %s..........\n", user_);
	if (user_ && (strlen(user_) > 0))
	{
		setenv("USER", user_, 1);
		create_detached_thread(thd_nonblock_login, 0);
	}
}

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
		HKLOG(L_DBG,"<><><><><><><><><><><>\n");
		fflush(stdout);
		SPAWN(pid, childproc, cmdline); // sleep(3);
		if (pid > 0)
			setpidfile(getenv("PIDFILE"), pid);
		CMD_PPAD("[ $$ -ne %d ] && exit 0", (int)pid); //CMD_PPAD("echo $0: $@");
	}
}

/*
   void SampleWaitDestroyVdecChn(HI_S32 s32ChnID)
   {
   HI_S32 s32ret;
   VDEC_CHN_STAT_S stStat;

   memset(&stStat,0,sizeof(VDEC_CHN_STAT_S));


   while(1)
   {
   usleep(40000);
   s32ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
   if(s32ret != HI_SUCCESS)
   {
   Printf("HI_MPI_VDEC_Query failed errno 0x%x \n", s32ret);
   return;
   }

   if(stStat.u32LeftPics == 0 && stStat.u32LeftStreamBytes == 0 )
   {
   printf("had no stream and pic left\n");
   break;
   }
   }

   s32ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
   if(s32ret != HI_SUCCESS)
   {
   Printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32ret);
   return;
   }
   }
 */


/*
 * Exit System
 */

/*
   static int Create_Timeconf(void)
   {
   FILE *fd_w;
   time_t now = time(0);
   if(access(HOME_DIR"/time.conf", F_OK) == 0)
   return 0;
   if( (fd_w = fopen(HOME_DIR"/time.conf", "w")) == NULL )
   return -1;
//fprintf(fd_w,"time=%d\n", (int)now);
fprintf(fd_w,"zone=%d\n", 8);
//fprintf(fd_w,"RESET=%d\n", (int)now);
fclose(fd_w);
return 1;
}
 */

/*
   static void SetSysTime()
   {
   struct timespec ts;
   ts.tv_sec = conf_get_int(HOME_DIR"/time.conf", "time");
   ts.tv_nsec = 0;
   if (time(0) + 60*15 < ts.tv_sec)
   {
   ts.tv_sec += 35;
   HKLOG(L_INFO, "%d", (int)time(0));
   clock_settime(CLOCK_REALTIME, &ts);
   HKLOG(L_INFO, "%d", (int)time(0));
   }
   }
 */
#if 0
static void* thd_restart_monitor(void* a)
{
	int seconds = 0, xcnt = *(int*)a;
	while (seconds == 0)
	{
		seconds = 60;
		while (!quit_ && seconds > 0)
		{
			seconds -= 3;
			seconds += sleep(3);
		}
		if (quit_)
		{
			seconds = 100;
		}
		else
		{
			int ycnt = xcnt;
			xcnt = *(volatile int*)a;
			if (xcnt == ycnt)
			{
				quit_ = Excode_Dead;
				seconds = 1;
			}
		}
		if( gbStartTime )
		{
			time_t nowt = time(0);
			if( nowt - gSysTime > HK_SYS_TIMEOUT )
			{
				fprintf(stderr,"timeout %s: <o>&<o>: quit=%d, count=%d=%d", __FUNCTION__, quit_, xcnt, *(int*)a);
				break;
			}
		}
	}
	fprintf(stderr,"%s: <o>&<o>: quit=%d, count=%d=%d", __FUNCTION__, quit_, xcnt, *(int*)a);
	sleep(3);
	fprintf(stderr,"%s: <o>&<o> RESET: %d", __FUNCTION__, quit_);
	//printf( "--restart-monirot..-----1111111111111---------------->QUIT\n" );

	wrap_sys_restart( );
	return 0;
}
#endif

static void server_time_sync(int x, void* a, int ac, char* av[])
{
	if (x > 0)
		return;
	struct timespec ts;
	ts.tv_sec = time(0) + atoi(av[0]);
	ts.tv_nsec = 0;
	clock_settime(CLOCK_REALTIME, &ts);
	//conf_set_int(HOME_DIR"/time.conf", "time", ts.tv_sec);
	gSysTime  = time(0);
	gResetTime = time(0);
	return;
}

static void cb_init(int x, const char* ev, unsigned int size)
{
	printf("cb_init....x=%d........\n", x);

	if (x == HK_CONNECT_SUCCESS)
	{
		be_present(2);
	}

	if (x == HK_NET_OUT)
	{
		g_nSignLogin = x;
		signcode_ = x;
	}

	if (x == HK_LOGIN_SUCCESS || x == HK_PASSWD_FAIL|| x==HK_USER_EXPIRE)
	{
		g_nSignLogin = x;
		signcode_ = x;
		if (ev)
		{
			HKFrameHead* hf = CreateFrameA(ev, size);
			if (hf)
			{
				time_t t = DictGetInt(hf, HK_KEY_TIME);
				if (t > 0)
				{
					int iSummer = conf_get_int(HOME_DIR"/time.conf", "summer");
					int tz = conf_get_int(HOME_DIR"/time.conf", "zone");
					int seconds;
					if( iSummer == 1 )
						seconds = tz + t - time(0)+3600; //+1h
					else
						seconds = tz + t - time(0);

					if (abs(seconds) > 1500)
					{
						char tmp[32];
						snprintf(tmp, sizeof(tmp), "%d", seconds);
						tq_post_v(tq_, 0, server_time_sync, 0, tmp, NULL);	//???
					}
				}
				monc_start(hf,x );		//mon

				if (x == HK_LOGIN_SUCCESS)
				{
					g_nSignLogin = 200;
				}
				DestroyFrame(hf);
			}
		}
	}
}

/*
   static const char* prefer_gw(const char* a, const char* b, const char* c)
   {
   int x;
   struct in_addr tmp;
   const char* g[] = { a, b, c };
   for (x = 0; x < 3; ++x)
   if (inet_aton(g[x], &tmp) && tmp.s_addr != 0)
   return g[x];
   return 0;
   }
 */

static unsigned long rand_in_addr(const char* myip, struct in_addr u)
{
	unsigned long x = ntohl(u.s_addr);
	inet_aton(myip, &u);
	return htonl((x & 0xffffff00) | (rand_r(&u.s_addr) % 190 + 32));
}

static int netconfig(int x, void* my, int ac, const char* av[])
{
	int ec = 0; 
	HKLOG(L_DBG, "ac=%d: %s", ac, av[0]);
	if (strcmp(av[0], "dhcp") == 0)
	{    
		conf_set(HOME_DIR"/net.cfg", "IPAddressMode","DHCP");
		conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "DHCP");
		//CMD_PPAD("/sbin/udhcpc -fq -i eth0 -n -s /usr/bin/udhcpc.conf &");
	}    
	else if (ac >= 6)
	{   
		if (strcmp(av[0], "static") == 0 || strcmp(av[0], "bound") == 0  || strcmp(av[0], "renew") == 0)
		{    
			conf_set(HOME_DIR"/net.cfg", "IPAddressMode","FixedIP");
			conf_set(HOME_DIR"/net.cfg", "IPAddress",av[1]);
			conf_set(HOME_DIR"/net.cfg", "DNSIPAddress1",av[4]);
			conf_set(HOME_DIR"/net.cfg", "DNSIPAddress2",av[5]);
			conf_set(HOME_DIR"/net.cfg", "Gateway",av[3]);
			conf_set(HOME_DIR"/net.cfg", "SubnetMask",av[2]);

			conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "FixedIP");
			conf_set(HOME_DIR"/wifinet.cfg", "IPAddress", av[1] );
			conf_set(HOME_DIR"/wifinet.cfg", "SubnetMask", av[2] );
			conf_set(HOME_DIR"/wifinet.cfg", "Gateway", av[3] );
			conf_set(HOME_DIR"/wifinet.cfg", "DNSIPAddress1", av[4] );
			conf_set(HOME_DIR"/wifinet.cfg", "DNSIPAddress2", av[5] );

			char aryCmd[128]={0};
			sprintf( aryCmd,"echo nameserver %s > /etc/resolv.conf",av[4] );
			system( aryCmd );
			system( "echo nameserver 8.8.8.8 >> /etc/resolv.conf" );
		}        
	}        
	return ec;
}

void OnWifiCallBack(unsigned short nWifiState, char *param )
{
	if( nWifiState == 0)
	{
		//printf("....state==0....\n");
		return;
	}
#define PRESENT_STR3 HK_KEY_MAINCMD"=WifiState;" \
	HK_KEY_STATE"=%d;" \
	"VERSION=%s;" \
	HK_KEY_DEVID"=%s;" 
	gTWifiStatus = nWifiState; 
	int n;
	char buf[sizeof(PRESENT_STR3)+256]={0};
	const char* version = getenv("VERSION");
	const char* user = getenv("USER");
	n = sprintf(buf, PRESENT_STR3, nWifiState, version, getenv("USER") );
	LanPresent( buf, n);
}
int write_configh(char* key,int set)
{
	FILE *fd_r;
	FILE *fd_w;
	char line[100];
	int WriteFlag = 0;

	if( (fd_r = fopen( "/mnt/sif/hkclient.conf", "r" )) == NULL )
		return -1;
	if( (fd_w = fopen( "/mnt/sif/hkclient.conf_", "w" )) == NULL )
	{
		fclose(fd_r);
		return -1;
	}

	while(fgets( line, 100, fd_r ) != NULL)
	{
		if(strstr(line,key) != NULL)
		{
			WriteFlag = 1;
			fprintf(fd_w,"%s=%d\n",key,set);
		}
		else
		{
			fputs(line,fd_w);
		}
	}
	if(WriteFlag == 0)
	{
		fprintf(fd_w,"%s=%d\n",key,set);
	}

	fclose(fd_r);
	fclose(fd_w);

	remove("/mnt/sif/hkclient.conf");
	rename("/mnt/sif/hkclient.conf_","/mnt/sif/hkclient.conf");
	return 1;
}


static int isMacMatch( const char *PMac )
{
	if( PMac == NULL )
	{
		return 0;
	}
	//printf( "match mac %s:%s\n", eth0Addr.mac, PMac );
	if( strcmp( eth0Addr.mac, PMac ) == 0 )
	{
		return 1;
	}
	/*
	   char *cMac=NULL;
	   if( ( cMac = get_mac_address("eth0") ) != NULL )
	   {
	   if( strcmp(cMac,PMac) == 0 ) return 1;
	   }

	   if( ( cMac = get_mac_address("rausb0") ) != NULL )
	   {
	   if( strcmp(cMac,PMac) == 0 ) return 1;
	   }

	   if( ( cMac = get_mac_address("ra0") ) != NULL )
	   {
	   if( strcmp(cMac,PMac) == 0 ) return 1;
	   }
	 */
	return 0;
}

static void hk_setSidList( const char *cSid, const char *cPswd )
{
	if( NULL==cSid)
		return;
	int iCount = conf_get_int(HOME_DIR"/sidlist.conf", "icount");
	int i=0;
	char cKeyBuf[64]={0};
	for(i=0;i<iCount; i++)
	{
		char cPsid[128]={0};
		snprintf(cKeyBuf,sizeof(cKeyBuf), "%s%d", HK_KEY_VALUEN,i);
		conf_get_space(HOME_DIR"/sidlist.conf", cKeyBuf, cPsid, sizeof(cPsid) );
		if(strcmp(cSid,cPsid)==0)
		{
			conf_set_space(HOME_DIR"/sidlist.conf", cKeyBuf, cPsid);
			snprintf(cKeyBuf,sizeof(cKeyBuf), "%s%d", HK_KEY_PASSWORD,i);
			conf_set_space(HOME_DIR"/sidlist.conf", cKeyBuf, cPswd);
			return;
		}
	}
	int iDelete = conf_get_int(HOME_DIR"/sidlist.conf", "idel");
	if( iDelete ==6)
	{
		iDelete=0;
	}
	if( iCount<6)
		iCount++;

	snprintf(cKeyBuf,sizeof(cKeyBuf), "%s%d", HK_KEY_VALUEN,iDelete);
	conf_set_space(HOME_DIR"/sidlist.conf", cKeyBuf, cSid);
	snprintf(cKeyBuf,sizeof(cKeyBuf), "%s%d", HK_KEY_PASSWORD,iDelete);
	conf_set_space(HOME_DIR"/sidlist.conf", cKeyBuf, cPswd);
	iDelete++;

	conf_set_int(HOME_DIR"/sidlist.conf", "icount", iCount);
	conf_set_int(HOME_DIR"/sidlist.conf", "idel", iDelete);
}

static char g_cSid[128]={0};
static char g_cPwd[128]={0};
static char g_cEnc[64]={0};

char *add_format(char *string)
{
	int i;
	char *val = NULL;
	int len = strlen(string);
	val =(char *)malloc(len + 3);
	val[0] = '\"';
	val[len + 2] = '\0';
	val[len + 1] = '\"';
	for(i = 0; i < len; i++)
	{
		val[i+1] = string[i];
		if(string[i] == '\n')
			return NULL;
	}
	return val;
}


static bool g_bupdatemac=false;
static int cb_lan(unsigned int hkid, int asc, const char* buf, unsigned int len)
{
	const char* mc = NULL;
	FILE *fp = NULL; 
	FILE *fp1 = NULL; 
	int ret = 10;
	char line_status[16] = {0};
	HKFrameHead* hf = CreateFrameA(buf, len);
	if (!hf)
		return 0;
	if ( !(mc = GetParamStr(hf, HK_KEY_MAINCMD)))
	{
		DestroyFrame(hf);
		return 0;
	}
	//printf("+++++++++++++++ mc: %s ++++++++++++++++\n", mc);
	if (strcmp(mc, "LocalData") == 0)
	{
		char aryInterface[64]={0};
		const char *myaddr = GetLocalIP( aryInterface );//getenv("IPAddress");
		int x = GetParamUN(hf, HK_KEY_STATE);
		int iTime = GetParamUN( hf, HK_KEY_END_TIME );
		if( b_localTime < 5 && iTime >0 )
		{
			b_localTime++;
			int iZone = GetParamUN( hf, HK_KEY_TIME );
			//int iZone = GetParamUN( hf, HK_KEY_TIME ) - 28800;
			int iSummerp = GetParamUN( hf, HK_WIFI_OPENORCLOSE );
			int iTime = GetParamUN( hf, HK_KEY_END_TIME );
			{
				conf_set_int(HOME_DIR"/time.conf", "summer", iSummerp);
				conf_set_int(HOME_DIR"/time.conf", "zone", iZone);
				conf_set_int(HOME_DIR"/time.conf", "time_", iTime);

				unsigned int seconds;
				if (1 == iSummerp)
					seconds = iZone+iTime - time(0)+3600; //+1
				else
					seconds = iZone+iTime - time(0); //+1

				char tmp[32]={0};
				snprintf(tmp, sizeof(tmp), "%d", seconds);
				tq_post_v(tq_, 0, server_time_sync, 0, tmp, NULL);
			}
		}
		if (x == 1)
		{
			const char* ct = GetParamStr(hf, HK_KEY_USERTYPE);
			if (ct /*&& strcmp(ct, getenv("CLIENT"))*/)
				be_present(2);
		}
		else if (x == 10)
		{
			unsigned int ec = GetParamUN(hf, "ErrorCode");
			if (ec == HKNETS_DISCONNECT)
				be_present(2);
		}
		if ( myaddr != 0 && strncmp(myaddr, "169.254", 7) == 0 )
		{
			g_bPresentCnt = 0;
			const char *peeraddr = GetParamStr(hf, HK_KEY_IP);
			if (peeraddr && 0!=strncmp(peeraddr, "169.254", 7))
			{
				struct in_addr pa;
				if (inet_aton(peeraddr, &pa))
				{
					struct stat st;
					if (stat("/sbin/zcip", &st) == 0 && (st.st_mode & S_IXUSR))
					{
						const char* msk = getPx(hf, HK_KEY_NETMASK, "255.255.255.0");
						const char* gw = getPx(hf, HK_KEY_NETWET, "");
						struct in_addr m;
						if (inet_aton(msk, &m) && gw != NULL )
						{
							pa.s_addr |= ~m.s_addr; 
							char CMD[128]={0};
							//sprintf( CMD,"export subnet=%s gateway=%s", msk, gw );
							//system( CMD );
							setenv( "subnet",msk,1 );
							setenv( "gateway",gw,1 );
							sprintf( CMD,"/sbin/zcip -f -q -r %s %s /usr/bin/zcip.script &", inet_ntoa(pa),aryInterface);
							system( CMD );
						}
					}
				}
			}
		}
	}
	else if (strcmp(mc, "NetConfig") == 0 )
	{
		const char* mac = GetParamStr(hf, HK_KEY_HWADDR);
		const char* newMac = GetParamStr( hf, HK_KEY_NEWMACADDR );
		if (mac)
		{
			if (isMacMatch(mac))
			{
				if (newMac)
				{
					if (strcmp(mac, newMac) != 0)
					{
						conf_set(HOME_DIR"/net.cfg", "MacAddress", newMac);
					}
				}
				const char* v[6];
				v[0] = getPx(hf, HK_NET_BOOTPROTO, "");
				v[1] = getPx(hf, HK_KEY_IP, "");
				v[2] = getPx(hf, HK_KEY_NETMASK, "");
				v[3] = getPx(hf, HK_KEY_NETWET, "");
				v[4] = getPx(hf, HK_NET_DNS0, "");
				v[5] = v[4]; // getPx(hf, HK_NET_DNS1, ""); // 
				netconfig(0, NULL, 6, v);

				wrap_sys_restart( );
			}
			else
			{
				HKLOG(L_INFO, "mac address not match: %s, %s", mac, get_mac_address("eth0"));
			}
		}
		else
		{
			HKLOG(L_ERR, "mac address required");
		}
	}
	else if ( strcmp(mc, "700") == 0 )
	{
		printf("=============== 700 =================\n");
		const char* mac = GetParamStr(hf, HK_KEY_HWADDR);
		if (mac)
		{
			printf("set wifi addr! mac = %s\n", mac);
			if (isMacMatch(mac))
			{
				int isOpenWifi = DictGetInt( hf, HK_WIFI_OPENORCLOSE );
				conf_set_int(HOME_DIR"/wifinet.cfg", HK_WIFI_OPENORCLOSE, isOpenWifi);
				printf("...... start wifi...open=%d......\n", isOpenWifi );
				if (1 == isOpenWifi)
				{
					char aryPSW[64] = {0};
					char *ssid = NULL, *psk = NULL;
					char *cSsid       = DictGetStr( hf, HK_WIFI_SID );
					char *cSafeType   = DictGetStr( hf, HK_WIFI_SAFETYPE );
					char *cSafeOption = DictGetStr( hf, HK_WIFI_SAFEOPTION );
					char *cKeyType    = DictGetStr( hf, HK_WIFI_KEYTYPE );
					char *cPasswd     = DictGetStr( hf, HK_WIFI_PASSWORD );
					char *cBootProto  = DictGetStr( hf, HK_NET_BOOTPROTO );
					char *cEnctyType  = DictGetStr( hf, HK_WIFI_ENCRYTYPE );

					strcpy( aryPSW, cPasswd );
					char *cDecPswd = DecodeStr(aryPSW);
					printf("===> ssid: %s\n", cSsid);
					printf("===> psk:  %s\n", cDecPswd);
					printf("===> safetype: %s\n", cSafeType);
					printf("===> encrytype: %s\n", cEnctyType);

#if 1
					/**check AP mode for phone set wifi connection**/
					char s_LocalIPAddr[16] = {0};
					//if (!(strcmp(cSafeType, "none")) && (!getLocalAddress("ra0", s_LocalIPAddr)) && (strlen(s_LocalIPAddr) > 0) && (0 == strcmp(s_LocalIPAddr, IPADDR_APMODE)))
					if ((!(strcmp(cSafeType, "auto"))) || (!(strcmp(cSafeType, "none")) && (!getLocalAddress("ra0", s_LocalIPAddr)) && (strlen(s_LocalIPAddr) > 0) && (0 == strcmp(s_LocalIPAddr, IPADDR_APMODE))))
					{
						HK_DEBUG_PRT("+++++++++++++++++ phone setting wifi connection +++++++++++++++++\n");
						int wificount = 0; //index the number of wifi node.
						char WifiListBuf[256] = {0};
						char tmpKey[32] = {0}, WifiSsid[SSID_LEN] = {0}, WifiBssid[BSSID_LEN] = {0};
						char WifiSignal[8] = {0}, WifiAuthmode[8] = {0}, WifiEnctype[8] = {0};

						FILE *fprd = fopen( CONF_WIFILIST, "r" ); //open wifilist.conf to get available wifi node.
						if (NULL == fprd)
						{
							fprintf(stderr, "fopen error with:%d, %s\n", errno, strerror(errno));
							return errno;
						}

						fgets( WifiListBuf, sizeof(WifiListBuf), fprd ); //first line: wifi count.
						WifiListBuf[strlen(WifiListBuf) - 1] = '\0'; //skip '\n' at the end of line.
						//HK_DEBUG_PRT("------> WifiListBuf: %s <--------\n", WifiListBuf);
						sscanf( WifiListBuf, "%[^=]=%d", tmpKey, &wificount);
						HK_DEBUG_PRT("------> tmpKey:%s, wifi count = %d <--------\n", tmpKey, wificount);

						// get wifi list data start.
						wificount = 0;
						while( fgets( WifiListBuf, sizeof(WifiListBuf), fprd ) )
						{
							WifiListBuf[strlen(WifiListBuf) - 1] = '\0'; //skip '\n' at the end of line.
							sscanf(WifiListBuf, "%s %s %s %s",  WifiSsid, WifiSignal, WifiAuthmode, WifiEnctype);
							//HK_DEBUG_PRT("[%d] ssid:%s signalLevel:%s authmode:%s enctype:%s\n",  wificount, WifiSsid, WifiSignal, WifiAuthmode, WifiEnctype);

							if (0 == strcmp(WifiSsid, cSsid)) //compare wifi ssid in wifilist.conf.
							{
								if (0 == atoi(WifiAuthmode))
									strcpy(cSafeType, "none");
								else if (1 == atoi(WifiAuthmode))
									strcpy(cSafeType, "wep");
								else if (2 == atoi(WifiAuthmode))
									strcpy(cSafeType, "wpa");
								else if (3 == atoi(WifiAuthmode))
									strcpy(cSafeType, "wpa2");

								if (1 == atoi(WifiEnctype))
									strcpy(cEnctyType, "NONE");
								else if (2 == atoi(WifiEnctype))
									strcpy(cEnctyType, "TKIP");
								else if (3 == atoi(WifiEnctype))
									strcpy(cEnctyType, "AES");

								HK_DEBUG_PRT("----->Phone Set AP to Wifi: cSsid:%s, cSafeType:%s, cEnctyType:%s <-----\n", cSsid, cSafeType, cEnctyType);
								break;
							}
							wificount++;
						}
						fclose( fprd ); //close wifilist.conf.
						HK_DEBUG_PRT("+++++++++++++++++ phone setting wifi connection end. +++++++++++++++++\n");
					}
#endif

					if ((0 == strcmp(g_cSid, cSsid)) && (0 == strcmp(g_cPwd, cPasswd)))
					{
						DestroyFrame(hf);
						return 0;
					}
					strcpy(g_cSid, cSsid);
					strcpy(g_cPwd, cPasswd);

					if (strcmp(cBootProto, HK_NET_BOOTP_STATIC) == 0) //static ip.
					{
						char *cIP   = DictGetStr( hf, HK_KEY_IP);
						char *cMask = DictGetStr( hf, HK_KEY_NETMASK);
						char *cWet  = DictGetStr( hf, HK_KEY_NETWET);
						char *cDns0 = DictGetStr( hf, HK_NET_DNS0);
						char *cDns1 = DictGetStr( hf, HK_NET_DNS1);
						conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "FixedIP");
						conf_set(HOME_DIR"/wifinet.cfg", "IPAddress", cIP );
						conf_set(HOME_DIR"/wifinet.cfg", "SubnetMask", cMask );
						conf_set(HOME_DIR"/wifinet.cfg", "Gateway", cWet );
						conf_set(HOME_DIR"/wifinet.cfg", "DNSIPAddress1", cDns0 );
						conf_set(HOME_DIR"/wifinet.cfg", "DNSIPAddress2", cDns1 );

						conf_set(HOME_DIR"/net.cfg", "IPAddressMode", "FixedIP");
						conf_set(HOME_DIR"/net.cfg", "IPAddress", cIP);
						conf_set(HOME_DIR"/net.cfg", "DNSIPAddress1", cDns0);
						conf_set(HOME_DIR"/net.cfg", "DNSIPAddress2", cDns1);
						conf_set(HOME_DIR"/net.cfg", "Gateway", cWet);
						conf_set(HOME_DIR"/net.cfg", "SubnetMask", cMask);
					}
					else //dhcp.
					{
						conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "DHCP");
						conf_set(HOME_DIR"/net.cfg", "IPAddressMode", "DHCP");
					}

					if( cEnctyType != NULL )
					{
						conf_set(HOME_DIR"/wifinet.cfg", HK_WIFI_ENCRYTYPE, cEnctyType );
					}
					conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_SID, cSsid);

					if( cSafeType != NULL )
					{
						conf_set(HOME_DIR"/wifinet.cfg", HK_WIFI_SAFETYPE, cSafeType);
					}

					if( cSafeOption != NULL )
					{
						conf_set(HOME_DIR"/wifinet.cfg", HK_WIFI_SAFEOPTION, cSafeOption);
					}

					if( cKeyType != NULL )
					{
						conf_set(HOME_DIR"/wifinet.cfg", HK_WIFI_KEYTYPE, cKeyType);
					}

					conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD, cDecPswd );

					/*set 'ssid' & 'psk' for wifi configurate*/
					ssid = add_format(cSsid);
					psk = add_format(cDecPswd);
					if (0 == strcmp(cSafeType, "none")) //authentication:NONE
					{
						conf_set(HOME_DIR"/open.conf", "ssid", ssid);
					}
					else if (0 == strcmp(cSafeType, "wep")) //authentication:WEP
					{
						conf_set(HOME_DIR"/wep.conf", "ssid", ssid);
						conf_set(HOME_DIR"/wep.conf", "wep_key0", psk);
						conf_set(HOME_DIR"/wep.conf", "wep_key1", psk);
						conf_set(HOME_DIR"/wep.conf", "wep_key2", psk);
						conf_set("/etc/Wireless/RT2870STA/RT2870STA.dat", "SSID", cSsid);
						conf_set("/etc/Wireless/RT2870STA/RT2870STA.dat", "WPAPSK", cDecPswd);
					}
					else //authentication:WPA-PSK or WPA2-PSK
					{
						if (strlen(psk) < 8)
						{
							printf("============> Note: wifi password is too short, should more than 8bits for WPA <============\n");
							conf_set(HOME_DIR"/wpa_supplicant.conf", "psk", "\"1234567890\"");
						}
						else
						{
							conf_set(HOME_DIR"/wpa_supplicant.conf", "psk", psk);
							conf_set(HOME_DIR"/wpa_supplicant.conf", "ssid", ssid);
							conf_set("/etc/Wireless/RT2870STA/RT2870STA.dat", "SSID", cSsid);
							conf_set("/etc/Wireless/RT2870STA/RT2870STA.dat", "WPAPSK", cDecPswd);
						}
					}
					free(ssid);
					free(psk);

					sleep(1);
					wrap_sys_restart();
				}
				else //close wifi, set 'isopen' to '0' in wifinet.cfg (2015-03-13).
				{
					conf_set(HOME_DIR"/wifinet.cfg", HK_WIFI_SAFETYPE, "auto");
					//conf_set(HOME_DIR"/wpa_supplicant.conf", "psk", "\"1234567890\"");
					//conf_set(HOME_DIR"/wpa_supplicant.conf", "ssid", "\"1234567890\"");
					system("cp -f /etc/wifiConf/* /mnt/sif/");
					printf("......%s...cp wifiConf...\n", __func__);
					conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD,"" );
					conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_SID, "");

					gTWifiStatus = 0;
					wrap_sys_restart();
				}
			}
		}
	}
	else if ( strcmp(mc, "703")==0 )
	{
		//printf("search AP list! mac = %s\n", mc);
		const char* mac = GetParamStr(hf, HK_KEY_HWADDR);
		if (mac)
		{
			if (isMacMatch( mac ))
			{
				unsigned int ulParam = GetParamUN(hf, HK_KEY_UIPARAM );
				HKFrameHead *hfwi = CreateFrameB();

				char s_LocalIPAddr[16] = {0};
				static REMOTE_WIFI_FIND wifiFind;
				memset(&wifiFind, '\0', sizeof(wifiFind));

				/**check AP mode for phone connect**/
				if ((!getLocalAddress("ra0", s_LocalIPAddr)) && (strlen(s_LocalIPAddr) > 0) && (0 == strcmp(s_LocalIPAddr, IPADDR_APMODE)))
				{
					HK_DEBUG_PRT("...current status is AP mode...local ip address: %s, len:%d\n", s_LocalIPAddr, strlen(s_LocalIPAddr)); 

					int s_num = 0; //index the number of wifi node.
					char WifiListBuf[256] = {0};
					char tmpKey[32] = {0}, WifiSsid[SSID_LEN] = {0}, WifiBssid[BSSID_LEN] = {0};
					char WifiSignal[8] = {0}, WifiAuthmode[8] = {0}, WifiEnctype[8] = {0};
					FILE *fprd = fopen( CONF_WIFILIST, "r" );
					if( NULL == fprd )
					{
						fprintf(stderr, "fopen error with:%d, %s\n", errno, strerror(errno));
						return errno;
					}

					fgets(WifiListBuf, sizeof(WifiListBuf), fprd); //first line: wifi count.
					WifiListBuf[strlen(WifiListBuf) - 1] = '\0'; //skip '\n' at the end of line.
					//HK_DEBUG_PRT("------> WifiListBuf: %s <--------\n", WifiListBuf);
					sscanf(WifiListBuf, "%[^=]=%d", tmpKey, &wifiFind.count);
					HK_DEBUG_PRT("------> tmpKey:%s, wifiFind.count = %d <--------\n", tmpKey, wifiFind.count);

					// get wifi list data.
					while( fgets( WifiListBuf, sizeof(WifiListBuf), fprd ) )
					{
						WifiListBuf[strlen(WifiListBuf) - 1] = '\0'; //skip '\n' at the end of line.
						sscanf(WifiListBuf, "%s %s %s %s",  WifiSsid, WifiSignal, WifiAuthmode, WifiEnctype);

						strcpy(wifiFind.wifi_info[s_num].ssid, WifiSsid);
						wifiFind.wifi_info[s_num].authmode = atoi(WifiAuthmode);
						wifiFind.wifi_info[s_num].enctype = atoi(WifiEnctype);
						wifiFind.wifi_info[s_num].iSignal = atoi(WifiSignal);

						//HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%d\t  enctype=%d\t iSingal=%d\n", s_num, wifiFind.wifi_info[s_num].ssid, wifiFind.wifi_info[s_num].authmode, wifiFind.wifi_info[s_num].enctype, wifiFind.wifi_info[s_num].iSignal);
						s_num++;
					}
					//HK_DEBUG_PRT("------> wifi total num: %d <--------\n", s_num);
					fclose( fprd ); //close wifilist.conf.

					int i = 0;
					//int iCount = wifiFind.count;
					int iCount = (wifiFind.count > 20) ? 20 : wifiFind.count;
					HK_DEBUG_PRT("--------> wifi total num: %d, iCount:%d <--------\n", s_num, iCount);
					SetParamUN( hfwi, HK_KEY_COUNT, iCount );

					//for (i = 0; i < iCount; i++)
					//{
					//    HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%d\t  enctype=%d\t iSingal=%d\n",
					//            i, wifiFind.wifi_info[i].ssid, wifiFind.wifi_info[i].authmode, wifiFind.wifi_info[i].enctype, wifiFind.wifi_info[i].iSignal);
					//}

					for (i = 0; i < iCount; i++)
					{
						char aryKey[64] = {0};
						char *cSsid = wifiFind.wifi_info[i].ssid;
						sprintf( aryKey, HK_KEY_VALUEN"%d", i );
						SetParamStr( hfwi, aryKey, cSsid );

						char *cAuthMode = authMode[wifiFind.wifi_info[i].authmode];
						char *cEncType = encType[wifiFind.wifi_info[i].enctype];
						sprintf( aryKey, HK_KEY_WIFIDATAENC"%d", i );
						SetParamStr( hfwi, aryKey, cEncType );

						sprintf( aryKey, HK_KEY_WIFISAFETYPE"%d", i );
						SetParamStr( hfwi, aryKey, cAuthMode );

						int iSignal = wifiFind.wifi_info[i].iSignal;
						sprintf( aryKey, "WifiSigan%d", i );
						SetParamUN( hfwi, aryKey, iSignal );

						HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%s\t  enctype=%s\t iSingal=%d\n", i, cSsid, cAuthMode, cEncType, iSignal);

						//sprintf(value+strlen(value), "%s""%s""%s""%s""%s\r\n\r\n",cSsid,"   ", cEncType,"   ", cAuthMode);
					}
					//SetParamStr( hfwi,HK_WIFI_COMMENT,value );

					int iPacketLen = GetFrameSize( hfwi );
					char *cBuf = malloc( iPacketLen );
					if (NULL != cBuf)
					{
						GetFramePacketBuf( hfwi, cBuf, iPacketLen );
						HK_DEBUG_PRT("cBuf: %s\n", cBuf);

						be_present2( iPacketLen, cBuf, "LocalWifi", ulParam );
						free( cBuf );
						cBuf = NULL;
					}
				}
				else //PC client scan wifi node.
				{
					if ( ScanWifiInfo(&wifiFind) ) //0:success, errno:failed.
					{
						SetParamUN( hfwi, HK_KEY_COUNT, 0 );

						int iPacketLen = GetFrameSize( hfwi );
						char *cBuf = malloc( iPacketLen );
						if (NULL != cBuf)
						{
							GetFramePacketBuf( hfwi, cBuf, iPacketLen );

							be_present2( iPacketLen, cBuf, "LocalWifi", ulParam );
							free( cBuf );
							cBuf = NULL;
						}
					}
					else //search wifi success.
					{
						int i = 0;
						//HK_DEBUG_PRT("search wifi count = %d\n", wifiFind.count);
						//for (i = 0; i < wifiFind.count; i++)
						//{
						//    HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%d\t  enctype=%d\t iSingal=%d\n",
						//            i, wifiFind.wifi_info[i].ssid, wifiFind.wifi_info[i].authmode, wifiFind.wifi_info[i].enctype, wifiFind.wifi_info[i].iSignal);
						//}

						if (wifiFind.count > 0)
						{
							if (Sort_WifiInfo(&wifiFind))
							{
								printf(".........sort wifi info failed.........\n"); 
							}
						}

						int iCount = (wifiFind.count > 20) ? 20 : wifiFind.count;
						HK_DEBUG_PRT("search wifi count: %d, iCount:%d\n", wifiFind.count, iCount);
						SetParamUN( hfwi, HK_KEY_COUNT, iCount );

						for (i = 0; i < iCount; i++)
						{
							char aryKey[64] = {0};
							char *cSsid = wifiFind.wifi_info[i].ssid;
							sprintf( aryKey, HK_KEY_VALUEN"%d", i );
							SetParamStr( hfwi, aryKey, cSsid );

							char *cAuthMode = authMode[wifiFind.wifi_info[i].authmode];
							char *cEncType = encType[wifiFind.wifi_info[i].enctype];
							sprintf( aryKey, HK_KEY_WIFIDATAENC"%d", i );
							SetParamStr( hfwi, aryKey, cEncType );

							sprintf( aryKey,HK_KEY_WIFISAFETYPE"%d", i );
							SetParamStr( hfwi,aryKey, cAuthMode );

							int iSignal = wifiFind.wifi_info[i].iSignal;
							sprintf( aryKey, "WifiSigan%d", i );
							SetParamUN( hfwi, aryKey, iSignal );

							HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%s\t  enctype=%s\t iSingal=%d\n", \
									i, cSsid, cAuthMode, cEncType, iSignal);

							//sprintf(value+strlen(value), "%s""%s""%s""%s""%s\r\n\r\n",cSsid,"   ", cEncType,"   ", cAuthMode);
						}
						//SetParamStr( hfwi,HK_WIFI_COMMENT,value );

						int iPacketLen = GetFrameSize( hfwi );
						char *cBuf = malloc( iPacketLen );
						if (NULL != cBuf)
						{
							GetFramePacketBuf( hfwi, cBuf, iPacketLen );

							be_present2( iPacketLen, cBuf, "LocalWifi", ulParam );
							free( cBuf );
							cBuf = NULL;
						}
					}
				}
				DestroyFrame( hfwi );
			}
		}
	} 
	else if (strcmp(mc, "updateDevPas") == 0)
	{
		int len;
		char res[256];
		const char* secret = GetParamStr(hf, HK_KEY_PASSWORD);
		const char* ckie = GetParamStr(hf, HK_KEY_UIPARAM);
		len = sprintf(res, HK_KEY_MAINCMD"=updateDevPas;"HK_KEY_UIPARAM"=%s;"HK_KEY_FLAG"=1;", ckie);
		if (!quit_)
		{
			NetSendLan(hkid, asc, res, len);
		}
		setenv("LAN_PASSWD", secret, 1);
		conf_set(HOME_DIR"/hkclient.conf", "LAN_PASSWD", secret); // set_config("hkclient.conf", "LAN_PASSWD", secret);
		//write_configh("LAN_PASSWD", secret);
		be_present(2);
	}
	else if (strcmp(mc, "ResetSystem") == 0 )
	{
		char * cMac = GetParamStr( hf, HK_KEY_MACIP );
		if( cMac )
		{
			if(isMacMatch( cMac ))
			{
				printf("scc...ResetSystem\n");
				wrap_sys_restart();
			}
		}

	}
	else if (strcmp(mc, "222")==0 )
	{
		char * cMac = GetParamStr(hf,HK_KEY_MACIP );
		if( cMac )
		{
			if(isMacMatch( cMac ))
			{
				int nClose = GetParamUN(hf, HK_KEY_DEVPARAM);
				int nPort = GetParamUN(hf,HK_KEY_LANPORT );
				if ( nPort > 0 )
					write_configh("LANPORT", nPort);

				write_configh("WANENABLE", nClose);

				unsigned int iTime = time(0);
				conf_set_int(HOME_DIR"/time.conf", "time_", iTime);

				//printf("...222......\n");
				wrap_sys_restart();
			}
		}
	}
	else if (strcmp(mc, "Resp"HK_IPC_INITIAL) == 0)
	{
		int iFlag = GetParamUN( hf, HK_KEY_FLAG);
		char *cOldDevid = GetParamStr( hf, HK_KEY_HWADDR );
		if( NULL != cOldDevid && NULL!=user_  && !g_bupdatemac && strcmp(cOldDevid, user_)==0)
		{
			g_bupdatemac = true;
			if( iFlag == 2 )//update id or mac
			{
				//printf("..update id or mac,devid=%s.\n", cOldDevid );
				on_resp_update_id(hf);
				wrap_sys_restart();
			}
			else if( iFlag == 1 )//update connect or mac
			{
				//printf("..update connect or mac.devid=%s.\n", cOldDevid );
				on_resp_update_connect(hf);
				wrap_sys_restart();
			}
		}
		if (req_ == HK_IPC_INITIAL)
		{
			on_resp_reqinit1(hf);
			wrap_sys_restart();
		}
	}
	else if (strcmp(mc, "Resp"HK_SYNC_TIME) == 0)
	{
		if (req_ == HK_SYNC_TIME)
		{
			HKLOG(L_DBG,"%s: %u/%d, %.*s", __FUNCTION__, hkid,asc, len, buf);
			on_resp_sync_time(hf);
		}
	}
	else
	{
		HKLOG(L_DBG,"%s: %u/%d, %.*s", __FUNCTION__, hkid,asc, len, buf);
	}
	DestroyFrame(hf);
	return 0;
}

static void init_fifo_ctl(const char* fn)
{
	int fd;
	mkfifo(fn, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if ( (fd = open(fn, O_RDONLY|O_NONBLOCK)) < 0)
	{
		fprintf(stderr, "open RDONLY|O_NONBLOCK: %s", fn);
		return;
	}
	//tq_post_v(tq_, 1000*10, checkctl, (char*)0 + fd, NULL);
}

#if 0
static void ppsignal(int sig)
{
	HKLOG(L_INFO, "signal %u: %s", watcher_pid_, sig==SIGCONT?"CONT":(sig==SIGUSR2?"USR2":"USR1"));
	if (watcher_pid_)
		kill(watcher_pid_, sig);
}
#endif
#define FLASH_BLK "/dev/mtdblock3"

typedef struct {
	unsigned char eth0_mac[20];
	unsigned char eth0_ip[20];
	unsigned char eth0_mask[20];
	unsigned char eth1_mac[20];
	unsigned char eth1_ip[20];
	unsigned char eth1_mask[20];
	unsigned int  super_key;
	unsigned int  user_key[10];
}tMacKey;

const tMacKey* mtd_write_raw(const tMacKey *buf)
{
	int     fd;
	if((fd = open(FLASH_BLK, O_WRONLY)) < 0)
	{
		perror("open error");
		return 0;
	}
	if(write(fd, buf, sizeof(*buf)) < sizeof(*buf))
	{
		perror("write error");
		close(fd);
		return 0;
	}
	fsync(fd);
	close(fd);
	return buf;
}

tMacKey* mtd_read_raw(tMacKey* buf)
{
	int    fd;
	memset(buf, 0, sizeof(*buf));
	if( (fd = open(FLASH_BLK, O_RDONLY)) < 0)
	{
		return 0;
	}
	if(read(fd, (void*)buf, sizeof(*buf)) < sizeof(*buf))
	{
		close(fd);
		return 0;
	}
	close(fd);
	return buf;
}

const char* get_if_hwaddr(char mac[18])
{
	tMacKey mk;
	if (mtd_read_raw(&mk))
		return strcpy(mac, mk.eth0_mac);
	return NULL;
}

static void lancast(const char* fmt, ...)
{
	int len;
	char buf[960];
	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(buf,sizeof(buf), fmt, ap);
	va_end(ap);
	if (len > 0 && len < sizeof(buf))
	{
		Dict* d = DictCreate(buf, len);
		len = DictEncode(buf,sizeof(buf),d);
		DictDestroy(d);
		HKLOG(L_DBG, "Length=%d: %s", len, buf);
		NetSendLan(0, 0, buf, len);
	}
}

static void req_init1(int x, void* a, int ac, char* av[])
{
	if (x > 0)
		return;
	if (req_ == HK_IPC_INITIAL)
	{
		lancast(HK_KEY_MAINCMD"=%s;"HK_KEY_HWADDR"=%s;", "Req"HK_IPC_INITIAL, "abc");
		tq_post_v(tq_, 5000, req_init1, 0, NULL);
	}
}

static int first_run_check(struct timedq *tq, volatile int* pcnt)
{
	if (strlen(getEnv("USER", "")) == 0)
	{
		//char umac[28] = {0};
		//GetLocalMac( &umac );

		//if (strncasecmp(umac, UNDEF_MAC_ADDRESS, 17)==0)
		//{
		req_ = HK_IPC_INITIAL;
		tq_post_v(tq, 0, req_init1, 0, NULL);
		return 0;
		//}
	}
	return 0;
}

/*
   static void daily_reset(int x, void* a, int ac, char* av[])
   {
   struct tm tm;
   time_t tsec = time(0);
   if (x > 0)
   return;
   localtime_r(&tsec, &tm);
   if( tsec - gResetTime > 60*60*96 )
   {
   wrap_sys_restart();
   return;
   }
   x = tsec;
   tq_post_v(tq_, 1000*60 * (rand_r(&x)%11+5), daily_reset, a, NULL);
   }
 */

/*
   static void checkup(int x, void* a, int ac, char* av[])
   {
   if (x > 0)
   return;
   if (watcher_pid_ > 0)
   {
//int idle = is_free_time();
//ppsignal(idle ? SIGUSR1 : SIGUSR2);
tq_post_v(tq_, 1000*100, checkup, 0, NULL);
return;
}
if (ac == 0)
{
SET_EXCODE(Excode_Update);
}
else
{
x = (char*)a++ - (char*)0;
tq_post_v(tq_, 1000*(x<18?10:60), checkup, a, av[0], NULL);
}
}
 */


/*******************************************************
 * func: get configuration params & init HKEMAIL_T.
 ******************************************************/
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
	hkSdParam.sdMoveOpen   = conf_get_int( HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM );

	HK_DEBUG_PRT("---> moveRec:%d, outMoveRec:%d, autoRec:%d, loopWrite:%d, splite:%d, audio:%d, sdrecqc:%d, sdIoOpen:%d, sdError:%d, sdMoveOpen:%d <---\n", hkSdParam.moveRec, hkSdParam.outMoveRec, hkSdParam.autoRec, hkSdParam.loopWrite, hkSdParam.splite, hkSdParam.audio, hkSdParam.sdrecqc, hkSdParam.sdIoOpen, hkSdParam.sdError, hkSdParam.sdMoveOpen);
}

//check MMC module.
static int CheckSDStatus()
{
	struct stat st;
	if (0 == stat("/dev/mmcblk0", &st))
	{
		if (0 == stat("/dev/mmcblk0p1", &st))
		{
			printf("...load TF card success...\n");
			return 1;
		}
		else
		{
			printf("...load TF card failed...\n");
			return 2;
		}
	}

	return 0;
}

/*******************************************
 * func: calculate SD card storage size.
 ******************************************/
int GetStorageInfo()
{
	struct statfs statFS;
	char *MountPoint = "/mnt/mmc/";

	if (statfs(MountPoint, &statFS) == -1)
	{  
		printf("error, statfs failed !\n");
		return -1;
	}

	hkSdParam.allSize   = ((statFS.f_blocks/1024)*(statFS.f_bsize/1024));
	hkSdParam.leftSize  = (statFS.f_bfree/1024)*(statFS.f_bsize/1024); 
	hkSdParam.haveUse   = hkSdParam.allSize - hkSdParam.leftSize;
	//HK_DEBUG_PRT("......SD totalsize=%ld...freesize=%ld...usedsize=%ld......\n", hkSdParam.allSize, hkSdParam.leftSize, hkSdParam.haveUse);

	return 0;
}


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

static int g_OpenSd=0;
static bool b_OpenSd=true;
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

	groupnum = 7;
	bitnum   = 7;  //GPIO:7_7 ==> IO alarm Out.
	val_set  = 0;  //pull down.
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);


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


/*******************************************************
 * func: reset wireless modules while system restart.
 *******************************************************/
int hk_WirelessCard_Reset(void)
{ 
	unsigned int groupnum = 0;
	unsigned int bitnum   = 0;
	unsigned int val_set = 0;
	short nRet = 0;
	unsigned int val_read_old = 0;

	/**read gpio5_1 status**/
	groupnum = 5;
	bitnum   = 1; //GPIO:5_1.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read_old );
	if (nRet)   return -1;
	HK_DEBUG_PRT("...Get GPIO %d_%d  read Value: %d...\n", groupnum, bitnum, val_read_old);

	val_set = 0; //pull down to reset wireless modules.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	nRet = Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	if (nRet)   return -1;
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);

	sleep(2);

	val_set = 1; //pull up.
	nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
	nRet = Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	if (nRet)   return -1;
	HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n\n", groupnum, bitnum, val_set);

	return 0;
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
		else if (1 == nboardtype) //ircut light board type: level 1.
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
	}
	return nRet;
}


//vbVideo
void hk_IOAlarm()
{
	Hi_SetGpio_SetDir( g_AlarmOut_grp, g_AlarmOut_bit, GPIO_WRITE );
	Hi_SetGpio_SetBit( g_AlarmOut_grp, g_AlarmOut_bit, 1 ); //pull up.
	sleep(2);
	Hi_SetGpio_SetDir( g_AlarmOut_grp, g_AlarmOut_bit, GPIO_WRITE );
	Hi_SetGpio_SetBit( g_AlarmOut_grp, g_AlarmOut_bit, 0 ); //pull down.
}


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


#define ETHTOOL_GLINK 0x0000000a
#define SIOCETHTOOL 0x8946
static int GetNetStat( )
{
	char    buffer[BUFSIZ];
	FILE    *read_fp;
	int      chars_read;
	int      ret;

	memset( buffer, 0, BUFSIZ );
	read_fp = popen("ifconfig eth0 | grep RUNNING", "r");
	if ( read_fp != NULL ) 
	{
		chars_read = fread(buffer, sizeof(char), BUFSIZ-1, read_fp);
		if (chars_read > 0) 
		{
			ret = -1;
		}
		else
		{
			ret = 1;
		}
		pclose(read_fp);
	}
	else
	{
		ret = 1;
	}
	return ret;
}

static int tatus()
{
	struct ifreq ifr;
	struct ethtool_value edata;
	memset(&ifr, 0, sizeof(struct ifreq));

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0) 
		return 1;
	strcpy(ifr.ifr_name, "eth0");

	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	ifr.ifr_data = (caddr_t)&edata;

	if(ioctl( sock, SIOCETHTOOL, &ifr ) >= 0)
	{
		if( !edata.data  )
		{
			close(sock);
			return 1;
		}
	}
	close(sock);
	return -1;
}

void hk_set_system_time()
{
	int tz = conf_get_int(HOME_DIR"/time.conf", "zone");
	unsigned int t = conf_get_int(HOME_DIR"/time.conf", "time_");
	if (t > 0)
	{
		unsigned int seconds = tz+t;
		//unsigned int seconds = t;
		struct timespec ts;
		ts.tv_sec = seconds;
		ts.tv_nsec = 0;
		clock_settime(CLOCK_REALTIME, &ts);
	}

	gResetTime = time(0);
}

static void CheckNetDevCfg()
{
	//printf("...zzzzzzzzzzzzzzzzzzzzzzz reboot 333333 zzzzzzzzzzzzzzzzzzzzzz...\n");
	char aryDHCP[64]={0};
	if( conf_get( HOME_DIR"/net.cfg", "IPAddressMode", aryDHCP, 64 ) == NULL || 
			( strcmp(aryDHCP,"DHCP")!=0 && strcmp(aryDHCP,"FixedIP")!=0 ) )
	{
		system("umount /mnt/mmc/");
		conf_set(HOME_DIR"/net.cfg","IPAddressMode","DHCP" );
		system("sync");
		system("reboot");
	}
	if( strcmp(aryDHCP,"FixedIP") == 0 )
	{
		if( conf_get( HOME_DIR"/net.cfg", "IPAddress", eth0Addr.ip, 64 ) == NULL ||
				conf_get( HOME_DIR"/net.cfg", "Gateway", eth0Addr.gateway, 64 ) == NULL ||
				conf_get( HOME_DIR"/net.cfg", "SubnetMask", eth0Addr.netmask,64 ) == NULL )
		{
			conf_set(HOME_DIR"/net.cfg","IPAddressMode","DHCP" );
			system("umount /mnt/mmc/");
			system("sync");
			system("reboot");
		}
		unsigned long  nIP      = inet_addr( eth0Addr.ip );
		unsigned long  nMask    = inet_addr( eth0Addr.netmask );
		unsigned long  nGW      = inet_addr( eth0Addr.gateway );
		if( (nGW&nMask)!=(nIP&nMask) )
		{
			system( "route add default dev eth0" );
		}
	}
}

static void GetWifiSdi()
{
	int isSid = conf_get_int(HOME_DIR"/wifinet.cfg", "sid");
	if( isSid == 0 )
	{
		system("touch /mnt/sif/wifisid.conf");

		conf_set_int(HOME_DIR"/wifinet.cfg", "sid", 1);
		char bufValue[64]={0};
		conf_get_space( HOME_DIR"/wifinet.cfg", HK_WIFI_SID, bufValue, 64);
		conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_SID, bufValue);

		conf_get_space(HOME_DIR"/wifinet.cfg", HK_WIFI_PASSWORD, bufValue, 64);
		conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD, bufValue);
	}
}

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

/*
   void hk_close_osd()
   {
   HI_MPI_VPP_DestroyRegion(handle[0]);
   }


   void hk_init_osd(int flip, int ciford1)
   {
   if(hk_init()==0)
   {
   printf("......numbers init fail\n");
   }
   else
   {
   if(HI_SUCCESS != SampleCtrl4OverlayRegion(  ) )
   {
   printf("....init OSD fail...\n");
   }
   }

   }
 */



static int hk_check_tvp5150()
{
	struct stat st;
	if (stat("/dev/misc/tvp5150dev", &st) == 0)
	{
		return 1;
	}
	return 0;
}

static int hk_check_99141()
{
	struct stat st;
	if (stat("/dev/misc/nt99141", &st) == 0)
	{
		return 1;
	}
	return 0;
}

static int hk_check_0v2643()
{
	struct stat st;
	if (stat("/dev/misc/ov2643", &st) == 0)
	{
		//printf("ov2643...\n");
		return 1;
	}
	return 0;
}

static int hk_check_0v7725()
{
	struct stat st;
	if (stat("/dev/misc/ov7725", &st) == 0)
	{
		//printf("ov2643...\n");
		return 1;
	}
	return 0;
}


/**********************************************
 * func: check SD insert status;
 *       check SD storage info;
 *       SD data FTP backup;
 *       SD data operation for client.
 *********************************************/
static void hk_load_sd()
{
	g_sdIsOnline = CheckSDStatus();
	if (g_sdIsOnline == 1) //index sd card inserted.
	{
		mkdir("/mnt/mmc", 0755);
		system("umount /mnt/mmc/");
		usleep(1000);
		system("mount -t vfat /dev/mmcblk0p1 /mnt/mmc/"); //mount SD.

		GetStorageInfo();

		int ftpIsOpen = conf_get_int( HOME_DIR"/ftpbakup.conf", HK_WIFI_OPENORCLOSE );
		if (ftpIsOpen == 1) //ftp service enable.
		{
			hk_start_ftp_server();
		}
	}
	else if(g_sdIsOnline == 2)
	{
		GetStorageInfo();
		hkSdParam.allSize = 14;
	}
	HK_DEBUG_PRT("......SD info: g_sdIsOnline:%d, totalsize=%ld...freesize=%ld...usedsize=%ld......\n", g_sdIsOnline, hkSdParam.allSize, hkSdParam.leftSize, hkSdParam.haveUse);

	if (1 == g_sdIsOnline)
	{
		printf("...........Check SD Upgrade & Device License............\n");
		HK_Check_SD_Upgrade(g_sdIsOnline); //system upgrade.

		HK_Check_SD_License(g_sdIsOnline); //conf device lincense and upgrade url, and so on.
	}

	SD_RSLoadObjects( &SysRegisterDev );
}


static void hk_load_pppoe()
{
	int isOpen = conf_get_int(HOME_DIR"/hkpppoe.conf", HK_KEY_ISALERT_SENSE);
	if(isOpen==1)//Open pppoe
	{
		system("/mnt/sif/bin/hkpppoe &");
	}
}

static int sccUpdateServerAdd()
{
	if( user_ != NULL && host_ !=NULL )
	{
		char tempUser[64]={0};
		strcpy(tempUser,user_);
		const char *p_temp = user_;
		char V02 = tempUser[0];
		char J02 = tempUser[1];
		if( V02 == 'V' && J02 == 'C' )
		{
			if( strcmp(host_,"server.viewcam.asia") != 0 )
			{
				conf_set(HOME_DIR"/hkclient.conf", "PROXY", "server.viewcam.asia");
			}
			return;
		}

		if( strcmp(host_,"camera.vije.co.kr") != 0 )
		{
			if( V02 == 'V' && J02 == 'J' )
			{
				conf_set(HOME_DIR"/hkclient.conf", "PROXY", "camera.vije.co.kr");
				return;
			}
			int tz = conf_get_int(HOME_DIR"/time.conf", "zone");
			if( tz == 28800 )//zh
			{
				if( strcmp(host_,"www.uipcam.com")==0 )
				{
					conf_set(HOME_DIR"/hkclient.conf", "PROXY", "www.scc21.com");
				}
			}
			else
			{
				if( strcmp(host_,"www.scc21.com")==0 )
				{
					conf_set(HOME_DIR"/hkclient.conf", "PROXY", "www.uipcam.com");
				}
			}
		}
		else if( p_temp!=NULL)
		{
			if( strcmp(host_,"camera.vije.co.kr") == 0)
			{
				char cUpdateUrl[256]={0};
				conf_get( HOME_DIR"/hkclient.conf", "UpdateUrl", cUpdateUrl, 256 );
				if( strcmp( cUpdateUrl, "http://www.savitmicro.co.kr/PnP/firmware/new/h3518e.txt" )!=0 )
				{
					conf_set(HOME_DIR"/hkclient.conf", "UpdateUrl", "http://www.savitmicro.co.kr/PnP/firmware/new/h3518e.txt" );
				}
				g_DevVer=conf_get_int(HOME_DIR"/hkclient.conf", "VERSION" );
			}
			else
				g_DevVer=9;
		}
	}
}


#if ENABLE_ONVIF
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

	//ONVIF协议初始化
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


int main(int argc, char* argv[])
{  
	unsigned long tmStartDDNS  =  0;
	unsigned long tmSTopDDNS   =  0;
	int DdnsTimeInterval = 0;
	int IRCutBoardType = 0;
	int threq = 0;

	//hk_load_pppoe();
	CheckNetDevCfg();
	init_conf(); //create system configurate file.
	hk_set_system_time(); //update device time.

	int counter = 0;
	if (argc >= 3)
	{
		user_ = argv[1];
		passwd_ = argv[2];
		if (argc >= 5)
		{
			host_ = argv[3];
			port_ = argv[4];
		}
	}
	else
	{
		//return 1;
	}
	Daemonize();
	install_sighandler(sig_handler);

	char cSensorType[32]={0};
	conf_get( HOME_DIR"/sensor.conf", "sensortype", cSensorType, 32 );
	if (strcmp(cSensorType, "ar0130") == 0)
	{
		printf("...scc...ar0130......\n");
		g_HK_SensorType = HK_AR0130; //ar0130.
	}
	else if (strcmp(cSensorType, "ov9712d") == 0)
	{
		printf("...scc...ov9712d......\n");
		g_HK_SensorType = HK_OV9712; //ov9712d.
	}
	else
	{
		printf("...scc...unknown sensor type, use default: ov9712d lib......\n");  
		g_HK_SensorType = HK_OV9712; //ov9712d.
	}
	g_isWifiInit       = conf_get_int(HOME_DIR"/wifinet.cfg", "isopen");
	g_HK_VideoResoType = conf_get_int(HOME_DIR"/hkipc.conf", "HKVIDEOTYPE");
	g_DevIndex         = conf_get_int(HOME_DIR"/hkclient.conf", "IndexID"); 
	g_isWanEnable      = conf_get_int(HOME_DIR"/hkclient.conf", "WANENABLE");
	g_lanPort          = conf_get_int(HOME_DIR"/hkclient.conf", "LANPORT");
	g_irOpen           = conf_get_int(HOME_DIR"/hkipc.conf", "iropen");
	g_onePtz           = conf_get_int(HOME_DIR"/hkipc.conf", "oneptz");
	g_DevPTZ           = conf_get_int(HOME_DIR"/ptz.conf", "HKDEVPTZ");
	DdnsTimeInterval   = conf_get_int("/mnt/sif/web.conf", "DdnsTimeInterval");
	IRCutBoardType     = conf_get_int("/mnt/sif/hkipc.conf", "IRCutBoardType");
	HK_DEBUG_PRT("g_isWifiInit:%d, g_HK_SensorType:%d, g_HK_VideoResoType:%d, g_DevIndex:%d, g_isWanEnable:%d, g_lanPort:%d, g_irOpen:%d, g_onePtz:%d, g_DevPTZ:%d, DdnsTimeInterval:%d, IRCutBoardType:%d\n", \
	g_isWifiInit, g_HK_SensorType, g_HK_VideoResoType, g_DevIndex, g_isWanEnable, \
	g_lanPort, g_irOpen, g_onePtz, g_DevPTZ, DdnsTimeInterval, IRCutBoardType);

	/**** init video Sub System. ****/
	if ( HI_SUCCESS != Video_SubSystem_Init() )
	{
		printf("[%s, %d] video sub system init failed !\n", __func__, __LINE__); 
	}
	HK_DEBUG_PRT("video sub system init OK!\n");

	/**GPIO init**/
	HI_SetGpio_Open();
	initGPIO();

	setpidfile(getenv("PIDFILE"), getpid());
	if (getenv("wppid"))
	{
		watcher_pid_ = atoi(getenv("wppid"));
	}

	atoi(getEnv("LogBackground","1")) ? LOG_Background() : LOG_Foreground();
	LOG_SetLevel(atoi(getEnv("LogLevel", "0")));

	//tq_ = tq_create();
	//SysInit(&cb_init);
	//SysRegistASLan_0(LOCAL_ASC, 0, &cb_lan);
	//first_run_check(tq_, &counter);
	//sleep(1);

	GetAlarmEmailInfo(); //get email configuration info
	GetSdAlarmParam(); //get sd card configuration info.

	/**video callbacks for client operations**/
	//video_RSLoadObjects( &SysRegisterDev );

	/**audio callbacks for client operations**/
	//audio_RSLoadObjects( &SysRegisterDev );

	hk_load_sd(); //mount sd card.

	//mpeg_.tq = tq_;
	//LanNetworking(1);
	//monc_start(NULL, HK_PASSWD_FAIL); 
	//if (g_isWanEnable != 1)
	//{
		//printf("...zzz...%s...g_isWanEnable:%d...\n", __func__, g_isWanEnable);
	//	start_nonblock_login();
	//}

#if (HK_PLATFORM_HI3518E)
	/*****neck Cruise*****/
	if (1 == g_DevPTZ) //0:device without PTZ motor; 1:PTZ device.
	{
		HK_PtzMotor();
	}
#endif

#if (DEV_INFRARED)
	HK_Infrared_Decode();
#endif


#if ENABLE_QQ
	printf("####################################################\n");
	initDevice();
#endif

/*add by biaobiao*/
#if ENABLE_P2P
        create_my_detached_thread(p2p_server_f);
#endif

	be_present( 1 );
	sccUpdateServerAdd();

/*add by biaobiao*/
#if ENABLE_P2P
	IPC_Video_Audio_Thread_Init();
#endif

#if ENABLE_ONVIF
	IPCAM_StartWebServer();
	HK_Onvif_Init();
#endif

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

	HK_WtdInit(60*2); //watchdog.
	g_KeyResetCount = 0;

	unsigned int groupnum = 0, bitnum = 0, val_set = 0;
	unsigned int valSetRun = 0;
	for ( ; !quit_; counter++)
	{
		if (1 != HI3518_WDTFeed())
		{
			printf("Feed Dog Failed!\n");
		}

		ISP_Ctrl_Sharpness();

		if (b_hkSaveSd)
		{
			printf("[%s, %d] scc stop sd....\n", __func__, __LINE__);
			b_OpenSd = true;
			sd_record_stop();
			GetSdAlarmParam();
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

				if (sd_record_start() < 0)  
				{
					printf("start sd record failed !\n");
				}
			}
		}

#if (!DEV_KELIV)
		if (m433enable == 0)
		{
			CheckIOAlarm();//check AlarmIn & AlarmOut.
		}
#endif

		hk_IrcutCtrl( IRCutBoardType );//check & control Ircut mode.

#if (HK_PLATFORM_HI3518E | DEV_INFRARED)
		HK_Check_KeyReset(); //system restart or reset to factory settings.
#endif

		sccSendTestEmailAlaram();//send test email

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
		//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", g_RUN_grp, g_RUN_bit, valSetRun);
		sleep(1);
		valSetRun = 0;
		Hi_SetGpio_SetDir( g_RUN_grp, g_RUN_bit, GPIO_WRITE );
		Hi_SetGpio_SetBit( g_RUN_grp, g_RUN_bit, valSetRun ); //pull down.
		//HK_DEBUG_PRT("....Set GPIO %d_%d  set Value: %d....\n", g_RUN_grp, g_RUN_bit, valSetRun);
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
			g_startCheckAlarm++;
		}
	}

	sd_record_stop();
	gSysTime = time(0);
	gbStartTime = 1;
	LanNetworking(0);
	//SysRegistASLan_0(LOCAL_ASC, &cb_lan, 0);
	//SysLogout();
	//SysExit();
	//tq_destroy(tq_);

	if (quit_ != Excode_Stop)
		exiting_progress();

	printf("\n [%s]: %d ", __FUNCTION__, __LINE__);
	exit (0);
}

/*
   void* conf_set(const char* cf, const char* nm, const char* val)
   {
   if (cf && nm)
   {
   FILE *tf;
   char tmpnm[128];
   snprintf(tmpnm, sizeof(tmpnm), "%s_%d%d", cf, (int)time(0), (int)getpid());
   if ( (tf = fopen(tmpnm, "w")))
   {
   int fnd = 0;
   FILE *rf;
   if ( (rf = fopen(cf, "r")))
   {
   char l[512];
   size_t nmlen = strlen(nm);
   while (fgets(l, sizeof(l), rf))
   {
   char *p = trim_right(skip_spaces(l));
   if (strncmp(p, nm, nmlen) == 0 && *(p + nmlen) == '=')
   {
   ++fnd;
   if (val)
   {
 *(p + nmlen) = '\0';
 fprintf(tf, "%s=%s\n", l, val);
 }
 }
 else
 {
 fprintf(tf, "%s\n", l);
 }
 }
 fclose(rf);
 }
 if (!fnd)
 {
 if (val)
 {
 fprintf(tf, "%s=%s\n", nm, val);
 }
 }
 fclose(tf);
 remove(cf);
 rename(tmpnm, cf);
 return (void*)cf;
 }
 }
 return NULL;
 }
 */

static void on_resp_sync_time(Dict* d)
{
	if (req_ == HK_SYNC_TIME)
	{
		struct timespec ts;
		ts.tv_sec = DictGetInt(d, HK_KEY_TIME);
		ts.tv_nsec = 0;
		if (ts.tv_sec > 0)
		{
			char mac[18];
			clock_settime(CLOCK_REALTIME, &ts);
			HKLOG(L_DBG, "NEW MAC: %s", mac);
			conf_set(HOME_DIR"/hkclient.conf", "HWAddress__TMP", mac);
			//WRAP_SYS_RESTART(Excode_MAC_Change);
			printf("....on_resp_sync_time....\n");
			wrap_sys_restart();
		}
	}
}

/*
   void* conf_set_int(const char* cf, const char* nm, int val)
   {
   char v[64];
   snprintf(v,sizeof(v), "%d", val);
   return conf_set(cf, nm, v);
   }
 */


const const char* set_if_hwaddr(const char* mac) 
{
	if (is_mac_well_format(mac))
	{
		tMacKey mk;
		if (mtd_read_raw(&mk))
		{
			strcpy(mk.eth0_mac, mac);
			if (mtd_write_raw(&mk))
				return mac;
		}
	}
	HKLOG(L_ERR, mac);
	return 0;
}

static void on_resp_update_connect(Dict* d)
{
	int iWifiOpen = DictGetInt( d, HK_WIFI_OPENORCLOSE );
	const char* pHost = DictGetStr(d, HK_KEY_PICHOST);
	const char* mac1 = DictGetStr(d, HK_KEY_MAC1 );
	//const char* mac2 = DictGetStr(d, HK_KEY_MAC2);
	printf("..pHost=%s,,mac1=%s...........\n", pHost, mac1);
	if( NULL!=pHost)
		conf_set(HOME_DIR"/hkclient.conf", "PROXY", pHost);
	if(NULL!=mac1)
		conf_set(HOME_DIR"/net.cfg", "MacAddress", mac1);
	conf_set_int(HOME_DIR"/hkclient.conf", "wifiopen", iWifiOpen);// wifi 0 open; 1 close
}

static void on_resp_update_id(Dict* d)
{
	const char* devid = DictGetStr(d, HK_KEY_NAME);
	const char* pwd = DictGetStr(d, HK_KEY_PASSWORD);
	int iWifiOpen = DictGetInt( d, HK_WIFI_OPENORCLOSE );
	const char* mac1 = DictGetStr(d, HK_KEY_MAC1 );
	//const char* mac2 = DictGetStr(d, HK_KEY_MAC2);

	printf("..devid=%s,pwd=%s,mac1=%s...........\n", devid,pwd, mac1);
	if(NULL!=devid)
		conf_set(HOME_DIR"/hkclient.conf", "USER", devid);
	if(NULL!=pwd)
		conf_set(HOME_DIR"/hkclient.conf", "PASSWD", pwd);
	if(NULL!=mac1)
		conf_set(HOME_DIR"/net.cfg", "MacAddress", mac1);
	conf_set_int(HOME_DIR"/hkclient.conf", "wifiopen", iWifiOpen);// wifi 0 open; 1 close
}


static void on_resp_reqinit1(Dict* d)
{
	char mac[18]={0};
	if (req_ != HK_IPC_INITIAL)
		return;
	int ec = DictGetInt(d, "ErrorCode");

	struct timespec ts;
	ts.tv_sec = DictGetInt(d, HK_KEY_TIME);
	ts.tv_nsec = 0;
	if (ts.tv_sec > 0)
	{
		clock_settime(CLOCK_REALTIME, &ts);

		if (ec == 0)
		{
			const char *pHost    = DictGetStr( d, HK_KEY_PICHOST );
			const char *usr      = DictGetStr( d, HK_KEY_NAME );
			const char *pwd      = DictGetStr( d, HK_KEY_PASSWORD );
			const char *name     = DictGetStr( d, HK_KEY_FROM );
			const char *mac1     = DictGetStr( d, HK_KEY_MAC1 );
			const char *mac2     = DictGetStr( d, HK_KEY_MAC2 );
			int iWifiOpen        = DictGetInt( d, HK_WIFI_OPENORCLOSE );
			int dev_PTZ          = DictGetInt( d, HK_KEY_PT ); //0:PTZ device;  1:no PTZ.
			int dev_NetType      = DictGetInt( d, HK_KEY_IP ); //0:RJ45; 1:wireless.
			int ircut_board_type = DictGetInt( d, HK_KEY_IRCUTBOARD ); //0: level 0;  1: level 1.
			printf("...%s...dev_PTZ:%d, dev_NetType:%d, ircut_board_type:%d...\n", __func__, dev_PTZ, dev_NetType, ircut_board_type);

			if (mac1 == NULL)
			{
				mac_generate(mac);
			}
			else
			{
				strcpy(mac, mac1);
			}
			req_ = NULL;

			if (usr && pwd)
			{
				if (name)
				{
					conf_set(HOME_DIR"/hkclient.conf", "NAME", name);
				}

				if (pHost != NULL)
					conf_set(HOME_DIR"/hkclient.conf", "PROXY", pHost);
				conf_set(HOME_DIR"/hkclient.conf", "USER", usr);
				conf_set(HOME_DIR"/hkclient.conf", "PASSWD", pwd);
				conf_set(HOME_DIR"/net.cfg", "MacAddress", mac);
				conf_set_int(HOME_DIR"/hkclient.conf", "wifiopen", iWifiOpen);// wifi 0 open; 1 close
			}

			if (0 == dev_PTZ) //PTZ device.
			{
				conf_set_int(HOME_DIR"/ptz.conf", "HKDEVPTZ", 1); //with PTZ.
			}
			else if (1 == dev_PTZ) //no PTZ.
			{
				conf_set_int(HOME_DIR"/ptz.conf", "HKDEVPTZ", 0); //without PTZ.
			}

			if (1 == dev_NetType) //wireless, no RJ45.
			{
				conf_set_int(HOME_DIR"/wifinet.cfg", "HKDEVWIRELESS", 1); 
			}
			else if (0 == dev_NetType) //with RJ45.
			{
				conf_set_int(HOME_DIR"/wifinet.cfg", "HKDEVWIRELESS", 0); 
			}

			if (1 == ircut_board_type) //ircut light board type: level 0 / level 1.
			{
				conf_set_int(HOME_DIR"/hkipc.conf", "IRCutBoardType", 1); //level 1.
			}
			else
			{
				conf_set_int(HOME_DIR"/hkipc.conf", "IRCutBoardType", 0); //level 0.
			}
		}
	}       
}

