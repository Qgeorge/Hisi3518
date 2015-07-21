#ifndef __WDT_HI3518_H__
#define __WDT_HI3518_H__
/*
*初始化看门狗
*/
int HK_WtdInit(int TimeOut);
/*
*喂狗
*/
int HI3518_WDTFeed (void);

#endif
