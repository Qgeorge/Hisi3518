#include "osd_region.h"
#include "osd_region_asc16.h"

HI_S32 gs_s32RgnCnt = 2; //姣忎釜绐楀彛鍙犲姞鍖哄煙鏁帮細2

int Get_Current_DayTime(unsigned char *pTime)
{
	if (NULL == pTime)
	{
		printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}
	memset(pTime, '\0', sizeof(pTime));

	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[32] = {0};

	time(&curTime);
	ptm = gmtime(&curTime);
	sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	//printf("[%s %d] tmp: %s\n", __func__, __LINE__, tmp);   
	strcpy(pTime, tmp);
	//printf("[%s %d] pTime: %s \n", __func__, __LINE__, pTime);   

	return 0;
}


#if 1
//////////////////////////////////////////////////////////////////////////////////////
#define OSD_ENGLISH 1  // 1 is english osd,0 is chinese osd

#define emALIGN(addr, boundary) (((addr) + (boundary) - 1) & ~((boundary) - 1))
#define FONT_DIR  	"/usr/lib/"

static unsigned char *g_pHZ12FontLib = NULL;
static unsigned char *g_pHZ16FontLib = NULL;
static unsigned char *g_pHZ32FontLib = NULL;

typedef enum _FONTLANGUAGE_
{
	FONT_HZ11x11 = 0,
	FONT_HZ12x12,
	FONT_HZ16x16,	
	FONT_HZ24x24,
	FONT_HZ32x32,
	FCE_UNKNOWN,
}FONTLANGUAGE;


//================================= make RGB1555 BMP by font ==========

inline int GetFontOffset(const unsigned char *pChar,int fontWidth)
{
	if((*pChar) >=0x80)	//中文
	{
		return 0;
	}
	//小写"m"和"w"
	else if( ((*pChar) == 0x77)||((*pChar) != 0x6d) )
	{
		switch(fontWidth)
		{
		default:
			break;
		case 16:
			{
				return 4;//return 2;
			}
			break;
		case 32:
			{
				return 4;
			}
			break;
		}
	}
	//大写"M" and "W"
	else if( ((*pChar) == 0x57)||((*pChar) != 0x4d) )
	{
		switch(fontWidth)
		{
		default:
			break;
		case 16:
			{
				return 1;
			}
			break;
		case 32:
			{
				return 2;
			}
		}
	}
	else if(  ((*pChar) >=0x61) && ((*pChar) <=0x7a) )//小写英文字
	{
		switch(fontWidth)
		{
		default:
			break;
		case 16:
			{
				return 3;
			}
			break;
		case 32:
			{
				return 4;
			}
			break;
		}
	}
	else
	{
		switch(fontWidth)
		{
		default:
			break;
		case 16:
			{
				return 2;
			}
			break;
		case 32:
			{
				return 3;
			}
			break;
		}
	}
}


int GetCharCount(const char *pCharInfo)
{
	int count=0;
	const unsigned char *pStrTemp;
	
	pStrTemp= (const unsigned char *)pCharInfo;
	
	int	strLen = strlen(pCharInfo);
	
	if(NULL == pCharInfo)
	{
		return -1;
	}
	
	while(strLen > 0)
	{
		if( (*pStrTemp)<0x80)
		{
			count++;
			pStrTemp +=1;
			strLen -= 1;
		}
		else
		{
			count++;
			pStrTemp += 2;
			strLen -= 2;
		}
	}
	return count;
}


int GetStringWidth(const char *pString,int fontWidth)
{
	const unsigned char *pStrTemp;
	int sumWidth = 0;
	int fontOffset;
	
	pStrTemp= (const unsigned char *)pString;
	
	int	strLen = strlen(pString);
	
	if(NULL == pString)
	{
		return -1;
	}
	
	while(strLen > 0)
	{
		if( (*pStrTemp)<0x80)
		{
			fontOffset = GetFontOffset(pStrTemp,fontWidth);
			fontOffset = fontWidth - 2*fontOffset;
			sumWidth += fontOffset;

			pStrTemp +=1;
			strLen -= 1;
		}
		else
		{
			sumWidth += fontWidth;
			pStrTemp += 2;
			strLen -= 2;
		}
	}
	
	return sumWidth;
}

int GetCharQWInfo(const char *pCharInfo, int *pQu, int *pWei)
{
	int qu,wei;
	int	offset = 0;
	const unsigned char *pString = (const unsigned char *)pCharInfo;
	
	if(*pString >= 128)
	{
		qu= *pString-160;
		pString++;
		
		if(*pString >= 128)
		{
			wei=*pString-160;
			
			offset = 2;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		qu=3;
		wei=*pString-32;
		offset = 1;
	}
	*pQu = qu;
	*pWei = wei;
	return offset;
}

int GetFont(const unsigned char *pFontLib,int fontWidth,int fontHeight, int qu, int wei, const unsigned char **ppFont)
{
	int fontMemSize ;
	int pos = 0;
	unsigned char *pTempBuf = NULL;
	int  TempBufLen = 0;

	fontMemSize = fontWidth*fontHeight/8;
	if( (qu<=0)||(wei <=0) )
	{
		if(NULL == pTempBuf)
		{
			pTempBuf = (unsigned char *)malloc(fontMemSize);
			if(pTempBuf == NULL)
			{
				return 0;
			}
			TempBufLen = fontMemSize;
		}
		else
		{
			if(TempBufLen != fontMemSize)
			{
				free(pTempBuf);
				pTempBuf = NULL;
				pTempBuf = (unsigned char *)malloc(fontMemSize);
				if(pTempBuf == NULL)
				{
					return 0;
				}
				TempBufLen = fontMemSize;
			}
		}
		memset(pTempBuf,0,TempBufLen);
		*ppFont = pTempBuf;
	}
	else
	{
		pos=(qu-1)*94+wei-1;
		*ppFont = pFontLib+pos*fontMemSize;
	}
	
	if(pTempBuf) free(pTempBuf);
	
	return 1;
}

int OsdFontRGB1555Bmp(int fontOffset,int fontWidth,int fontHeight,unsigned char *pFont, int bmpOffset,int bmpStride,unsigned char *pBmpBuf)
{
	unsigned short *pBmpData ;
	int i,j;

	unsigned char bitBmp,tmp;
	int bitIndex;

	unsigned char	fR,fG,fB;
	unsigned char	bR,bG,bB;

	int xStart,xEnd;
	int xBmpPos=0;

	fR =fG =fB = 31;
	bR = bG = bB = 255&0x0c;//黑色//255&0x17;

	xStart = fontOffset;
	xEnd = fontWidth - fontOffset;
	pBmpData = (unsigned short *)pBmpBuf;

	for(i=0;i<fontHeight;i++)
	{
		xBmpPos = bmpOffset + i*bmpStride/2;
		for(j=xStart;j<xEnd;j++)
		{
			bitIndex = i*fontWidth + j;
			bitBmp = pFont[bitIndex/8];
			
			tmp = (0x80 >> (j&0x07) );
			
			if(bitBmp&tmp)
			{
				pBmpData[xBmpPos] = 0x8000 | (fR << 10) | (fG << 5) | (fB);
			}
			else
			{
				pBmpData[xBmpPos] = (bR << 10) | (bG << 5) | (bB);
			}
			
			xBmpPos++;
		}
	}
	
	return 1;
}

HI_S32 VOSD_Text2Bmpfont(char *pOsdStr,int fontWidth,int fontHeight,unsigned char *pFontLib,BITMAP_S *pBmp)
{
	int errorNum=0;
	int charOffset = 0;
	char *pCharInfo=pOsdStr;
	unsigned char *pFontData=NULL;
	
	int qu=0;
	int wei=0;
	int fontOffset=0;
	int charCount = 0;

	//bmp info
	unsigned char *pBmpData=NULL;
	unsigned char *pBmpBuf = NULL;
	int bmpWidth = 0;
	int bmpStride = 0;
	int bmpOffset=0;
	
	if( (NULL == pCharInfo)||(NULL == pFontLib)||(NULL == pBmp) ||(NULL == pBmp->pData) )
	{
		SAMPLE_PRT("[VOSD_Text2Bmpfont]------------------------ERROR\n");
		return HI_FAILURE;
	}
	
	bmpWidth = GetStringWidth(pCharInfo,fontWidth);
	charCount = GetCharCount(pCharInfo);
	bmpStride = emALIGN((bmpWidth*2),4);
	
	pBmpBuf = (unsigned char *)pBmp->pData;
	pBmp->u32Width = bmpWidth;
	pBmp->u32Height = fontHeight;
	pBmp->enPixelFormat = PIXEL_FORMAT_RGB_1555;
	
	while(charCount > 0)
	{
		pFontData = NULL;
		charOffset = GetCharQWInfo(pCharInfo,&qu,&wei);
		fontOffset = GetFontOffset((const unsigned char *)pCharInfo,fontWidth);
		
		if(!GetFont(pFontLib,fontWidth,fontHeight,qu,wei,&pFontData))
		{
			SAMPLE_PRT("[GetFont]------------------------ERROR\n");
			return HI_FAILURE;
		}
	
		if(!OsdFontRGB1555Bmp(fontOffset,fontWidth,fontHeight,pFontData,bmpOffset,bmpStride,pBmpBuf))
		{
			SAMPLE_PRT("[OsdFontRGB1555Bmp]------------------------ERROR\n");	
			return HI_FAILURE;
		}
		bmpOffset += (fontWidth - 2*fontOffset);		
		pCharInfo += charOffset;
		charCount--;
	}
	
	return HI_SUCCESS;
}

HI_S32 FONT_ReadFile(const char *strFileName,unsigned char **ppFileBuf)
{
	char	szPath[60];
	int fileLen = 0;

	FILE *hFile = NULL;
	unsigned char *pBuffer=NULL;
	int nReadLen = 0;

	memset(szPath , 0 , sizeof(szPath));
	sprintf(szPath , "%s/%s" , FONT_DIR , strFileName);
	if(NULL == (hFile = fopen(szPath,"rb")))
	{
		return HI_FAILURE;
	}
	if(fseek(hFile,0,SEEK_END)!=0)
	{
		goto ERROR;
	}	

	fileLen = ftell(hFile);
	if(fseek(hFile,0,SEEK_SET)!=0)
	{
		goto ERROR;
	}
	if(NULL == (pBuffer = (unsigned char *)malloc(fileLen)))
	{
		goto ERROR;
	}
	memset(pBuffer,0,fileLen);
	nReadLen = fread(pBuffer,1,fileLen,hFile);
	if(nReadLen != fileLen)
	{
		goto ERROR;
	}	
	*ppFileBuf = pBuffer;
	if(NULL != hFile)
	{
		fclose(hFile);
		hFile = NULL;
	}	
	return fileLen;

ERROR:
	if(pBuffer)free(pBuffer);
	if(hFile)  fclose(hFile);
	return HI_FAILURE;	
}

HI_S32 FONT_Init()
{	
	////加载16X16中文字体	VGA从码流
	if(HI_FAILURE == FONT_ReadFile("ct_libhz16x16.so",&g_pHZ16FontLib))
	{
		return HI_FAILURE;
	}
	////加载32X32中文字体用于720P以上主码流
	if(HI_FAILURE == FONT_ReadFile("GB2312_32.DZK",&g_pHZ32FontLib))
	{
		return HI_FAILURE;
	}		
	return HI_SUCCESS;
}

int FONT_Exit()
{
	if(g_pHZ16FontLib) free(g_pHZ16FontLib);
	if(g_pHZ32FontLib) free(g_pHZ32FontLib);	
	return HI_SUCCESS;
}

/******************************************************************************
* funciton : osd region change (bgAlpha, fgAlpha, layer)
******************************************************************************/
HI_S32 VRGN_Change(RGN_HANDLE RgnHandle, VENC_GRP VencGrp, SAMPLE_RGN_CHANGE_TYPE_EN enChangeType, HI_U32 u32Val)
{
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	HI_S32 s32Ret;

	stChn.enModId = HI_ID_GROUP;
	stChn.s32DevId = VencGrp;
	stChn.s32ChnId = 0;
	s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n",\
			   RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	switch (enChangeType)
	{
		case RGN_CHANGE_TYPE_FGALPHA:
			stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = u32Val;
			break;
		case RGN_CHANGE_TYPE_BGALPHA:
			stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = u32Val;
			break;
		case RGN_CHANGE_TYPE_LAYER:
			stChnAttr.unChnAttr.stOverlayChn.u32Layer = u32Val;
			break;
		default:
			SAMPLE_PRT("input paramter invaild!\n");
			return HI_FAILURE;
	}
	s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n",\
			   RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

HI_S32 VOSD_ShowTextRgn(RGN_HANDLE RgnHandle ,HI_S32 nChn ,char* pStr)
{
   	HI_U32 u32Alpha;
	BITMAP_S stBitmap = {0};
	HI_S32	 nFontType = 0;
	HI_S32 	s32Ret = HI_FAILURE;

	unsigned char * pFontLib 	= NULL;
	HI_S32 			nFontWidth 	= 0;
	HI_S32 			nFontHeight = 0;
	
	if(NULL == pStr){
		return HI_FAILURE;
	}
	switch(nChn)
	{
		case 0:
			nFontType = FONT_HZ32x32;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			nFontType = FONT_HZ16x16;
			break;
		default:
			break;
	}
	switch(nFontType)
	{
		case FONT_HZ16x16:
			pFontLib = g_pHZ16FontLib;
			nFontWidth = 16;
			nFontHeight = 16;			
			break;
		case FONT_HZ32x32:
			pFontLib = g_pHZ32FontLib;
			nFontWidth = 32;
			nFontHeight = 32;
			break;
		default:
			return HI_FAILURE;
	}

	stBitmap.pData = malloc(64*1024); 
	if(HI_NULL == stBitmap.pData) 
	{ 
		return HI_FAILURE; 
	} 
	memset(stBitmap.pData,0,64*1024);
	if(HI_FAILURE == VOSD_Text2Bmpfont(pStr,nFontWidth,nFontHeight,pFontLib,&stBitmap))
	{
		free(stBitmap.pData); 
		return HI_FAILURE;
	}
	s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle,&stBitmap);
	if(s32Ret != HI_SUCCESS)
	{
		free(stBitmap.pData); 
		SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	if(stBitmap.pData) free(stBitmap.pData); 

	return HI_SUCCESS;	
}


HI_S32 VRGN_Create(RGN_HANDLE RgnHandle, VENC_GRP VencGrp , RECT_S* pRect , HI_U32 u32BgColor)
{
	HI_S32 s32Ret = HI_FAILURE;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	/****************************************
	step 1: create overlay regions
	****************************************/
	stRgnAttr.enType = OVERLAY_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stSize.u32Width	= pRect->u32Width;
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = pRect->u32Height;
	stRgnAttr.unAttr.stOverlay.u32BgColor = u32BgColor;

	SAMPLE_PRT("HI_MPI_RGN_Create u32Width=%d,u32Height=%d \n",pRect->u32Width,pRect->u32Height);

	s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n",RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	 SAMPLE_PRT("the handle:%d,creat success!\n",RgnHandle);

	/*********************************************
	 step 2: display overlay regions to venc groups
	*********************************************/
	stChn.enModId = HI_ID_GROUP;
	stChn.s32DevId = VencGrp;
	stChn.s32ChnId = 0;

	memset(&stChnAttr,0,sizeof(stChnAttr));
	stChnAttr.bShow = HI_TRUE;
	stChnAttr.enType = OVERLAY_RGN;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = pRect->s32X;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = pRect->s32Y;
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 50;//128;
	stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
	stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;	
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;
	s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d) failed with %#x!\n",\
			   RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////
#endif

static int OSD_Draw_BitMap( int len, const unsigned char *pdata, unsigned char *pbitmap)
{
    if( 0 == len || !pdata || !pbitmap )
    {
        printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
        return -1;
    }

    int i, w, h, flag, offset;
    unsigned char ch;
    unsigned char *code, *pDst;

    //printf("[%s %d]: len = %d, data = %s\n", __func__, __LINE__, len, pdata);
    int xx = 0;

   /***move 1 Byte to can set color***/
    pbitmap = pbitmap+1; 
   
    /* get the first row, then next*/
    for( h = 0; h < 16; h++ )	// height ; 
    {			
        for( i = 0; i < len; i++ )
        {
            ch 		= pdata[i];
            offset 	= ch * 16;
            code 	= asc16 + offset;
            flag 	= 0x80;

            for ( w = 0; w < 8; w++ )
            {
                pDst = (unsigned short *)( pbitmap + xx );

		  if( (code[h]) & flag )
                {
                    *pDst = (0x00<< 10) | (0x00<< 5) | (0x80);//display font
                }
                else
                {
                    *pDst = (0x88<< 10) | (0x88<< 5) | (0x77);//(0xff<< 10) | (0x00<< 5) | (0x00);//background
                }
	         flag >>= 1;
                xx += 2;
            }
        }
    }

    //printf("[%s %d]: xx = %d\n", __func__, __LINE__, xx);
    return 0;
}

/***************************************************************************
 * function: change region diplay position or change venc channel.
 * params:
 *      RgnHandle   :  overlay region handle.
 *      RgnVencChn  :  venc channel.
 *      ContentLen  :  overlay region display contents byte size.
 * note:
 *      mode HI_ID_GROUP only support channel 0.
 * return:
 *      0 on success, and -1 on error.
 ***************************************************************************/
int OSD_Overlay_RGN_Display_Change(RGN_HANDLE RgnHandle, VENC_GRP RgnVencChn, unsigned int ContentLen)
{
	HI_S32 s32Ret = HI_FAILURE;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	//RGN_ATTR_S stRgnAttr;

	stChn.enModId  = HI_ID_GROUP;
	stChn.s32DevId = RgnVencChn;//0;
	stChn.s32ChnId = 0;//RgnVencChn; //change venc channel ?????

	s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	switch(RgnVencChn)
	{
		case 0://720p
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = OSD_POSITION_Y;
			if(RgnHandle == RgnVencChn)
				stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = OSD_POSITION_X;
			else if(RgnHandle == RgnVencChn+3)
				stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 1280 - (ContentLen*8) - OSD_POSITION_X;
			break;
		case 1://vga
		case 2:
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = OSD_POSITION_Y-8;
			if(RgnHandle == RgnVencChn)
				stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = OSD_POSITION_X-8;
			else if(RgnHandle == RgnVencChn+3)
				stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 640 - (ContentLen*8)-8;
			break;
		default:
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = OSD_POSITION_X;
			break;
	}
	//stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = OSD_POSITION_Y;

	s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/*************************************************************************
 * function: create overlay region handle, and attach it to venc channel.
 * params:
 *      RgnHandle   :  overlay region handle.
 *      RgnVencChn  :  venc channel the region handle attached to.
 *      ContentLen  :  overlay region display contents byte size.
 * return:
 *      0 on success, and -1 on error.
 ************************************************************************/
int OSD_Overlay_RGN_Handle_Init( RGN_HANDLE RgnHandle, VENC_GRP RgnVencChn, unsigned int ContentLen )
{
    HI_S32 s32Ret = HI_FAILURE;
    RGN_ATTR_S stRgnAttr;
    MPP_CHN_S stChn;
    VENC_GRP VencGrp;
    RGN_CHN_ATTR_S stChnAttr;

    /******************************************
     *  step 1: create overlay regions
     *****************************************/
    stRgnAttr.enType                            = OVERLAY_RGN; //region type.
    stRgnAttr.unAttr.stOverlay.enPixelFmt 		= PIXEL_FORMAT_RGB_1555; //format.
    stRgnAttr.unAttr.stOverlay.stSize.u32Width  = ASC_FONT_WIDTH * ContentLen; //8*Len byte
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = ASC_FONT_HEIGHT; //ASC16: 8*16 <==>w*h point per charactor display
    stRgnAttr.unAttr.stOverlay.u32BgColor		= OSD_PALETTE_COLOR_RED;

    s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", RgnHandle, s32Ret);
        return HI_FAILURE;
    }
    SAMPLE_PRT("create handle:%d success!\n", RgnHandle);

    /***********************************************************
     * step 2: attach created region handle to venc channel.
     **********************************************************/
    VencGrp = RgnVencChn;
    stChn.enModId  = HI_ID_GROUP;
    stChn.s32DevId = VencGrp;//0;
    stChn.s32ChnId = 0;//VencGrp;

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.bShow 	= HI_TRUE;
    stChnAttr.enType 	= OVERLAY_RGN;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = OSD_POSITION_X; //
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = OSD_POSITION_Y; //y position.
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 30;// 50;//48; //[0,128]
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha      = 128; //
    stChnAttr.unChnAttr.stOverlayChn.u32Layer 	     = 0;//RgnHandle; //0;

    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

    s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d to %d) failed with %#x!\n", RgnHandle, VencGrp, s32Ret);
        return HI_FAILURE;
    }
    //SAMPLE_PRT("attach handle:%d, to venc channel:%d success, position x: %d, y: %d !\n", RgnHandle, stChn.s32ChnId, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y);

    return HI_SUCCESS;
}

/*************************************************************************
 * function: display region contents,such as device ID, current time.
 * params:
 *      RgnHandle   : osd region handle created before.
 *      pRgnContent : osd region display contents.
 * return:
 *      0 on success, and -1 on error.
 *************************************************************************/
int OSD_Overlay_RGN_Display( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent )
{
    HI_S32 s32Ret = HI_FAILURE;
    BITMAP_S stBitmap;
    int ContentLen = 0;

    if (NULL == pRgnContent)
    {
        printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
        return -1;
    }
    ContentLen = strlen(pRgnContent);

    //SAMPLE_PRT("RgnHandle: %d, pRgnContent: %s, ContentLen: %d,.\n", RgnHandle, pRgnContent, ContentLen);

    /* HI_MPI_RGN_SetBitMap */	
    unsigned char *BitMap = (unsigned char *) malloc(ContentLen*8*16*2); //RGB1555: 2 bytes(R:5 G:5 B:5).
    if (NULL == BitMap)
    {
        SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
        return HI_FAILURE;
    }
    memset( BitMap, '\0', ContentLen*8*16*2 );

    OSD_Draw_BitMap( ContentLen, pRgnContent, BitMap );

    stBitmap.enPixelFormat 	= PIXEL_FORMAT_RGB_1555;
    stBitmap.u32Width		= 8*ContentLen;
    stBitmap.u32Height 	    = 16;
    stBitmap.pData 			= BitMap;

    s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
	 if (BitMap)  free(BitMap);
        return HI_FAILURE;
    }

    if (BitMap)  free(BitMap);
    return 0;
}


int OSD_Overlay_RGN_Display_DevID( RGN_HANDLE RgnHandle ,VENC_GRP RgnVencChn)
{
    unsigned char buf_DevID[32] = {0};
    memset( buf_DevID, '\0', sizeof(buf_DevID) );
    conf_get( CONF_HKCLIENT, DEVID, buf_DevID, sizeof(buf_DevID) );
    printf("[%s, %d] DevID: %s.\n", __FUNCTION__, __LINE__, buf_DevID);
    unsigned int bufLen = strlen( buf_DevID );

    OSD_Overlay_RGN_Display_Change( RgnHandle,RgnVencChn, bufLen );
    OSD_Overlay_RGN_Display( RgnHandle, buf_DevID );

    return 0;
}

int OSD_Overlay_RGN_Display_Time( RGN_HANDLE RgnHandle,VENC_GRP RgnVencChn)
{
    unsigned char buf_Time[32] = {0};
    Get_Current_DayTime( buf_Time );
    unsigned int bufLen = strlen( buf_Time );

#if OSD_ENGLISH
   OSD_Overlay_RGN_Display_Change( RgnHandle,RgnVencChn, bufLen );
   OSD_Overlay_RGN_Display( RgnHandle, buf_Time );
#else
   VOSD_ShowTextRgn(RgnHandle, RgnVencChn ,buf_Time);
#endif

    return 0;
}

/*************************************************************************
 * function: detach region handle from venc channel, and destroy handle.
 * params:
 *      RgnHandle   : osd region handle.
 *      RgnVencChn  : the venc channel which region handle attached to.
 * return:
 *      0 on success, and -1 on error.
 *************************************************************************/
int OSD_Overlay_RGN_Handle_Finalize( RGN_HANDLE RgnHandle, VENC_GRP RgnVencChn )
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_GRP VencGrp;
    MPP_CHN_S stChn;

//    printf("[%s, %d] gs_s32RgnCnt = %d\n", __FUNCTION__, __LINE__, gs_s32RgnCnt);

    VencGrp = RgnVencChn;
    //stChn.enModId = HI_ID_VIU;
    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = VencGrp;//0;
    stChn.s32ChnId = 0;//VencGrp;

    s32Ret = HI_MPI_RGN_DetachFrmChn(RgnHandle, &stChn);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_DetachFrmChn (%d) failed with %#x!\n", RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_RGN_Destroy(RgnHandle);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_Destroy [%d] failed with %#x\n", RgnHandle, s32Ret);
    }

    SAMPLE_PRT("detach region:%d from channel:%d success!\n", RgnHandle, RgnVencChn);
    return HI_SUCCESS;
}


/*************************************************************************
 * function:video osd init
 *other:wangshaoshu
 *************************************************************************/
#if OSD_ENGLISH
int VOSD_Init()
{
	RGN_HANDLE RgnHandle = 0;
	VENC_GRP RgnVencChn = 3;  // 1 main stream,1 slave stream,1 jgpeg stream
	int nChn = 0;

	unsigned char buf_RGN[32] = {0};
	memset( buf_RGN, '\0', sizeof(buf_RGN) );
	conf_get( CONF_HKCLIENT, DEVID, buf_RGN, sizeof(buf_RGN) );
	unsigned int  DevBufLen = strlen( buf_RGN );
    
	unsigned char buf_Time[32] = {0};
	Get_Current_DayTime( buf_Time );
	unsigned int TimeBufLen = strlen( buf_Time );

	//five stream dev name osd init
	for (nChn = 0; nChn < RgnVencChn; nChn++)
	{
		RgnHandle = nChn;
		OSD_Overlay_RGN_Handle_Init( RgnHandle, nChn,DevBufLen); 
		OSD_Overlay_RGN_Display_DevID(RgnHandle,nChn); 
	}

	//five stream time osd init
	RgnHandle = 0;
	for (nChn = 0; nChn < RgnVencChn; nChn++)
	{
		RgnHandle = RgnVencChn+nChn;
		OSD_Overlay_RGN_Handle_Init( RgnHandle, nChn,TimeBufLen); 
		OSD_Overlay_RGN_Display_Time(RgnHandle,nChn); 
	}

	return 0;
}
#else
HI_S32 VOSD_Init()
{
	HI_S32 fontWidth;
	HI_U32 u32BgColor = 0x0000ffff;
	RECT_S Rect[5] = {0};
	HI_S32 u32Width = 0;
	RGN_HANDLE RgnHandle = 0;
	VENC_GRP RgnVencChn = 5;  // 2 main stream,2 slave stream,1jgpeg stream
	int nChn = 0;
	
	if(HI_FAILURE== FONT_Init())
	{
		return HI_FAILURE;	
	}
	
	unsigned char buf_DevID[32] = {0};
	conf_get( CONF_HKCLIENT, DEVID, buf_DevID, sizeof(buf_DevID) );

	unsigned char buf_Time[32] = {0};
	Get_Current_DayTime( buf_Time );
	unsigned int TimeBufLen = strlen( buf_Time );

	for(nChn=0;nChn<RgnVencChn;nChn++)
	{
		switch(nChn)
		{
			case 0:
				fontWidth = 32;
				Rect[nChn].u32Width = 32*32;
				Rect[nChn].u32Height= 32;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				fontWidth = 16;
				Rect[nChn].u32Width = 16*16;
				Rect[nChn].u32Height= 16;
				break;
			default:
				break;
		}
		
		/***************dev name osd******/
		RgnHandle = nChn;
		switch(nChn)
		{
			case 0:
				Rect[nChn].s32Y = OSD_POSITION_Y;
				Rect[nChn].s32X = OSD_POSITION_X;
				break;
			case 1:
			case 2:
			case 4:
				Rect[nChn].s32Y = OSD_POSITION_Y-8;
				Rect[nChn].s32X = OSD_POSITION_X-8;
				break;
			case 3:
				Rect[nChn].s32Y = OSD_POSITION_Y-12;
				Rect[nChn].s32X = OSD_POSITION_X-12;
				break;
			default:
				Rect[nChn].s32X= OSD_POSITION_X;
				break;
		}
		Rect[nChn].u32Width = GetStringWidth(buf_DevID,fontWidth);
				if(HI_FAILURE == VRGN_Create(RgnHandle, nChn , &Rect[nChn] , u32BgColor))
		{
			return HI_FAILURE;
		}			
		VOSD_ShowTextRgn(RgnHandle, nChn ,buf_DevID);

		/***************time osd******/
		RgnHandle = RgnVencChn + nChn; 
		switch(nChn)
		{
			case 0:
				Rect[nChn].s32Y = OSD_POSITION_Y;
				Rect[nChn].s32X = 1280 - (TimeBufLen*32) + 7*OSD_POSITION_X;
				break;
			case 1:
			case 2:
			case 4:
				Rect[nChn].s32Y = OSD_POSITION_Y-8;
				Rect[nChn].s32X = 640 - (TimeBufLen*16)+8*OSD_POSITION_X;
				break;
			case 3:
				Rect[nChn].s32Y = OSD_POSITION_Y-12;
				Rect[nChn].s32X = 320 - (TimeBufLen*16)+9*OSD_POSITION_X;
				break;
			default:
				Rect[nChn].s32X= OSD_POSITION_X;
				break;
		}
		Rect[nChn].u32Width = GetStringWidth(buf_Time,fontWidth);	
		if(HI_FAILURE == VRGN_Create(RgnHandle, nChn , &Rect[nChn] , u32BgColor))
		{
			return HI_FAILURE;
		}
		VOSD_ShowTextRgn(RgnHandle, nChn ,buf_Time);
	}
	
	return HI_SUCCESS;
}
#endif

