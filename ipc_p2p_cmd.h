/*
 * =====================================================================================
 *
 *       Filename:  ipc_cmd.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/13/2015 04:20:12 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#if 1
int get_sd_valid(PEER_INFO * _pLink);

int get_sd_size(PEER_INFO * _pLink);

int start_store_video(PEER_INFO * _pLink, int flag);

int get_video_list(PEER_INFO * _pLink, int locattime);

int locat_video_time(PEER_INFO * _pLink, int locattime);

int start_alarm();

int set_alarm_time();

int set_alarm_area();

int get_ipc_status(PEER_INFO * _pLink);

int get_ipc_direction(PEER_INFO * _pLink, int direction);

int get_audio_volum(PEER_INFO * _pLink, int audio_volum);

int set_audio_stat(PEER_INFO * _pLink, int stat);
#endif
