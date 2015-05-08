#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>


#include <linux/watchdog.h>

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define     WTD_DEV     "/dev/watchdog"
static int  g_wtd_Fd    =   -1;

#define    WATCHDOG_IOCTL_BASE    'W'

#define    WDIOC_KEEPALIVE      _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define    WDIOC_SETTIMEOUT     _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define    WDIOC_GETTIMEOUT     _IOR(WATCHDOG_IOCTL_BASE, 7, int)


#define	HK_CREATE_THREADEX(Func, Args,Ret)	do{					\
		pthread_t		__pth__;									\
		if (0 == pthread_create(&__pth__, NULL, (void *)Func, (void *)Args))	\
			Ret = TRUE; \
		else \
			Ret = FALSE; \
      }while(0)

#define	IPCAM_PTHREAD_EXIT		do{ pthread_exit((void*)pthread_self()); }while(0)

static int HI3518_WDTSetTmOut(int TimeOut)
{
	if(g_wtd_Fd < 0){
		printf("Please Init WatchDog Before This!\n");
		return FALSE;
	}
	if(ioctl(g_wtd_Fd, WDIOC_SETTIMEOUT, TimeOut)){
		printf("Set Time Out Failed!\n");
		return FALSE;
	}

	return TRUE;
}
static int HI3518_WDTInit(int TimeOut)
{
	if (g_wtd_Fd < 0)
	{
		g_wtd_Fd = open(WTD_DEV, O_RDWR);
		if (g_wtd_Fd < 0)
		{
			printf("open watch dog device failed!\n");
			return FALSE;
		}
	}

	if(ioctl(g_wtd_Fd, WDIOC_KEEPALIVE, 0))
	{
		printf("Feed Dog Failed!\n");
		return FALSE;
	}
	HI3518_WDTSetTmOut(TimeOut);
	
	return TRUE;
}

static int HI3518_WDTDeInit(VOID)
{
	if(g_wtd_Fd < 0)
	{
		printf("Watch Dog has already been closed!\n");
		return FALSE;
	}
	close(g_wtd_Fd);
	g_wtd_Fd = -1;
	return TRUE;
}


int HI3518_WDTFeed (VOID)
{
	if(g_wtd_Fd < 0)
	{
		printf("Please Init WatchDog Before This!\n");
		return FALSE;
	}

	if(ioctl(g_wtd_Fd, WDIOC_KEEPALIVE, 0))
	{
		printf("Feed Dog Failed!\n");
		return FALSE;
	}	
	return TRUE;
}

void * HK_FeedDog()
{
	while (1)
	{
		if (TRUE != HI3518_WDTFeed())
		{
			printf("Feed Dog Failed!\n");
		}
		usleep(100*1000);
	}
	
	IPCAM_PTHREAD_EXIT;
	
	return NULL;
}

int HK_WtdInit(int TimeOut)
{
	int nRet = -1;
	HI3518_WDTInit(TimeOut);
	 
	//HK_CREATE_THREADEX(HK_FeedDog, NULL, nRet);

	return nRet;
}

