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
int play_minute()
{
	av_record_t *play_handle;
	av_frame_t av;
	int ret;
	av_record_init("/nfsroot/record");
circle:
	play_handle = av_record_open(2015, 6, 27, 23, 19);
	while(1)
	{
//		printf("ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg\n");
		record_search();
		ret = av_record_read(play_handle, &av);
		if(ret < 0)
		{
			av_record_close(play_handle);
			goto circle;
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
			P2PNetServerChannelDataSndToLink(0, 0, av.data, av.size, av.keyframe, 0);
			P2PNetServerChannelDataSndToLink(0, 1, av.data, av.size, av.keyframe, 0);
		}
	}
}
