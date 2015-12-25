/*
 * =====================================================================================
 *
 *       Filename:  ipc_cmd.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/13/2015 04:17:38 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "P2Pserver.h"
#include "ipc_sd.h"

typedef struct info                                                                                                                                                     {   
	int cmdid;
	int pinfo[0];
}info;

extern HK_SD_PARAM_ hkSdParam;
int g_start_video = 1;



void  SendCmdIntLink(PEER_INFO * _pLink, int *buf, int len)
{
	P2PNetServerSndMsgToLink(_pLink,(char *)buf, len);
}

/*==============================================
 *检测SD卡是否可用
 *==============================================*/
extern short g_sdIsOnline_f;
int get_sd_valid(PEER_INFO * _pLink)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*1);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 2;

	int flag;
	if(g_sdIsOnline_f == 1)
	{
		flag = 1;
	}
	else
	{
		flag = 0;
	}
	sendinfo->pinfo[0] = flag;
	SendCmdIntLink(_pLink, (int*)sendinfo, 2*sizeof(int));
	free(sendinfo);
}
/*==============================================
 *检测SD卡大小
 *==============================================*/
int get_sd_size(PEER_INFO * _pLink)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*7);
	int timeline[2] = {0};
	int size = 0;
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 3;

	printf("************%s*****%d\n", __FILE__, __LINE__);
	GetStorageInfo();
	sendinfo->pinfo[0] = hkSdParam.allSize;
	sendinfo->pinfo[1] = hkSdParam.haveUse;
	sendinfo->pinfo[2] = hkSdParam.leftSize;
	get_time_line(timeline);
	printf("************%s*****%d\n", __FILE__, __LINE__);

	sendinfo->pinfo[3] = timeline[0];
	printf("************%s*****%d\n", __FILE__, __LINE__);
	sendinfo->pinfo[4] = timeline[1];
	printf("************%s*****%d\n", __FILE__, __LINE__);
	size = get_file_count();
	if(size != -1)
	{
		sendinfo->pinfo[5] = size;
	}
	printf("************%s*****%d\n", __FILE__, __LINE__);
	sendinfo->pinfo[6] = hkSdParam.leftSize/2;
	printf("the mesg is %d\n", sendinfo->pinfo[0]);
	printf("the mesg is %d\n", sendinfo->pinfo[1]);
	printf("the mesg is %d\n", sendinfo->pinfo[2]);
	SendCmdIntLink(_pLink, (int*)sendinfo, 8*sizeof(int));
	return 0;
}

/*==============================================
 *开关录像命令
 *==============================================*/
int start_store_video(PEER_INFO * _pLink, int flag)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*1);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 5;
	g_start_video = flag;
	printf("*************set record_status g_start_video %d\n", g_start_video);
	
	sendinfo->pinfo[0] = 0;
	SendCmdIntLink(_pLink, (int*)sendinfo, 2*sizeof(int));

	free(sendinfo);
	return 0;
}

/*==============================================
 *获取视频列表
 *==============================================*/
int get_video_list(PEER_INFO * _pLink, int locattime)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*64*24);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 6;

	int count;
	struct tm* tmp_time  = (struct tm*)malloc(sizeof(struct tm));
	printf("**********locattime********************%d\n", locattime);
	tmp_time = gmtime(&locattime);

	count = record_list(sendinfo->pinfo+1, 64*24);
	//count = av_record_search(tmp_time->tm_year+1900, tmp_time->tm_mon+1, tmp_time->tm_mday+1, sendinfo->pinfo+1, 60*24);
	sendinfo->pinfo[0] = count;
	int i;
	for(i = 0;i < (count+1);i++)
	{
		printf("the remsg is %d\n", sendinfo->pinfo[i]);
	}
	SendCmdIntLink(_pLink, sendinfo, (count+1)*sizeof(int));

	free(sendinfo);
	free(tmp_time);
	return 0;
}
extern int g_play_minute;
int play_minute();
/*==============================================
 *定位视频
 *==============================================*/
int locat_video_time(PEER_INFO * _pLink, int locattime)
{
	pthread_t   thrd;
	printf("@@@@@@@@@@@@locattime*******%d*****\n",__LINE__);
	pthread_attr_t   attr;
//	pthread_cancel(thrd);
	pthread_attr_init(&attr);   
	printf("@@@@@@@@@@@@locattime*******%d*****\n",__LINE__);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	printf("@@@@@@@@@@@@locattime*******%d*****\n",__LINE__);
	if(pthread_create(&thrd, &attr, play_minute, &locattime))
	{
		perror( "pthread_create   error ");
		return 0;
	}
	printf("@@@@@@@@@@@@locattime*******%d*****\n",__LINE__);
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*1);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 7;	
	sendinfo->pinfo[0] = g_play_minute - 1;
	SendCmdIntLink(_pLink, (int*)sendinfo, 2*sizeof(int));
	return 0;
}

/*==============================================
 *开启报警
 *==============================================*/
int start_alarm()
{
	return 0;
}

/*==============================================
 *设置报警时间
 *==============================================*/
int set_alarm_time()
{
	return 0;
}

/*==============================================
 *设置报警区域
 *==============================================*/

int set_alarm_area()
{
	return 0;
}
int g_direction = 1;
int g_audio_volum = 1;
int g_audio_status = 1;
/*==============================================
 *获取ipc状态
 *==============================================*/
int get_ipc_status(PEER_INFO * _pLink)
{
	//int record_status = 1;

	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*4);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 11;
	sendinfo->pinfo[0] = g_start_video;
	sendinfo->pinfo[1] = g_direction;
	sendinfo->pinfo[2] = g_audio_volum;
	sendinfo->pinfo[3] = g_audio_status;
	SendCmdIntLink(_pLink, (int*)sendinfo, 5*sizeof(int));
	free(sendinfo);
}

/*==============================================
 *设置ipc图像位置
 *==============================================*/
int get_ipc_direction(PEER_INFO * _pLink, int direction)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*1);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 12;
	g_direction = direction;
	sendinfo->pinfo[0] = 1;
	SendCmdIntLink(_pLink, (int*)sendinfo, 5*sizeof(int));
	free(sendinfo);
}

/*==============================================
 *设置音量大小
 *==============================================*/
int get_audio_volum(PEER_INFO * _pLink, int audio_volum)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*1);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	sendinfo->cmdid = 13;
	g_audio_volum = audio_volum;
	sendinfo->pinfo[0] = 1;
	SendCmdIntLink(_pLink, (int*)sendinfo, 5*sizeof(int));
	free(sendinfo);
}

/*==============================================
 *设置音频状态，关或者开
 *==============================================*/
int set_audio_stat(PEER_INFO * _pLink, int stat)
{
	info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*1);
	if(sendinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	g_audio_status = stat;
	sendinfo->cmdid = 14;
	sendinfo->pinfo[0] = 1;
	SendCmdIntLink(_pLink, (int*)sendinfo, 5*sizeof(int));
	free(sendinfo);
}
