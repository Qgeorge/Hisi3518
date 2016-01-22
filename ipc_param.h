#ifndef __IPC_PARAM_H__
#define __IPC_PARAM_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/HKUtilAPI.h"
#include "ipc_alias.h"

typedef struct hk_sd_msg
{
	uint8 ip[20]; 
	uint8 gw[20];
	uint8 productid[10];
	uint8 manufacturerid[10];
	bool isTestMode;
}HK_SD_MSG_T,*HK_SD_MSG_P;


extern void init_param_conf();
extern void get_manufacturer_id(uint8 *str);
extern void get_product_id(uint8 *str);
extern void get_gateway(uint8 *str);
extern void get_ipaddr(uint8 *str);
extern bool get_isTestMode();
extern void set_testmode(bool param);
extern zlog_category_t *zc;
#endif 
