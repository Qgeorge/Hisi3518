#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ipc_sd.h"
#include "zlog.h"

#define VIDEO_PATH "/mnt/mmc/uusmt/"
#define MAX 1024

/***************** SD Card ******************/

//g_sdIsOnline 检测是否有sd卡
//g_sdIsOnline_f 检测是否已经挂载上了
short g_sdIsOnline = 0;
short g_sdIsOnline_f = 0;

HK_SD_PARAM_ hkSdParam;

/*过滤文件*/
static int customFilter(const struct dirent *pDir)
{
	if (strcmp(pDir->d_name, ".") && strcmp(pDir->d_name, ".."))
	{
		return 1;
	}
	return 0;
}
/*
 *将字符串转换为时间戳
 */
int get_times(char *cstr)
{
	printf("the cstring is %s\n", cstr);	
	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
	memset(tmp_time, 0, sizeof(struct tm));
	strptime(cstr,"%Y-%m-%d",tmp_time);
	//  strptime("2014-11-04","%Y-%m-%d",tmp_time);
	// printf("the year is %d\n", tmp_time->tm_year);
	// printf("the day is %d\n", tmp_time->tm_mday);
	// printf("the mon is %d\n", tmp_time->tm_mon);
	time_t t = mktime(tmp_time);
	printf("%ld\n",t);
	free(tmp_time);
	return t;

}
/*
 *获取录像的开始时间和结束时间
 */
int get_time_line(int *timeline)
{
	struct dirent **namelist;

	int n;
	int i;
	char stime[20] = {0};
	char etime[20] = {0};

	n = scandir(VIDEO_PATH, &namelist, customFilter, alphasort);

	if (n < 0)
	{
		perror("scandir");
	}
	else
	{
		timeline[0] = get_times(namelist[0]->d_name);
		timeline[1] = get_times(namelist[n-1]->d_name);

		for (i = 0; i < n; i++)
		{
			printf("%s\n", namelist[i]->d_name);
			free(namelist[i]);
		}
		free(namelist);
	}
	printf("get time is over\n");
}
#if 0
int get_file_count()
{
	DIR *dir;
	struct dirent * ptr;
	int total = 0;
	char path[MAX];
	char *root = VIDEO_PATH;
	dir = opendir(root); /* 打开目录*/
	if(dir == NULL)
	{
		perror("fail to open dir");
		return -1;
	}

	errno = 0;
	while((ptr = readdir(dir)) != NULL)
	{
		//顺序读取每一个目录项；
		//跳过“..”和“.”两个目录
		if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)
		{
			continue;
		}
		//printf("%s%s/n",root,ptr->d_name);
		//如果是目录，则递归调用 get_file_count函数

		if(ptr->d_type == DT_DIR)
		{
			sprintf(path,"%s%s/",root,ptr->d_name);
			//printf("%s/n",path);
			total += get_file_count(path);
		}

		if(ptr->d_type == DT_REG)
		{
			total++;
			//printf("%s%s/n",root,ptr->d_name);
		}
	}
	if(errno != 0)
	{
		printf("fail to read dir"); //失败则输出提示信息
		return -1;
	}
	closedir(dir);
	return total;
}
#endif
#if 1
/*
 *获取文件的数量
 */
int in_get_file_count(char *root)
{
	DIR *dir;
	struct dirent * ptr;
	int total = 0;
	char path[MAX];
	dir = opendir(root); /* 打开目录*/
	if(dir == NULL)
	{
		perror("fail to open dir");
		exit(1);
	}

	errno = 0;
	while((ptr = readdir(dir)) != NULL)
	{
		//顺序读取每一个目录项；
		//跳过“..”和“.”两个目录
		if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)
		{
			continue;
		}
		//printf("%s%s/n",root,ptr->d_name);
		//如果是目录，则递归调用 get_file_count函数

		if(ptr->d_type == DT_DIR)
		{
			sprintf(path,"%s%s/",root,ptr->d_name);
			//printf("%s/n",path);
			total += in_get_file_count(path);
		}

		if(ptr->d_type == DT_REG)
		{
			total++;
			//printf("%s%s/n",root,ptr->d_name);
		}
	}
	if(errno != 0)
	{
		printf("fail to read dir"); //失败则输出提示信息
		exit(1);
	}
	closedir(dir);
	return total;
}
#endif
int get_file_count()
{
	int size = 0;
	size = in_get_file_count(VIDEO_PATH);
	return size;
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

/**********************************************
 * func: check SD insert status;
 *       check SD storage info;
 *       SD data FTP backup;
 *       SD data operation for client.
 *********************************************/
void hk_load_sd()
{
	g_sdIsOnline = CheckSDStatus();
#if 0
	if (0 == access("/mnt/mmc", F_OK | R_OK | W_OK))
	{
		if(g_sdIsOnline == 1)
		  return;
	}
#endif
	if (g_sdIsOnline == 1) //index sd card inserted.
	{
		if(g_sdIsOnline_f == 1)
		{
			return;
		}
		mkdir("/mnt/mmc", 0755);
		system("umount /mnt/mmc/");
		usleep(1000);
		system("mount /dev/mmcblk0p1 /mnt/mmc/"); //mount SD.
		g_sdIsOnline_f = 1;
		ZLOG_INFO(zc, "mount tf card successfull!!\n");
		if(0 != access("/mnt/mmc/uusmt", F_OK))
		{
			system("/bin/mkdir -p /mmt/mmc/uusmt");
		}
		av_record_init("/mnt/mmc/uusmt");
	}
}
/*
 *获取SD卡里的配置信息
 */
void get_sd_conf()
{

	/*add for test----->sd to connect route*/
	memset(&hk_net_msg,0,sizeof(hk_net_msg));
	ZLOG_DEBUG(zc,"current func: get_sd_conf!\n");
	conf_get("/mnt/mmc/uusmt/configure","productid",hk_net_msg.productid,20);	
	conf_get("/mnt/mmc/uusmt/configure","manufacturerid",hk_net_msg.manufacturerid,20);	
	conf_get("/mnt/mmc/uusmt/configure","gateway",hk_net_msg.gw,20);	
	conf_get("/mnt/mmc/uusmt/configure","ip",hk_net_msg.ip,20);	

	printf(zc,"productid:%s\nmanufacturerid:%s\ngateway:%s\nip:%s\n",
				hk_net_msg.productid,hk_net_msg.manufacturerid,hk_net_msg.gw,hk_net_msg.ip);	

	ZLOG_INFO(zc,"productid:%s\nmanufacturerid:%s\ngateway:%s\nip:%s\n",
				hk_net_msg.productid,hk_net_msg.manufacturerid,hk_net_msg.gw,hk_net_msg.ip);	
}
/*
 *检查SD卡的状态
 */
int CheckSDStatus()
{
	struct stat st;
	if (0 == stat("/dev/mmcblk0", &st))
	{
		if (0 == stat("/dev/mmcblk0p1", &st))
		{
			//printf("...load TF card success...\n");
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

#if 0
int main()
{
	get_file_count();
}
#endif
