
#ifndef __PTZ_H__
#define __PTZ_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define PTZ_PRINT_ENABLE    1
#if PTZ_PRINT_ENABLE
    #define PTZ_DEBUG_PRT(fmt...) \
        do {                      \
            printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
            printf(fmt);          \
        }while(0)
#else
    #define PTZ_DEBUG_PRT(fmt...) \
        do { ; } while(0) //do nothing.
#endif

//thread.
#define	HK_CREATE_THREADEX(Func, Args,Ret)	do{					\
		pthread_t		__pth__;									\
		if (0 == pthread_create(&__pth__, NULL, (void *)Func, (void *)Args))	\
			Ret = TRUE; \
		else \
			Ret = FALSE; \
      }while(0)

#define GPIO_READ  0
#define GPIO_WRITE 1

#define CLOCKWISE 1
#define COUNTER_CLOCKWISE 2

#define PTZ_CONF  "/mnt/sif/ptz.conf"
#define PRESETNUM 9 //8 //preset level: 1~8.

#define PTZ_RANGE_LR 2020 //L & R max length.
#define PTZ_RANGE_UD 620 //U & D max length.

typedef struct ptz_preset 
{
    unsigned int presetX;
    unsigned int presetY;
} st_PtzPreset;

st_PtzPreset g_stPtzPreset[PRESETNUM];

#endif  /* ptz.h */

