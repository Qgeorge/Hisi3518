/*
 * =====================================================================================
 *
 *       Filename:  play_record.c
 *
 *    Description:  回放模块
 *
 *        Version:  1.0
 *        Created:  09/19/2015 04:38:48 AM
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
#include "record.h"
#include "P2Pserver.h"
#if 0
int record_search()
{
	record_info_t infos[20];
	int num = av_record_search(2015, 6, 27, infos, 20);
	int i;
	for(i = 0; i < num; i++)
	{
		record_info_t *info = &infos[i];
		//printf("year is %d, month is %d, day is %d, hour is %d, minute is %d\n", info->year, info->month, info->hour, info->minute);
	}
}
#endif
int g_play_minute = 1;

/*播放一分钟的视频文件*/
int play_minute(void *locate_time)
{
	int playtime = 0;
	playtime = *((int *)locate_time);

	av_record_t *play_handle;
	av_frame_t av;
	int ret;
	if(g_play_minute == 0)
	{
		return 0;
	}
	g_play_minute = 0;
circle:
	play_handle = av_record_open(2014, 4, 11, 17, 4);
	while(1)
	{
//		printf("ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg\n");
//		record_search();
		ret = av_record_read(play_handle, &av);
		if(ret < 0)
		{
			av_record_close(play_handle);
			g_play_minute = 1;
			//goto circle;
			return 0;
		}
		if(av.keyframe = 0)
		{
			av.keyframe = 1;
		}else
		{	
			av.keyframe = 0;
		}

		if(av.codec == 0)
		{
			P2PNetServerChannelDataSndToLink(1, 0, av.data, av.size, av.keyframe, 0);
//			P2PNetServerChannelDataSndToLink(1, 1, av.data, av.size, av.keyframe, 0);
		}
		usleep(1000*10);
	}
}
