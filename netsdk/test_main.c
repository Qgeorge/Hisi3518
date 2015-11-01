/*
 * =====================================================================================
 *
 *       Filename:  test_main.c
 *
 *    Description:  测试网络
 *
 *        Version:  1.0
 *        Created:  09/04/2015 05:57:30 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <signal.h>
static void sig_handle()
{
	printf("get signal\n");
}

#include "protocol_josn.h"

extern int net_bind_device (char *UserId, char *DeviceId);

extern int net_create_device (char *DeviceId);

extern int net_get_key (char *DeviceId, int *key);

extern int get_device_id(char *DeviceId);

extern int net_get_upgrade();
#if 0
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
	char device_id[50] = {0};
	int key;
	get_device_id(device_id);
	signal(SIGSEGV, sig_handle);
	while(1)
	{
		net_get_key ("1001", &key);
		net_create_device(device_id);
		//net_bind_device ("biaobiao");
		usleep(1000);
	}
	printf("end main\n");
	return 0;
}
/* ----------  end of function main  ---------- */
#endif
int main()
{
	char UserId[20] = "17727532515";
	char DeviceId[20] = "1da099";
	net_create_device(DeviceId);
	net_bind_device(UserId, DeviceId);
}

