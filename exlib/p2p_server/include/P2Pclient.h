
#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "BaseType.h"
#include "P2Pproto.h"


#if defined(__cplusplus)
extern "C" {
#endif




/*
    消息回调函数用于处理P2P协助服务器传过来的信息(比如报警信息)
    
       _pContext----P2PNetClientSdkInit函数传入的最后一个参数  
*/
typedef INT32  (*P2PEVENT_PROCESS)(EVENTTYPE _EventID,void *_pMsg,void *_pContext);



/*
    消息回调函数用于处理server端(IPC)发送过来的信息
       _pContext----OpenStream函数传入的最后一个参数  
*/
typedef INT32  (*P2PMSG_PROCESS)(PEER_INFO * _pNvs, CHAR *_pMsg,INT32 _nLen,void *_pContext);


/*
   码流回调函数
   _pNvs----ConnectNvs 函数的返回值
   _cBuf----数据地址
   _iLen----数据长度
   nDataType---数据类型 具体见enum DATA_TYPE_E
   _iFrameType---帧类型  注意音频为I帧类型 具体见enum FRAME_TYPE_E
   _pContext----OpenStream函数传入的最后一个参数  
*/
typedef INT32  (*P2PSTREAM_DATA_PROCESS)(PEER_INFO * _pNvs,CHAR *_cBuf,UINT32 _iLen,INT16 nDataType,INT16 _iFrameType,void *_pContext);



#if 0

/*
    用户注册函数
    参数:
  cUser-----------用户名
   cPassWd----密码
   _sPort----所使用的本地端口
   cServer----P2P协助服务器地址可以为IP或者域名
   返回值:  0---成功   非0---失败
*/
DLL_DECLARE INT32 P2PNetClientUserRegister(CHAR * cUser,CHAR * cPassWd,CHAR * cServer);
/*
    用户登陆函数
  cUser-----------用户名
   cPassWd----密码
   _sPort----所使用的本地端口
   cServer----P2P协助服务器地址可以为IP或者域名
*/
DLL_DECLARE INT32 P2PNetClientUserLogon(CHAR * cUser,CHAR * cPassWd,CHAR * cServer);
DLL_DECLARE INT32 P2PNetClientAddDev(CHAR * cDevID,CHAR * cDevName,CHAR * cUserName,CHAR * cUserPassWd,INT32 iChannelNum);

DLL_DECLARE INT32 P2PNetClientModyDev(CHAR * cDevID,CHAR * cDevName,CHAR * cUserName,CHAR * cUserPassWd,INT32 iChannelNum);

DLL_DECLARE INT32 P2PNetClientDelDev(CHAR * cDevID);

DLL_DECLARE INT32 P2PNetClientShareDev(CHAR * cDevID,CHAR *cUser);

DLL_DECLARE INT32 P2PNetClientCancelShareDev(CHAR * cDevID,CHAR *cUser);

DLL_DECLARE INT32 P2PNetClientUserGetDevList(Dev_Infor *pDev,INT32 *iNum);


#endif



/*
   SDK初始化函数
    cID-----------客户端ID号请使用P2PNetClientUserRegister注册的用户名
   _iNvsNum----同是支持的最大IPC及DVR的数量
   _sPort----所使用的本地端口
   cServer----P2P协助服务器地址可以为IP或者域名
   P2PServerCallBack---消息回调函数 统一接受报警等信息
   _pContext--------P2PServerCallBack回调出去的最后一个参数
*/
#ifdef _WIN32
DLL_DECLARE INT32 P2PNetClientSdkInit(CHAR * cID,INT32 _iNvsNum,UINT16 _sPort,CHAR * cServer,P2PEVENT_PROCESS P2PServerCallBack,void *_pContext);
#else
DLL_DECLARE INT32 P2PNetClientSdkInit(CHAR * cID,INT32 _iNvsNum,UINT16 _sPort,CHAR * cServer,UINT32 u32Key,P2PEVENT_PROCESS P2PServerCallBack,void *_pContext);
#endif
/*
   获取SDK状态
   返回值1----注册到协助服务器
                   其他 ------注册失败
*/
DLL_DECLARE INT32 P2PNetClientGetSdkState(void);



DLL_DECLARE INT32 P2PNetClientSetSdkKey(UINT32 u32Key);



/*
    获取
    cDevID-----------需要获取的电量信息的设备ID
   iStartTime----开始时间
   iEndTime----结束时间
   pDevElec----电量参数保存地址
   iNum---传入最大获取个数  传出获取到的个数
*/
DLL_DECLARE INT32 P2PNetClientUserGetDevElectricity(CHAR * cDevID,INT32 iStartTime,INT32 iEndTime,Dev_Electricity *pDevElec,INT32 *iNum);


/*
   获取当前网络信息
   peer_infor-----见BaseType.h
*/
DLL_DECLARE INT32 P2PNetClientGetPeerInfor(peer_infor *pInfor);


/*
   连接IPC或者DVR   NVR
   参数:
   cNvsId----IPC的设备ID号
   _iConnectMod----连接中模式0----自动  1---中转
   _iIsReconnect----是否需要重连 0--不重连   1---SDK自动重连

   返回值:
   NULL-----失败
   其他---成功IPC的唯一标示
   
   此函数运行的时间根据对方是否可以P2P 而不同最大10S
*/
DLL_DECLARE PEER_INFO * P2PNetClientConnectNvs(CHAR * cNvsId,INT32 _iConnectMod,INT32 _iIsReconnect);


/*
   登录IPC或者DVR   NVR
   参数:
   _pNvs----ConnectNvs的返回值
   _pUserName----IPC的用户名
   _pUserPasswd----IPC的密码
   iTimeOut----超时时间毫秒
    返回值
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientLoginNvs(PEER_INFO * _pNvs,CHAR * _pUserName,CHAR * _pUserPasswd,INT32 iTimeOut,P2PMSG_PROCESS _MsgDataCallBack,void *_pContext);

/*
   连接音视频流此函数可以不用调用关闭码流的接口重复调用用于修改码流类型
   参数:
   _pNvs----ConnectNvs的返回值
   _nChannel----连接的通道号
   _nStream----连接的主副码流  0--主码流 1--子码流
   _nSreamType----连接码流的类型1---视频 2---音频  3---音视频参看STREAM_TYPE_T   
   _StreamDataCallBack----码流回调函数
   _pContext-----码流回调时使用
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientOpenStream(PEER_INFO * _pNvs,INT32 _nChannel,INT32 _nStream,INT32 _nSreamType,P2PSTREAM_DATA_PROCESS _StreamDataCallBack,void * _pContext);

/*
   获取音视频流
   如果不是用回调函数获取音视频数据,在调用OpenStream函数成功后
   可以使用本函数获取音视频数据
   本函数不保证第一次调用的时候获取的是I帧
   参数:
   _pNvs----ConnectNvs的返回值
   _nChannel----连接的通道号
   _nStream----连接的主副码流  0--主码流 1--子码流
   pData-------音视频数据BUF  由调用者申请要足够大
   pLen--------音视频数据的长度
   pDataType-----数据类型  音频或者视频具体参考枚举DATA_TYPE_T
   pFrameType -----帧类型具体参看枚举FRAME_TYPE_T
    _bBlock   -----1阻塞  0---非阻塞  
   返回值:
    0---成功
    非0----失败小于0说明连接已经关闭
*/

DLL_DECLARE INT32 P2PNetClientGetStreamData(PEER_INFO * _pNvs,INT32 _nChannel,INT32 _nStream,char * pData,INT32 *pLen,INT32 * pDataType,INT32 *pFrameType,INT32 _bBlock);


/*
   修改前端登录密码
   参数:
   _pNvs----ConnectNvs的返回值
  _pOldPasswd--- 旧密码最长15字节
   _pNewPasswd----新密码最长15字节         
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientChangePasswd(PEER_INFO * _pNvs,CHAR * _pOldPasswd,CHAR * _pNewPasswd);


/*
   获取风光发电系统的参数
   参数:
   _pNvs----ConnectNvs的返回值
   pDevElecState ---风光发点系统状态  需要上层分配空间
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientGetElecState(PEER_INFO * _pNvs,Dev_ElecState *pDevElecState);



/*
   控制前端关开机风光发电项目用
   参数:
   _pNvs----ConnectNvs的返回值
   _iCmd----命令码1--开机  0--关机 2--设置定时开关机
   _u64StartTime----定时开机时间
                            从CUT（Coordinated Universal Time）时间1970年1月1日00:00:00（称为UNIX系统的Epoch时间）到当前时刻的秒数
   _u64StopTime----定时关机时间
                            从CUT（Coordinated Universal Time）时间1970年1月1日00:00:00（称为UNIX系统的Epoch时间）到当前时刻的秒数
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientControlDev(PEER_INFO * _pNvs,INT32 _iCmd,UINT64 _u64StartTime,UINT64  _u64StopTime);




DLL_DECLARE INT32 P2PNetClientGetDevElectricity(PEER_INFO * _pNvs,Dev_Electricity *pDevElec);



/*
   断开音视频流
   参数:
   _pNvs----ConnectNvs的返回值
   _nChannel----连接的通道号
   _nStream----连接的主副码流  0--主码流 1--子码流
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientCloseStream(PEER_INFO * _pNvs,INT32 _nChannel,INT32 _nStream);

/*
   登出IPC
   参数:
   _pNvs----ConnectNvs的返回值
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientLogoutNvs(PEER_INFO * _pNvs);

/*
  断开与IPC的连接
   参数:
   _pNvs----ConnectNvs的返回值
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientDisconnectNvs(PEER_INFO * _pNvs);

/*
  获取与IPC的连接
   参数:
   _pNvs----ConnectNvs的返回值
   返回值:
    0---登录成功
    非0----不在线连接或者登录失败
*/

DLL_DECLARE INT32 P2PNetClientGetNvsState(PEER_INFO * _pNvs);


/*
  设置视频参数 亮度色度对比度
   参数:
   _pNvs----ConnectNvs的返回值
   _nChannel----通道号
   _pVideoPara---需要设置的值 具体看该结构体(内存 应用层管理)
   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientSetVideoPara(PEER_INFO * _pNvs,INT32 _nChannel,VideoPara *_pVideoPara);

/*
  设置视频通道参数
   参数:
   _pNvs----ConnectNvs的返回值
   _nChannel----通道号
   _nStream  ----主副码流  0--主码流  1--子码流
   _pVideo---需要设置的值 具体看该结构体(内存 应用层管理)
   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientSetChannelInfo(PEER_INFO * _pNvs,INT32 _nChannel,INT32 _nStream,ChannelIfor * _pVideo);

/*
   云镜控制
   参数:
   _pNvs----ConnectNvs的返回值
   _nChannel----通道号
   _pPtz---需要设置的值 具体看该结构体(内存 应用层管理)
   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientPtzControl(PEER_INFO * _pNvs,INT32 _nChannel,PtzControlReq * _pPtz);

/*
   获取设备能力
   参数:
   _pNvs----ConnectNvs的返回值
   pCapabilities---具体看该结构体(内存 应用层管理)
   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientGetNvsCapabilities(PEER_INFO * _pNvs,DevCapabilities * pCapabilities);

/*
   获取通道视频参数
   参数:
   _pNvs----ConnectNvs的返回值   
   _nChannel----通道号
   _pVideoPara---具体看该结构体(内存 应用层管理)
   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientGetVideoPara(PEER_INFO * _pNvs,INT32 _nChannel,VideoPara *_pVideoPara);

/*
   获取通道视频编码参数
   参数:
   _pNvs----ConnectNvs的返回值   
   _nChannel----通道号
   _nStream  ----主副码流  0--主码流  1--子码流
   _pVideoPara---具体看该结构体(内存 应用层管理)
   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientGetChannelInfo(PEER_INFO * _pNvs,INT32 _nChannel,INT32 _nStream,ChannelIfor * _pInfor);

/*
   请求与前端对讲
   参数:
   _pNvs----ConnectNvs的返回值   
   _pTalk----用于获取前端支持的音频参数由调用者分配内存   
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientTalkReq(PEER_INFO * _pNvs,P2pTalkMsg * _pTalk);




/*
   关闭与前端对讲
   参数:
   _pNvs----ConnectNvs的返回值   
    返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE INT32 P2PNetClientTalkClose(PEER_INFO * _pNvs);


/*
   向IPC端发送透明通道消息IPC端会数据回调函数中收到信息
   参数:
   _pNvs----ConnectNvs的返回值   
   cBuf----消息内容
   nLen  ----消息长度
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE void P2PNetClientSndMsgToServer(PEER_INFO * _pNvs,CHAR * cBuf, INT32 nLen);


/*
   向IPC端发送对讲音频数据
   参数:
   _pNvs----ConnectNvs的返回值   
   cBuf----音频数据内容
   nLen  ----音频数据长度
   返回值:
    0---成功
    非0----失败
*/
DLL_DECLARE void P2PNetClientSndTalkDataToServer(PEER_INFO * _pNvs,CHAR * cBuf, INT32 nLen);


DLL_DECLARE INT32 P2PNetClientGetFile(PEER_INFO * _pNvs,CHAR *pSrcFileName,CHAR *pDstFileName,INT32 iOutTime);


DLL_DECLARE INT32 P2PNetClientSdkUnInit(void);


#if defined(__cplusplus)
}
#endif


#endif


