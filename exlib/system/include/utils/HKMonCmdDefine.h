#pragma once
#include <time.h>

#define HK_KEY_MONSERVER "HKMONServer"

enum _HK_RES_TYPE
{
	HK_RES_AUDIO=1,     
	HK_RES_VIDEO=2,
	HK_RES_AUDIOVIDEO=3,
	HK_RES_REC=4,       //录像
	HK_RES_PALY=5,      //点播
	HK_RES_AUDIOIN=6,	//听
	HK_RES_AUDIOOUT=7,	//说
	HK_RES_MRS=8,
	HK_RES_ID_PWD=9,	//通过ID密码访问
};

//录像文件类型区分..
enum _FILE_FRAME_DATA_TYPE
{
	HK_MRS_MPEG4 = 1,
	HK_MRS_MJPEG = 2,
	HK_MRS_H264 = 3,
	HK_MRS_G723 = 4,
	HK_MRS_G729 = 5,
};

enum _HK_MON_OPERATION_CODE
{
	HK_MON_REGISTER =		101,	//获取MON业务信息
	HK_MON_FRIENDNOTIFY =	102,	//通知好友上线  (一个)
	HK_MON_MOREFRIENDONLINE = 103,	//多个好友上线(登录时)

	HK_MON_GETITEM =		104,	//获取文件夹列表
	HK_MON_ADDFILE =		105,	//添加文件夹
	HK_MON_UPDATEFILE =		106,	//修改文件夹名称
	HK_MON_DELETEFILE =		107,	//删除文件夹
	HK_MON_UPDATEDEVNAME =	108,	//用户修改资源名称
    HK_MON_MOVEDEV =		109,	//用户移动资源
	HK_MON_DEVRESOLUTION =	110,	//修改资源分辨率（码流）
	HK_MON_UPDATEDEVRATE =	111,	//修改资源帧率
	HK_MON_UPDATECOMPRESS = 112,	//修改资源压缩方式

	HK_MON_DEVCONFIGINFO =	113,	//资源配制信息
	HK_MON_SHOOTINGINFO =	114,	//定时抓拍查询
	HK_MON_UPDATESHOOINFO = 115,	//定时抓拍设置
	HK_MON_CONTROLPT =		116,	//云台控制
	HK_MON_CONTROLFOCUS =	117,	//焦聚调整（散焦,自动,聚焦）
	HK_MON_CONTROLFU =		118,	//焦聚 近/远
	HK_MON_FINDCARDINFO =	119,	//SD卡状态关闭开启
	HK_MON_FINDCARDINFORET =120,	//SD卡状态信息查询的返回
	HK_MON_FORMATCARD =		121,	//SD卡格式化
	HK_MON_FORMATCARDRET =	122,	//SD卡格式化的返回
	HK_MON_QUERYPHOTODOW =	123,	//SD照片下载
	HK_MON_QUERYPHOTOLIST = 124,	//SD照片文件夹照片
	HK_MON_DELETEPHOTO =	125,	//SD删除照片
	HK_MON_DELETEPHOTORET = 126,	//SD删除照片的返回
	HK_MON_PHOTODOWN =		127,	//照片下载
	HK_MON_PHOTODOWNRET =	128,	//照片下载的返回
	HK_MON_ALARMNOTIFY =	129,	//报警通知
	HK_MON_ALARMNOTIFYRET = 130,	//报警通知
	HK_MON_CARDLOSTREPORT = 131,	//SD卡丢失报告
	HK_MON_CARDLOSTREPORTRET=132,	//SD卡丢失报告的返回
	HK_MON_REPLAYVIDEO =	133,	//远程录像
	HK_MON_STOPREPLAYVIDEO =134,	//停止录像
	HK_MON_FINDMRS = 135,			//搜索资源
	HK_MON_PLAYMRS = 136,			//点播资源
	HK_MON_PLAYORSTOP = 137,		//暂停/开始点播
	HK_MON_MRSDOWN = 138,			//资源下载
	HK_MON_DELETEMRS = 139,			//删除资源

	HK_MON_INVITE =    140,			//创建会话
	HK_MON_INVITERESULT = 141,		//对方同意或是接受
	HK_MON_INVITEACK = 142,       
	HK_MON_CANCEL  =   143,			//取消对话
	HK_MON_CANCELACK = 144,			//取消会话的返回
	HK_MON_BYE  =	   145,			//结束会话  
	HK_MON_BYEACK =    146, 

	HK_MON_UNREGISTER = 147,		//注销
	HK_MON_SUBSCRIBE = 148,			//订阅
	HK_MON_NOTIFY = 149,			//通知
	HK_MON_FRIENDOFFLINE = 150,		//单个用户下线
	
	HK_MON_SET = 151,
	HK_MON_GET = 152,
	HK_MON_GOTOANCHOR = 153,		//快速定位 
	HK_MON_SENSITIVITY = 154,		//报警敏感度设置

	//新增功能点    0609
	HK_MON_GETDEVLIST =		155,	//获取我的设备列表
	HK_MON_ADDDEV =			156,	//添加我的设备
	HK_MON_DELETEDEV =		157,	//删除我的设备
	HK_MON_UPDATEDEVPASSW = 158,	//修改设备的管理密码
	HK_MON_TRANSFERRIGHT =	159,	//转让设备
	HK_MON_GETDEVRIGHT =	160,	//获取设备的权限
	HK_MON_UPDATEDEVRIGHT = 161,	//修改设备的权限
	HK_MON_UPDATEDEVALIAS = 162,	//修改设备的别名
	HK_MON_GETRIGHTLIST =	163,	//获取权限列表
	HK_KEY_UPDATEDEVRIGHT = 164,	//修改权限列表
	HK_MON_DELETEUSRBYLIST =165,	//删除某个用户（从权限列表中）
	HK_MON_SEARCHUSR =		166,	//模糊搜索用户（用于添加在权限列表中）
	HK_MON_ADDUSERTOLIST =	167,	//添加某个用户到权限列表

	HK_MON_CONTRAST =	168,		//设置设备的对比度
	HK_MON_EXPOSURE =	169,		//设置设备的曝光度
	HK_MON_SATURATION = 170,		//设置设备的饱和度
	HK_MON_BRIGHTNESS = 171,		//设置设备的亮度
	HK_MON_COLOR =		172,		//设置设备的色彩
	HK_MON_SHARPNESS =	173,		//设置设备的锐度
	HK_MON_ADDMONDEV =	174,		//监控列表中，添加设备
	HK_MON_ADDDEVBYPROBLEM = 175,	//监控列表中，添加设备时，认证方式是问题，问题验证

	HK_MON_DELMONDEV =	176,		//监控列表中，删除设备
	HK_MON_SEARCHDEV =	177,		//监控列表中，搜索设备
	HK_MON_DEVINFO =	178,		//监控列表中，查询设备详细信息
	HK_MON_SEARCHALARMSET = 179,	//读取报警设置信息
	HK_MON_RECINFOREAD =	180,	//读取录像信息
	HK_MON_UPDATEALARMORRECINFO=181,//录像相关属性的设置、录像计划设置
	HK_MON_SEARCHALERTRECORD =	182,//报警记录查询
	HK_MON_SEARCHVISTDEV =		183,//设备访问查询
	HK_MON_SEARCHSOURCEINFO =	184,//搜索点播资源
	HK_MON_MRS =			185,	//提交mrs服务器信息
	HK_MON_MRSSYN =			186,	//提交MRS报警和录像的设置 同步
	HK_MON_MRSSYNRET =		187,	//提交MRS同步的返回
	HK_MON_SEARCHINSEEDEV =	188,	//搜索有多少人正在访问的设备
	HK_MON_UPDATEDEVRIGHTRET=189,	//转让设备的确认的返回
	HK_MON_ADDDEVNOTIFY =	190,    //在设备管理中添加设备时，将该设备同步到监控列表的首级目录
	HK_MON_DELETEDEVINFO =	191,	//删除录像记录与报警记录
	HK_MON_DEVVIDEOINFO =	192,	//录像信息记录发送..
	HK_MON_DEVVIDEOREVERSAL=193,	//控制图像翻转...
	HK_MON_MONTOMRSINFO =	194,	//MON服务器信息同步到MRS服务器.
	HK_MON_GETALARMVIDEOINFO=195,	//获取报警录像信息..
	HK_MON_MRSDELETEFILE =	196,	//空间大小不够删除文件记录
	HK_WEB_USERSTARTMRS =	197,	//用户开启录像服务器.
	HK_MON_CALLPROBLEM =	198,	//呼叫问题验证..
	HK_MON_PHONEINFO =		199,	//短信发送..
	HK_WEB_GETITEM =		200,	//WEB获取用户在线列表..
	HK_WEB_USERLOGINSTATUS =201,	//WEB登录到MON服务器订阅所有设备的状态.
	HK_MON_LPTSPEED	=		202,	//云台速度控制..
	HK_MON_AGREEMENT =		203,	//协议：PELCO-D/PELCO-P
	HK_MON_BAUDRATE =		204,	//波特率：2400/9600/4800
	HK_MON_DEVIOARAM =		205,	//设备IO端口设置..
	HK_MON_GETDEVPOPEDOM=	206,	//获取用户访问设备的权限
	HK_MON_SETALARMINFO =	207,	//设置报警设置信息
	HK_MON_FINDPUBLICDEV=	208,	//查找所有公开的设备.
	HK_WEB_BROADCASTINFO=	209,	//广播消息web->monserver->ui
	HK_MON_FREQUENCY =		210,	//设置设备的频率
	HK_MON_AUTOLPTSPEED=	211,	//云台水平自动转动
	HK_MON_MYDEVUPDATE=		212,	//所属设备被别人添加走，通知所属人.
	HK_MON_PTTOGGLE =		213,	//云台切换左右，上下
	HK_MON_SETVBR =			214,	//VBR调整
	HK_MON_ALARM_EMAIL_TEST=215,	//报警邮件测试
	HK_MON_DEV_IRATE=		216,	//修改设备I帧间隔
	HK_MON_DEV_RESOLUTION = 217,	//修改分辨率
	HK_MON_DEV_TRANSLATE =	218,	//透传信令
	HK_MON_DEV_NETPROXY =	219,	//连接poxy修改
	HK_MON_DEV_SHAREADDR =	220,	//生成WEB访问页面
	HK_MON_CLOSE_SHAREADDR=	221,	//关闭WEB访问页面
	HK_MON_DEV_INTERNET =	222,	//关闭开启互联网功能
	HK_MON_SET_LOCAL_PORT=	223,	//设置端口
	HK_MRS_DRIVE_NOTIFY	=	224,	//录像驱动心跳发送
	HK_MON_DEV_IRORSIGNAL = 225,	//打开信号灯IR打开与关闭
	HK_MON_DEV_PRESET     = 226,	//设置设置调用预置位
	HK_MON_DEV_SD_SET     = 227,	//设置SD卡相关信息
	HK_MON_SET_DEV_APERTURE=228,	//光圈调节
	HK_MON_SET_VBRORCBR	   =229,	//码流模式 0:CBR 1:VBR
	HK_MON_SET_RESET_PARAM =230,	//参数恢复默认值
	HK_MON_SET_IO_IN	   =231,	//设备IO设置.输入
	HK_MON_SET_ALARM_RNG   =232,	//报警区域设置
	HK_MON_SET_ACCESS_PWD  =233,	//修改设置访问密码
	HK_MON_ADD_SHARE_ANY   =234,	//分享设备
	HK_MON_SET_FTPSERVER  = 235,	//设置SD卡FTP备份数据

	HK_MON_SET_DEVPARAM = 236,
	HK_MON_GET_DEVPARAM = 237,

	HK_MON_PPOE =			 238,	//PPOE拨号
	HK_MON_RESTORATION_PARAM=239,	//恢复出厂参数
	HK_MON_UPDATE_DEV =		 240,	//手动升级设备
	HK_MON_SET_AUDIOALARM=	 241,	//声音报警开关
	HK_MON_SET_THERMALALARM= 242,	//热感应报警开关
	HK_MON_ALARMPARAM =		 243,	//报警设置
	HK_MON_CONTROL_DEV=		 244,	//控制外接设备(433) 
	HK_MON_GET_CONTROL_DEV=	 245,	//获取外接设备状态(433)
	HK_MON_DEV_FOCUSING =    246,	//聚焦10+ 11- 
	HK_MON_DEV_LIGHT	=	 247,	//1.开启灯泡; 2关闭灯泡
	HK_MON_DEV_ALARM_LIGHT=  248,	//1.开启移动侦测亮灯; 2.关闭移动侦测亮灯
	HK_MON_DEV_WIFISTATE =   249,	//WIFI配置状态检测
	HK_MON_NAS_DIRECTORY =   250,	//获取NAS共享目录
	HK_MON_GET_DEVSTATUS =   251,	//获取设备状态
	HK_MON_LAN_DEVVIDEOINFO= 252,	//在局域网获取视频参数
	HK_MON_ALARM_PHONE	=    253,	//报警电话
	HK_MON_ONVIF_INFO =      254,	//onvif相关配置

	//-------voip-------
	HK_VOIP_GETITEM =	301,	//获取VOIP联系列表..
	HK_VOIP_GETMYITEM = 302,	//获取我的VOIP群组.
	HK_VOIP_ADDMYVOIP = 303,	//添加我的VOIP群组.
	HK_VOIP_ADDUSER =	304,	//添加VOIP群人员
	HK_VOIP_DELUSER =	305,	//删除VOIP群人员
	HK_VOIP_UPDATEUSER=	306,	//修改VOIP群人员
	HK_VOIP_UPDATEPASSWD=307,	//修改VOIP群密码.

	//--------mrs-----------
	HK_MRS_GTEMYITEM =	400,	//获取我的MRS服务器名称
	HK_MRS_GETITEM =	401,	//获取指定服务器的用户
	HK_MRS_ADDMYMRS =	402,	//添加我的录像服务器
	HK_MRS_DELMYMRS =	403,	//删除我的录像服务器
	HK_MRS_MRSTOUSER =	404,	//转让我的录像服务器
	HK_MRS_ADDUSERTOMRS=405,	//添加用户到指定录像服务器
	HK_MRS_UPDATEUSER = 406,	//修改用户的录像服务器信息
	HK_MRS_DELUSER =	407,	//从录像服务器删除用户
	HK_MRS_UPDATEPASW =	408,	//修改录像服务器的管理密码
	HK_MRS_ADDDEVMRS =	409,	//添加设备到指定录像服务器
	HK_MRS_DELETEDEVMRS=410,	//删除录像服务器中的设备
	HK_MRS_GETMRSDEV =	411,	//获取指定服务器的设备
	HK_MRS_GETREADINDEX = 412,	//获取文件索引

	//--------dev-----------
	HK_DEV_REGISTER =	  500,	//设备注册MON业务
	HK_DEV_UPDATEPASSWD = 501,	//重置设备局域网访问密码
	HK_DEV_RESYSTEM	=	  502,	//互联网重启设备

	//--------local server mrs-------
	HK_MRS_LOCAL_VIDEOSET =			600,	//设置本地服务器录像设置
	HK_MRS_LOCAL_VIDEOREAD =		601,	//读取本地服务器录像设置
	HK_MRS_LOCAL_UPDATEPASWD =		602,	//更新录像服务器密码
	HK_MRS_LOCAL_GETVIDEOINFO =		603,	//获取录像记录，支持分页
	HK_MRS_LOCAL_DELETEVIDEOINFO=	604,	//删除录像记法，支持多条
	HK_MRS_LOCAL_SETSERVERTIME =	605,	//设置时区

	//---------wifi-------------------
	HK_WIFI_START =			700,	//获取取wifi连接点
	HK_WIFI_STOP =			701,	//停止wifi连接
	HK_WIFI_CONNECT =		702,	//获取wifi连接状态
	HK_WIFI_GET_WIFIINFO =	703,	//

	//-------web info-----------------
	HK_WEB_WIFI_INFO =		704,	//获取与设置WIFI信息
	HK_WEB_WIFI_SSID_INFO = 705,	//搜索SSID信息
	HK_WEB_SD_INFO		=	706,	//获取与设置SD卡录像
	HK_WEB_VIDEO_INFO	=	707,	//获取与设置视频参数
	HK_WEB_NET_INFO		=   708,	//获取与设置有线网络参数

	//------------phone--------------
	HK_PHONE_GET_DEV_PARAM= 800,	//获取
	HK_PHONE_SET_DEV_PARAM= 801,	//设置
	HK_GET_PHONE_DEVPARAM = 802,	//获取各参数
	HK_SET_PHONE_DEVPARAM = 803,	//设置各参数
	HK_PHONE_RESTORE =		804,	//恢复各参数restore
	HK_GET_PHONE_SDPARAM =  805,	//获取SD参数
	HK_SET_PHONE_SDPARAM =  806,	//设置SD参数
	HK_GET_PUSH_INFO	=	807,	//获取推送信息
	HK_REGISTRATION_PUSH=	808,	//注册推送
	HK_GET_PUSH_LIST_INFO=  809,	//获取推送记录
	HK_DELETE_PUSH_LIST_INFO=810,	//删除推送记录

	HK_PHONE_SET_QTIO =      820,	//门铃系统信息推送
	HK_PHONE_SET_UNLOCK_TIME=821,	//设置延时锁门时间
	HK_PENETRATE_DATA =		 831,	//数据直传

	//--------re----------------------
	HK_MRS_RE_LOCAL_VIDEO_REC= 900,	//从录像服务器获取数据返回
};

#ifdef __cplusplus
typedef struct _SOURCE_INFO
{
	
	std::string    sessionId;	//资源sid
	int   type;					//类型
	std::string    devId;		//设备名
	std::string    nodeId;		//节点名


	_SOURCE_INFO(void)
	{
		sessionId ="";
		type =0;
		devId = "";
		nodeId = "";

	}

	_SOURCE_INFO & operator = ( const _SOURCE_INFO & rSInfo )
	{
		if( this == &rSInfo )
		{
			return *this;

		}
		this->sessionId = rSInfo.sessionId;
		this->type = rSInfo.type;
		this->devId = rSInfo.devId;
		this->nodeId = rSInfo.nodeId;
		return *this;

	}

	bool operator == ( const _SOURCE_INFO & rSInfo )const
	{
		if(rSInfo.sessionId == sessionId)
		{
			return true;
		}
		return false;

	}



}SOURCE_INFO;

typedef struct _SID_INFO
{
	//std::string    sid;           
	std::string    fromid;	     //发起者
	int    uiprama;              //发起者带的UI参数
	std::string    devid;       //接受者（设备ID）
	std::string    callid ;      //会话ID
	time_t     beginTime;      //开始时间


	_SID_INFO(void)
	{
		//sid = "";
		fromid ="";
		devid = "";
		callid = "";
		uiprama = 0;
		beginTime = 0;

	}

	_SID_INFO & operator = ( const _SID_INFO & rSInfo )
	{
		if( this == &rSInfo )
		{
			return *this;

		}
		this->uiprama = rSInfo.uiprama;
		this->fromid = rSInfo.fromid;
		this->devid = rSInfo.devid;
		this->callid = rSInfo.callid;
		this->beginTime = rSInfo.beginTime;
		return *this;

	}


}SID_INFO;

typedef struct _SUB_TYPE
{
	int subOnlineFlag;   //是否订阅上线的消息
	int subOffLineFlag;  //是否订阅下线的消息
	int subAlarmFlag;    //是否定义报警消息
	int devStatus;       //设备状态

	_SUB_TYPE(void)
	{
		subOnlineFlag =0;
		subOffLineFlag = 0;
		subAlarmFlag = 0;
		devStatus = 0;

	}

	_SUB_TYPE & operator = ( _SUB_TYPE & rStype )
	{
		if( this == &rStype )
		{
			return *this;

		}
		this->subOnlineFlag = rStype.subOnlineFlag;
		this->subOffLineFlag = rStype.subOffLineFlag;
		this->subAlarmFlag = rStype.subAlarmFlag;
		this->devStatus = rStype.devStatus;
		return  *this;

	}
}SUB_TYPE;
#endif

#define HK_KEY_FROM_TEMP	"idtemp"
#define HK_KEY_CHANNEL		"channel"
#define HK_KEY_PT			"ControlPT"		//云台控制参数
#define HK_KEY_FOCUS		"Focus"			//焦聚
#define HK_KEY_VOIDEID		"VoideId"		//录像id
#define HK_KEY_TREEPID      "parendid"		//获得用户组织结构的时,用户的parentid
#define HK_KEY_TREEID       "formid"		//文件夹ID
#define HK_KEY_TREENAME     "formname"		//文件夹名
#define HK_KEY_TREETYPE     "TreeType"		//节点类型,(文件夹,直接节点,空)
#define HK_KEY_DEVID        "devid"			//设备ID(MD5值)
#define HK_KEY_DEVNAME      "DEVNAME"		//设备名称
#define HK_KEY_TEMPVALUE	"TempValue"
#define HK_KEY_CALLINGSDP   "INGSDP"		
#define HK_KEY_CALLEDSDP    "EDSDP"
#define HK_KEY_NODETYPE     "NodeType"
#define HK_KEY_SWITCH       "SWITCH"		//设备状态记录数(int)表示一个设备下有多少个可用通道.
#define HK_KEY_SENSITIVITY	"Sensitivity"	//敏感度值
#define HK_KEY_SOLOMONSID   "DEVSID"		//设备的SID

#define HK_KEY_SIDVOID		 "SIDVOID"		//录像SID
#define HK_KEY_MEDIATYPE     "MEDIATYPE"	//媒体类型
#define HK_KEY_VIDEONAME     "VideoName"	//录像文件名称

#define HK_KEY_ALIAS         "alias"        //别名
#define HK_KEY_ADDR			 "addr"			//地址
#define HK_KEY_EMAIL		 "email"		//电子邮箱
#define HK_KEY_PHONE		 "phone"		//电话
#define HK_KEY_REMARK		 "remark"		//备注信息
#define HK_KEY_ADMINID		 "adminid"		//所属人id
#define HK_KEY_FILEFLAG      "fileFlag"     //文件标识   0，文件夹下没有设备 1，文件夹里面有设备 2 文件夹里面有文件夹

//#define HK_KEY_VIDEORES      "videoRes"	//视频资源
//#define HK_KEY_AUDIORES      "audioRes"	//音频资源
//#define HK_KEY_SUBTYPE       "subType"	//订阅类型 0 全订阅； 1 某一用户的订阅 2 某一设备的订阅
#define HK_KEY_RESTYPE       "resType"		//资源类型
#define HK_KEY_ALLARRIBUTE   "allAttribute"	//所有属性
#define HK_KEY_CURRATTRIBUTE "currAttribute"//当前属性

#define HK_KEY_MAC1			 "mac1"
#define HK_KEY_MAC2			 "mac2"
#define HK_KEY_POINTX        "pointX"        //X坐标
#define HK_KEY_POINTY        "pointY"        //Y坐标

#define HK_KEY_FRISTFROMID   "fristFromid"   //首级formid
#define HK_KEY_FRIENDID      "friendid"      //好友ID
#define HK_KEY_FRIENDNAME    "friendName"    //好友名

#define HK_KEY_ACCEPT          "Accept"      //同意或是拒绝 1 同意 ；0 拒绝
#define HK_KEY_VER             "ver"         //版本号
#define HK_KEY_MYVER           "myver"       //版本号

#define HK_KEY_DEVALIAS        "devAlias"		//设备别名
#define HK_KEY_ADMINDID        "admindid"		//所有者
#define HK_KEY_FUCTION         "fuction"		//是否开通外网功能（0：没有开通。1：开通）
#define HK_KEY_AUTHSTATUS      "authstatus"		//认证方式（0：私有。1：问题。2：公开）
#define HK_KEY_PROBLEM         "problem"		//认证方式为问题时此项被激活，由用户输入认证问题。
#define HK_KEY_SOLUTION        "solution"		//问题认证的答案.
#define HK_KEY_POPEDOM         "popedom"		//认证方式所对应的权限.(0：私有，1：允许看，2：允许录像，3：允许操作(指调云台，分辨率等等…)
#define HK_KEY_NEWPASSWD       "newpasswd"		//新密码
#define HK_KEY_FRIENDID        "friendid"		//好友ID
#define HK_KEY_NEWPOPEDOM      "newPopedom"		//新的认证方式
#define HK_KEY_ISALERT_SENSE   "isAlertSens"	//是否启动报警侦探
#define HK_KEY_ALERT_SENSE     "alertSens"		//报警敏感度
#define HK_KEY_ALERT_LONGTIME  "alertLongTime"	//报警持续时间
#define HK_KEY_ALERT_TYPE      "alertType"		//报警方式（抓图，录像，邮件）
#define HK_KEY_ALERT_VIDEOTIME "alertVideoTime" //报警录像时间长度
#define HK_KEY_AHEAD_IMAGETIME "aheadImageTime" //提前抓拍时间
#define HK_KEY_CAPTUREFR       "captureFR"      //抓图频率
#define HK_KEY_ALERT_IMAGETIME "alertImageTime" //报警抓图时间长度
#define HK_KEY_ALERT_EMAIL     "alertEmail"     //报警发送者
#define HK_KEY_REEMAIL			"reEmail"		//邮件接收者
#define HK_KEY_SMTPSERVER		"smtpServer"	//SMTP服务器
#define HK_KEY_SMTPUSER			"smtpUser"		//STMP用户
//#define HK_KEY_ALERT_TIME     "alertTime"     //报警时间
#define HK_KEY_EMAIL_SD_ERROR   "sderror"		//sd卡错误是否发邮件
#define HK_KEY_EMAIL_MOVE_ALARM "sdmove"        //sd卡移动侦测是否发邮件
#define HK_KEY_EMAIL_INFO       "emailinfo"     //邮件内容

#define HK_KEY_FTPSERVER		"ftpserver"		//FTP服务器
#define HK_KEY_FTPUSER			"ftpuser"		//ftp用户
#define HK_KEY_PPPOEID			"pppoeid"

#define HK_KEY_REC_ID          "recID"          //记录编号
#define HK_KEY_ALERT_TIME      "alertTime"      //报警时间
#define HK_KEY_VIDEO_NUMID     "videoRecID"     //报警录像编号
#define HK_KEY_IMAGE_RECPATH   "imageRecPath"   //报警抓拍图像保存文件夹
#define HK_KEY_ALERT_INFO      "alertInfo"      //报警内容
#define HK_KEY_BEGINE_TIME     "beginTime"      //开始时间
#define HK_KEY_END_TIME        "endTime"        //结束时间
#define HK_KEY_VISIT_INFO      "visitInfo"      //访问信息
#define HK_KEY_DEVPARAM        "devParam"       //设置设备相关的参数(分辨率。曝光度。帧率等)
#define HK_KEY_WEEK_DAY        "week_day"       //星期几
#define HK_KEY_ALERTINFO       "alertInfo"
#define HK_KEY_RECINFO         "recInfo"
#define HK_KEY_LOCALORREMOTE   "localOrRemote"  //是查找本地还是远程 1 本地  2，远程
#define HK_KEY_AUDIOALARM		"audioAlarm"	//声音报警
#define HK_KEY_THERAMLALARM		"ThermalAlarm"	//热感应报警

#define HK_KEY_REC_NAME        "recName"        //文件名称
#define HK_KEY_REC_FILE        "recFile"        //录像文件位置
#define HK_KEY_USERID          "userid"         //
#define HK_KEY_DAYINFO         "dayInfo"
#define HK_KEY_CODETYPE        "codeType"       //压缩类型（0：MPEG-4 1:  MJPEG  2:  H263    3: H264）

#define HK_KEY_COMPRESS			"Compress"		//视频压缩方式
#define HK_KEY_SHARPNESS		"sharpness"		//锐度
#define HK_KEY_HUE				"hue"			//色彩
#define HK_KEY_BRIGHTNESS		"brightness"	//亮度
#define HK_KEY_SATURATION		"saturation"	//饱和度
#define HK_KEY_EXPOSURE			"exposure"		//曝光度
#define HK_KEY_CONTRAST			"contrast"		//对比度
#define HK_KEY_RATE				"rate"			//帧率
#define HK_KEY_IRATE			"irate"			//主帧率
#define HK_KEY_RESOLU			"Reselution"	//分辨率(码流)
#define HK_KEY_FREQUENCY		"frequency"		//频率
#define HK_KEY_AUTOPT			"autopt"		//云台自动转动
#define HK_KEY_QUALITE			"qualite"		//视频质量
#define HK_KEY_BITRATE			"bitrate"		//分辨率
#define HK_KEY_LANPORT			"lanport"
#define HK_KEY_WANENABLE		"wannetable"
#define HK_KEY_LIGHT			"light"			//灯光
#define HK_KEY_ALARMLIGHT		"allight"		//报警灯光
#define HK_KEY_SCENE            "scene"			//场景

#define HK_KEY_MAX_FILESIZE    "maxfilesize"    //录像文件最大限制值
#define HK_KEY_ALERT_SPACE     "alertSpace"     //录像空间警戒值（单位M）
#define HK_KEY_ALERT_DEAL      "alertDeal"      //达到警戒值时的处理方式（1：停止录像 0：覆盖最早的录像文件）
#define HK_KEY_IS_RECAUDIO     "isRecAudio"     //是否录制声音（0：否 1：是）
#define HK_KEY_MRSSYN          "mrsSyn"         //是否已经同步到MRS。0：否，1：同步
#define HK_KEY_PAGESIZE        "pagesize"       //每页有多少条记录
#define HK_KEY_CURRENTPAGE     "currentpage"    //当前页
#define HK_KEY_LISTCOUNT       "listCount"      //总共多少条记录
#define HK_KEY_ISRECFLAG       "isRecFlag"      //是否已经在录像，0没有。1在录像
#define HK_KEY_FILEMAX		   "filemax"		//录像查询返回的文件大小
#define HK_KEY_USER_ID         "userid"         //设备观看者的ID
#define HK_KEY_USER_MRS        "user_mrs"       //用户是否有录像服务器 0 没有， 1 有
#define HK_KEY_TOTALTIME	   "total_time"		//录像文件总时长

#define HK_KEY_VOIPID		"voipid"		//VOIP群ID
#define HK_KEY_VOIPNUM		"voipnum"		//群人员呼叫号码
#define HK_KEY_MRSNAME		"mrsname"		//录像服务器名称
#define HK_KEY_MRSMYSERVER	"mrsmyserver"	//我的录像服务器
#define HK_KEY_MRSSERVER	"mrserver"		//别人给我分配的录像服务器
#define HK_KEY_IOCOUNT		"iocount"		//第几个io口

#define HK_KEY_IOHIGHORLOW	"highorlow"		//设置io口高低电 输出
#define HK_KEY_IOIN			"ioin"			//io输入
#define HK_KEY_CBRORVBR		"cbrorvbr"		//CBR or VBR
#define HK_KEY_SIGNAL		"signal"		//信号灯打开关闭
#define HK_KEY_IS433		"is433"			//控制433开关
#define HK_KEY_IRCUTBOARD   "ircutboard"	//灯板控制

//----------onvif---------------
#define HK_KEY_OPENONVIF	"openonvif"		//开关设备Onvif

//--------wifi---------------------
#define HK_WIFI_OPENORCLOSE	"isopen"		//是否开启WIFI，开启1，关闭0
#define HK_WIFI_SID			"wifisid"
#define HK_WIFI_CHANNEL		"wifichn"
#define HK_WIFI_SAFETYPE	"safetype"     //none,wep或wpa
#define HK_WIFI_SAFEOPTION	"safeoption"   // open或者share 只在wep时有效
#define HK_WIFI_KEYTYPE		"keytype"      // ascii或者hex
#define HK_WIFI_ARRKEY		"arrkey"
#define HK_WIFI_ARRKEYBIS	"arrkeybits"
#define HK_WIFI_ENC			"wifienc"
#define HK_WIFI_AUTH		"wifiauth"
#define HK_WIFI_PASSWORD	"wifipassword"
#define HK_WIFI_DEVICE		"wifidev"
#define HK_WIFI_ENCRYTYPE   "encrytype"		//key
#define HK_WIFI_COMMENT		"COMMENT"

#define HK_WIFI_ENCAES      "AES"
#define HK_WIFI_ENCTIP      "TKIP"
#define HK_WIFI_VALUES_NONE	"none"
#define HK_WIFI_VALUES_WEP	"wep"

#define HK_PUSH_TOKE	"toke"				//手机key
#define HK_KEY_ALARM	"alarm"
#define HK_KEY_OFFLINE	"offline"
//none,wep或wpa
//open或者share 只在wep时有效
//ascii或者hex

#define HK_WIFI_VALUES_AUTO		"auto"

#define HK_WIFI_VALUES_WPA		"wpa"
#define HK_WIFI_VALUES_OPEN		"open"
#define HK_WIFI_VALUES_SHARE	"share"
#define HK_WIFI_VALUES_WPA2		"wpa2"

#define HK_WIFI_VALUES_ASCII	"ascii"	
#define HK_WIFI_VALUES_HEX		"hex"

#define HK_DEV_TYPE  502		//代表设备
#define G_PHONE_TYPE 1			//代表手机
#define REG_SYSM_USER 2			//用户注册
#define REG_SET_EMAIL_USERPWD 5	//通邮件找回用户密码
