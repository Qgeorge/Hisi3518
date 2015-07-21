#ifndef __IPC_FILE_SD_H__
#define __IPC_FILE_SD_H__
#include "rs.h"
/*
*SD卡开始记录
*/
int sd_record_start();
/*
*SD卡停止记录
*/
void sd_record_stop();

/*
*推动sd卡的数据
*/
int sccPushTfData(int pStreamType, char *pData, unsigned int nSize, short iFrame, int iResType, int iStreamType );
/*
*删除sd卡的数据
*/
int file_SdDeleteFile( const char *cFileName );
/*
*注册sd卡对象
*/
void SD_RSLoadObjects(RegisterFunctType reg);

#endif
