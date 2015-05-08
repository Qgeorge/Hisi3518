
#ifndef __OSD_REGION_H__
#define __OSD_REGION_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "sample_comm.h"

#define TEXT_DIRECTION  1	
#define TEXT_SLANT      0

#define MAX_OSD_FONTS		32

#define ASC_FONT_WIDTH      8  
#define ASC_FONT_HEIGHT     16 //ascii font standard:8*16 points for per charactor display

#define OSD_POSITION_X      16 //x 起始位置
#define OSD_POSITION_Y      16 //y 起始位置

#define WIDTH_HD720P    1280
#define WIDTH_VGA       640
#define WIDTH_QVGA      320
#define WIDTH_CIF       352

#define DEVID           "USER"
#define CONF_HKCLIENT   "/mnt/sif/hkclient.conf"

/*  here we list HTML 4.0 support color rgb value */
#define OSD_PALETTE_COLOR_BLACK		0x000000		// 黑色
#define OSD_PALETTE_COLOR_NAVY		0x000080		// 深蓝色
#define OSD_PALETTE_COLOR_GREEN		0x008000		// 绿色
#define OSD_PALETTE_COLOR_TEAL		0x008080		// 墨绿色
#define OSD_PALETTE_COLOR_SILVER	0xC0C0C0		// 银灰色
#define OSD_PALETTE_COLOR_BLUE		0x0000FF		// 蓝色
#define OSD_PALETTE_COLOR_LIME		0x00FF00		// 浅绿色
#define OSD_PALETTE_COLOR_AQUA		0x00FFFF		// 青色
#define OSD_PALETTE_COLOR_MAROON	0x800000		// 栗色
#define OSD_PALETTE_COLOR_PURPLE	0x800080		// 紫色
#define OSD_PALETTE_COLOR_OLIVE		0x808000		// 橄榄色
#define OSD_PALETTE_COLOR_GRAY		0x808080		// 灰色
#define OSD_PALETTE_COLOR_RED		0xFF0000		// 红色
#define OSD_PALETTE_COLOR_FUCHSIA	0xFF00FF		// 品红色
#define OSD_PALETTE_COLOR_YELLOW	0xFFFF00		// 黄色
#define OSD_PALETTE_COLOR_WHITE		0xFFFFFF		// 白色

typedef struct __osd_region_st {
	int osd_rgn_enable;     //1:enable OSD region display,   0:disable.
	unsigned int pos_x;     //dispaly start position: x.
	unsigned int pos_y;     //dispaly start position: y.
	unsigned int width;		//region width
	unsigned int height;	//region height
	int osd_content_len;    //Indicate content length to display, 8 per byte.
	unsigned char osd_content_table[MAX_OSD_FONTS];   //OSD table, that is the contents to display.
	int win_palette_idx;    //window background color, GM8120 index from 0~15
} st_OSD_Region;

typedef struct st_Color
{
	unsigned int R; //red
	unsigned int G; //green
	unsigned int B; //blue
} Color;

typedef struct _CaptionAdd
{
	int mRowSpace;	// 2 字的行间距
	int mColSpace;	// 2 字的列间距
	int mStartRow;	// 文字叠加的起始行， 默认为图像高度-20
	int mStartCol;	// 文字叠加的起始列， 默认为10
	Color mTextColor; // 叠加文字的颜色； 默认为黑色；
	unsigned char*mImagePtr;	// 图像的起始地址；
	int mImageWidth, mImageHeight;
	unsigned char *HZK16;	//汉字模库 buffer
	unsigned char *ASC16;	//ascii 
	unsigned char mat[16*2];	//字模	
}CaptionAdd;


//function call:
//int Get_Current_DayTime(unsigned char *pTime); //获取系统当前日期和时间
//int OSD_RGN_Init( st_OSD_Region *pOsdRgn, RGN_HANDLE RgnHandle, const char *pRgnContent ); //初始化OSD显示区域
//int OSD_RGN_Display(st_OSD_Region *pOsdRgn, RGN_HANDLE RgnHandle, const char *pRgnContent, VENC_GRP RgnVencChn);//OSD内容显示
//int OSD_RGN_Handle_Initialize(void); //准备要显示的内容,初始化,并创建区域句柄
//int OSD_RGN_Handle_Display_Update(VENC_GRP RgnVencChn); //区域显示刷新
//int OSD_RGN_Handle_Finalize(VENC_GRP RgnVencChn); //销毁已经创建的区域句柄
//
////int OSD_Overlay_RGN_Handle_Init(unsigned int ContentLen, VENC_GRP RgnVencChnStart);
//int OSD_Overlay_RGN_Handle_Init(RGN_HANDLE RgnHandle, unsigned int ContentLen, VENC_GRP RgnVencChn, int RgnVdWidth, int LorR);
//int OSD_Overlay_RGN_Display_Static( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent );
//int OSD_Overlay_RGN_Handle_Finalize( RGN_HANDLE RgnHandle, VENC_GRP RgnVencChn );
#endif /* osd_region.h */

