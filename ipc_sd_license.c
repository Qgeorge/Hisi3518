
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


/**device config files**/
#define CONF_HKCLIENT   "/mnt/sif/hkclient.conf"
#define CONF_NET        "/mnt/sif/net.cfg"
#define CONF_WIFINET    "/mnt/sif/wifinet.cfg"
#define CONF_PTZ        "/mnt/sif/ptz.conf"
#define CONF_AP         "/mnt/sif/hostapd.conf"
//#define CONF_SD_LICENSEVERSION "/mnt/mmc/License/version.txt"
#define CONF_SD_LICENSE "/mnt/mmc/License/license.txt"


/**device config params**/
#define DEV_USER        "USER"
#define DEV_PWD         "PASSWD"
#define DEV_MAC1        "MacAddress"
#define DEV_MAC2        "MacAddress"
#define DEV_PROXY       "PROXY"
#define DEV_NETTYPE     "HKDEVWIRELESS"
#define DEV_PTZTYPE     "HKDEVPTZ"

/**sd card config params Key**/
#define SD_USER        "id"
#define SD_PWD         "Psw"
#define SD_MAC1        "mac1"
#define SD_MAC2        "mac2"
#define SD_PROXY       "pichost"
#define SD_NETTYPE     "nettype"
#define SD_PTZTYPE     "ptztype"

/**************************************************************************************************************************
 * license config params on sd card, for example:
 *    id=E12345678;Psw=abcdef;mac1=00:14:04:18:20:19;mac2=00:14:04:18:2B:CC;pichost=www.uipcam.com;nettype=0;ptztype=1;
 *    id=E87654321;Psw=fedcba;mac1=00:14:04:18:20:19;mac2=00:14:04:18:2B:CC;pichost=www.uipcam.com;
 **************************************************************************************************************************/
typedef struct camera_license_info {
    char CamID[32];
    char CamPwd[32];
    char CamMac1[32];
    char CamMac2[32];
    char CamProxy[32];
    char CamNettype[32];
    char CamPtztype[32];
    char CamAPName[32];
    char CamM433Enable[32];
} License_Info_st;


static int Get_SD_License_Info(License_Info_st *pLicense)
{
    if (NULL == pLicense)
    {
        printf("error with <null> ptr transferred !\n"); 
        return -1;
    }

    FILE *fp = NULL;
    char cmd[128] = {0};
    char line_buf[256] = {0};
    int count = 0; //id counter.

    /* first: get the id count num */
    memset(cmd, '\0', sizeof(cmd));
    memset(line_buf, '\0', sizeof(line_buf));
    //sprintf(cmd, "sed -n '%dp' %s", count+2, CONF_SD_LICENSE);
    sprintf(cmd, "sed -n '1p' %s", CONF_SD_LICENSE); //print the first line.
    printf("cmd1: %s\n", cmd);

    if (NULL == (fp = popen(cmd, "r")))
    {
        printf("popen failed with:%d, %s\n", errno, strerror(errno));
        return errno;
    }
    fgets(line_buf, sizeof(line_buf), fp);
    sscanf(line_buf, "idcount=%d;", &count);
    printf("[%s, %d] idcount:%d\n", __func__, __LINE__, count);
    if (fp)  pclose(fp);
    fp = NULL;

    /* second: update id counter */
    memset(cmd, '\0', sizeof(cmd));
    memset(line_buf, '\0', sizeof(line_buf));
    sprintf(cmd, "sed -i 's/^idcount.*/idcount=%d;/' %s", count+1, CONF_SD_LICENSE);
    printf("cmd2: %s\n", cmd);
    system(cmd);
    sleep(1);

    /* third: get the next line buffer */
    memset(cmd, '\0', sizeof(cmd));
    sprintf(cmd, "sed -n '%dp' %s", count+2, CONF_SD_LICENSE);
    printf("cmd3: %s\n", cmd);
    if (NULL == (fp = popen(cmd, "r")))
    {
        printf("popen failed with:%d, %s\n", errno, strerror(errno));
        return errno;
    }
    fgets(line_buf, sizeof(line_buf), fp);
    printf("[%s, %d] line_buf:%s, len:%d\n", __func__, __LINE__, line_buf, strlen(line_buf));
    sscanf(line_buf, "%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]%*[^=]=%[^;]", 
            pLicense->CamID, pLicense->CamPwd, pLicense->CamMac1, pLicense->CamMac2, pLicense->CamProxy, pLicense->CamNettype, pLicense->CamPtztype, pLicense->CamAPName,pLicense->CamM433Enable);

    if (fp)  pclose(fp);    
    fp = NULL;
    return 0;
}

/*****************************************************************
 * func: check license info on sd card, and set into device.
 * return:
 *      0 on success, and -1 on error.
 *****************************************************************/
int HK_Check_SD_License(int SdOnline)
{
    if (0 == SdOnline)
    {
        printf("[%s, %d] There is no SD Card here !\n", __func__, __LINE__);
        return -1;
    }

    char devUser[32] = {0};
    int sd_version = 0, dev_version = 0;

    if (0 == access(CONF_SD_LICENSE, F_OK | R_OK)) //check file existence and readalbe.
    {
        conf_get(CONF_HKCLIENT, DEV_USER, devUser, sizeof(devUser));
        //sd_version = conf_get_int(CONF_SD_LICENSEVERSION, "VERSION");
        //dev_version = conf_get_int(CONF_HKCLIENT, "VERSION");
        printf("[%s, %d] devUser:%s, sd_version:%d, dev_version:%d\n", __func__, __LINE__, devUser, sd_version, dev_version);

        //if ((0 == strlen(devUser)) && (sd_version > dev_version))
        if (0 == strlen(devUser))
        {
            License_Info_st SD_LicenseInfo; //license info on sd card.

            memset(&SD_LicenseInfo, '\0', sizeof(SD_LicenseInfo));
            if (Get_SD_License_Info( &SD_LicenseInfo ) < 0)
            {
                printf("get sd license info failed !\n");
                return -1;
            }
            printf("[%s, %d] CamID:%s, CamPwd:%s, CamMac1:%s, CamMac2:%s, CamProxy:%s, CamNetType:%s, CamPtzType:%s, CamAPName:%s, CamM433:%s\n", \
                    __func__, __LINE__, SD_LicenseInfo.CamID, SD_LicenseInfo.CamPwd, SD_LicenseInfo.CamMac1,            \
                    SD_LicenseInfo.CamMac2, SD_LicenseInfo.CamProxy, SD_LicenseInfo.CamNettype, SD_LicenseInfo.CamPtztype, SD_LicenseInfo.CamAPName,\
                    SD_LicenseInfo.CamM433Enable);

            if (strlen(SD_LicenseInfo.CamID) > 0)
            {
                conf_set(CONF_HKCLIENT, "USER", SD_LicenseInfo.CamID);
            }

            if (strlen(SD_LicenseInfo.CamPwd) > 0)
            {
                conf_set(CONF_HKCLIENT, "PASSWD", SD_LicenseInfo.CamPwd);
            }

            if (strlen(SD_LicenseInfo.CamProxy) > 0)
            {
                conf_set(CONF_HKCLIENT, "PROXY", SD_LicenseInfo.CamProxy);
            }

            if (strlen(SD_LicenseInfo.CamMac1) > 0)
            {
                conf_set(CONF_NET, "MacAddress", SD_LicenseInfo.CamMac1);
            }

            if ((strlen(SD_LicenseInfo.CamNettype) > 0) && (0 != atoi(SD_LicenseInfo.CamNettype)))
            {
                conf_set_int(CONF_WIFINET, "HKDEVWIRELESS", atoi(SD_LicenseInfo.CamNettype));
            }

            if ((strlen(SD_LicenseInfo.CamPtztype) > 0) && (0 != atoi(SD_LicenseInfo.CamPtztype)))
            {
                if (0 == access(CONF_PTZ, F_OK | R_OK | W_OK))
                {
                    conf_set_int(CONF_PTZ, "HKDEVPTZ", atoi(SD_LicenseInfo.CamPtztype));
                }
            }

            if (strlen(SD_LicenseInfo.CamAPName) > 0)
            {
                if (0 == access(CONF_AP, F_OK | R_OK | W_OK))
                {
                    conf_set(CONF_AP, "ssid", SD_LicenseInfo.CamAPName);
                }
            }
            
            if(strlen(SD_LicenseInfo.CamM433Enable) > 0)
            {
                if(strcmp(SD_LicenseInfo.CamM433Enable, "1") == 0)
                {
                    conf_set_int("/mnt/sif/hkipc.conf", "m433enable",1);
                }
                else
                {
                    conf_set_int("/mnt/sif/hkipc.conf", "m433enable",0);
                }
            }
            //conf_set_int(CONF_HKCLIENT, "VERSION", sd_version);

            system("sync");
            sleep(2);
            system("reboot");
        }
    }

    return 0;
}

