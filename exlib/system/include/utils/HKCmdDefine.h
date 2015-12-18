#pragma  once

// ver 10003
#define HK_VER 0x01

#define  HK_MAXBH  30
#define  HK_WARNBH 15
#define  HK_BTBH     29

enum _HK_AS_CODE
{
	HK_SRV_RES		= 100,
	HK_SRV_ROUTER	= 101,
	HK_SYS_REG      = 333,
	HK_SYS_REG_OK   = 334,
	HK_SYS_REG_FAIL = 335,
	HK_AS_IM        = 336,
	HK_AS_MONS      = 337,
	HK_AS_MRS		= 338,
	HK_AS_MCU       = 400,
	HK_AS_AAA		= 999,	
	HK_AS_EVENT     = 555,       
	HK_AS_PROXY	    = 777,
	HK_AS_SWITCH_1	= 1000,
};

enum _HK_OPERATION_TYPE
{
	HK_AS_VALIDATE = 10,		//AS验证（存不存在这种服务）
	HK_VALIDATE_OK = 11,
	HK_VALIDATA_FAIL = 12,

	HK_AS_STATUS = 13 ,			//业务服务器启动还是退出
	HK_AS_UNREGISTER = 14,		//sys退出
	HK_AS_SUBSCRIBE = 15,		//订阅
	HK_AS_UNSUBSCRIBE = 16,		//退订
	HK_AS_NOTIFY = 17,			//通知

    HK_RS_EVENT = 18,
	HK_VPIPE_NOTIFY_OK = 19,
	HK_VPIPE_NOTIFY_FAIL = 20,
	HK_RS_SET = 21,
	HK_MCU_STATUS = 22,			//MCU状态的修改
    HK_AS_REAUTH = 23,			//服务器重新认证

    HK_RS_NEW_VISITOR = 24,
	HK_AAA_UPDATEPASSWD = 25,	//修改密码
	HK_AAA_UPDATEPC	=	26,		//手动升级
	HK_AAA_GUSTMON = 27,		//游客信息处理
	HK_AAA_MONEYCARD = 28,		//充值.
	HK_AAA_GETPOSTYPE = 29,		//获取类型.
	HK_AAA_ACCOUNTS = 30,		//客户端申请帐号(登录成功情况)
	HK_AAA_ACTIVATION=31,		//激活帐号(登录成功情况)

};

enum _HK_SUBTYPE
{
	HK_SUBTYPE_ALL =0,    //订阅所有的
	HK_SUBTYPE_EP,        //订阅某一用户
	HK_SUBTYPE_DEV,       //订阅设备
	HK_SUBTYPE_SID,       //订阅SID
};

enum ToolBarStateWan
{
	All_False = 1,
	Except_7_T,
	Except_1_F,
	Except_2_F,
	Except_6_F,
	Except_1_T,
};

//登陆失败的原因
enum _HK_UNREGISTER
{
	HK_LOGIN_SUCCESS,
	HK_PASSWD_FAIL = 1,
	HK_USER_RELOGIN= 2,
	HK_NET_OUT = 3,
	HK_UPDATE_PASWD_SUCCESS=4,	//修改密码成功
	HK_UPDATE_PASWD_FAIL=5,		//修改密码失败
    HK_CONNECT_SUCCESS=6,
	HK_UPDATE_PC=7,				//手动升级
	HK_USER_EXPIRE=8,			//此帐号已经过期.
	HK_USER_USE=9,				//已经存在用户
	HK_USER_NOACTIVATION=10,	//帐号没有激活..
	HK_MONEY_SUCCESS = 11,		//充值成功
	HK_MONEY_FAIL = 12,			//充值失败
	HK_USER_NO_PHONE = 13,		//用户未开通手机访问
    HK_SIGN_TIME_OUT,
};

enum _HK_VOIP_TYPE
{
	HK_VOIP_RECALL=200,		//来电
	HK_VOIP_RESUCCESS=201,	//接续成功
	HK_VOIP_RESPONSION=202,	//远端应答
	HK_VOIP_CLOSECALL=203,	//远端挂断
	HK_VOIP_OFFHOOK=204,	//摘机
	HK_VOIP_KEYSTOKE=205,	//按键
	HK_VOIP_CALLKEY=206,	//呼叫键
	HK_VOIP_BYECALL=207,	//挂机
	HK_VOIP_OVERTIME=208,	//超时
	HK_VOIP_DTMF=209,		//dtmf
	HK_VOIP_CALLBUSY=210,	//忙
};

enum _HK_CLIENT_TYPE
{
	HK_CLIENT_ALL = 999,	//所有客户都发
	HK_CLIENT_SYSM = 1000,	//1.2合凯
	HK_CLIENT_NEWSYSM=1001,	//1.3合凯
	HK_CLIENT_TOP = 1010,	//顶好佳
	HK_CLIENT_YDW = 1020,	//宇达为
	HK_CLIENT_ASD = 1030,	//艾赛德
	HK_CLIENT_DX = 1040,	//电信
	HK_CLIENT_LIXUN = 1050,	//力讯
	HK_CLIENT_SHENZHOU=1060,//神舟
	HK_CLIENT_EBAIJN = 1070,//E百佳
	HK_CLIENT_SUNCOMM = 1080, //SunComm
	HK_CLIENT_AT = 1090,
	HK_CLIENT_3GWU = 1091,    // 广州3G 吴
	HK_CLIENT_SINGCAM = 1092,    //新加坡 SINGCAM
	HK_CLIENT_WI = 1093,    //新加坡 SINGCAM
	HK_CLIENT_EASYN = 1094, // 普顺达
	HK_CLIENT_HBA = 1095, // 华百安
	HK_CLIENT_HST = 1096, // 互视通
	HK_CLIENT_LY  = 1097,//联亚
	HK_CLIENT_SAVIT = 1098,//SAVIT 韩国
	HK_CLIENT_WISYS = 1099,// 智慧谷
	HK_CLIENT_TISG = 1100  // 世宁
};

#define HK_KEY_FROM         "id"		//设备名/设备ID,用户名/用户ID.
#define HK_KEY_FROMNOTE     "fromNote"  //发送方登陆名
#define HK_KEY_PASSWORD     "Psw"		//用户密码,设备密码
#define HK_KEY_OLDPASSWD    "OldPsw"    //旧密码
#define HK_KEY_TO           "To"		//代表设备ID
#define HK_KEY_TONOTE       "toNote"    //接收方的登陆名
#define HK_KEY_ENCODE       "encode"    //内码
#define HK_KEY_ERRORCOD     "errorcod"
#define HK_KEY_PORT         "Prot"		//端口
#define HK_NET_BOOTPROTO	"bootproto" //netconfig boot protocol
#define HK_NET_BOOTP_DHCP	"dhcp"		//dhcp configuration
#define HK_NET_BOOTP_STATIC	"static"	//static configuration
#define HK_NET_BOOTP_PPPOE  "PPPoE"     //Huqing 09-08 PPPoE configuration
#define HK_NET_DNS0	        "dns0"	    //dns
#define HK_NET_DNS1	        "dns1"	    //dns
#define HK_KEY_IP			"Ip"		//ip
#define HK_KEY_NETWET		"netwet"	//网关
#define HK_KEY_NETMASK		"netmask"	//子网掩码
#define HK_KEY_MACIP        "MacIP"     //Mac IP 物理地址
#define HK_KEY_NEWMACADDR   "NewMac"     //新MAC 地址
#define HK_KEY_DEVFLAG      "DevFlag"
#define HK_KEY_CHECKSEQ     "CheckSEQ"
#define HK_KEY_LOSTSEQ      "LostSEQ"
#define HK_KEY_VALUEN		"ValueN"	//多条记录的时候的key，N是一个增值
#define HK_KEY_ASCODE       "AsCode"
#define HK_KEY_ASPASSWORD	"AsPassword"
#define HK_KEY_ASNAME       "AsName"	//服务器名称
#define HK_KEY_ASTYPE       "AsType"	//服务器；类型
#define HK_KEY_COUNT		"Count"		//个数
#define HK_KEY_STATUS       "Status"	//状态
#define HK_KEY_STATE        "status"	//状态
#define HK_KEY_CALLID       "Callid"
#define HK_KEY_MONEYCARD	"moneycard"	//充值卡号
#define HK_KEY_IPCDAY		"ipcday"	//ipc帐号天数
#define HK_KEY_NETINFO		"netinfo"
#define HK_KEY_PORXYIP		"porxyip"
#define HK_KEY_WIFISTATE	"wifistatus"
#define HK_KEY_INDEX		"index"

#define HK_KEY_PCHOST		"pchost"
#define HK_KEY_PHONEHOST	"phonehost"
#define HK_KEY_PICHOST		"pichost"

//2009/03/25 Vincent add
#define HK_KEY_CONNECT_TYPE			"Connect_Type"
#define HK_KEY_CONNECT_UDP_IP		"Connect_Udp_Ip"
#define HK_KEY_CONNECT_UDP_PORT		"Connect_Udp_Port"
#define HK_KEY_CONNECT_TCP_IP		"Connect_TCP_Ip"
#define HK_KEY_CONNECT_TCP_PORT		"Connect_TCP_Port"
#define HK_KEY_CONNECT_CURRENT		"Connect_Current"

#define HK_KEY_PROXY_TYPE			"Proxy_Type"
#define HK_KEY_PROXY_HTTP_USER		"Proxy_HTTP_User"
#define HK_KEY_PROXY_HTTP_PWD		"Proxy_HTTP_Pwd"
#define HK_KEY_PROXY_HTTP_IP		"Proxy_HTTP_Ip"
#define HK_KEY_PROXY_HTTP_PORT		"Proxy_HTTP_Port"
#define HK_KEY_PROXY_SOCKETS4_USER	"Proxy_SOCKETS4_User"
#define HK_KEY_PROXY_SOCKETS4_PWD	"Proxy_SOCKETS4_Pwd"
#define HK_KEY_PROXY_SOCKETS4_IP	"Proxy_SOCKETS4_Ip"
#define HK_KEY_PROXY_SOCKETS4_PORT	"Proxy_SOCKETS4_Port"
#define HK_KEY_PROXY_SOCKETS5_USER	"Proxy_SOCKETS5_User"
#define HK_KEY_PROXY_SOCKETS5_PWD	"Proxy_SOCKETS5_Pwd"
#define HK_KEY_PROXY_SOCKETS5_IP	"Proxy_SOCKETS5_Ip"
#define HK_KEY_PROXY_SOCKETS5_PORT	"Proxy_SOCKETS5_Port"
#define HK_KEY_PROXY_CURRENT		"Proxy_Current"
////////////////////////////////////////

#define HK_KEY_OPERATOR		  "operator"		//会话关系//---rs----
#define HK_KEY_READ			  "read"			//读
#define HK_kEY_WRITE		  "write"			//写
#define HK_KEY_FROMSTR        "from"
#define HK_KEY_TOSTR          "to"
#define HK_KEY_TOUSER         "user"
#define HK_KEY_SIDNUM         "sidNum"
#define HK_KEY_SIDN           "sidN" 
#define HK_KEY_FROMSID        "fromsid"
#define HK_KEY_TOSID          "tosid"
#define HK_KEY_RESTYPEN       "resTypeN"
#define HK_KEY_DEVIDN         "devidN"
#define HK_KEY_NODEN          "nodeN"
#define HK_KEY_RESINFO        "resInfo"            
#define HK_KEY_RESOUCE        "resouce"
#define HK_RES_EXCLUSIVELY    "exclusively"     //0，共享 ，1，非共享
#define HK_KEY_VIDEOINFO	  "videoinfo"

#define HK_KEY_CALLRESN			"callResN"
#define HK_KEY_READADNWRITE		"readAndwrite"		//可读可写
#define HK_KEY_OPN				"opN"				//
#define HK_KEY_FTN				"ftN"				//
#define HK_KEY_CALLN			"callN"
#define HK_KEY_INPUT			"input"
#define HK_KEY_ECHO				"echo"

#define HK_KEY_OUTPUT			"output"
#define HK_KEY_OUTECHO			"outecho"
#define HK_KEY_LANGUAGE			"language"		   //语言
#define HK_KEY_RUNFAIL			"RUN_FAIL"
#define HK_KEY_SE_OP			"SessionAct"       // 会话操作
#define HK_VAL_SE_CREATE		"SessionCreate"    // 创建会话
#define HK_VAL_SE_DELETE		"SessionDelete"    // 删除会话
#define HK_KEY_INVITE			"invite:"          //邀请方
#define HK_KEY_ACCPETOR			"Acceptor:"        //接受方
//#define HK_KEY_RESOUCE		"Resouce:"         //资源
#define HK_KEY_NAME				"name"             //
#define HK_KEY_FLAG				"Flag"			   //成功失败标记 1 成功 0 失败
#define HK_KEY_UIPARAM			"lParam"		   //UI标识,不变量,服务器应答的时候要带回来
#define HK_KEY_MAINCMD			"MainCmd"
#define HK_KEY_SUBCMD			"SubCmd"

#define HK_KEY_VERTYPE			"vertype"			//升级版本类型.
#define HK_KEY_VERNUM			"verNum"			//版本号
#define HK_KEY_USERTYPE			"userType"			//用户类型
#define HK_KEY_URL				"url"				//
#define HK_KEY_SUBNUM			"subNum"			//订阅数目      //
#define HK_KEY_SUBRESOURCE		"subResource"		//订阅某一资源
#define HK_KEY_SUBTYPE			"subType"			//订阅类型 0 全订阅； 1 某一用户的订阅 2 某一设备的订阅
#define HK_KEY_SUBVALUE			"subValue"			//订阅类型为1时，表示订阅用户名，为2时表示设备名
#define HK_KEY_EVENT			"event"				//订阅事件
#define HK_EVENT_ONLINE			"eventOnline"		//上线的订阅
#define HK_EVENT_OFFLINE		"eventOffline"		//下线的订阅
#define HK_EVENT_ANY			"eventAny"
#define HK_KEY_ENFTP            "enftp"
#define HK_KEY_ENFIE            "enie"


#define HK_DEV_TYPE_IPC   2		//IPC帐号
#define HK_DEV_TYPE_SIPC  3		//一路SIPC帐号
#define HK_DEV_TYPE_4SIPC 9		//四路SIPC帐号
#define HK_DEV_TYPE_8SIPC 11	//八路SIPC帐号
//#define HK_DEV_TYPE_2SIPC 13	//二路SIPC帐号
#define HK_DEV_TYPE_DVR4  4		//四路DVR帐号
#define HK_DEV_TYPE_MJPEG 5		//M_JPEG帐号
#define  HK_DEV_TYPE_H264 7		//h264
#define HK_MRS_TYPE		  21	//录像服务器帐号

#define HK_DEV_TYPE_DVRN	"4"
#define HK_DEV_TYPE_SIPCN	"3"

#define HK_VAL_H3G21			 "321"   // 3G 设备

#define HK_VAL_HKIPCAMEEX		"322"	 //富泓智源标清H264
#define HK_VAL_HKIPCAMEFHGM		"323"	 //富泓智源高清H264

#define HK_VAL_HKIPCAMEHXBQ     "331"    // 黄晓智源标清H264
#define HK_VAL_HKIPCAMEHXGQ     "332"    // 黄晓智源高清H264

#define HK_VAL_HKIPCAMETPSBQ    "351"    // 托普生海思标清H264
#define HK_VAL_HKIPCAMETPSGQ    "352"    // 托普生海思高清H264

#define HK_VAL_HKIPCAMEOJTBQ    "381"    // OJ标清T H264
#define HK_VAL_HKIPCAMEOJTGQ    "382"    // OJT高清 H264

#define HK_VAL_HKIPCAMEMJ       "311"    // MJARM9方案

#define HK_VAL_HKIPCAME5350MJ       "312"    // MJARM9方案
#define HK_VAL_HKIPCAME5350264       "313"    // MJARM9方案


#define HK_VAL_MRSNAME			"hkmrs"			//录像服务器
#define HK_VAL_HKIPCAME			"hkipcame"		//设备

#define HK_VAL_HKDVR			"hkdvr"			//DVR
#define HK_VAL_HKDVRC			"hkdevc"		//DVRC
#define HK_VAL_HKSIPC			"hksipc"
#define HK_VAL_HKSIPCC			"hksipcc"		//SIPCC
#define HK_VAL_H264				"H264"
#define HK_VAL_MPEG4			"MPEG4"
#define HK_VAL_M_JPEG			"M_JPEG"

#define HK_EVENT_RESONLINE		"eventResOnline"	//VOIP上线的订阅
#define HK_EVENT_RESOFFLINE		"eventOffOnline"	//VOIP下线的订阅
#define HK_EVNET_INVITESTATUS	"eventInviteStatus"	//RS inviteACK的返回
#define HK_EVENT_INBAND_DATA_DESC "eventInbandDesc"  
#define HK_EVENT_FOCUS			"eventFocus"
#define HK_EVENT_ALARM			"alarm"				//报警
#define HK_EVENT_INFO			"eventInfo"			//订阅事件的相关信息
#define HK_KEY_HKIDSIZE			"HKIDSIZE"
#define HK_EVENT_PENETRATE_DATA	"PenetrateData"		//透传数据

#define HK_KEY_VPIPE			"HKVPipe"
#define HK_KEY_POS				"Pos"


//////--------------------------new add by ali
#define HK_KEY_EPID           "EPID"
#define HK_KEY_ICMD           "ICMD"
#define HK_KEY_HKID           "HKID"
#define HK_KEY_DSTHKID		  "DSTHKID"
#define HK_KEY_PIPEID         "PIPEID"
#define HK_KEY_NEXTHKID		  "NEXTID"
#define HK_KEY_EPNETINFO	  "EPNETINFO"
#define HK_KEY_HPOTO          "HPOTO"
#define HK_KEY_LPOTO          "LPOTO"
#define HK_KEY_LOCALINFO	  "LOCALINFO"
#define HK_KEY_NATINFO		  "NATINFO"
#define HK_KEY_SEQ			  "SEQ"
#define	HK_KEY_GUARDSEQ       "GUARDSEQ"
#define HK_KEY_TIME			  "TIME"	
#define HK_KEY_LOCALCNT		  "LOCALCNT"
#define HK_KEY_NATCNT		  "NATCNT"
#define HK_KEY_CONTYPE		  "CONTYPE"
#define HK_KEY_NPLAN		  "NPLAN"
#define HK_KEY_MSGTYPE		  "MSGTYPE"
//#define SERVERPROXY 1
//#define SERVERMCU   2
//#define SERVERMRS   4
#define MRS_SERVER    1		//服务器录像
#define DEV_SDCARD    2		//SD卡抓图
#define VIDEO_EMAIL   4		//发送邮件
#define ALERT_PHONE	  8		//短信通知
#define ALERT_PLAN	  16	//报警计划
#define HK_KEY_SZIE			 "HKSIZE"
#define HK_KEY_PROI			 "PROI"
#define HK_KEY_LEVEL         "LEVEL"

#define HK_KEY_EMAILCNT        "EPICNT"

#define HK_KEY_LOCALIP        "LOCALIP"
#define HK_KEY_LOCALPORT      "LOCALPORT"
#define HK_KEY_NATIP          "NATIP"
#define HK_KEY_NATPORT        "NATPORT"
#define HK_KEY_MAINID		  "MAINID"
#define  HK_KEY_IPTYPE        "IPTYPE"
#define HK_KEY_ROUNTID        "ROUNTID"
#define HK_KEY_OLDROUNTID     "OLDROUNTID"
#define HK_KEY_DATAID         "DATAID"
#define HK_KEY_CONLAN		  "ConLan"
#define HK_KEY_SERVERIP		  "ServerIP"		//MCU的IP
#define HK_KEY_SERVERPORT     "ServerPort"		//MCU的PORT

#define  HK_KEY_UDPSIZE       "UDPSize"
#define  HK_KEY_UDPPORT       "UDPPort"
#define  HK_KEY_TCPSIZE       "TCPSize"
#define  HK_KEY_TCPPORT       "TCPPort"


#define	HK_KEY_GETWAY		  "GetWay"

#define	HK_KEY_WIFIDATAENC		  "WifiDataEnc"   // AES TKIP
#define	HK_KEY_WIFISAFETYPE		  "WifiSafeType"  // WEP,WAP,WAP2

#define HK_KEY_SDENABLESD        "EnableSD"
#define HK_KEY_SDMOVEREC         "MoveRec"		//移动报警录像
#define HK_KEY_SDOUTMOVEREC      "OutMoveRec"	//报警输出录像
#define HK_KEY_SDAUTOREC         "AutoRec"		//自动录像
#define HK_EKY_SDLOOPREC         "LoopWrite"	//循环写
#define HK_EKY_SDSPILTE          "Splite"		//文件分割大小
#define HK_EKY_SDALLSIZE         "AllSize"		//SD总大小
#define HK_KEY_SDHAVEUSE         "HaveUse"		//已经使用大小
#define HK_KEY_SDLEFTUSE         "LeftSize"		//空闲大小
#define HK_KEY_SDAUDIOMUX        "sdAudioMux"
#define HK_KEY_SDRCQC            "SDRecQC"

#define HK_KEY_VIDEOTFINFO	  "tfinfo"



