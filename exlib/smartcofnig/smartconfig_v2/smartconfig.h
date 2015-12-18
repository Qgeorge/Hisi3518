/*
 *  Copyright (C) 2014 -2015  By younger
 *
 */
#ifndef __SMARTCONFIG_H__
#define __SMARTCONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <linux/in.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "list.h"
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>

#define RECEVEBUF_MAXLEN  90
#define SCAN_TIME  1000

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned short u16;

typedef enum SMART_STATUS
{
  SMART_CH_INIT = 0x1,
  SMART_CH_LOCKING = 0x2,
  SMART_CH_LOCKED = 0x4
}SMARTSTATUS;

typedef enum _encrytion_mode {
    ENCRY_NONE           = 1,
    ENCRY_WEP,
    ENCRY_TKIP,
    ENCRY_CCMP
} ENCYTPTION_MODE;

typedef struct router{
	struct list_head list;
	u8 essid[100];
	u8 bssid[50];
	u8 channel;
	u8 encryption_mode;
}router_t, *router_p;



extern uint8 calcrc_1byte(uint8 onebyte);    
extern uint8 calcrc_bytes(uint8 *p,uint8 len);  
extern int smart_config_decode(uint8* pOneByte);
extern void smart_analyze(uint8* buf,int len,int channel,int fd);
extern void sniffer_hopping(int current_channel);
extern void router_scanf(router_p *router_info_head);
extern void router_discover();
extern void *thread_routine(void *arg);
extern void set_channel_timer();
#endif
