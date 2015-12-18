/*
 * =====================================================================================
 *
 *       Filename:  ipc_time.c
 *
 *    Description:  ipc 时间设置
 *
 *        Version:  1.0
 *        Created:  12/15/2015 09:25:36 AM
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
#include "ipc_hk.h"


/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  init_time
 *  Description: 开机初始化时间  
 * =====================================================================================
 */
int init_time()
{
	//插入时间模块
	
	//更新至最后的时间
	int tz = conf_get_int(HOME_DIR"/time.conf", "zone");
	unsigned int t = conf_get_int(HOME_DIR"/time.conf", "time_");
	if (t > 0)
	{
		unsigned int seconds = tz+t;
		//unsigned int seconds = t;
		struct timespec ts;
		ts.tv_sec = seconds;
		ts.tv_nsec = 0;
		clock_settime(CLOCK_REALTIME, &ts);
	}
	return 0;
}
/* -----  end of function init_time  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  update_time
 *  Description: 更新存储的时间
 * =====================================================================================
 */
int	update_time()
{
	int st = 0;
	int tz = conf_get_int(HOME_DIR"/time.conf", "zone");
	st = time(NULL);
	if(st != 0)
	{
		conf_set_int(HOME_DIR"/time.conf", "time_",st-tz);
	}
	return 0;
}
/* -----  end of function update_time  ----- */


/*-----------------------------------------------------------------------------
 *  同步网络时间
 *-----------------------------------------------------------------------------*/
int sync_time_net()
{
	system("/bin/ntpdate cn.pool.ntp.org");
	return 0;
}
