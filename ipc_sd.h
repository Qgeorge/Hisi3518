#ifndef __IPC_SD_H__
#define __IPC_SD_H__
/*
 * =====================================================================================
 *
 *       Filename:  ipc_sd.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/03/2015 06:05:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef bool
#define bool int
#define false 0
#define true 1
#endif
#include "zlog.h"
typedef struct hk_sd_param
{
    short audio;
    short moveRec;
    short outMoveRec;
    short autoRec;
    short loopWrite;
    short splite;
    short sdrecqc;//1 one, 0 two
    short sdIoOpen;//1 open, 0 close
    short sdError;//1 open, 0 close
    short sdMoveOpen;//0 open, 1 close
    unsigned long allSize;
    unsigned long haveUse;
    unsigned long leftSize;
}HK_SD_PARAM_;

typedef struct hk_sd_msg
{
	char ip[20]; 
	char gw[20];
	char productid[10];
	char manufacturerid[10];

}HK_SD_MSG;

extern short g_sdIsOnline;
extern bool b_hkSaveSd;
extern HK_SD_PARAM_ hkSdParam;
extern HK_SD_MSG hk_net_msg;
extern short g_sdIsOnline_f;
extern void get_sd_conf();
extern void hk_load_sd();
extern zlog_category_t *zc;

#endif
