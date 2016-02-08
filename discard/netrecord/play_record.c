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
int play_minute()
{
	av_record_t play_handle;
	av_frame_t av_frame;
	int ret;

	av_record_init("/nfsroot/record");
circle:
	play_handle = av_record_open(2015, 1, 29, 11, 48 );
	while(1)
	{
		ret = av_record_read(play_handle, av_frame);
		if(av_frame->codec == 0)
		{	
			P2PNetServerChannelDataSndToLink(0, 0, av_frame->data, av_frame->size, av_frame->keyframe, 0);
			P2PNetServerChannelDataSndToLink(0, 1, av_frame->data, av_frame->size, av_frame->keyframe, 0);
		}
		if(ret < 0)
		{
			av_record_close(play_handle);
			goto circle;
		}
	}
}
