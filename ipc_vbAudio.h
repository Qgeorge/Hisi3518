#ifndef __IPC_VBAUDIO_H__
#define __IPC_VBAUDIO_H__
/*
*创建音频流的线程
*/
int CreateAudioThread(void);
/*
 *播发一个音频文件 
 */
int PlaySound(char *soundfile);

#endif

