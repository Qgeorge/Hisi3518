
#ifndef _SERVER_H_
#define _SERVER_H_

#include "BaseType.h"
#include "P2Pproto.h"

#if defined(__cplusplus)
extern "C" {
#endif



/*
收到客户端数据后的回调函数
*/
typedef INT32 (*P2PNET_READ_CALLBACKEX)(PEER_INFO* pPeerInfo, CHAR* Data, INT32 length);

/*
   SDK初始化函数
   cID----设备ID
   _sPort----所使用的本地端口
   cServer----P2P协助服务器地址可以为IP或者域名
   _NetReadCallBack---收到客户端数据后的回调函数
   nMainSteamBuf-----主码流缓冲大小 单位是K
   nSubSteamBuf-----子码流缓冲大小 单位是K
   
*/
DLL_DECLARE INT32 P2PNetServerSdkInit(CHAR * cID,INT16 sPort,CHAR *cServer,UINT32 u32Key,P2PNET_READ_CALLBACKEX _NetReadCallBack,UINT16 nMainSteamBuf,UINT16 nSubSteamBuf);

DLL_DECLARE INT32 P2PNetServerSetSdkKey(UINT32 u32Key);
/*
     向一个客户端   发送命令
     cBuf------命令地址
     nLen------命令长度
     _reply-----是否需要可靠传输0--不需要  1---可靠传输

*/
DLL_DECLARE INT32 P2PNetServerSndCmd2Link(void * _pLink,CHAR * cBuf,UINT32 nLen);

/*
     发送码流数据澹(sdk会自动分发到需要的客户端)?
     nChannel------通道号从0开始
     nStream------主副码流0--主  1--子
     cBuf-----数据地址
     nLen---数据长度
     cFrameType---帧类型具体定义参照enum FRAME_TYPE_E  音频一直未I帧 
     cDataType----数据类型 具体定义参照enum DATA_TYPE_E 
*/
DLL_DECLARE void  P2PNetServerChannelDataSndToLink(INT32 nChannel,INT32 nStream,CHAR * cBuf, INT32 nLen,INT16 cFrameType,INT16 cDataType);


/*
    发送透明通道信息
     _pLink------客户端标示
     cBuf-----数据地址
     nLen---数据长度
*/
DLL_DECLARE void P2PNetServerSndMsgToLink(PEER_INFO * _pLink,CHAR * cBuf, INT32 nLen);


/*
    向所有已经连接的客户端发送透明通道信息 (例广播报警信息)
     _pLink------客户端标示
     cBuf-----数据地址
     nLen---数据长度
*/
DLL_DECLARE void P2PNetServerSndMsgToAllLink(CHAR* _u8Buf,INT32 _iBufLength);



/*
     获取对讲数据
     _pLink------对接客户端句柄
     pData------获取到的对讲数据
     pLen-----对讲数据长度
     _bBlock---阻塞标志1--阻塞 0--非阻塞
     函数返回值
     阻塞获取的时候0---成功  -1表示对讲连接断开
     非阻塞的时候0---成功 大于零失败  小于0表示连接断开
*/
DLL_DECLARE INT32 P2PNetServerGetTalkData(PEER_INFO * _pLink,char * pData,INT32 *pLen,INT32 _bBlock);





DLL_DECLARE void P2PNetServerSndAlarm(ALARM_TYPE _eAlarmType,INT32 _iTime,INT32 _iChan,CHAR * cTmp);








#if defined(__cplusplus)
}
#endif
#endif






