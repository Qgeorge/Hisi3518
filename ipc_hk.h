
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <time.h>


/******************* debug print (zqjun)***********************/
#define PRINT_ENABLE    1
#if PRINT_ENABLE
    #define HK_DEBUG_PRT(fmt...)  \
        do {                      \
            printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
            printf(fmt);          \
        }while(0)
#else
    #define HK_DEBUG_PRT(fmt...)  \
        do { ; } while(0) //do nothing.
#endif

/************** 2014-09-18 add by zqj *************/
/* different consumer macro
 *   0: disable;
 *   1: enable
 */
//#define HK_PLATFORM_HI3518C 0
#define HK_PLATFORM_HI3518E 1
#define DEV_KELIV       0 //klv 3518e (steven).
#define DEV_INFRARED    0 //xhd infrared remoter.

//#define ENABLE_ONVIF    1 //0:disable onvif; 1:enable onvif.
#define ENABLE_P2P	1

#define JPEG_SNAP       0

/*sensor type*/
#define HK_AR0130   1 //sensor: ar0130
#define HK_OV9712   2 //sensor: ov9712
#define HK_OV2643   9
#define HK_OV7725   5
#define HK_99141    8
#define HK_5150     6


#define HOME_DIR "/mnt/sif"

//sd card configuration:
#define HK_SD_SPLITE 400 //MB
#define SD_REC_PHONE "phone_rec_grade" //phone record.
#define SD_REC_MOVE  "alarm_move" //motion detect.
#define SD_REC_OUT   "alarm_out"  //IO alarm out.
#define SD_REC_AUTO  "auto_rec"   //sd auto record while power on.
#define SD_REC_LOOP  "loop_write" //sd loop record.
#define SD_REC_SPLIT "splite"     //sd record splite size.
#define SD_REC_AUDIO "audio"      //sd record audio and video at the same time.


#define HK_WIFI_FIND_LEN 100 
#define SSID_LEN         37

typedef struct hk_remote_wffi_info
{
    char nettype;	//0:Infra; 1:Adhoc	
    char enctype;   //1:AUTO; 2:TKIP; 3:AES
    char authmode;  //0:none; 1:wep; 2:wpa/wpa2; 3:wpa-psk/wpa2-psk
    char ssid[SSID_LEN];  //wifi node name.
    //char bssid[SSID_LEN]; //wifi mac address.
    int  iSignal;
}REMOTE_INFO;       

typedef struct hk_remote_wifi_find          
{                                           
    int count;                              
    REMOTE_INFO wifi_info[HK_WIFI_FIND_LEN];
}REMOTE_WIFI_FIND;     
/****************** WIFI Params End *****************/


typedef struct hk_sd_param
{
    short audio;
    short moveRec;
    short outMoveRec;
    short autoRec;
    short loopWrite;
    short splite;
    short sdrecqc;//1 one, 0 two
    short sdIoOpen;//1 open, 0 close
    short sdError;//1 open, 0 close
    short sdMoveOpen;//0 open, 1 close
    unsigned long allSize;
    unsigned long haveUse;
    unsigned long leftSize;
}HK_SD_PARAM_;


enum HKV_SingleProperty
{
    HKV_VEncCodec,
    HKV_VinFormat,
    HKV_VinFrameRate,
    HKV_VEncIntraFrameRate,
    HKV_Cbr,
    HKV_Vbr,
    HKV_AnalogEncodeMode,
    HKV_CamContrastLevel,
    HKV_CamEffectLevel,
    HKV_CamExposureLevel,
    HKV_CamLightingModeLevel,
    HKV_CamSaturationLevel,
    HKV_HueLevel,
    HKV_BrightnessLevel,
    HKV_SharpnessLevel,
    HKV_DividedImageEncodeMode,
    HKV_MotionSensitivity,
    HKV_FrequencyLevel,
    HKV_VscFormat,
    HKV_BitRate,
    HKV_MotionStatus,
    HKV_LCDFormat,
    HKV_VoutFrameRate,
    HKV_SyncMode,
    HKV_Checksum,
    HKV_Flip,
    HKV_Mirror,
    HKV_Yuntai,
    HKV_Focus,
    HKV_FocuMax,
    HKV_BaudRate,
    HKV_NightLight,
    HKV_Autolpt,
    HKV_Properties_Count,    
    HKV_SoundDetection, // <-------------END-------------

};

struct HKVProperty
{
    unsigned int vv[HKV_Properties_Count];
};


typedef struct TAlarmSet
{
    unsigned char bMotionDetect;    //open
    unsigned short RgnX;
    unsigned short RgnY;
    unsigned short RgnW;
    unsigned short RgnH;
    unsigned char Sensitivity;  
    unsigned char Threshold;    
}TAlarmSet_;

#define MAX_CHAN  3        
// Max macro cell count : (1600/16) * (1200/16)
// //#define MAX_MACROCELL_NUM         7500  
// // Max macro cell count : (1280/16) * (960/16)
//#define MAX_MACROCELL_NUM         4800 
// // Max macro cell count : (720/16) * (576/16)
#define MAX_MACROCELL_NUM         1620  
//
// //#define C_W     320/352
// //#define C_H     240/288
#define C_W     1
#define C_H     1
//
#define CIF_WIDTH_PAL            320
#define CIF_HEIGHT_PAL           240

enum enumVGAMode
{
    ENUM_NONE   = 0,  
    ENUM_QQ720P = 1,    /* 320*180 */
    ENUM_CIF    = 2,    /* 352*288 */
    ENUM_QVGA   = 3,    /* 320*240 */
    ENUM_Q720P  = 4,    /* 640*360 */
    ENUM_VGA    = 5,    /* 640*480 */
    ENUM_D_ONE  = 6,    /* 704*480 */
    ENUM_PAL_D1 = 7,    /* 704*576 */
    ENUM_XVGA   = 8,    /* 1024*768 */
    ENUM_720P   = 9,    /* 1280*720 */      
    ENUM_960P   = 10,   /* 1280*960 */
    ENUM_RESOLUTION_ALL,  
};


typedef enum
{
    VR_D1    = 0x01,
    VR_HD1   = 0x02,
    VR_CIF   = 0x04,
    VR_QCIF  = 0x08,
}enVideoResolution;

typedef unsigned char byte;

#define PSTREAMONE 0
#define PSTREAMTWO 1
#define PSTREAUDIO 2


#define POOLSIZE 20+1
//#define MAX_VIDEODATA_HOSTSTREAM 1024*1024
//#define MAX_VIDEODATA_SLAVETREAM 512*1024 //1024*1024
//#define MAX_VIDEODATA_HOSTSTREAM 400*1024
//#define MAX_VIDEODATA_SLAVETREAM 200*1024 //1024*1024
#define MAX_VIDEODATA_HOSTSTREAM 200*1024 //200K
#define MAX_VIDEODATA_SLAVETREAM 200*1024 //200K

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

typedef struct hk_wifi_cfg
{
    char apmode[64];
    char nettype[64];
    char enctype[64];
    char authmode[64];
    char ssid[128];
    char password[64];
}WIFICfg;  

typedef struct _VideoData
{
    short IFrame;
    short iEnc;
    short StreamType;
    unsigned int  nSize;
    unsigned int  nPos;
}VideoData;

typedef struct _VideoDataRecord
{
    int g_readIndex;
    int g_writeIndex;
    int g_bLost;
    int g_bAgain;

    int g_writePos;
    int g_readPos;
    int g_allDataSize;
    int g_CurPos;

    unsigned int g_haveFrameCnt;
    char *g_videoBuf;

    VideoData g_VideoData[POOLSIZE+1];
}VideoDataRecord;

