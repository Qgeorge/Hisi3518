
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#ifndef	NULL
#define NULL 	0L
#endif

#define DEV_DEBUG   1

#define CONF_HKCLIENT       "/mnt/sif/hkclient.conf"
#define CONF_SD_UPGRADE     "/mnt/mmc/SDUpgrade/sd_upgrade.txt"
#define DIR_SD_UPGRADE      "/mnt/mmc/SDUpgrade"
#define UPGRADE_VERSION     "upgrade_version"
#define UPGRADE_IMGTYPE     "upgrade_image_type"
#define UPGRADE_TYPE_KERNEL "KERNEL"
#define UPGRADE_TYPE_ROOTFS "ROOTFS"
#define UPGRADE_IMG_KERNEL  "uImage"
#define UPGRADE_IMG_ROOTFS  "rootfs_64k.jffs2"
#define CONF_UPGRADE_FILE	"/mnt/mmc/SDUpgrade/app.img"

typedef struct conf_info
{
    char confKey[128];
    char confVal[128];
} __attribute__ ((packed)) Conf_Info_st;
//unsigned int g_ConfInfoCount = 0;
//Conf_Info_st g_ConfInfo[64];

//short int g_UpgradeVersion = 0; //new image version.


#if 0
void Reboot_System()
{
    printf("...... Reboot System ......\n");
	sync(); //flush file system buffers.
	sleep(1);
	system("reboot"); //reboot system.
    return;
}

/********************************************************
 * func: update new key=value to specified conf file.
 *******************************************************/
static int Set_Conf_Info(char *pFile, const char *pKey)
{
    if (NULL == pFile)
    {
        printf("file is not valid !\n"); 
        return -1;
    }

    if (NULL == pKey)
    {
        printf("key is not valed !\n"); 
        return -1;
    }
    printf("[%s, %d] file:%s, key:%s\n", __func__, __LINE__, pFile, pKey);

    FILE *fprd = NULL, *fpwt = NULL;
    char tmpbuf[256];
    unsigned int i = 0, count = 0;
    Conf_Info_st ConfInfo[32];

    fprd = fopen(pFile, "r"); //open net.cfg.
    if (NULL == fprd)
    {
        printf( "fopen failed with:%d, %s\n", errno, strerror(errno));
        return errno;
    }

    // get config data.
    memset( ConfInfo, '\0', sizeof(Conf_Info_st)*32);
    memset( tmpbuf, '\0', sizeof(tmpbuf) );
    while( fgets(tmpbuf, sizeof(tmpbuf), fprd) )
    {
        sscanf( tmpbuf, "%[^=]=%[^'\n']", ConfInfo[count].confKey, ConfInfo[count].confVal);
        printf("count:%d key:%s,   val:%s\n", count, ConfInfo[count].confKey, ConfInfo[count].confVal);

        if (! strcasecmp(ConfInfo[count].confKey, pKey))
        {
            strcpy(ConfInfo[count].confVal, g_UpgradeVersion); 
            ConfInfo[count].confVal[strlen(g_UpgradeVersion)] = '\0';
        }
        count++; //index the number of config data.
    }
    printf("count = %d\n", count);

    fpwt = fopen("temp.conf", "w+");
    if (NULL == fpwt)
    {
        printf( "fopen failed with:%d, %s\n", errno, strerror(errno));
        return errno;
    }

    for (i = 0; i < count; i++)
    {
        printf("i:%d key:%s,   val:%s\n", i, ConfInfo[i].confKey, ConfInfo[i].confVal);
        fprintf(fpwt, "%s=%s\n", ConfInfo[i].confKey, ConfInfo[i].confVal); 
    }

    rename("temp.conf", pFile);

    if (fprd) 
    {
        fclose(fprd);
        fprd = NULL;
    }
    if (fpwt) 
    {
        fclose(fpwt);
        fpwt = NULL;
    }

    return 0;
}
#endif

/******************************************************
 * func: Read config data from existed conf file.
 *****************************************************/
static int Get_Conf_Info(const char *pFile, const char *pKey, char *pVal)
{
    if (NULL == pFile)
    {
        printf("file is not valid!\n"); 
        return -1;
    }
    printf("[%s, %d] file:%s, key:%s\n", __func__, __LINE__, pFile, pKey);

    FILE *fprd = NULL;
    char tmpbuf[256];
    unsigned int i = 0;
    Conf_Info_st ConfInfo[32];

    memset( tmpbuf, '\0', sizeof(tmpbuf) );
    fprd = fopen(pFile, "r"); //open net.cfg.
    if (NULL == fprd)
    {
        printf( "fopen failed with:%d, %s\n", errno, strerror(errno));
        return errno;
    }

    // get config data.
    while( fgets(tmpbuf, sizeof(tmpbuf), fprd) )
    {
        sscanf( tmpbuf, "%[^=]=%[^'\n']", ConfInfo[i].confKey, ConfInfo[i].confVal);
        //printf("i:%d key:%s,   val:%s\n", i, ConfInfo[i].confKey, ConfInfo[i].confVal);

        if (! strcmp(ConfInfo[i].confKey, pKey))
        {
             if (NULL != ConfInfo[i].confVal)
             {
                strcpy(pVal, ConfInfo[i].confVal); 
                pVal[strlen(ConfInfo[i].confVal)] = '\0';
                break;
             }
        }
        i++; //index the number of config data.
    }

    //printf("\ni = %d\n", i);
    fclose( fprd ); //close net.cfg.
    return 0;
}


/************************************************************************
 * func: check upgrade image version & image name on SD Card. (zqj@HK)
 * return:
 *      -1: access sd card error.
 *      0:  not find upgrade image on sd card.
 *      1:  find new version image of kernel.
 *      2:  find new version image of rootfs.
 ************************************************************************/
static int Check_Upgrade_VersionAndImage()
{	
	int i = 0, ret = 0;	
    FILE *fp = NULL;
    char cmd[64] = {0};
    char tmpBuf[64] = {0};
    int NewImgVersion = 0; //new image version.
    char CurImgVersion[8] = {0};
    char UpgradeImgType[8] = {0};
	char UpgradeKernel[32] = {0};
	char UpgradeRootfs[32] = {0};

    //if (-1 == access(CONF_SD_UPGRADE, F_OK)) //check upgrade directory existence & operation.
    if (0 == access(CONF_SD_UPGRADE, F_OK | R_OK | W_OK)) //check upgrade directory existence & operation.
    {
        /** get sd upgrade image version **/
        memset(cmd, '\0', sizeof(cmd));
        memset(tmpBuf, '\0', sizeof(tmpBuf));
        sprintf(cmd, "sed -n '1p' %s", CONF_SD_UPGRADE); //print the first line.
        printf("cmd1: %s\n", cmd);
        if (NULL == (fp = popen(cmd, "r")))
        {
            printf("popen failed with:%d, %s\n", errno, strerror(errno));
            return -1;
        }
        fgets(tmpBuf, sizeof(tmpBuf), fp);
        sscanf(tmpBuf, "upgrade_version=%d", &NewImgVersion);
        printf("[%s, %d] NewImgVersion:%d\n", __func__, __LINE__, NewImgVersion);
        if (fp)  pclose(fp);
        fp = NULL;

        /** get sd upgrade image type: kernel or rootfs **/
        memset(cmd, '\0', sizeof(cmd));
        memset(tmpBuf, '\0', sizeof(tmpBuf));
        sprintf(cmd, "sed -n '2p' %s", CONF_SD_UPGRADE); //print the second line.
        printf("cmd2: %s\n", cmd);
        if (NULL == (fp = popen(cmd, "r")))
        {
            printf("popen failed with:%d, %s\n", errno, strerror(errno));
            return -1;
        }
        fgets(tmpBuf, sizeof(tmpBuf), fp);
        sscanf(tmpBuf, "upgrade_image_type=%s", UpgradeImgType);
        printf("[%s, %d] UpgradeImgType:%s\n", __func__, __LINE__, UpgradeImgType);
        if (fp)  pclose(fp);
        fp = NULL;

        /** get device current image version **/
        Get_Conf_Info(CONF_HKCLIENT, "VERSION", CurImgVersion);
        printf("[%s, %d] CurImgVersion:%s\n", __func__, __LINE__, CurImgVersion);

        /** check image version for upgrade **/
        if (atoi(CurImgVersion) >= NewImgVersion) //there is no need to upgrade.
        {
            printf("Note: there is no need to upgrade your firmware !\n"); 
            return 0;
        }
        else
        {
            struct dirent *entry = NULL;
            DIR *dir = opendir(DIR_SD_UPGRADE); //look for upgrade image file.
            if (NULL == dir)
            {
                printf("opendir failed with:%d, %s\n", errno, strerror(errno));
                return -1;
            }
            printf("[%s, %d] open upgrade dir:%s succuss!\n", __func__, __LINE__, DIR_SD_UPGRADE);

            while (NULL != (entry = readdir(dir)))
            {
                if (strstr(entry->d_name, UPGRADE_IMG_KERNEL))
                {
                    //printf("%s\n", entry->d_name);
                    strcpy(UpgradeKernel, entry->d_name); 
                    continue;
                }

                if (strstr(entry->d_name, UPGRADE_IMG_ROOTFS))
                {
                    //printf("%s\n", entry->d_name);
                    strcpy(UpgradeRootfs, entry->d_name); 
                    continue;
                }
            }
            closedir(dir);
            dir = NULL;
            entry = NULL;

            printf("......UpgradeImgType:%s, UpgradeRootfs:%s, UpgradeKernel:%s......\n", UpgradeImgType, UpgradeRootfs, UpgradeKernel);
            /*upgrade kernel, and kernel image existed*/
            if ((0 == strcmp(UpgradeImgType, UPGRADE_TYPE_KERNEL)) && (0 == strcmp(UpgradeKernel, UPGRADE_IMG_KERNEL)))
            {
                printf("Note: There is a new version of kernel image here !\n");

                /** update device image version **/
                memset(cmd, '\0', sizeof(cmd));
                memset(tmpBuf, '\0', sizeof(tmpBuf));
                sprintf(cmd, "sed -i 's/^VERSION.*/VERSION=%d/' %s", NewImgVersion, CONF_HKCLIENT); //update 'VERSION'.
                printf("cmd3: %s\n", cmd);
                system(cmd);
                sleep(1);

                return 1;
            }

            /*upgrade rootfs, and rootfs image existed*/
            if ((0 == strcmp(UpgradeImgType, UPGRADE_TYPE_ROOTFS)) && (0 == strcmp(UpgradeRootfs, UPGRADE_IMG_ROOTFS)))
            {
                printf("Note: There is a new version of rootfs image here !\n");

                /** update device image version **/
                memset(cmd, '\0', sizeof(cmd));
                memset(tmpBuf, '\0', sizeof(tmpBuf));
                sprintf(cmd, "sed -i 's/^VERSION.*/VERSION=%d/' %s", NewImgVersion, CONF_HKCLIENT); //update 'VERSION'.
                printf("cmd3: %s\n", cmd);
                system(cmd);
                sleep(1);

                return 2;
            }
        }
        return -1;
    }
    return 0;
}


static int Check_Upgrade_Image()
{
	char tmpBuf[64] = {0};
	FILE *fp;
	char cmd[64] = {0};
	int ret;
    	int NewImgVersion = 0; //new image version.
    	char CurImgVersion[8] = {0};
	#define UP_DEBUG 1
	#if UP_DEBUG
	printf("**********************check**************************************\n");
	#endif
	system("pkill crond");
	system("pkill udhcpc");
	system("pkill globefish");
	
	if (0 == access(CONF_SD_UPGRADE, F_OK | R_OK | W_OK))
	{
		memset(cmd, '\0', sizeof(cmd));
		memset(tmpBuf, '\0', sizeof(tmpBuf));
		sprintf(cmd, "sed -n '1p' %s", CONF_SD_UPGRADE);
		if (NULL == (fp = popen(cmd, "r")))
		{
			printf("popen failed with:%d, %s\n", errno, strerror(errno));
			return -1;
		}

		fgets(tmpBuf, sizeof(tmpBuf), fp);
		sscanf(tmpBuf, "upgrade_version=%d", &NewImgVersion);

		printf("[%s, %d] NewImgVersion:%d\n", __func__, __LINE__, NewImgVersion);
		if (fp)  pclose(fp);
		fp = NULL;
		Get_Conf_Info(CONF_HKCLIENT, "VERSION", CurImgVersion);
		if (atoi(CurImgVersion) >= NewImgVersion) //there is no need to upgrade.
		{
			printf("Note: there is no need to upgrade your firmware !\n");
			return 0;
		}
		else
		{
			memset(cmd, '\0', sizeof(cmd));
			sprintf(cmd, "dd if=%s of=/dev/mtdblock3", CONF_UPGRADE_FILE);
			ret = system(cmd);
			#if UP_DEBUG
			printf("**********************update success**************************************\n");
			#endif	
			if(ret == -1)
			{
				printf("upgrade img fail\n");
			}
			ret = system(cmd);
			#if UP_DEBUG
			printf("**********************update success**************************************\n");
			system("reboot -f");
			#endif	
			if(ret == -1)
			{
				printf("upgrade img fail\n");
			}
		}
	}
}
/****************************************************
 * func: check upgrade file on sd card.
 ****************************************************/
int HK_Check_SD_Upgrade(int SdOnline)
{
    Check_Upgrade_Image();
    #if 0
    int ret = 0;

    if (0 == SdOnline)
    {
        printf("[%s, %d] There is no SD Card here !\n", __func__, __LINE__);
        return -1;
    }

    ret = Check_Upgrade_VersionAndImage();
    if (ret < 0)
    {
        printf("...sd upgrade failed !\n");
        return -1;
    }
    else if (0 == ret)
    {
        //printf("[%s, %d] There is no need to upgrade your system firmware!\n", __func__, __LINE__);
        return 0;
    }
    else if (1 == ret)
    {
        //printf("it will upgrade your kernel !\n");
        sleep(1);
    #if DEV_DEBUG
        system("/bin/write_norflash_tool 1"); //upgrade kernel.
    #endif
    }
    else if (2 == ret)
    {
        //printf("it will upgrade your root filesystem !\n");
        sleep(1);
    #if DEV_DEBUG
        system("/bin/write_norflash_tool 2"); //upgrade filesyste.
    #endif
    }

    return 0;
    #endif
}

