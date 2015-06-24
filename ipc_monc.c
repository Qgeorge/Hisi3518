#include <ctype.h>
#include <dirent.h>

#include "ipc_hk.h"
#include "utils/HKMonCmdDefine.h"
#include "utils/HKCmdPacket.h"
#include "sys.h"
#include "ov7725.h"
#include "ipc_email.h"

#if ENABLE_ONVIF
    #include "IPCAM_Export.h"
#endif

extern int g_sccDevCode;
extern char authMode[4][8];
extern char encType[4][8];
extern short g_nFtpIsOpen;
extern HKEMAIL_T hk_email;
extern short g_onePtz;
extern bool b_hkSaveSd;
int g_delcount;
int g_allcount;
HKFrameHead *frameRec;
extern struct HKVProperty video_properties_;
extern HK_SD_PARAM_ hkSdParam;

//extern bool g_bsMonut;
extern int g_irOpen;
extern int g_iCifOrD1;

extern HKIPAddres eth0Addr,wifiAddr;
extern WIFICfg   mywifiCfg;
extern volatile int g_nSignLogin;
extern int  g_DevVer;
static bool b_testEmail = false;

extern short g_sdIsOnline;
static void OnMonDeletePhoto(const char *cFileName, int iDel );

unsigned long get_file_size(const char *filename)
{
    struct stat buf;

    if(stat(filename, &buf)<0)
    {
        return 0;
    }
    return (unsigned long)buf.st_size;
}

static void sys_restart( )
{
    wrap_sys_restart( );
    return;
}

void OnDevUpdateLoaclPasswd( int nCmd,  Dict* d )
{
    int iFlag = 1;
    int iLen = 0;
    char buf[1024]={0};
    char *cNewPasswd = DictGetStr( d, HK_KEY_PASSWORD );
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );

    setenv("LAN_PASSWD", cNewPasswd, 1);
    conf_set(HOME_DIR"/hkclient.conf", "LAN_PASSWD", cNewPasswd);
    be_present(2);

    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_FLAG, iFlag );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );

    iLen = DictEncode(buf, sizeof(buf), DictPacket);
    buf[iLen] = '\0';
    NetSend( HK_KEY_MONSERVER, buf, iLen );
    DictDestroy(DictPacket);
}
 
static void OnDevResystem()
{
    sys_restart();
}

Dict *g_TFPacket=NULL;
static int g_iTfCount=0;
void OnDevQueryPhotoList( int nCmd, Dict *d )
{
    if(g_sdIsOnline==0)
        return;
    int allCount = g_allcount - g_delcount;
    int iCount =0;
    int iFilst;// = g_delcount+1;
    Dict *DictPacket = DictCreate( 0, 0 );

    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    int iPopedom = DictGetInt( d, HK_KEY_POPEDOM );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    int currentPage = DictGetInt( d, HK_KEY_CURRENTPAGE );
    int pageSize = DictGetInt( d, HK_KEY_PAGESIZE );

    //printf("...read sd data currpage=%d...pageSize=%d..allCount=%d.\n", currentPage, pageSize,allCount);
    //
    if( g_TFPacket == NULL)
        g_TFPacket = DictCreate( 0, 0 );

    if( currentPage == 1)
    {
        FILE *fp = NULL;
        if (!(fp = popen("ls -rt /mnt/mmc/snapshot", "r"))) 
        {
            printf("OnLocalReadSdData popen failed with:\n");
            return; 
        }

        char tmpbuf[512] = {0};
        char buf[512]={0};
        g_iTfCount=0;
        while (fgets(buf, sizeof(buf), fp)) 
        {
            char keyBuf[12]={0};
            sprintf(keyBuf, "%d", iCount++ );
            sscanf(buf, "%[^\n]", tmpbuf);
            DictSetStr( g_TFPacket, keyBuf, tmpbuf );

            g_iTfCount++;
            //printf("scc buf=%s", buf);
        }
        if (fp)  
            pclose(fp);

    }
    
    if( g_iTfCount > 0)
    {
        int filstCount = currentPage*pageSize-pageSize;//1*20-20
        int j=0;
        int iFileCount=0;
        for( j=0; j<pageSize; j++)
        {
            char keyBuf[64]={0};
            sprintf(keyBuf, "%d", g_iTfCount-j-filstCount-1);// filstCount+j );
            char *cFile = DictGetStr(g_TFPacket, keyBuf );//...hkv
            if(cFile != NULL)
            {
                //printf("get file name=%s.ilen=%d.\n", cFile, strlen(cFile));
                char cPatch[64]={0};
                snprintf(cPatch,sizeof(cPatch), "/mnt/mmc/snapshot/%s", cFile);
                unsigned long iFleSzie = get_file_size( cPatch );


                char cNewFile[64]={0};
                strcpy( cNewFile, cFile );
                char *pt = strstr(cNewFile,".hkv");
                if( pt != NULL )   *pt='\0';
                sprintf(keyBuf, "%s%d", HK_KEY_REC_NAME, j );
                DictSetStr( DictPacket, keyBuf, cNewFile );
                
                sprintf(keyBuf, "%s%d", HK_KEY_REC_ID, j );
                DictSetInt( DictPacket, keyBuf, g_iTfCount-j-filstCount-1);//filstCount+j );

                sprintf(keyBuf, "%s%d", HK_KEY_MAX_FILESIZE, j);
                DictSetInt( DictPacket, keyBuf, iFleSzie );

                iFileCount++;
            }
            else
            {
                break;
            }
        }
        //printf(".sd list count=%d...\n", iCount);
        DictSetInt( DictPacket, HK_KEY_COUNT, iFileCount );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, g_iTfCount );
    }
    else
    {
        //printf(".sd list count=0...\n");
        DictSetInt( DictPacket, HK_KEY_COUNT, 0 );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, 0 );
    }

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, iPopedom );

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+100;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
        return;
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
}

#if 0
void OnDevQueryPhotoList( int nCmd, Dict *d )
{
    int allCount = g_allcount - g_delcount;
    int iCount =0;
    int iFilst;// = g_delcount+1;
    Dict *DictPacket = DictCreate( 0, 0 );

    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    int iPopedom = DictGetInt( d, HK_KEY_POPEDOM );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    int currentPage = DictGetInt( d, HK_KEY_CURRENTPAGE );
    int pageSize = DictGetInt( d, HK_KEY_PAGESIZE );

    //printf("...read sd data currpage=%d...pageSize=%d..allCount=%d.\n", currentPage, pageSize,allCount);
    if(allCount>0 && currentPage>0 )
    {
        int filstCount = currentPage*pageSize-pageSize;
        int i=0;
        iFilst = g_allcount-filstCount;
        int j=1;
        for( j=1; j<=pageSize; j++)
        {
            char keyBuf[64]={0};
            sprintf(keyBuf, "%s%d", HK_KEY_VALUEN,iFilst-j );
            char *cFile = DictGetStr(frameRec, keyBuf );//time
            if(cFile != NULL)
            {
                char cPatch[64]={0};
                snprintf(cPatch,sizeof(cPatch), "/mnt/mmc/snapshot/%s.hkv", cFile);
                unsigned long  iFleSzie = get_file_size( cPatch );
                //if( iFleSzie >0 )
                {
                    sprintf(keyBuf, "%s%d", HK_KEY_REC_NAME,i );
                    DictSetStr( DictPacket, keyBuf, cFile );
                    sprintf(keyBuf, "%s%d", HK_KEY_REC_ID,i );
                    DictSetInt( DictPacket, keyBuf, iFilst-i );
                    
                    sprintf(keyBuf, "%s%d",HK_KEY_MAX_FILESIZE, i);
                    DictSetInt( DictPacket, keyBuf, iFleSzie );
                    iCount++;
                    i++;
                }
            }
            else
            {
                if ((allCount-filstCount) > pageSize )
                {
                    pageSize++;
                    filstCount++;
                }
            }
        }
        DictSetInt( DictPacket, HK_KEY_COUNT, iCount );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, allCount );
    }
    else
    {
        DictSetInt( DictPacket, HK_KEY_COUNT, 0 );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, 0 );
    }
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, iPopedom );

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+100;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
        return;
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
}
#endif 

static void OnMonDevSdSet( Dict *hf )
{
    int iMove       = GetParamUN(hf, HK_KEY_SDMOVEREC);
    int iOutMove    = GetParamUN(hf, HK_KEY_SDOUTMOVEREC);
    int iAutoRec    = GetParamUN(hf, HK_KEY_SDAUTOREC);
    int iLoopWrite  = GetParamUN(hf, HK_EKY_SDLOOPREC);
    int iSplite     = GetParamUN(hf, HK_EKY_SDSPILTE);
    int iAudio      = GetParamUN(hf, HK_KEY_SDAUDIOMUX);
    int iSdrcqc     = GetParamUN(hf, HK_KEY_SDRCQC);//1 one, 0 two

    //printf("..iMove=%d,,,,iOtMove=%d....iAutoRec=%d.iAudio=%d..\n", iMove, iOutMove, iAutoRec,iAudio );
    conf_set_int(HOME_DIR"/sdvideo.conf", "alarm_move", iMove);
    conf_set_int(HOME_DIR"/sdvideo.conf", "alarm_out", iOutMove);
    conf_set_int(HOME_DIR"/sdvideo.conf", "auto_rec", iAutoRec);
    conf_set_int(HOME_DIR"/sdvideo.conf", "loop_write", iLoopWrite);
    conf_set_int(HOME_DIR"/sdvideo.conf", "splite", iSplite);
    conf_set_int(HOME_DIR"/sdvideo.conf", "audio", iAudio);
    conf_set_int(HOME_DIR"/sdvideo.conf", HK_KEY_SDRCQC, iSdrcqc);

    b_hkSaveSd=true;
}

static int sdFormatChect()
{
    struct stat st;
    if (stat("/dev/mmcblk0", &st) == 0)
    {
        if(stat("/dev/mmcblk0p1", &st) == 0)
        {
            return 1; //partition existed.
        }
        else
        {
            return 2; //there needs to add new partition.
        }
    }
    return 0;
}

static void OnMonFormatCard(Dict *hf )
{
    g_sdIsOnline=0;
    sd_record_stop();
    sleep(1);
    system("umount /mnt/mmc/");
    sleep(1);

    int iSdFr = sdFormatChect();
    HK_DEBUG_PRT("......format SD card, system type: %d......\n", iSdFr);
    if ( iSdFr == 2 )
    {
        system("mkdosfs -F 32 /dev/mmcblk0");
        system("fdisk -f /dev/mmcblk0"); //add new partition.
        system("mkdosfs -F 32 /dev/mmcblk0p1"); //format new partition.

    }
    else if( iSdFr == 1 )
    {
        system("mkdosfs -F 32 /dev/mmcblk0p1");
    }
    
    g_delcount=0;
    g_allcount=0;
    sys_restart( );
}


static void OnMonSetZone(Dict *hf)
{
    int iZone = GetParamUN( hf, HK_KEY_TIME);
    conf_set_int(HOME_DIR"/time.conf", "zone", iZone);
    unsigned int iloadtime = GetParamUN( hf, HK_KEY_END_TIME );
    conf_set_int(HOME_DIR"/time.conf", "time_", iloadtime-iZone);
    int iSummerp = GetParamUN( hf, HK_WIFI_OPENORCLOSE );
    conf_set_int(HOME_DIR"/time.conf", "summer", iSummerp);

    sys_restart( );
}

static void OnPhoneSetDevParam(Dict *hf )
{
    //int iPhStream = DictGetInt( hf, "phstream");
    int iSha    = DictGetInt( hf, HK_KEY_SHARPNESS);
    int iEnc    = DictGetInt( hf, "enc" );
    int iRate   = DictGetInt( hf, HK_KEY_RATE);
    int iBri    = DictGetInt( hf, HK_KEY_BRIGHTNESS);
    int iSat    = DictGetInt( hf, HK_KEY_SATURATION);
    int iCon    = DictGetInt( hf, HK_KEY_CONTRAST);
    int iStream = DictGetInt( hf, "stream");

    //if(iSha==0 && iBri==0)
    //{
        //hk_SetPhonePlaram( iPhStream, iEnc, iRate );
        //return;
    //}

    printf("[%s, %d] iStream:%d, iRate:%d, iEnc:%d, iSha:%d, iBri:%d, iSat:%d, iCon:%d\n", __func__, __LINE__, iStream, iRate, iEnc, iSha, iBri, iSat, iCon); 
    if ((iRate >= 1) && (iRate <= 25))
    {
        conf_set_int(HOME_DIR"/subipc.conf", "stream", iStream);
        conf_set_int(HOME_DIR"/subipc.conf", "rate", iRate);
        //conf_set_int(HOME_DIR"/subipc.conf", "enc", iEnc);
    }

#if 0
    int ret = 0, s_BitRate = 0;
    if (iStream > 0)
    {
        s_BitRate = (iStream > 48) ? 48 : iStream;
        ret = HISI_SetBitRate(iEnc, s_BitRate);
        if (ret)
        {
            printf("[%s, %d] set bitrate failed !\n", __func__, __LINE__);
            return -1;
        }
    }
#endif

    if (iSha > 0)
    {
        video_properties_.vv[HKV_SharpnessLevel] = iSha;
        conf_set_int(HOME_DIR"/hkipc.conf", "SharpnessLevel", iSha);

        if ( HISI_SetSharpNess(iSha) )
        {
            printf("[%s, %d] phone set sharpness failed!\n", __func__, __LINE__); 
        }
    }
    if( iSat >0 )
    {
        video_properties_.vv[HKV_CamSaturationLevel] = iSat;
        conf_set_int(HOME_DIR"/hkipc.conf", "CamSaturationLevel", iSat);
    }
    if (iBri > 0)
    {
        video_properties_.vv[HKV_BrightnessLevel] = iBri;
        conf_set_int(HOME_DIR"/hkipc.conf", "BrightnessLevel", iBri );
    }
    if (iCon > 0)
    {
        video_properties_.vv[HKV_CamContrastLevel] = iCon;
        conf_set_int(HOME_DIR"/hkipc.conf", "CamContrastLevel", iCon );
    }

    if ( HISI_SetCSCAttr(iSat, iBri, iCon, 0) )
    {
        printf("[%s, %d] phone set 'saturation or brightness or contrast failed !\n", __func__, __LINE__); 
    }

    return;
}

static void OnPhoneRestore()
{
    conf_set_int(HOME_DIR"/subipc.conf", "stream", 5);
    conf_set_int(HOME_DIR"/subipc.conf", "enc", 5);
    //conf_set_int(HOME_DIR"/subipc.conf", "rate", 25);
	conf_set_int(HOME_DIR"/subipc.conf", "rate", 15);
    conf_set_int(HOME_DIR"/subipc.conf", "smooth", 1);
    OnMonSetResetParam( );
}

static void OnWanGetDevParam(int nCmd, Dict *hf, int mc )
{
    char *cDevid = DictGetStr( hf, HK_KEY_FROM );
    char *cUserid = DictGetStr( hf, HK_KEY_TO );
    int iStream = conf_get_int(HOME_DIR"/subipc.conf", "stream" );
    int iRate = conf_get_int(HOME_DIR"/subipc.conf", "rate");
    int iEnc = conf_get_int(HOME_DIR"/subipc.conf", "enc");

    int iSat = conf_get_int(HOME_DIR"/hkipc.conf", "CamSaturationLevel");
    int iCon = conf_get_int(HOME_DIR"/hkipc.conf", "CamContrastLevel");
    int iBri = conf_get_int(HOME_DIR"/hkipc.conf", "BrightnessLevel");
    int iSha = conf_get_int(HOME_DIR"/hkipc.conf", "SharpnessLevel");
    int iSensitivity = conf_get_int(HOME_DIR"/hkipc.conf", "MotionSensitivity");

    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, "stream", iStream );
    DictSetInt( DictPacket, HK_KEY_ALERT_SENSE, iSensitivity );
    DictSetInt( DictPacket, HK_KEY_RATE, iRate );
    DictSetInt( DictPacket, "enc", iEnc ); 
    DictSetInt( DictPacket, HK_KEY_SATURATION, iSat );
    DictSetInt( DictPacket, HK_KEY_CONTRAST, iCon );
    DictSetInt( DictPacket, HK_KEY_BRIGHTNESS, iBri );
    DictSetInt( DictPacket, HK_KEY_SHARPNESS, iSha );

    DictSetInt( DictPacket, HK_KEY_FUCTION,1);//ver
    DictSetInt( DictPacket, HK_KEY_COUNT,hk_email.mcount);
    if(hkSdParam.moveRec==1 && hkSdParam.outMoveRec==1 )
    {
        DictSetInt( DictPacket, HK_KEY_SDMOVEREC,1);
    }
    else
    {
        DictSetInt( DictPacket, HK_KEY_SDMOVEREC,0);
    }


    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, mc );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );

    char cBuf[2048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    DictDestroy( DictPacket );
}

//Get wan sd
static void OnWanGetDevSDParam(int nCmd, Dict *hf, int  mc )
{
    char *cDevid = DictGetStr( hf, HK_KEY_FROM );
    char *cUserid = DictGetStr( hf, HK_KEY_TO );

    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_SDMOVEREC, hkSdParam.moveRec );
    DictSetInt( DictPacket, HK_KEY_SDOUTMOVEREC, hkSdParam.outMoveRec );
    DictSetInt( DictPacket, HK_KEY_SDAUTOREC, hkSdParam.autoRec );
    DictSetInt( DictPacket, HK_EKY_SDLOOPREC, hkSdParam.loopWrite );
    DictSetInt( DictPacket, HK_EKY_SDSPILTE, hkSdParam.splite );
    DictSetInt( DictPacket, HK_EKY_SDALLSIZE, hkSdParam.allSize );
    DictSetInt( DictPacket, HK_KEY_SDHAVEUSE, hkSdParam.haveUse );
    DictSetInt( DictPacket, HK_KEY_SDLEFTUSE, hkSdParam.leftSize );
    DictSetInt( DictPacket, HK_KEY_SDAUDIOMUX, hkSdParam.audio );
    DictSetInt( DictPacket, HK_KEY_SDRCQC, hkSdParam.sdrecqc );

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, mc );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );

    char cBuf[2048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    DictDestroy( DictPacket );
}

static void OnWanPhoneGetDevParam(int nCmd, Dict* d )
{
    int mc = GetParamUN( d,HK_KEY_SUBCMD );
    switch (mc )
    {
        case HK_GET_PHONE_DEVPARAM:
            OnWanGetDevParam(nCmd, d, mc );
            break;
        case HK_GET_PHONE_SDPARAM:
            OnWanGetDevSDParam(nCmd, d, mc );
            break;
        default:
            break;
    }
}

static void OnWanPhoneSetDevParam(Dict* d, const char* buf)
{
    int mc = GetParamUN( d,HK_KEY_SUBCMD );
    switch (mc )
    {
        case HK_SET_PHONE_DEVPARAM:
            OnPhoneSetDevParam( d );
            break;
        case HK_SET_PHONE_SDPARAM:
            //OnPhoneSetSdParam( d );
            break;
        case HK_PHONE_RESTORE:
            OnPhoneRestore();
            break;
        case HK_MON_CONTROLPT:
            OnMonPtz(buf);
            break;
        case HK_MON_AUTOLPTSPEED:
            OnAutoLptspeed( buf );
            break;
        case HK_MON_DEV_PRESET:
            OnDevPreset( d );
            break;
        default:
            OnPhoneSetDevParam( d );
            break;
    }
}


//Get Wan email
static void OnGetWanAlarmInfo(int nCmd, int iSubCmd, Dict *d )
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    //int isMoveOpen = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM);
    //int isSdError = conf_get_int(HOME_DIR"/emalarm.conf",HK_KEY_EMAIL_SD_ERROR);
    char cEmailInfo[256]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_INFO, cEmailInfo, 256);

    /*
    //int ioIsOpen = conf_get_int(HOME_DIR"/emalarm.conf",HK_KEY_IOIN );
    //int iPort = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_PORT);
    //int isOpen = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_ISALERT_SENSE);
    //int iCount = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_COUNT);
    char sedEmail[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_ALERT_EMAIL, sedEmail, 128);
    char recEmail[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_REEMAIL,recEmail, 128);
    char smtpServer[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_SMTPSERVER,smtpServer, 128);
    char smtpUser[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_SMTPUSER,smtpUser, 128);
    char smtpPswd[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_PASSWORD,smtpPswd, 128);
    */
    //printf("SCC.Wan..GetAlarmEmail isOpen=%d,,iPort=%d..sedEmail=%s...\n",isOpen,iPort,sedEmail);
    
    int nDataEnc = conf_get_int( HOME_DIR"/emalarm.conf", HK_WIFI_ENCRYTYPE );
    DictSetInt( DictPacket, HK_KEY_EMAIL_MOVE_ALARM, hkSdParam.sdMoveOpen );
    DictSetInt( DictPacket, HK_KEY_EMAIL_SD_ERROR, hkSdParam.sdError );
    DictSetInt( DictPacket, HK_KEY_IOIN, hkSdParam.sdIoOpen );
    DictSetStr( DictPacket, HK_KEY_EMAIL_INFO, cEmailInfo );

    DictSetInt( DictPacket, HK_KEY_ISALERT_SENSE, hk_email.isOpen );
    DictSetInt( DictPacket, HK_KEY_PORT, hk_email.port ); 
    DictSetStr( DictPacket, HK_KEY_ALERT_EMAIL, hk_email.mailfrom );
    DictSetStr( DictPacket, HK_KEY_REEMAIL, hk_email.mailto );
    DictSetStr( DictPacket, HK_KEY_SMTPSERVER, hk_email.mailserver);
    DictSetStr( DictPacket, HK_KEY_SMTPUSER, hk_email.username );
    DictSetStr( DictPacket, HK_KEY_PASSWORD, hk_email.passwd );
    DictSetInt( DictPacket, HK_KEY_COUNT, hk_email.mcount );
    DictSetInt( DictPacket, HK_WIFI_ENCRYTYPE, hk_email.secType );

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );

    char *cBuf = NULL;
    int iPackSize = DictEncodedSize(DictPacket)+1;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
    {
        DictDestroy( DictPacket );
        return;
    }
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
}

//Wan ftp
static void OnGetWanFtpInfo(int nCmd, int iSubCmd, Dict *d)
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    int isOpen = conf_get_int(HOME_DIR"/ftpbakup.conf", HK_WIFI_OPENORCLOSE );
    int iPort = conf_get_int(HOME_DIR"/ftpbakup.conf", HK_KEY_PORT);
    char ftpServer[128]={0};
    conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_FTPSERVER, ftpServer, 128);
    char ftpPswd[128]={0};
    conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_PASSWORD,ftpPswd, 128);
    char ftpUser[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_FTPUSER,ftpUser, 128);
    char ftpType[64]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_CODETYPE,ftpType, 64);

    //printf("SCC.Wan GetFtp.isOpen=%d,ftpServer=%s,ftpUser=%s..\n",isOpen, ftpServer, ftpUser);
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_ISALERT_SENSE, isOpen );
    DictSetInt( DictPacket, HK_KEY_PORT, iPort ); 
    DictSetStr( DictPacket, HK_KEY_FTPSERVER, ftpServer );
    DictSetStr( DictPacket, HK_KEY_PASSWORD, ftpPswd );
    DictSetStr( DictPacket, HK_KEY_FTPUSER, ftpUser );
    DictSetStr( DictPacket, HK_KEY_CODETYPE, ftpType );

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+1;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
    {
        DictDestroy( DictPacket );
        return;
    }
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
}

//Wan ppoe
static void OnGetWanPpoe( int nCmd, int iSubCmd, Dict *d)
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    int isOpen = conf_get_int(HOME_DIR"/hkpppoe.conf", HK_KEY_ISALERT_SENSE );
    char cPppoeUser[128]={0};
    conf_get( HOME_DIR"/hkpppoe.conf", HK_KEY_PPPOEID, cPppoeUser, 128);
    char cPswd[128]={0};
    conf_get( HOME_DIR"/hkpppoe.conf", HK_KEY_PASSWORD,cPswd, 128);

    //printf("SCC.Wan.ppoeUser=%s,cPswd=%d.isOpen=%d.\n", cPppoeUser, cPswd, isOpen);
    DictSetInt( DictPacket, HK_KEY_ISALERT_SENSE, isOpen );
    DictSetStr( DictPacket, HK_KEY_PASSWORD, cPswd );
    DictSetStr( DictPacket, HK_KEY_PPPOEID, cPppoeUser );

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+1;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
    {
        DictDestroy( DictPacket );
        return;
    }
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
}

static void OnPenetrateData( int nCmd, int iSubCmd, Dict *d)
{
#if 0
    char *cData = DictGetStr(d, HK_KEY_DEVPARAM );
    printf("scc...Penetrate...%s...\n",cData );

    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    //unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    //DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );
    DictSetStr( DictPacket, HK_KEY_DEVPARAM, cData );

    char cBuf[4048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    DictDestroy( DictPacket );
#endif
}

static void OnGetWebWifiInfo(int nCmd, int iSubCmd, Dict *d)
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    DictSetInt( DictPacket, HK_WIFI_OPENORCLOSE, wifiAddr.bStatus );
    SetParamStr( DictPacket, HK_WIFI_SID,mywifiCfg.ssid );
    SetParamStr( DictPacket, HK_WIFI_SAFETYPE, mywifiCfg.authmode );
    {
        char aryPSW[64]={0};
        strcpy( aryPSW,mywifiCfg.password );
        char* cEncPswd =  (char *)EncodeStr( aryPSW );
        SetParamStr( DictPacket, HK_WIFI_PASSWORD, cEncPswd );
    }
    SetParamStr( DictPacket, HK_WIFI_ENCRYTYPE, mywifiCfg.enctype );
    SetParamStr( DictPacket, HK_NET_BOOTPROTO, wifiAddr.ipMode );
    SetParamStr( DictPacket,HK_KEY_IP, wifiAddr.ip );
    SetParamStr( DictPacket,HK_KEY_NETMASK, wifiAddr.netmask );
    SetParamStr( DictPacket,HK_KEY_NETWET, wifiAddr.gateway );
    SetParamStr( DictPacket,HK_NET_DNS0, wifiAddr.dns1 );
    SetParamStr( DictPacket,HK_NET_DNS1, wifiAddr.dns2 );
    SetParamUN( DictPacket, HK_KEY_STATE, g_nSignLogin );
    SetParamUN( DictPacket, HK_KEY_VER,g_DevVer);

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );
    char cBuf[4048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    printf("scc...GetWebWifiInfo..cBuf=%s...\n", cBuf );
    DictDestroy( DictPacket );
}

static void OnGetWebSsidInfo(int nCmd, int iSubCmd, Dict *d)
{
    printf("scc....len SSID.......\n");
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *hfwi = DictCreate( 0, 0 );

    static REMOTE_WIFI_FIND wifiFind;
    memset(&wifiFind, '\0', sizeof(wifiFind));
    if ( ScanWifiInfo(&wifiFind) ) //0:success, errno:failed.
    {
        int iPacketLen=0;
        DictSetInt( hfwi, HK_KEY_MAINCMD, nCmd );
        DictSetInt( hfwi, HK_KEY_SUBCMD, iSubCmd );
        DictSetInt( hfwi, HK_KEY_FLAG, 1 );
        DictSetInt( hfwi, HK_KEY_UIPARAM, ulParam );
        DictSetStr( hfwi, HK_KEY_FROM, cUserid );
        DictSetStr( hfwi, HK_KEY_TO, cDevid );
        DictSetInt( hfwi, HK_KEY_POPEDOM, 3 );

        iPacketLen = GetFrameSize( hfwi );
        char *cSenBuf = malloc(iPacketLen+1);
        if( NULL != cSenBuf )
        {
            int iLen = GetFramePacketBuf( hfwi, cSenBuf, iPacketLen+1);
            cSenBuf[iLen]='\0';

            NetSend( HK_KEY_MONSERVER, cSenBuf, iLen );
            printf("scc...GetWebSSIDInfo..cSenBuf=%s...\n", cSenBuf );
            free( cSenBuf);
            cSenBuf=NULL;
        }
        DictDestroy( hfwi );
        return;
    }
    else //search wifi success.
    {
        int i = 0;
        int iCount = wifiFind.count;
        printf("get Web wifiCount=%d..\n", iCount);
        SetParamUN( hfwi, HK_KEY_COUNT, iCount );

        //HK_DEBUG_PRT("search wifi count = %d\n", iCount);
        //for (i = 0; i < iCount; i++)
        //{
        //    HK_DEBUG_PRT("[%d]: ssid=%s\t authmode=%d\t  enctype=%d\t iSingal=%d\n",
        //            i, wifiFind.wifi_info[i].ssid, wifiFind.wifi_info[i].authmode, wifiFind.wifi_info[i].enctype, wifiFind.wifi_info[i].iSignal);
        //}

        if (iCount > 0)
        {
            if (Sort_WifiInfo(&wifiFind))
            {
                printf(".........sort wifi info failed.........\n"); 
            }
        }

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

        HKFrameHead *framePacket = CreateFrameB();
        DictSetInt( framePacket, HK_KEY_MAINCMD, nCmd );
        DictSetInt( framePacket, HK_KEY_SUBCMD, iSubCmd );
        DictSetInt( framePacket, HK_KEY_FLAG, 1 );
        DictSetInt( framePacket, HK_KEY_UIPARAM, ulParam );
        DictSetStr( framePacket, HK_KEY_FROM, cUserid );
        DictSetStr( framePacket, HK_KEY_TO, cDevid );
        DictSetInt( framePacket, HK_KEY_POPEDOM, 3 );

        int iPacketLen = GetFrameSize( hfwi );
        char *cBuf = malloc( iPacketLen );
        if (NULL != cBuf)
        {
            GetFramePacketBuf( hfwi, cBuf, iPacketLen );
            DictSetStr( framePacket, HK_KEY_VALUEN, cBuf );

            char *cSenBuf = malloc(iPacketLen+2048);
            if(NULL != cSenBuf)
            {
                int iLen = GetFramePacketBuf( framePacket, cSenBuf, iPacketLen+2048 );
                cSenBuf[iLen]='\0';

                NetSend( HK_KEY_MONSERVER, cSenBuf, iLen );
                printf("scc Get WEB wifiinfo=%s..\n", cSenBuf);
                free(cSenBuf);
                cSenBuf=NULL;
            }
            free( cBuf );
            cBuf = NULL;
        }
        DictDestroy( hfwi );
        DictDestroy( framePacket );
    }
}

//get video info
static void OnWebGetVideoInfo(int nCmd ,int iSubCmd, Dict *d)
{
    char bufMD[512]={0};
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    
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
    HK_KEY_EXPOSURE"=0;"\
    HK_KEY_QUALITE"=3;"\
    HK_KEY_FLAG"=2;"
    unsigned int nLevel = (2<<8);
    char prop[128];
    char buf[sizeof(HK_STR_INBAND_DESC) + sizeof(HK_STR_INBAND_DIGIT) + 256];
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
            cUserid,
            bufMD,
            video_properties_.vv[HKV_VEncIntraFrameRate],
            video_properties_.vv[HKV_VEncCodec],
            video_properties_.vv[HKV_MotionSensitivity],
            video_properties_.vv[HKV_VinFormat],
            video_properties_.vv[HKV_BitRate],
            iHue,
            video_properties_.vv[HKV_SharpnessLevel],
            g_iCifOrD1,
            iBrig+1,
            iSatur,
            iContr,
            FrequencyLevel,
            video_properties_.vv[HKV_Cbr],
            g_DevVer,
            video_properties_.vv[HKV_Yuntai],
            g_irOpen
            ); 

    Dict *DictPacket = DictCreate(buf , strlen(buf) );
    SetParamUN( DictPacket, HK_KEY_STATE, g_nSignLogin );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );
    char cBuf[4048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    printf("scc...GetWebVideoInfo..cBuf=%s...\n", cBuf );
    DictDestroy( DictPacket );
}

static void OnWebGetNetInfo(int nCmd, int iSubCmd, Dict *d)
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    DictSetStr( DictPacket, HK_NET_BOOTPROTO, eth0Addr.ipMode);
    DictSetStr( DictPacket, HK_KEY_IP, eth0Addr.ip);
    DictSetStr( DictPacket, HK_NET_DNS0, eth0Addr.dns1);
    DictSetStr( DictPacket, HK_NET_DNS1, eth0Addr.dns2);
    DictSetStr( DictPacket, HK_KEY_NETMASK, eth0Addr.netmask);
    DictSetStr( DictPacket, HK_KEY_NETWET, eth0Addr.gateway);
    SetParamUN( DictPacket, HK_KEY_STATE, g_nSignLogin );
    SetParamUN( DictPacket, HK_KEY_VER,g_DevVer);

    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );
    char cBuf[4048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    printf("scc...OnWebGetNetInfo..cBuf=%s...\n", cBuf );
    DictDestroy( DictPacket );
}


static void OnGetWanDevParam( int nCmd,  Dict *d )
{
    int iSubCmd = GetParamUN( d,HK_KEY_SUBCMD );
    switch (iSubCmd )
    {
        case HK_MON_SETALARMINFO://Wan email
            OnGetWanAlarmInfo( nCmd, iSubCmd, d );
            break;
        case HK_MON_SET_FTPSERVER://Wan ftp
            OnGetWanFtpInfo(nCmd, iSubCmd, d);
            break;
        case HK_MON_PPOE://Wan ppoe
            OnGetWanPpoe( nCmd, iSubCmd, d);
            break;
        case HK_PENETRATE_DATA:
            OnPenetrateData( nCmd, iSubCmd, d);
            break;
        case HK_WEB_WIFI_INFO:
            OnGetWebWifiInfo(nCmd, iSubCmd, d);
            break;
        case HK_WEB_WIFI_SSID_INFO:
            OnGetWebSsidInfo(nCmd, iSubCmd, d);
            break;
        case HK_WEB_SD_INFO:
            OnWanGetDevSDParam( nCmd, d, iSubCmd );
            break;
        case HK_WEB_VIDEO_INFO:
            OnWebGetVideoInfo(nCmd ,iSubCmd, d);
            break;
        case HK_WEB_NET_INFO:
            OnWebGetNetInfo(nCmd, iSubCmd, d);
            break;

        default:
            break;
    }

}

//email alarm info
static void OnWanSetAlarmInfo(int nCmd, int iSubCmd, Dict *d )
{
    //memset(&hk_email, 0, sizeof(hk_email));

    //char *cDevid = DictGetStr( d, HK_KEY_FROM );
    //char *cUserid = DictGetStr( d, HK_KEY_TO );
    //unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );

    char *sedEmail   = DictGetStr(d, HK_KEY_ALERT_EMAIL);
    char *recEmail   = DictGetStr(d, HK_KEY_REEMAIL);
    char *smtpServer = DictGetStr(d, HK_KEY_SMTPSERVER);
    char *smtpUser   = DictGetStr(d, HK_KEY_SMTPUSER);
    char *smtpPswd   = DictGetStr(d, HK_KEY_PASSWORD);
    int iPort        = DictGetInt(d, HK_KEY_PORT);
    int secType      = DictGetInt(d, HK_WIFI_ENCRYTYPE);
    int isOpen       = 1;// DictGetInt(d, HK_KEY_ISALERT_SENSE);
    
    int iFlag = DictGetInt( d, HK_KEY_FUCTION );//(0, 1)5 5+1-send test email

    int iCount = 0;
    char *cEmailInfo = NULL;
    if ((iFlag != 5) && (iFlag != 6))
    {
        iCount          = DictGetInt(d, HK_KEY_COUNT);
        int isIoOpen    = DictGetInt(d, HK_KEY_IOIN); 
        int isMoveOpen  = DictGetInt(d, HK_KEY_EMAIL_MOVE_ALARM);
        int isSdError   = DictGetInt(d, HK_KEY_EMAIL_SD_ERROR);
        cEmailInfo      = DictGetStr(d, HK_KEY_EMAIL_INFO );

        if (NULL != cEmailInfo)
            conf_set(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_INFO, cEmailInfo);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_COUNT, iCount);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM, isMoveOpen);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_SD_ERROR, isSdError);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_IOIN, isIoOpen);

        hk_email.mcount      = iCount;
        hkSdParam.sdIoOpen   = isIoOpen;
        hkSdParam.sdError    = isSdError;
        hkSdParam.sdMoveOpen = isMoveOpen;

        //sccInitPicCount( iCount );
    }

    
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_PORT, iPort);
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_ISALERT_SENSE, isOpen);
    conf_set_int(HOME_DIR"/emalarm.conf", HK_WIFI_ENCRYTYPE, secType);
    if(NULL!=sedEmail)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_ALERT_EMAIL, sedEmail );
    if(NULL!=recEmail)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_REEMAIL, recEmail );
    if(NULL!=smtpServer)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_SMTPSERVER, smtpServer );
    if(NULL!=smtpUser)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_SMTPUSER, smtpUser );
    if(NULL!=smtpPswd)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_PASSWORD, smtpPswd );

    InitEmailInfo(isOpen,smtpServer,sedEmail,recEmail,smtpUser,smtpPswd,iPort,iCount,secType);
    if( iFlag == 0|| iFlag==5 )
        return;

    b_testEmail = true;
}

//ftp server
static void OnSetWanFtpServer(int nCmd, int iSubCmd, Dict *d )
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    //Dict *DictPacket = DictCreate( 0, 0 );

    int isOpen = DictGetInt( d, HK_KEY_ISALERT_SENSE);
    int iPort = DictGetInt(d, HK_KEY_PORT);
    char *ftpServer = DictGetStr(d, HK_KEY_FTPSERVER );
    char *ftpPswd = DictGetStr(d, HK_KEY_PASSWORD );
    char *ftpUser = DictGetStr(d, HK_KEY_FTPUSER );

    conf_set_int(HOME_DIR"/ftpbakup.conf", HK_KEY_PORT, iPort);
    conf_set_int(HOME_DIR"/ftpbakup.conf", HK_WIFI_OPENORCLOSE, isOpen);
    if(NULL!=ftpServer)
        conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_FTPSERVER, ftpServer );
    if(NULL!=ftpPswd)
        conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_PASSWORD, ftpPswd );
    if(NULL!=ftpUser)
        conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_FTPUSER, ftpUser );

    /*
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_FROM, cUserid );
    DictSetStr( DictPacket, HK_KEY_TO, cDevid );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 );

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+1;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
    {
        DictDestroy( DictPacket );
        return;
    }
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';
    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
    */
}

//wan set pppoe
static void OnSetWanPpoe(int nCmd, int iSubCmd, Dict *d )
{
    char *cDevid = DictGetStr( d, HK_KEY_FROM );
    char *cUserid = DictGetStr( d, HK_KEY_TO );
    unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
    Dict *DictPacket = DictCreate( 0, 0 );

    int isOpen = DictGetInt( d, HK_KEY_ISALERT_SENSE);
    char *pppoePswd = DictGetStr(d, HK_KEY_PASSWORD );
    char *pppoeUser = DictGetStr(d,HK_KEY_PPPOEID );

    //printf("SCC.Wan Set.ppoeUser=%s...ppoePswd=%s..isOpen=%d..\n", pppoeUser,pppoePswd,isOpen);
    conf_set_int(HOME_DIR"/hkpppoe.conf", HK_KEY_ISALERT_SENSE, isOpen);
    if(NULL!=pppoePswd && NULL !=pppoeUser )
    {
        conf_set(HOME_DIR"/hkpppoe.conf", HK_KEY_PPPOEID, pppoeUser );
        conf_set(HOME_DIR"/hkpppoe.conf", HK_KEY_PASSWORD, pppoePswd );
    }
    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+1;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
    {
        DictDestroy( DictPacket );
        return;
    }
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';
    NetSend( HK_KEY_MONSERVER, cBuf, iPacketLen );
    free( cBuf );
    DictDestroy( DictPacket );
}

void OnRestorationParam( )
{
    HK_DEBUG_PRT("......restore camera params to factory settings......\n");
  
    system("rm -rf /mnt/sif/RngMD.conf");
    system("rm -rf /mnt/sif/hkpppoe.conf");
    system("rm -rf /mnt/sif/ftpbakup.conf");
    system("rm -rf /mnt/sif/emalarm.conf");
    system("rm -rf /mnt/sif/sidlist.conf");

    /**reset hkclient.conf**/
    conf_set_int(HOME_DIR"/hkclient.conf", "LAN_PASSWD", 123456);

    /**reset hkipc.conf**/
    //conf_set_int(HOME_DIR"/hkipc.conf", "VinFormat", 1024); //bit rate.
    conf_set_int(HOME_DIR"/hkipc.conf", "VinFormat", 768); //bit rate.
    conf_set_int(HOME_DIR"/hkipc.conf", "CamSaturationLevel", 32);
    conf_set_int(HOME_DIR"/hkipc.conf", "SharpnessLevel", 38);
    conf_set_int(HOME_DIR"/hkipc.conf", "HueLevel", 0); //no need to set this.
    conf_set_int(HOME_DIR"/hkipc.conf", "BrightnessLevel", 32);
    conf_set_int(HOME_DIR"/hkipc.conf", "CamContrastLevel", 32 );
    conf_set_int(HOME_DIR"/hkipc.conf", "CamSaturationLevel", 32);
    conf_set_int(HOME_DIR"/hkipc.conf", "MotionSensitivity", 0);
    conf_set_int(HOME_DIR"/hkipc.conf", "FrequencyLevel", 1);
    //conf_set_int(HOME_DIR"/hkipc.conf", "Flip", 0);
    //conf_set_int(HOME_DIR"/hkipc.conf", "Mirror", 0);
    //conf_set_int(HOME_DIR"/hkipc.conf", "BitRate", 15 ); //frame rate.
    conf_set_int(HOME_DIR"/hkipc.conf", "BitRate", 25 ); //frame rate.
    conf_set_int(HOME_DIR"/hkipc.conf", "Cbr", 1);
    conf_set_int(HOME_DIR"/hkipc.conf", "CifOrD1", 9); //10:960P, 9:720P.
    conf_set_int(HOME_DIR"/hkipc.conf", "oneptz", 1 );
    conf_set_int(HOME_DIR"/hkipc.conf", "REPWD", 1);
    conf_set_int(HOME_DIR"/hkipc.conf", "iropen", 0);

    /**reset subipc.conf**/
    //conf_set_int(HOME_DIR"/subipc.conf", "stream", 64);
    conf_set_int(HOME_DIR"/subipc.conf", "stream", 48); //bit rate.
    //conf_set_int(HOME_DIR"/subipc.conf", "rate", 15); //frame rate.
    conf_set_int(HOME_DIR"/subipc.conf", "rate", 25); //frame rate.
    conf_set_int(HOME_DIR"/subipc.conf", "sat", 32); //saturation.
    conf_set_int(HOME_DIR"/subipc.conf", "con", 32); //contrast.
    conf_set_int(HOME_DIR"/subipc.conf", "bri", 32); //brightness.
    conf_set_int(HOME_DIR"/subipc.conf", "sha", 38); //sharpness.
    conf_set_int(HOME_DIR"/subipc.conf", "enc", 5); //encode resolution.
    conf_set_int(HOME_DIR"/subipc.conf", "smooth", 1);

    /**reset sdvideo.conf**/
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_PHONE, 0);
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_MOVE, 0);
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_OUT, 0);
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_AUTO, 1);
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_LOOP, 1);
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_SPLIT, 50);
    conf_set_int(HOME_DIR"/sdvideo.conf", SD_REC_AUDIO, 0);
    conf_set_int(HOME_DIR"/sdvideo.conf", HK_KEY_SDRCQC, 1);

    /**reset emalarm.conf**/
    conf_set_int(HOME_DIR"/emalarm.conf", "Count", 3);
    conf_set_int(HOME_DIR"/emalarm.conf", "isAlertSens", 1);
    conf_set_int(HOME_DIR"/emalarm.conf", "Prot", 25);
    conf_set_int(HOME_DIR"/emalarm.conf", "ioin", 1);
    conf_set_int(HOME_DIR"/emalarm.conf", "sdmove", 0);
    conf_set_int(HOME_DIR"/emalarm.conf", "sderror", 1);
    conf_set_int(HOME_DIR"/emalarm.conf", "nDataEnc", 0);
    conf_set(HOME_DIR"/emalarm.conf", "alertEmail", "");
    conf_set(HOME_DIR"/emalarm.conf", "reEmail", "");
    conf_set(HOME_DIR"/emalarm.conf", "smtpServer", "");
    conf_set(HOME_DIR"/emalarm.conf", "smtpUser", "");
    conf_set(HOME_DIR"/emalarm.conf", "Psw", "");

    /**reset net.cfg**/
    conf_set(HOME_DIR"/net.cfg", "IPAddressMode", "DHCP");

    /**reset wifinet.cfg**/
    conf_set_int(HOME_DIR"/wifinet.cfg", "isopen", 0); //isopen = 0;
    conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "DHCP"); //dhcp.
    conf_set(HOME_DIR"/wifinet.cfg", "safetype", "wpa2");
    conf_set(HOME_DIR"/wifinet.cfg", "encrytype", "AES");

    /**reset wifisid.conf**/
    conf_set_space(HOME_DIR"/wifisid.conf", "wifisid", "");
    conf_set_space(HOME_DIR"/wifisid.conf", "wifipassword", "");

    /**reset wpa_supplicant.conf**/
    conf_set(HOME_DIR"/wpa_supplicant.conf", "psk", "\"1234567890\"");
    conf_set(HOME_DIR"/wpa_supplicant.conf", "ssid", "\"1234567890\"");
    conf_set(HOME_DIR"/wep.conf", "ssid", "\"1234567890\"");
    conf_set(HOME_DIR"/open.conf", "ssid", "\"1234567890\"");

    /**reset port.conf**/
    conf_set_int(HOME_DIR"/port.conf", "NetRtspPort", 8554);
    conf_set_int(HOME_DIR"/port.conf", "NetOnvifPort", 8000);
    conf_set_int(HOME_DIR"/port.conf", "NetServerPort", 4001);
    conf_set_int(HOME_DIR"/port.conf", "NetWebPort", 80);
    conf_set_int(HOME_DIR"/port.conf", "NetDataPort", 4001);
    conf_set_int(HOME_DIR"/web.conf", "bOnvifEnable", 0); //disable onvif.

    /**ptz.conf**/
    conf_set_int(HOME_DIR"/ptz.conf", "preset1x", 0);
    conf_set_int(HOME_DIR"/ptz.conf", "preset1y", 0);
    conf_set_int(HOME_DIR"/ptz.conf", "preset9x", 0);
    conf_set_int(HOME_DIR"/ptz.conf", "preset9y", 0);

    printf("...restore camera params to factory settings ok!...\n");
    sleep(1);
    system("sync");
    sys_restart();
}

//update dev
static void OnUpdateDev()
{
    system("/mnt/sif/bin/updateApp.sh &");
    //sccSetSysInfo( g_pDvrInst,0, 109, 0 );
}

static void OnAlarmParam( Dict *d, const char *buf )
{
    int iThermaiAlarm= GetParamUN(d, HK_KEY_THERAMLALARM);// 1close 2open
    int iAudioAlarm = GetParamUN(d, HK_KEY_AUDIOALARM);//1close 2 3 4

    int iSensitivity = GetParamUN( d, HK_KEY_SENSITIVITY );
    printf("alarm param set sens=%d..\n", iSensitivity);
    OnMonSenSitivity( buf );
    if( iSensitivity > 0 )
    {
        hkSdParam.sdIoOpen = 1;
        hkSdParam.sdError  = 1;
        hkSdParam.sdMoveOpen = 0;
    }
    else
    {
        hkSdParam.sdIoOpen = 0;
        hkSdParam.sdError  = 0;
        hkSdParam.sdMoveOpen = 1;
    }
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM, hkSdParam.sdMoveOpen);
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_SD_ERROR, hkSdParam.sdError);
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_IOIN, hkSdParam.sdIoOpen);

    int iSdRec = GetParamUN(d, HK_KEY_SDMOVEREC);
    if( iSdRec > 0 )
    {
        conf_set_int(HOME_DIR"/sdvideo.conf", "alarm_move", 1);
        conf_set_int(HOME_DIR"/sdvideo.conf", "alarm_out", 1);
        //conf_set_int(HOME_DIR"/sdvideo.conf", HK_KEY_SDRCQC, iSdrcqc);
        conf_set_int(HOME_DIR"/sdvideo.conf", "loop_write", 1);
    }
    else
    {
        conf_set_int(HOME_DIR"/sdvideo.conf", "alarm_move", 0);
        conf_set_int(HOME_DIR"/sdvideo.conf", "alarm_out", 0);
    }

    int iPictures = DictGetInt(d, HK_KEY_COUNT );

    hk_email.mcount = iPictures;
    //sccInitPicCount( iPictures );
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_COUNT, iPictures);

    b_hkSaveSd=true;
}

//set web wifi info
static void OnWebSetWifiInfo(int nCmd, int iSubCmd, Dict *hf )
{
    int isOpenWifi = DictGetInt( hf, HK_WIFI_OPENORCLOSE );
    conf_set_int(HOME_DIR"/wifisid.conf", HK_WIFI_OPENORCLOSE, isOpenWifi);
    printf("sccc.open=%d.....\n", isOpenWifi);
    if (1 == isOpenWifi)
    {
        char *cSsid = DictGetStr( hf, HK_WIFI_SID);
        char *cSafeType = DictGetStr(hf, HK_WIFI_SAFETYPE );
        char *cSafeOption = DictGetStr(hf, HK_WIFI_SAFEOPTION );
        char *cKeyType = DictGetStr( hf, HK_WIFI_KEYTYPE );
        char *cPasswd = DictGetStr( hf, HK_WIFI_PASSWORD );
        char *cBootProto = DictGetStr( hf, HK_NET_BOOTPROTO );
        char *cEnctyType = DictGetStr( hf, HK_WIFI_ENCRYTYPE );
        if (cSsid == NULL )
        {
            return;
        }

        if(strcmp(cBootProto, HK_NET_BOOTP_STATIC)==0 ) 
        {
            char *cIP = DictGetStr( hf, HK_KEY_IP);
            char *cMask = DictGetStr( hf, HK_KEY_NETMASK);
            char *cWet = DictGetStr( hf, HK_KEY_NETWET);
            char *cDns0 = DictGetStr( hf, HK_NET_DNS0);
            char *cDns1 = DictGetStr( hf, HK_NET_DNS1);
            conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "FixedIP");
            conf_set(HOME_DIR"/wifinet.cfg", "IPAddress", cIP );
            conf_set(HOME_DIR"/wifinet.cfg", "SubnetMask", cMask );
            conf_set(HOME_DIR"/wifinet.cfg", "Gateway", cWet );
            conf_set(HOME_DIR"/wifinet.cfg", "DNSIPAddress1", cDns0 );
            conf_set(HOME_DIR"/wifinet.cfg", "DNSIPAddress2", cDns1 );
        }
        else
        {
            conf_set(HOME_DIR"/wifinet.cfg", "IPAddressMode", "DHCP");
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
        char aryPSW[64]={0};
        strcpy( aryPSW,cPasswd );
        char* cDecPswd = (char *)DecodeStr( aryPSW );
        conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD, cDecPswd );
        StartWifiAP( hf, NULL );
        wrap_sys_restart();
    }
    else
    {
        conf_set(HOME_DIR"/wifinet.cfg", HK_WIFI_SAFETYPE, "");
        conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_PASSWORD,"" );
        conf_set_space(HOME_DIR"/wifisid.conf", HK_WIFI_SID, "");

        wrap_sys_restart();
    }
}

static void OnMonSetWebNetInfo( Dict *d )
{
    char *ipMode = DictGetStr( d, HK_NET_BOOTPROTO);
    char *iP = DictGetStr(d, HK_KEY_IP);
    char *dns1 = DictGetStr(d, HK_NET_DNS0 );
    char *dns2 = DictGetStr(d, HK_NET_DNS1 );
    char *netmask = DictGetStr( d, HK_KEY_NETMASK );
    char *gateway = DictGetStr( d, HK_KEY_NETWET );
    if( ipMode == NULL || iP == NULL || gateway ==NULL)
        return;
    if (strcmp(ipMode, "dhcp") == 0)
    {    
        conf_set(HOME_DIR"/net.cfg", "IPAddressMode","DHCP");
    }    
    else
    {   
        if (strcmp(ipMode, "static") == 0 || strcmp(ipMode, "bound") == 0  || strcmp(ipMode, "renew") == 0)
        {    
            conf_set(HOME_DIR"/net.cfg", "IPAddressMode","FixedIP");
            conf_set(HOME_DIR"/net.cfg", "IPAddress",iP);
            conf_set(HOME_DIR"/net.cfg", "DNSIPAddress1",dns1);
            conf_set(HOME_DIR"/net.cfg", "DNSIPAddress2",dns2);
            conf_set(HOME_DIR"/net.cfg", "Gateway",gateway);
            conf_set(HOME_DIR"/net.cfg", "SubnetMask",netmask);

            char aryCmd[128]={0};
            sprintf( aryCmd,"echo nameserver %s > /etc/resolv.conf",dns1 );
            system( aryCmd );
            system( "echo nameserver 8.8.8.8 >> /etc/resolv.conf" );
        }
    }
    sys_restart( );
}

static void OnSendData( Dict *d )
{
    char *cData = DictGetStr(d, HK_KEY_DEVPARAM );
    unsigned int ulParam = DictGetInt(d, HK_KEY_UIPARAM );
    printf("scc..sccRecvAPPData=%s......\n", cData);
    //sleep(20);
    //sccRecvAPPData( cData, ulParam );
    //printf("scc..sccRecvAPPData=%s..111111111....\n", cData);
}


//wan set
static void OnSetWanDevParam( int nCmd,  Dict *d,const char *buf)
{
    int iSubCmd = GetParamUN( d,HK_KEY_SUBCMD );
    switch (iSubCmd )
    {
        case HK_MON_SETALARMINFO://wan set
            OnWanSetAlarmInfo( nCmd, iSubCmd, d );
            break;
        case HK_MON_SET_FTPSERVER://wan set ftp
            OnSetWanFtpServer( nCmd, iSubCmd, d );
            break;
        case HK_MON_PPOE://wan set pppoe
            OnSetWanPpoe(nCmd, iSubCmd, d );
            break;
        case HK_MON_RESTORATION_PARAM:
            OnRestorationParam(  );
            break;
        case HK_MON_UPDATE_DEV:
            OnUpdateDev();
            break;
        case HK_MON_ALARMPARAM:
            OnAlarmParam( d, buf );
            break;
        case HK_WEB_WIFI_INFO:
            OnWebSetWifiInfo(nCmd, iSubCmd, d );
            break;
        case HK_WEB_SD_INFO:
            OnMonDevSdSet( d );
            break;
        case HK_WEB_NET_INFO:
            OnMonSetWebNetInfo( d );
            break;
        case HK_PENETRATE_DATA:
            OnSendData( d );
            break;
        default:
            break;
    }
}

static void OnUpdateUserPwd()
{
    //printf(".........update user pwd....................\n");
    conf_set_int(HOME_DIR"/hkipc.conf", "REPWD", 0);
}

int monc_callback(int len, const char* buf)
{
    Dict* d;
    if (len <= 0)
        return len;
    d = DictCreate(buf, len);
    if (!d)
        return -1;

    int nCmd = DictGetInt(d, HK_KEY_MAINCMD);
    HK_DEBUG_PRT("......cmd: %d......\n", nCmd);
    switch ( nCmd )
    {
        case HK_DEV_REGISTER:
            OnUpdateUserPwd();
            break;
        case HK_DEV_UPDATEPASSWD:
            OnDevUpdateLoaclPasswd(nCmd, d);
            break;
        case HK_DEV_RESYSTEM:
            OnDevResystem();
            break;
        case HK_MON_QUERYPHOTOLIST:
            OnDevQueryPhotoList( nCmd, d );
            break;
         case HK_MON_DELETEPHOTO:
            {
                int iDel = DictGetInt( d, HK_KEY_REC_ID );
                char *cFileName = DictGetStr( d, HK_KEY_REC_NAME );
                OnMonDeletePhoto( cFileName, iDel );
            }
            break;
         case HK_MON_DEV_SD_SET:
            OnMonDevSdSet(d );
            break;
         case HK_MON_FORMATCARD://format sd
            OnMonFormatCard(d );
            break;
         case HK_MRS_LOCAL_SETSERVERTIME://zone
            OnMonSetZone( d );
            break;
         case HK_PHONE_GET_DEV_PARAM:
            OnWanPhoneGetDevParam(nCmd, d );
            break;
        case HK_PHONE_SET_DEV_PARAM:
            OnWanPhoneSetDevParam( d, buf );
            break;
        case HK_MON_SETALARMINFO://email alarm info
            OnWanSetAlarmInfo( nCmd, nCmd, d );
            break;
        //case HK_MON_SET_FTPSERVER://ftp server
        //    OnSetFtpServer( d );
        //    break;
        case HK_MON_GET_DEVPARAM://Get Wan
            OnGetWanDevParam( nCmd,  d );
           break;
        case HK_MON_SET_DEVPARAM://Set Wan 
           OnSetWanDevParam( nCmd,  d, buf);
           break;
        default:
            //printf("%.*s\n", len, buf);
            break;
    }

    DictDestroy(d);
    return 0;
}

void OnLocalReadSdData( int nCmd, unsigned int hkid,int currentPage,int pageSize, unsigned int ulParam  )
{
    if(g_sdIsOnline==0)
        return;
    Dict *DictPacket = DictCreate( 0, 0 );
    if( g_TFPacket == NULL)
        g_TFPacket = DictCreate( 0, 0 );

    int iCount=0;
    if( currentPage == 1)
    {
        FILE *fp = NULL;
        if (!(fp = popen("ls -rt /mnt/mmc/snapshot", "r"))) 
        {
            printf("OnLocalReadSdData popen failed with:\n");
            return; 
        }

        char tmpbuf[512] = {0};
        char buf[512]={0};
        g_iTfCount=0;
        while (fgets(buf, sizeof(buf), fp)) 
        {
            char keyBuf[12]={0};
            sprintf(keyBuf, "%d", iCount++ );
            sscanf(buf, "%[^\n]", tmpbuf);
            DictSetStr( g_TFPacket, keyBuf, tmpbuf );

            g_iTfCount++;
            //printf("scc buf=%s", buf);
        }
        if (fp)  
        {
            pclose(fp);
            fp = NULL;
        }
    }
    
    if( g_iTfCount > 0)
    {
        int filstCount = currentPage*pageSize-pageSize;//1*20-20
        //printf("scc...filstC=%d..g_iTfCount=%d,currpage=%d,\n", filstCount,g_iTfCount,currentPage);
        int j=0;
        int iFileCount=0;
        for( j=0; j<pageSize; j++)
        {
            char keyBuf[64]={0};
            sprintf(keyBuf, "%d", g_iTfCount-j-filstCount-1);// filstCount+j );
            char *cFile = DictGetStr(g_TFPacket, keyBuf );//...hkv
            if(cFile != NULL)
            {
               //printf("get file name=%s.ilen=%d.\n", cFile, strlen(cFile));
                char cPatch[64]={0};
                snprintf(cPatch,sizeof(cPatch), "/mnt/mmc/snapshot/%s", cFile);
                unsigned long iFleSzie = get_file_size( cPatch );


                char cNewFile[64]={0};
                strcpy( cNewFile, cFile );
                char *pt = strstr(cNewFile,".hkv");
                if( pt != NULL ) 
                    *pt='\0';
                sprintf(keyBuf, "%s%d", HK_KEY_REC_NAME, j );
                DictSetStr( DictPacket, keyBuf, cNewFile );
                
                sprintf(keyBuf, "%s%d", HK_KEY_REC_ID, j );
                DictSetInt( DictPacket, keyBuf, g_iTfCount-j-filstCount-1);//filstCount+j );

                sprintf(keyBuf, "%s%d", HK_KEY_MAX_FILESIZE, j);
                DictSetInt( DictPacket, keyBuf, iFleSzie );

                iFileCount++;
            }
            else
            {
                break;
            }
        }
        //printf(".sd list count=%d...\n", iFileCount);
        DictSetInt( DictPacket, HK_KEY_COUNT, iFileCount );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, g_iTfCount );
    }
    else
    {
        //printf("11111111.sd list count=0...\n");
        DictSetInt( DictPacket, HK_KEY_COUNT, 0 );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, 0 );
    }
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 ); 

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+100;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
        return;
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    if( NULL!=cBuf )
        free( cBuf );
    DictDestroy( DictPacket );
}

#if 0
void OnLocalReadSdData( int nCmd, unsigned int hkid,int currentPage,int pageSize, unsigned int ulParam  )
{
    int allCount = g_allcount - g_delcount;
    int iCount =0;
    int iFilst;// = g_delcount+1;
    Dict *DictPacket = DictCreate( 0, 0 );

    if(allCount>0 && currentPage>0 )
    {
        int filstCount = currentPage*pageSize-pageSize;
        int i=0;
        iFilst = g_allcount-filstCount;
        int j=1;
        for( j=1; j<=pageSize; j++)
        {
            char keyBuf[64]={0};
            sprintf(keyBuf, "%s%d", HK_KEY_VALUEN, iFilst-j );
            char *cFile = DictGetStr(frameRec, keyBuf );//time
            if(cFile != NULL)
            {
                //printf("get file name=%s.ilen=%d.\n", cFile, strlen(cFile));
                char cPatch[64]={0};
                snprintf(cPatch,sizeof(cPatch), "/mnt/mmc/snapshot/%s.hkv", cFile);
                unsigned long iFleSzie = get_file_size( cPatch );
                //if( iFleSzie > 0 )
                {
                    sprintf(keyBuf, "%s%d", HK_KEY_REC_NAME, i );
                    DictSetStr( DictPacket, keyBuf, cFile );
                    sprintf(keyBuf, "%s%d", HK_KEY_REC_ID, i );
                    DictSetInt( DictPacket, keyBuf, iFilst-i );

                    sprintf(keyBuf, "%s%d", HK_KEY_MAX_FILESIZE, i);
                    DictSetInt( DictPacket, keyBuf, iFleSzie );
                    iCount++;
                    i++;
                }
            }
            else
            {
                if ((allCount-filstCount) > pageSize )
                {
                    pageSize++;
                    filstCount++;
                }
            }
        }
        //printf(".sd list count=%d...\n", iCount);
        DictSetInt( DictPacket, HK_KEY_COUNT, iCount );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, allCount );
    }
    else
    {
        //printf(".sd list count=0...\n");
        DictSetInt( DictPacket, HK_KEY_COUNT, 0 );
        DictSetInt( DictPacket, HK_KEY_LISTCOUNT, 0 );
    }
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetInt( DictPacket, HK_KEY_POPEDOM, 3 ); 

    char *cBuf=NULL;
    int iPackSize = DictEncodedSize(DictPacket)+100;
    cBuf = malloc( iPackSize );
    if ( NULL == cBuf )
        return;
    int iPacketLen = DictEncode( cBuf, iPackSize, DictPacket );
    cBuf[iPacketLen]='\0';

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    if( NULL!=cBuf )
        free( cBuf );
    DictDestroy( DictPacket );
}
#endif

static void OnMonDeletePhoto(const char *cFileName, int iDel )
{
    char buf[256]={0};
    if( iDel == 0 && NULL != cFileName )
    {
        sprintf( buf, "rm -rf /mnt/mmc/snapshot/%s", cFileName );
        system ( buf );
        file_SdDeleteFile( cFileName );
    }
    else if( iDel == 1 )
    {
        sd_record_stop();
        sleep(1);
        g_delcount=0;
        g_allcount=0;
        
        sprintf( buf, "rm -rf /mnt/mmc/snapshot/*" );
        system ( buf );
        
        //file_SdDeleteAllFile();

        b_hkSaveSd=true;
    }
}

static void OnPhoneGetDevParam(int nCmd, int hkid, Dict *hf )
{
    char *cDevid = GetParamStr( hf, HK_KEY_FROM ); 
    int iStream = conf_get_int(HOME_DIR"/subipc.conf", "stream" );
    int iRate = conf_get_int(HOME_DIR"/subipc.conf", "rate");
    int iEnc = conf_get_int(HOME_DIR"/subipc.conf", "enc");

    int iSat = conf_get_int(HOME_DIR"/hkipc.conf", "CamSaturationLevel");
    int iCon = conf_get_int(HOME_DIR"/hkipc.conf", "CamContrastLevel");
    int iBri = conf_get_int(HOME_DIR"/hkipc.conf", "BrightnessLevel");
    int iSha = conf_get_int(HOME_DIR"/hkipc.conf", "SharpnessLevel");
    int iSensitivity = conf_get_int(HOME_DIR"/hkipc.conf", "MotionSensitivity");

    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_ALERT_SENSE, iSensitivity );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetStr( DictPacket, HK_KEY_FROM, cDevid );
    DictSetInt( DictPacket, "stream", iStream );
    DictSetInt( DictPacket, HK_KEY_RATE, iRate );
    //DictSetInt( DictPacket, "enc", iEnc ); 
    DictSetInt( DictPacket, HK_KEY_SATURATION, iSat );
    DictSetInt( DictPacket, HK_KEY_CONTRAST,iCon );
    DictSetInt( DictPacket, HK_KEY_BRIGHTNESS, iBri );
    DictSetInt( DictPacket, HK_KEY_SHARPNESS, iSha );

    DictSetInt( DictPacket, HK_KEY_FUCTION,1);//ver
    DictSetInt( DictPacket, HK_KEY_COUNT,hk_email.mcount);
    if(hkSdParam.moveRec==1 && hkSdParam.outMoveRec==1 )
    {
        DictSetInt( DictPacket, HK_KEY_SDMOVEREC,1);
    }
    else
    {
        DictSetInt( DictPacket, HK_KEY_SDMOVEREC,0);
    }


    char cBuf[2048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    DictDestroy( DictPacket );
}

static void OnLANPhoneSetDevParam(Dict* d )
{
    int mc = GetParamUN( d,HK_KEY_SUBCMD );
    switch (mc )
    {
        case HK_SET_PHONE_DEVPARAM:
            OnPhoneSetDevParam( d );
            break;
        case HK_PHONE_RESTORE:
            OnPhoneRestore();
            break;
        default:
            OnPhoneSetDevParam( d );
            break;
    }
}

//Get alarm email info
static void OnGetAlarmInfo( int nCmd, int hkid, int iSubCmd, Dict *d )
{
    //char *cDevid = GetParamStr( hf, HK_KEY_FROM ); 
    int isOpen = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_ISALERT_SENSE);
    int iPort = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_PORT);
    int iCount = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_COUNT);
    int isIoOpen = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_IOIN);
    int nDataEnc = conf_get_int(HOME_DIR"/emalarm.conf", HK_WIFI_ENCRYTYPE);
    int isMoveOpen = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM);
    int isSdError = conf_get_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_SD_ERROR);
    char cEmailInfo[256]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_INFO, cEmailInfo, 256);

    char sedEmail[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_ALERT_EMAIL, sedEmail, 128);
    char recEmail[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_REEMAIL, recEmail, 128);
    char smtpServer[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_SMTPSERVER, smtpServer, 128);
    char smtpUser[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_SMTPUSER, smtpUser, 128);
    char smtpPswd[128]={0};
    conf_get( HOME_DIR"/emalarm.conf", HK_KEY_PASSWORD, smtpPswd, 128);
    //printf("SCC.Lan..GetAlarmEmail isOpen=%d,,iPort=%d..sedEmail=%s...\n",isOpen,iPort,sedEmail);

    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_EMAIL_MOVE_ALARM, isMoveOpen);
    DictSetInt( DictPacket, HK_KEY_EMAIL_SD_ERROR, isSdError);
    DictSetStr( DictPacket, HK_KEY_EMAIL_INFO, cEmailInfo );
    DictSetInt( DictPacket, HK_WIFI_ENCRYTYPE, nDataEnc  );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_ISALERT_SENSE, isOpen );
    DictSetInt( DictPacket, HK_KEY_PORT, iPort ); 
    DictSetInt( DictPacket, HK_KEY_IOIN, isIoOpen );
    DictSetStr( DictPacket, HK_KEY_ALERT_EMAIL, sedEmail );
    DictSetStr( DictPacket, HK_KEY_REEMAIL, recEmail );
    DictSetStr( DictPacket, HK_KEY_SMTPSERVER, smtpServer );
    DictSetStr( DictPacket, HK_KEY_SMTPUSER, smtpUser );
    DictSetStr( DictPacket, HK_KEY_PASSWORD, smtpPswd );
    DictSetStr( DictPacket, HK_KEY_TO, getenv("USER"));
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
    DictSetInt( DictPacket, HK_KEY_COUNT, iCount );
    //printf("....devid=%s.....\n", getenv("USER"));

    char cBuf[3048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    DictDestroy( DictPacket );
}

//ftp info
static void OnGetFtpInfo( int nCmd, int hkid, int iSubCmd, Dict *d)
{
    //char *cDevid = GetParamStr( hf, HK_KEY_FROM ); 
    int isOpen = conf_get_int(HOME_DIR"/ftpbakup.conf", HK_WIFI_OPENORCLOSE );
    int iPort = conf_get_int(HOME_DIR"/ftpbakup.conf", HK_KEY_PORT);
    char ftpServer[128]={0};
    conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_FTPSERVER, ftpServer, 128);
    char ftpPswd[128]={0};
    conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_PASSWORD,ftpPswd, 128);
    char ftpUser[128]={0};
    conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_FTPUSER,ftpUser, 128);
    char ftpType[64]={0};
    conf_get( HOME_DIR"/ftpbakup.conf", HK_KEY_CODETYPE,ftpType, 64);

    //printf("SCC.GetFtp..isOpen=%d,ftpServer=%s,ftpUser=%s..\n",isOpen, ftpServer, ftpUser);
    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_ISALERT_SENSE, isOpen );
    DictSetInt( DictPacket, HK_KEY_PORT, iPort ); 
    DictSetStr( DictPacket, HK_KEY_FTPSERVER, ftpServer );
    DictSetStr( DictPacket, HK_KEY_PASSWORD, ftpPswd );
    DictSetStr( DictPacket, HK_KEY_FTPUSER, ftpUser );
    DictSetStr( DictPacket, HK_KEY_CODETYPE, ftpType );
    DictSetStr( DictPacket, HK_KEY_TO, getenv("USER"));
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );

    char cBuf[3048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    DictDestroy( DictPacket );
}

static void OnGetPpoe( int nCmd, int hkid ,int iSubCmd, Dict *d)
{
    int isOpen = conf_get_int(HOME_DIR"/hkpppoe.conf", HK_KEY_ISALERT_SENSE );
    char cPppoeUser[128]={0};
    conf_get( HOME_DIR"/hkpppoe.conf", HK_KEY_PPPOEID, cPppoeUser, 128);
    char cPswd[128]={0};
    conf_get( HOME_DIR"/hkpppoe.conf", HK_KEY_PASSWORD,cPswd, 128);

    //printf("SCC.Get.ppoeUser=%s,cPswd=%s.isOpen=%d.\n", cPppoeUser, cPswd, isOpen);
    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    //DictSetStr( DictPacket, HK_KEY_FROM, cDevid );
    DictSetInt( DictPacket, HK_KEY_ISALERT_SENSE, isOpen );
    DictSetStr( DictPacket, HK_KEY_PASSWORD, cPswd );
    DictSetStr( DictPacket, HK_KEY_PPPOEID, cPppoeUser );
    DictSetStr( DictPacket, HK_KEY_TO, getenv("USER"));
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );

    char cBuf[3048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    DictDestroy( DictPacket );
}

//get video info
static void OnGetDevVideoInfo(int nCmd, int hkid ,int iSubCmd, Dict *d)
{
    char buf[1024*8]={0};
    unsigned int ulParam = DictGetInt(d, HK_KEY_UIPARAM);
    sccGetVideoInfo(buf, nCmd, iSubCmd, ulParam );

    printf("Get Dev Video Info=%s....\n", buf);
    NetSendLan( hkid, HK_AS_MONS, buf, strlen(buf) );
}

//get onvif open or close
static void OnGetOnvifInfo(int nCmd, int hkid ,int iSubCmd, Dict *d)
{
    unsigned int ulParam = DictGetInt(d, HK_KEY_UIPARAM);
    int isOpen = conf_get_int(HOME_DIR"/web.conf", "bOnvifEnable" );
    printf(".....%s....isOpen:%d....\n", __func__, isOpen);

    Dict *DictPacket = DictCreate( 0, 0 );
    DictSetInt( DictPacket, HK_KEY_MAINCMD, nCmd );
    DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
    DictSetInt( DictPacket, HK_KEY_OPENONVIF, isOpen );
    DictSetInt( DictPacket, HK_KEY_UIPARAM, ulParam );
    DictSetStr( DictPacket, HK_KEY_TO, getenv("USER"));
    DictSetInt( DictPacket, HK_KEY_FLAG, 1 );

    char cBuf[3048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
    cBuf[iPacketLen]='\0';
    printf("Get Dev Onvif Open=%s....\n", cBuf);

    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
    DictDestroy( DictPacket );

    printf("Get Dev Onvif Open=%s....\n", cBuf);
}

//Lan email alarm info
static void OnGetDevParam(int nCmd, int hkid, Dict *d )
{
    int iSubCmd = GetParamUN( d,HK_KEY_SUBCMD );
    switch (iSubCmd )
    {
        case HK_MON_SETALARMINFO://email
            OnGetAlarmInfo( nCmd, hkid, iSubCmd, d );
            break;
        case HK_MON_SET_FTPSERVER://ftp
            OnGetFtpInfo(nCmd, hkid, iSubCmd, d);
            break;
        case HK_MON_PPOE://ppoe
            OnGetPpoe( nCmd, hkid ,iSubCmd, d);
            break;
        case HK_MON_LAN_DEVVIDEOINFO:
            OnGetDevVideoInfo(nCmd, hkid ,iSubCmd, d);
            break;
        case HK_MON_ONVIF_INFO:
            OnGetOnvifInfo(nCmd, hkid ,iSubCmd, d);
            break;
        default:
            break;
    }
}

static void OnSetPpoe( Dict *d )
{
    int isOpen = DictGetInt( d, HK_KEY_ISALERT_SENSE);
    char *pppoePswd = DictGetStr(d, HK_KEY_PASSWORD );
    char *pppoeUser = DictGetStr(d,HK_KEY_PPPOEID );

    conf_set_int(HOME_DIR"/hkpppoe.conf", HK_KEY_ISALERT_SENSE, isOpen);
    if(NULL!=pppoePswd && NULL !=pppoeUser )
    {
        conf_set(HOME_DIR"/hkpppoe.conf", HK_KEY_PPPOEID, pppoeUser );
        conf_set(HOME_DIR"/hkpppoe.conf", HK_KEY_PASSWORD, pppoePswd );
    }
}

//email alarm info
static void OnLanSetAlarmInfo(int nCmd, int hkid, Dict *d)
{
    //memset(&hk_email, 0, sizeof(hk_email));
    //usleep(40*1000);
    char *sedEmail   = DictGetStr(d, HK_KEY_ALERT_EMAIL);
    char *recEmail   = DictGetStr(d, HK_KEY_REEMAIL);
    char *smtpServer = DictGetStr(d, HK_KEY_SMTPSERVER);
    char *smtpUser   = DictGetStr(d, HK_KEY_SMTPUSER);
    char *smtpPswd   = DictGetStr(d, HK_KEY_PASSWORD);
    int iPort        = DictGetInt(d, HK_KEY_PORT);
    int secType      = DictGetInt(d, HK_WIFI_ENCRYTYPE);
    int isOpen       = 1;//DictGetInt(d, HK_KEY_ISALERT_SENSE);
     
    int iFlag = DictGetInt(d, HK_KEY_FUCTION);
    
    printf("Lan Set Alarm......flag:%d.......\n", iFlag);
    int iCount = 0;
    char *cEmailInfo = NULL;
    if ((iFlag != 5) && (iFlag != 6))
    {
        iCount          = DictGetInt(d, HK_KEY_COUNT);
        cEmailInfo      = DictGetStr(d, HK_KEY_EMAIL_INFO);
        int isIoOpen    = DictGetInt(d, HK_KEY_IOIN);
        int isMoveOpen  = DictGetInt(d, HK_KEY_EMAIL_MOVE_ALARM);
        int isSdError   = DictGetInt(d, HK_KEY_EMAIL_SD_ERROR);

        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_MOVE_ALARM, isMoveOpen);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_SD_ERROR, isSdError);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_COUNT, iCount);
        conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_IOIN, isIoOpen);
        if (NULL != cEmailInfo)
            conf_set(HOME_DIR"/emalarm.conf", HK_KEY_EMAIL_INFO, cEmailInfo);

        hkSdParam.sdIoOpen   = isIoOpen;
        hkSdParam.sdError    = isSdError;
        hkSdParam.sdMoveOpen = isMoveOpen;
        hk_email.mcount      = iCount;
        printf("scc...set email alarm, sdMoveOpen:%d, sdIoOpen:%d, sdError:%d, mcount:%d...\n", hkSdParam.sdMoveOpen, hkSdParam.sdIoOpen, hkSdParam.sdError,hk_email.mcount);
        //sccInitPicCount( iCount );
    }

    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_PORT, iPort);
    conf_set_int(HOME_DIR"/emalarm.conf", HK_KEY_ISALERT_SENSE, isOpen);
    conf_set_int(HOME_DIR"/emalarm.conf", HK_WIFI_ENCRYTYPE, secType);
    if (NULL != sedEmail)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_ALERT_EMAIL, sedEmail);
    if (NULL != recEmail)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_REEMAIL, recEmail);
    if (NULL != smtpServer)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_SMTPSERVER, smtpServer);
    if (NULL != smtpUser)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_SMTPUSER, smtpUser);
    if (NULL != smtpPswd)
        conf_set(HOME_DIR"/emalarm.conf", HK_KEY_PASSWORD, smtpPswd);
    
    InitEmailInfo(isOpen, smtpServer, sedEmail, recEmail, smtpUser, smtpPswd, iPort, iCount, secType);
    if ((iFlag == 0) || (iFlag == 5))
        return;

    b_testEmail = true;
}

void sccSendTestEmailAlaram()
{
    //printf("[%s, %d] send test email !!!!\n", __func__, __LINE__);
    if (! b_testEmail)
        return;

    b_testEmail = false;
    int j = 0;
    char emailMSG[512]={0};
    sprintf(emailMSG, "Your camera(%s) has noticed movement!", getenv("USER"));

    if (0 == hk_email.secType)
    {
        j = send_mail_to_user(hk_email.mailserver, hk_email.passwd, hk_email.mailfrom, hk_email.mailto, hk_email.username, emailMSG, 0);
    }
    else
    {
        j = send_ssl_email(hk_email.mailserver, hk_email.passwd, hk_email.mailfrom, hk_email.mailto, hk_email.username, emailMSG, 0, hk_email.port, hk_email.secType);
    }

    if (0 == j)
    {
        printf("-----Test---Send Success!--------\n"); 
    }
}


//ftp server
static void OnSetLanFtpServer(int nCmd, int hkid, Dict *d )
{
    int isOpen = DictGetInt( d, HK_KEY_ISALERT_SENSE);
    int iPort = DictGetInt(d, HK_KEY_PORT);
    char *ftpServer = DictGetStr(d, HK_KEY_FTPSERVER );
    char *ftpPswd = DictGetStr(d, HK_KEY_PASSWORD );
    char *ftpUser = DictGetStr(d, HK_KEY_FTPUSER );
    
    char cServer[64]={0};
    conf_get(HOME_DIR"/ftpbakup.conf",HK_KEY_FTPSERVER,cServer,sizeof(cServer));
    char cPswd[64]={0};
    conf_get(HOME_DIR"/ftpbakup.conf",HK_KEY_PASSWORD,cPswd,sizeof(cPswd));
    char cUser[64]={0};
    conf_get(HOME_DIR"/ftpbakup.conf",HK_KEY_FTPUSER,cUser,sizeof(cUser));
    
    conf_set_int(HOME_DIR"/ftpbakup.conf", HK_KEY_PORT, iPort);
    conf_set_int(HOME_DIR"/ftpbakup.conf", HK_WIFI_OPENORCLOSE, isOpen);
    conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_FTPSERVER, ftpServer );
    conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_PASSWORD, ftpPswd );
    conf_set(HOME_DIR"/ftpbakup.conf", HK_KEY_FTPUSER, ftpUser );

    if( NULL==ftpServer || NULL==ftpPswd || NULL==ftpUser)
        return;
    if(strcmp(cServer,ftpServer)!=0||strcmp(cPswd,ftpPswd)!=0||strcmp(cUser,ftpUser)!=0)
    {
        if( g_nFtpIsOpen == 1)
        {
            char *cKill = "killall -9 ftp_server";
            system(cKill);
            sleep(1);
        }

        hk_start_ftp_server();
        return;
    }
}

static void OnSetLanPpoe(int nCmd, int hkid, Dict *d )
{
    int isOpen = DictGetInt( d, HK_KEY_ISALERT_SENSE);
    char *pppoePswd = DictGetStr(d, HK_KEY_PASSWORD );
    char *pppoeUser = DictGetStr(d,HK_KEY_PPPOEID );

    //printf("SCC.Set.ppoeUser=%s...ppoePswd=%s..isOpen=%d..\n", pppoeUser,pppoePswd,isOpen);
    conf_set_int(HOME_DIR"/hkpppoe.conf", HK_KEY_ISALERT_SENSE, isOpen);
    if(NULL!=pppoePswd && NULL !=pppoeUser )
    {
        conf_set(HOME_DIR"/hkpppoe.conf", HK_KEY_PPPOEID, pppoeUser );
        conf_set(HOME_DIR"/hkpppoe.conf", HK_KEY_PASSWORD, pppoePswd );
    }

    DictSetInt( d, HK_KEY_FLAG, 1 );
    char cBuf[3048]={0};
    int iPacketLen = DictEncode( cBuf, sizeof(cBuf), d );
    cBuf[iPacketLen]='\0';
    NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
}

//204 open,205 alarm, 206 alarm close, 207 test, 208 Get Dev.
static void OnControlDev( int nCmd,int iSubCmd, int hkid, Dict *d )
{
    int iCmd = DictGetInt( d, HK_KEY_DEVPARAM);
    int iValues = DictGetInt( d, HK_KEY_CHANNEL);
    if( iCmd == 208 )
    {
        Dict *DictPacket = DictCreate( 0, 0 );
        DictSetInt( DictPacket, HK_KEY_MAINCMD, HK_MON_GET_DEVPARAM );
        DictSetInt( DictPacket, HK_KEY_SUBCMD, iSubCmd );
        DictSetStr( DictPacket, HK_KEY_TO,getenv("USER"));
        DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
        if( g_sccDevCode == -1 )
        {
            DictSetInt( DictPacket, HK_KEY_PORT, 0 );
        }
        else if( g_sccDevCode == 0 )
        {
            DictSetInt( DictPacket, HK_KEY_PORT, -1 );
        }
        else
        {
            DictSetInt( DictPacket, HK_KEY_PORT, g_sccDevCode );
            g_sccDevCode = -1;
        }

        char cBuf[2048]={0};
        int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
        cBuf[iPacketLen]='\0';

        NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
        DictDestroy( DictPacket );
        return;
    }
    else if( iCmd == 209 )
    {
        Dict *DictPacket = DictCreate( 0, 0 );
        DictSetInt( DictPacket, HK_KEY_MAINCMD, HK_MON_GET_DEVPARAM );
        DictSetInt( DictPacket, HK_KEY_SUBCMD, HK_MON_GET_CONTROL_DEV );
        DictSetStr( DictPacket, HK_KEY_TO,getenv("USER"));
        DictSetInt( DictPacket, HK_KEY_FLAG, 1 );
        int nValue=0;
        sccGetSysInfo( 0, 0, iCmd, &nValue );
        DictSetInt( DictPacket, HK_KEY_PORT, nValue );

        char cBuf[2048]={0};
        int iPacketLen = DictEncode( cBuf, sizeof(cBuf), DictPacket );
        cBuf[iPacketLen]='\0';

        NetSendLan( hkid, HK_AS_MONS, cBuf, iPacketLen );
        DictDestroy( DictPacket );
        return;
    }

    printf("scc ControlDev Cmd=%d,,,values=%d...\n", iCmd, iValues);
    sccSetSysInfo(0, 0, iCmd, iValues);
}

//set Onvif Open or close
static void OnSetOnvifInfo(int nCmd,int iSubCmd, int hkid, Dict *d)
{
    printf(".....%s......\n", __func__);
    int isOpen = DictGetInt( d, HK_KEY_OPENONVIF);
    if( isOpen != 1)
        isOpen = 0;

    printf("set Onvif OpenOrclose=%d...\n", isOpen);
    conf_set_int(HOME_DIR"/web.conf","bOnvifEnable", isOpen );
    sys_restart( );
}

//Set Lan
static void OnSetDevParam( int nCmd, int hkid, Dict *d,const char *buf)
{
    int iSubCmd = GetParamUN( d,HK_KEY_SUBCMD );
    switch (iSubCmd )
    {
        case HK_MON_SETALARMINFO:
            OnLanSetAlarmInfo( nCmd, hkid, d );
            break;
        case HK_MON_SET_FTPSERVER:
            OnSetLanFtpServer( nCmd, hkid, d );
            break;
        case HK_MON_PPOE:
            OnSetLanPpoe(nCmd, hkid, d );
            break;
        case HK_MON_RESTORATION_PARAM:
            OnRestorationParam(  );
            break;
        case HK_MON_UPDATE_DEV:
            OnUpdateDev();
            break;
        case HK_MON_CONTROL_DEV:
            OnControlDev(nCmd, iSubCmd, hkid, d );
            break;
        case HK_MON_ALARMPARAM:
            OnAlarmParam( d, buf );
            break;
        case HK_MON_ONVIF_INFO:
            OnSetOnvifInfo(nCmd, iSubCmd, hkid, d);
            break;
        default:
            break;
    }
}

static int cb_lan_monc(unsigned int hkid, int asc, const char* buf, unsigned int len)
{
    Dict* d;
    if ( len <= 0 )
        return 0;
    d = DictCreate( buf, len );
    if( !d )
        return -1;

    int nCmd = DictGetInt( d, HK_KEY_MAINCMD );
    //printf("[%s, %d] ......nCmd: %d......\n", __func__, __LINE__, nCmd);
    switch ( nCmd )
    {
        case HK_MON_QUERYPHOTOLIST:
            {
                //printf("...Get Sd data.List...\n");
                //char *cFileName = DictGetStr( d, HK_KEY_TREENAME );
                int currentPage = DictGetInt( d, HK_KEY_CURRENTPAGE );
                int pageSize = DictGetInt( d, HK_KEY_PAGESIZE );
                unsigned int ulParam = DictGetInt( d, HK_KEY_UIPARAM );
                OnLocalReadSdData( nCmd, hkid, currentPage,pageSize, ulParam );
            }
            break;
        case HK_MON_DELETEPHOTO:
            {
                int iDel = DictGetInt( d, HK_KEY_REC_ID );
                char *cFileName = DictGetStr( d, HK_KEY_REC_NAME );
                OnMonDeletePhoto( cFileName, iDel );
            }
            break;
        case HK_MON_DEV_SD_SET:
            OnMonDevSdSet(d );
            break;
        case HK_MON_FORMATCARD://format sd
            OnMonFormatCard(d );
            break;
        case HK_MRS_LOCAL_SETSERVERTIME://zone
            OnMonSetZone( d );
            break;
        case HK_PHONE_GET_DEV_PARAM:
            OnPhoneGetDevParam(nCmd, hkid, d );
            break;
        case HK_PHONE_SET_DEV_PARAM:  //lan
            OnLANPhoneSetDevParam(d );
            break;
        case HK_MON_GET_DEVPARAM://Get Lan
            OnGetDevParam( nCmd, hkid, d );
           break;
        case HK_MON_SET_DEVPARAM://Set Lan
           OnSetDevParam( nCmd, hkid, d, buf);
           break;
        default:
            break;
    }
    DictDestroy( d );
    return 0;
}

static bool g_bflag = false;
void monc_start(Dict* d, int nTyp)
{
    if (! g_bflag)
    {
        g_bflag = true;
        SysRegistASLan_0(HK_AS_MONS, 0, &cb_lan_monc);

        SysRegistAS(HK_AS_MONS, &monc_callback);
    }
    
    if ( nTyp == HK_LOGIN_SUCCESS ) 
    {
        //SysRegistAS(HK_AS_MONS, &monc_callback);
        int iFlag = conf_get_int(HOME_DIR"/hkipc.conf", "REPWD");
        char buf[128]={0};
        int iLen = 0;
        char *pDevid = DictGetStr(d, HK_KEY_FROM );
        Dict *DictPacket = DictCreate(0,0);
        DictSetInt(DictPacket, HK_KEY_MAINCMD, HK_DEV_REGISTER);
        DictSetInt( DictPacket,HK_KEY_FLAG, iFlag );
        DictSetStr( DictPacket, HK_KEY_FROM, pDevid);

        iLen = DictEncode(buf, sizeof(buf), DictPacket);
        buf[iLen] = '\0';
        NetSend( HK_KEY_MONSERVER, buf, iLen );
        DictDestroy(DictPacket);
    }
}

void monc_stop()
{
    SysRegistAS(HK_AS_MONS, NULL);
}

static char* trim_right(char s[])
{
    int x = strlen(s);
    while (isspace(s[x-1]))
        --x;
    s[x] = '\0';
    return s;
}

int lspath(char* buf[], size_t len, const char* path, void* (*alloc)(size_t))
{
    int cnt = 0;
    FILE* f;
    char l[96];
    snprintf(l,sizeof(l), "/bin/ls -1 -- %s", path);
    if ( (f = popen(l, "r")))
    {
        while (fgets(l, sizeof(l), f))
        {
            trim_right(l);
            if (cnt < len)
            {
                size_t n = strlen(l) + 1;
                char *tmp = alloc(n);
                buf[cnt++] = strcpy(tmp, l);
            }
        }
        pclose(f);
    }
    return cnt;
}

int listhk(char* buf[], size_t len, const char* path, void* (*alloc)(size_t))
{
    char p[512];
    while (*path == '/')
        ++path;
    snprintf(p,sizeof(p), "/mnt/mmc/snapshot/%s", path);
    return lspath(buf, len, p, alloc);
}

//int main(int argc, char* const argv[])
//{
//    char* v[512];
//    int i, len = lspath(v, 512, argc>1?argv[1]:".", malloc);
//    int i, len = lspath(v, 512, "", malloc);
//    int i, len = lspath(v, 512, "2009-10-10", malloc);
//    for (i = 0; i < len; ++i)
//    {
//        HKLOG(L_DBG,v[i]);
//        free(v[i]);
//    }
//    return 0;
//}

