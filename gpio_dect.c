#include <stdio.h>
#include <unistd.h>
#include "gpiodriver.h"

#define  K    P0_7                 //独立按键
#define  ON    0                   //按键按下的电平值
#define  OFF   1
#define JSQ_L  2                  //消除抖动计数器门限下限
#define JSQ_H 40                  //消除抖动计数器门限上限
#define  KEY_SHORT  0X20          //自定义短按标志
#define  KEY_LOG     0X22         //自定义长按标志
#define  DLEPY   f()              //f()是系统中实时性要求较高的函数，在这里插入充当消抖可以兼顾实时性方面的要求


#define GPIO_READ  0
#define GPIO_WRITE 1

unsigned int g_KeyReset_grp = 5;
unsigned int g_KeyReset_bit = 2; //GPIO:5_2 ==> reset key.

unsigned int read_io()
{
	unsigned int val_read = 1;
	int nRet;
	nRet = Hi_SetGpio_SetDir( g_KeyReset_grp, g_KeyReset_bit, GPIO_READ );
	nRet = Hi_SetGpio_GetBit( g_KeyReset_grp, g_KeyReset_bit, &val_read );
	return val_read;
}

/********************************************************************
  函数名称：按键检测
 **********************************************************************/
/*
unsigned char key_scan(void)
{
	unsigned char timer=0,key_vlu=0;
	while(K==ON)
	{
		DELPY;
		timer++;                  //按键计数器计数
		if(timer>JSQ_H) break;    //溢出退出
	}
	if(timer> JSQ_L  && timer< JSQ_H)  key_vlu=KEY_SHORT;   //判为短按
	if( timer >JSQ_H )   key_vlu=KEY_LOG;                   //判为长按
	return ( key_vlu );
}*/

/*
return 2 no dect
return 1 long
return 0 short
*/
int key_scan()
{
	int timer = 0;
	HI_SetGpio_Open();
	while (read_io() == ON)
	{
		timer++;
		usleep(1000*100);

		if( timer> JSQ_L  && timer< JSQ_H )
		{
			printf("blink**************\n");
			Hi_SetGpio_SetDir( 7, 5, GPIO_WRITE );
			Hi_SetGpio_SetBit( 7, 5, timer%2); //pull up.
		}
		if(timer > JSQ_H)
		{
			Hi_SetGpio_SetDir( 7, 5, GPIO_WRITE );
			Hi_SetGpio_SetBit( 7, 5, 0); //pull up.
			break;
		}
	}
	if(timer > JSQ_H)
	{
		Hi_SetGpio_SetDir( 7, 5, GPIO_WRITE );
		Hi_SetGpio_SetBit( 7, 5, 0); //pull up.
		return 1;
	}
	if( timer> JSQ_L  && timer< JSQ_H )
	{
		timer = 0; 
		Hi_SetGpio_SetDir( 7, 5, GPIO_WRITE );
		Hi_SetGpio_SetBit( 7, 5, 0); //pull up.
		printf("short\n");
		return 0;
	}
		Hi_SetGpio_SetDir( 7, 5, GPIO_WRITE );
		Hi_SetGpio_SetBit( 7, 5, 0); //pull up.
	return 2;
}
