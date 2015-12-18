#ifndef _PROTO_H_
#define _PROTO_H_

#include "BaseType.h"

#define PEER_INFO void 

#define HEART_TIME_OUT 30

#define MAX_CHANNNEL_NUM 4

#define MAX_STREAM_NUM 2

#define MAX_USERNAME 15
#define MAX_PASSWORD 15
#define STREAM_FRAM_ID 0x00000002
#define  DATAPACKHEAD  0xF9EAF5F1    //数据包包头 最后4位为版本号
#define  CMDPACKHEAD   0xF5EAF5F1    //命令包包头


#define  MAX_FRAME_SIZE  (512*1024)
#define  MAX_AUDIO_SIZE  (4*1024)
#define  MAX_TALK_SIZE  (4*1024)
#define  MAX_CMD_SIZE  (64*1024)


typedef enum _EVENTTYPE
{
	EVENT_ALARM,    //报警
	EVENT_DATA      //数据
}EVENTTYPE;


//报警类型
typedef enum
{
  ALARM_VIDEO_LOST = 100,  
  ALARM_VIDEO_MOTION,  
  ALARM_VIDEO_OCCLUSION,  
  ALARM_PORT_IN,  
}ALARM_TYPE;




typedef struct 
{
    INT32 m_iDataType; 
    INT32 m_iFrameType; 
    INT32 m_iFrameSize; 
}FramePack;

//视频编码类型枚举
typedef enum STREAM_TYPE_E
{
    STREAM_VIDEO = 1,     
    STREAM_AUDIO,   
    STREAM_VIDEOAUDIO,       
    STREAM_BUTT
} STREAM_TYPE_T;

//视频编码类型枚举
typedef enum VIDEO_ENCTYPE_E
{
    VIDEO_H264 = 1,     //h.264
    VIDEO_MPEG,         //mpeg
    VIDEO_BUTT
} VIDEO_ENCTYPE_T;

//音频编码类型枚举ADPCM_A :0x03;G726:0x04  PCM 0x05
typedef enum AUDIO_ENCTYPE_E
{
    AUDIO_G711A = 1,     //G711A
    AUDIO_G711U,         //G711U    
    AUDIO_ADPCMA,    
    AUDIO_G726,
    AUDIO_PCM,
    AUDIO_BUIT
} AUDIO_ENCTYPE_T;



typedef enum DATA_TYPE_E
{
    DATA_VIDEO = 0,   
    DATA_AUDIO,   
    DATA_TALK,         
    DATA_BUTT
} DATA_TYPE_T;


typedef enum FRAME_TYPE_E
{
    FRAME_I = 0,   
    FRAME_P,      
    FRAME_BUTT
} FRAME_TYPE_T;


typedef enum{
	PTZ_TU_P2P = 1, //上
	PTZ_TD_P2P,		//下
	PTZ_PL_P2P,		//左
	PTZ_PR_P2P,		//右
	PTZ_TUPR_P2P,	//右上
	PTZ_TUPL_P2P,	//左 上
	PTZ_TDPR_P2P,	//右下
	PTZ_TDPL_P2P,	//左下
	PTZ_STOP_P2P,	//停止
	PTZ_ZIN_P2P,	// 焦距拉近
	PTZ_ZOUT_P2P,	//焦距拉远
	PTZ_IST_P2P,	//变化停止
	PTZ_FN_P2P,		//焦点调近
	PTZ_FR_P2P,		//焦点调远
	PTZ_FST_P2P,	//焦点变化停止
	PTZ_IA_ON_P2P,	//光圈自动调整
	PTZ_IO_P2P,		//光圈增大
	PTZ_IC_P2P,		//光圈缩小
	PTZ_WP_ON_P2P,//擦拭启动
	PTZ_WP_OFF_P2P,//擦拭停止
	PTZ_LP_ON_P2P,	//灯光开启
	PTZ_LP_OFF_P2P,//灯光停止
	PTZ_AUTO_START_P2P,//巡航开始
	PTZ_AUTO_STOP_P2P,//巡航停止
	PTZ_GOTO_PRESET_P2P,//转到预设点
	PTZ_START_TRACK_CRUISE_P2P,//开始轨迹巡航
	PTZ_STOP_TRACK_CRUISE_P2P,//停止轨迹巡航
	PTZ_SET_PRESET_P2P,
	PTZ_POWER_ON_P2P,	        //电源开启
	PTZ_POWER_OFF_P2P,          //电源关闭
	PTZ_3D_POSITION_P2P,		//PTZ 3D定位
	PTZ_3D_ROCKER_P2P,			//对3维摇杆的响应
	PTZ_ASSISTANT_ON_P2P,		//辅助开关开
	PTZ_ASSISTANT_OFF_P2P,		//辅助开关关
	PTZ_RM_PRESET_P2P,         //删除预置点
}ENUM_PTZ;


#if defined(_MSC_VER) // Microsoft compiler
#define __attribute__(x)
#elif defined(__GNUC__) // GNU compiler
#else
#endif

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{
	UINT32	  m_ui32Head;		//数据包包头
	UINT16    m_ui16Channel;    //通道号
    UINT16    m_ui16Stream;     //主副码流
	UINT16    m_ui16Size;       //数据长度
	UINT16    m_u16Type;        //类型  对讲或者音视频流
	UINT16    m_u16Postion;     //位置1----开始  2----结束  
	UINT16    m_u16FrameType;   //帧类型  
	UINT32    m_u32FrameID;     //帧ID
	UINT32    m_u32TotalLen;     //帧总长度
	UINT32    m_u32Pad[4];     //扩展  0字节为1表示录像回放数据
}__attribute__ ((packed))DATAHEADP2P,*PDATAHEADP2P;

//命令包包头
typedef struct
{
	INT32    m_ui32Head;			//命令包包头
	INT32    m_iMsgType;
	UINT16    m_u16Postion;         //位置1----开始  2----结束  
	UINT16    m_ui16Size;     
	UINT32    m_u32TotalLen;     //命令总长度
	UINT32    m_u32Pad[4];     //扩展
}__attribute__ ((packed))HEADP2P,*PHEADP2P;

//命令包包头
typedef struct
{
	INT32     m_iMsgType; 
	UINT32    m_u32TotalLen;     //命令总长度
	UINT32    m_u32Pad[4];     //扩展
}__attribute__ ((packed))CMDHEADP2P,*PCMDHEADP2P;

typedef struct
{	
    INT16 m_iChannel;	
	INT16 m_iStream;
	INT16 m_iAlarmNum;
	INT16 m_iPad;
}__attribute__ ((packed))DevCapabilities;

typedef struct 
{ 
	INT32 Width;   //视频宽度
	INT32 Height;  //视频高度
	INT32 BitRat;   //视频码率
	INT32 FrameRate; //视频帧率
	INT32 IFrameRate;//视频I帧帧率
	INT32 Quity;    //视频质量
	INT32 VideoType;//视频频编码类型1---H264 
	INT32 AudioType;//音频编码类型G711_A:0x01; G711_U:0x02;ADPCM_A :0x03;G726:0x04  PCM 0x05
}__attribute__ ((packed))ChannelIfor;

typedef struct 
{       
	INT32 Brightness;                       
	INT32 Contrast;                         
	INT32 Saturation;                       
	INT32 Hue;                              
}__attribute__ ((packed))VideoPara;


//用户登录
#define REQ_LOGIN 1
typedef struct 
{
	CHAR szUserName[MAX_USERNAME]; //用户名
	CHAR szPassword[MAX_PASSWORD]; //密码
}__attribute__ ((packed))LogInReq;


//用户登录
#define REQ_LOGIN2 1000
typedef struct 
{
	CHAR szUserName[MAX_USERNAME+1]; //用户名
	CHAR szPassword[MAX_PASSWORD+1]; //密码
}__attribute__ ((packed))LogInReq2;

typedef struct 								//用户登录回应
{
	INT32 m_iLogInRes;							//返回码
    DevCapabilities m_Capabilities;
    ChannelIfor  m_ChannelIfor[MAX_CHANNNEL_NUM][MAX_STREAM_NUM];
    VideoPara m_VideoPara[MAX_CHANNNEL_NUM];
} __attribute__ ((packed))LogInRsp;

//用户登出
#define REQ_LOGOUT 0x02
typedef struct 								    //用户登录回应
{
	INT32 m_iLogOutRs;							//返回码
} __attribute__ ((packed))LogOutRsp;


//获取某通道视频参数
#define REQ_GETCHANNELINFOR 0x03

typedef struct 
{
	UINT16 nChannelId;   //通道号0----
	UINT16 nStreamId;    //码流号0--主 1--子           
}__attribute__ ((packed))GetChannelIforReq;

typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子        
    ChannelIfor mChannelIfor;
}__attribute__ ((packed))GetChannelIforRsp;


//设置某通道视频参数
#define REQ_SETCHANNELINFOR 0x04
typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子 
    ChannelIfor mChannelIfor;
}__attribute__ ((packed))SetChannelIforReq;

typedef struct 
{
	INT32 m_iSetRs;							//返回码
}__attribute__ ((packed))SetChannelIforRsp;

//视频参数 亮度对比度?
#define REQ_GETVIDEOPARA 0x05

typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子           
}__attribute__ ((packed))GetVideoParaReq;

typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子          
    VideoPara mVideoPara;                            
}__attribute__ ((packed))GetVideoParaRsp;


#define REQ_SETVIDEOPARA 0x06
typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子    
    VideoPara mVideoPara;                                        
}__attribute__ ((packed))SetVideoParaReq;

//请求码流
#define REQ_PLAYSTREAM 0x07
typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子     
	INT16 nStreamType;    //     
	INT16 nPad;    //               
}__attribute__ ((packed))PlayStreamReq;

typedef struct 
{
	INT32 m_iSetRs;							//返回码
}__attribute__ ((packed))PlayStreamRsp;

#define REQ_SHUTDOWNSTREAM 0x08
typedef struct 
{
	INT16 nChannelId;   //通道号0----
	INT16 nStreamId;    //码流号0--主 1--子           
}__attribute__ ((packed))ShutDownStreamReq;

typedef struct 
{
	INT32 m_iSetRs;							//返回码
}__attribute__ ((packed))ShutDownStreamRsp;


//PTZ控制
#define REQ_PTZCONTRIL 0x09
typedef struct 
{
    INT32 iAction;  //具体见ENUM_PTZ
	INT32 iData;    //命令所带参数比如设置预置点的命令  表示预置点
	INT32 iSpeedX;  //左右速度
	INT32 iSpeedY;  //上下速度
	INT32 iCount;   //暂时没用
	CHAR DecBuf[32];//暂时没用
}PtzData;

typedef struct 
{
	INT16 nChannelId;			   //请求通道
	PtzData mPtzData;
}__attribute__ ((packed))PtzControlReq;


#define REQ_ACTIVE    0x0a   //心跳

#define MSG_TRANS 0x0B

#define REQ_TALK 0x0C

#define REQ_TALKCLOSE 0x0D

typedef struct 
{
	INT16 nAudioTYpe;  //音频编码类型G711_A:0x01; G711_U:0x02;ADPCM_A :0x03;G726:0x04  PCM 0x05
	INT16 nAudioSample; //采样率
	INT16 nAudioBit; //采样位宽	
	INT16 nAudioChannel; //声道数
}__attribute__ ((packed))P2pTalkMsg;




#define REQ_GETRECORDLIST 0x10  //获取录像文件列表

typedef struct _P2PRecordFile_Sql
{
    INT32 m_iStartTime;
    INT32 m_iEndTime;
    INT32 m_iType;
}__attribute__ ((packed))P2PRecordFile_Sql;


typedef struct _P2PRecordFile_Infor
{
    INT32 m_iStartTime;
    INT32 m_iEndTime;
    INT32 m_iType;
}__attribute__ ((packed))P2PRecordFile_Infor;







typedef struct 
{
    CHAR m_cID[MAX_ID_LEN + 1];	  //节点ID
    INT32 m_iTime;               //报警时间
    INT32 m_iChannel;               //报警时间
    INT32 m_iAlarmType;          //报警类型
    CHAR m_cTmp[32];               //预留
}__attribute__ ((packed))PeerAlarm_Msg;











typedef struct
{
    CHAR m_cID[MAX_ID_LEN + 1];	//要搜索的节点ID 节能为空    
	int m_iChanNum;
}__attribute__ ((packed))PeerLook;















typedef struct _User_Infor
{
    CHAR m_cUser[MAX_ID_LEN + 1];
    CHAR m_cPassWord[MAX_ID_LEN + 1];
    INT32 m_iRole;
}__attribute__ ((packed))User_Infor;

typedef struct _Dev_Infor
{
    CHAR m_cDevID[MAX_ID_LEN + 1];
    CHAR m_cDevName[MAX_DEVNAME_LEN + 1];
    CHAR m_cUser[MAX_ID_LEN + 1];
    CHAR m_cPassWord[MAX_ID_LEN + 1];
    INT32 m_iChannelNum;
}__attribute__ ((packed))Dev_Infor;


typedef struct _DevElec_Sql
{
    CHAR m_cDevID[MAX_ID_LEN + 1];
    INT32 m_iStartTime;
    INT32 m_iEndTime;
}__attribute__ ((packed))DevElec_Sql;


#define REQ_GETELEC 0x70
typedef struct _Dev_Electricity
{
    CHAR m_cDevID[MAX_ID_LEN + 1];
    INT32 m_iTime;
    INT32 m_iWindBV;            //风能电池电压:0.0V~62.0V，精度为0.1V
    INT32 m_iWindBI;            //电池充电电流：0.0~60.0A  精度为0.1A
    INT32 m_iWindBW;            //风机充电功率：0.1~2000.0W  精度为0.1W
    INT32 m_iWindRPM;           //风机转速：0~2000 RPM   精度为 1 RPM
    INT32 m_iWindE;             //风机累积发电量：0~999.9 KWH（度） 精度为0.1KWH
    INT32 m_iLightV;            //光伏 光伏输入电压：0.0V~100.0V  精度为0.1V
    INT32 m_iLightI;            //光伏输入电流：0.0V~40.0A  精度为0.1A    
    INT32 m_iLightBV;           //电池电压:0.0V~62.0V，精度为0.1V
    INT32 m_iLightBI;           //电池充电电流：0.0~60.0A  精度为0.1A
    INT32 m_iLightBW;           //光伏充电功率：0.1~2000.0W  精度为0.1W
    INT32 m_iLightE;            //光伏累积发电量：0~6553.5 KWH  精度为0.1KWH    (预留)
    INT32 m_iDCBV;              //电池电压：0.0V~50.0V，精度为0.1V
    INT32 m_iDCV;               //输出电压：0.0V~300.0V 
    INT32 m_iDCI;               //输出电流：0.0A~30.0A，
    INT32 m_iDCW;               //输出功率：0W~3000W
    INT32 m_iDCBQ;              //电池电量：0%~100%
    INT32 m_iDevStat;           //设备工作状态0---正常  其他---异常(故障码)
    INT32 m_iDevPowerStat;       //开关机状态1--开机  0--关机
    UINT64 m_u64StartTime; //自动开机时间
    UINT64 m_u64StopTime; //自动关机时间
    
}__attribute__ ((packed))Dev_Electricity;


#define REQ_DEVPOWCONTORL 0x71
typedef struct _Dev_CONTORL
{
    INT32 m_iDevContorl;    //1--开机0---关机 2---自动开关机设置
    UINT64 m_u64StartTime; //自动开机时间
    UINT64 m_u64StopTime; //自动关机时间   
    INT32 m_iState;
}__attribute__ ((packed))Dev_CONTORL;


#define REQ_CHANGPASSWD 0x72
typedef struct _Dev_CHPASSWD
{
    CHAR m_cPasswdOld[MAX_ID_LEN + 1];
    CHAR m_cPasswdNew[MAX_ID_LEN + 1];
    INT32 m_iState;
}__attribute__ ((packed))Dev_CHPASSWD;


#define REQ_FILE 0x73
#define RSP_FILE 0x74

typedef struct _File_Infor
{
    CHAR  m_cFileName[MAX_FILENAME_LEN + 1];
    CHAR  m_cPad[4];
    INT32 m_iSsrc;
    INT32 m_iFlag; 
    INT32 m_iPackLen; 
    void *m_pData;
}__attribute__ ((packed))File_Infor;


typedef struct _Dev_ElecState
{   
    INT32 m_iDCBQ;              //电池电量：0%~100%
    INT32 m_iDevStat;           //设备工作状态0---正常  其他---异常(故障码)
    INT32 m_iDevPowerStat;       //开关机状态1--开机  0--关机
    UINT64 m_u64StartTime; //自动开机时间0---表示未设置
    UINT64 m_u64StopTime; //自动关机时间  0----表示未设置
    INT32 m_iPad;
}__attribute__ ((packed))Dev_ElecState;



#if defined(_MSC_VER)
#pragma pack()
#endif



typedef INT32  (*PEERNVSSERCH_PROCESS)(void * _pData,PeerLook * _pNvs);





#endif



