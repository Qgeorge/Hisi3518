#pragma once


#define HK_KEY_IMSERVER	"HKIMServer"



enum _HK_IM_OPERATION_CODE
{
	
	HK_IM_IMREGISTER = 101 ,    //自己的IM业务的注册信息
	HK_IM_GETBASEINFO = 102,          //获取基本信息
	HK_IM_UPDATEBASEINFO = 103,       //修改基本信息 
	HK_IM_SETUSERSTATUS,        //设置用户状态

	HK_IM_GETGRPOUPLIST,        //获取组列表
	HK_IM_GETFRIENDLIST,        //获取好友列表

	HK_IM_FRIENDNOTIFY,         //通知好友上线  (一个)
	HK_IM_SEARCHFRIEND,         //搜索好友
	HK_IM_ADDFRIEND,            //添加好友
	HK_IM_GROUPINGFRIEND,       //好友分组
	HK_IM_MODIFYFRIENDNAME,     //修改好友备注名称
	HK_IM_DELETEFRIEND,         //删除好友

	HK_IM_MOREFRIENDONLINE ,    //多个好友上线(登录时)
	HK_IM_CREATGROUP,           //新建组名
	HK_IM_MODIFYGROUPNAME,      //修改组名
	HK_IM_DELETEGROUP,          //删除组
	//HK_IM_IMINVITE,           //建立一个连接

	/////////////////
	HK_IM_ADDFRIENDANSER,       //需问题验证的返回
	HK_IM_ADDFRIENDAUTH,        //需对方认证的返回
	HK_IM_ADDFRIENDACCEPT ,     //需对方认证的时候，返回给用户好友同意或是拒绝


	HK_IM_INVITE ,              //创建会话
	HK_IM_INVITERESULT,         //对方同意或是接受
	HK_IM_ACK,
	HK_IM_CANCEL,               //关闭某个资源
	HK_IM_CANCELACK,            //关闭某个资源的返回
	HK_IM_BYE,                  //关闭整个会话
	HK_IM_BYEACK,               //关闭整个会话的返回
	HK_IM_FRIENDBASEINFO,       //获取好友详细信息

	

	HK_IM_UNREGISTER,           //注销

	HK_IM_READCONFIGINFO,		//读取配制信息
	HK_IM_UPDATECONFIGINFO,		//修改and增加配制信息
	HK_IM_TRANSFERSFILE,        //传输文件
	HK_IM_FILERESULT,           //对方接受或是拒绝
	HK_IM_FILEACK,              //传输文件的确认

	HK_IM_SUBSCRIBE,            //订阅
	HK_IM_NOTIFY,               //通知

	HK_IM_FRIENDOFFLINE,        //单个用户下线
	HK_IM_CANCELFILE,           //取消文件传输  （单个）
	HK_IM_BYEFILE,              //关闭文件传输 （所有）
	HK_IM_UPORREADAUTH,         //读取或修改认证方式   
	HK_IM_FILETRANSFINISH,      //文件传输成功
	HK_IM_NOTIFYFRIENDSTATE,    //通知给添加的好友，该用户的状态
	HK_IM_CANCELREQUST,         //取消请求（还没有收到ACK时的取消）
	
};

enum _HK_RES_TYPE
{
	HK_RES_TEXT=1,
	HK_RES_AUDIO,
	HK_RES_VIDEO,
	HK_RES_RESHOW,       //本地回显
	HK_RES_FILE,         //文件传输
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

	_SOURCE_INFO & operator = (const  _SOURCE_INFO & rSInfo )
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
#endif


////////idiograph
#define HK_KEY_REALNAME        "realname"
#define HK_KEY_ALIAS           "alias"          //别名
#define HK_KEY_IDIOGRAPH       "idiograph"      //个性签名
#define HK_KEY_AGE             "age"            //年龄
#define HK_KEY_SEX             "sex"            //性别
#define HK_KEY_REMARK          "remark"         //备注

#define HK_KEY_BIRTHDAY        "birthday"       //生日
#define HK_KEY_COUNTRY         "country"        //国家
#define HK_KEY_PROVINCE        "province"       //省
#define HK_KEY_CITY            "city"           //城市
#define HK_KEY_ADDR            "address"        //地址
#define HK_KEY_ZIPCODE         "zipcode"        //邮编
#define HK_KEY_MOBILE          "mobile"         //手机
#define HK_KEY_PHONE           "pohone"         //电话
#define HK_KEY_EMAIL           "email"          //邮箱
//#define HK_KEY_STATE           "State"        //状态
#define HK_KEY_GROUPID         "formid"         //组ID
#define HK_KEY_GROUPNAME       "formname"       //组名
#define HK_KEY_FRIENDID        "friendid"       //好友ID
#define HK_KEY_TYPE            "Type"           //类型
#define HK_KEY_LOGINCOUNT      "Count"          //登录次数
#define HK_KEY_AUTHSTATUS      "Authstatus"     //认证方式 1已经存在 2 对方认证,3问题认证,4公开,5拒绝
#define HK_KEY_PROBLEM         "Problem"        //问题
#define HK_KEY_SOLUTION        "Solution"       //答案
#define HK_KEY_ACCEPT          "Accept"         //同意或是拒绝 1 同意 ；0 拒绝
#define HK_KEY_OLDGROUPID      "OldGroupid"                 
#define HK_KEY_FROMRES         "FromRes"                
#define HK_KEY_TORES           "ToRes"
#define HK_KEY_FRIENDNUM       "FriendNum"      //好友数

#define HK_KEY_CONFIGINFO		"configinfo"	//配制信息参数

//--------------chat text-----
#define HK_KEY_CHATTEXT			"text"		//聊天内容
#define HK_KEY_CHATTIME			"time"		//聊天时间
#define HK_KEY_CHATTONAME		"toname"	//接收者名称
#define HK_KEY_CHATFROMNAME		"fromname"	//发送者名称
#define HK_KEY_CHATCOUNT		"rowCount"	//当前请求总共有多少条记录
#define HK_KEY_CHATPAGE			"pageCount"	//有多少页

//------------file-----------
#define HK_KEY_FILEFROM		 "FromFilePatch"	//文件路径
#define HK_KEY_FILETO		 "ToFilePatch"		//文件路径
//#define HK_KEY_VIDEORES    "videoRes"			//视频资源
//#define HK_KEY_AUDIORES    "audioRes"			//音频资源
#define HK_KEY_RESTYPE		 "resType"			//资源类型
#define HK_KEY_ALLARRIBUTE   "allAttribute"		//所有属性
#define HK_KEY_CURRATTRIBUTE "currAttribute"	//当前属性

#define HK_KEY_FILESIZE        "fileSize"		//文件大小
#define HK_KEY_RESHOWFSID      "reshowFsid"		//回显FROM Sid
#define HK_KEY_RESHOWTSID      "reshowTsid"		//回显 TO SID




//2009/03/26 Vincent add
//个性化设置
#define HK_KEY_TYPE_ADVANCED		"Type_Advanced"			//类型 高级设置
#define HK_KEY_TYPE_CONNECT			"Type_Connect"			//类型 连接
#define HK_KEY_TYPE_HOTKEY			"Type_Hotkey"			//类型 热键
#define HK_KEY_TYPE_SESSIONSTYLE	"Type_Sessionstyle"		//类型 会话样式
#define HK_KEY_TYPE_SESSIONSETTING	"Type_Sessionsetting"	//类型 会话设置
#define HK_KEY_TYPE_CALL			"Type_Call"				//类型 呼叫
#define HK_KEY_TYPE_RING			"Type_Ring"				//类型 铃声
#define HK_KEY_TYPE_TIP				"Type_Tip"				//类型 提示
#define HK_KEY_TYPE_STOPUSER		"Type_Stopuser"			//类型 阻止用户
#define HK_KEY_TYPE_PRIVACY			"Type_Privacy"			//类型 隐私设置
#define HK_KEY_TYPE_VIDEODEV		"Type_Videodev"			//类型 视频设备
#define HK_KEY_TYPE_SOUNDDEV		"Type_sounddev"			//类型 音频设备
#define HK_KEY_TYPE_ROUTINE			"Type_Routine"			//类型 常规
#define HK_KEY_TYPE_SYSSETTING		"Type_Syssetting"		//类型 系统设置
#define HK_KEY_TYPE_CONTACT			"Type_Contact"			//类型 联系方式
#define HK_KEY_TYPE_BASEINFO		"Type_Baseinfo"			//类型 基本信息
#define HK_KEY_TYPE_FONTATTRITUBE	"Type_FontAttribute"	//类型 字体属性

//高级设置
#define HK_KEY_VALUE_FILEPATH		"Value_Filepath"		//
#define HK_KEY_VALUE_MSGPATH		"Value_Msgpath"			//
#define HK_KEY_VALUE_VIDEOPATH		"Value_Videopath"		//	
#define HK_KEY_VALUE_DOWNTYPE		"Value_Downtype"		//	

//常规
#define HK_KEY_VALUE_HIDEEXIT		"Value_Hideexit"		//
#define HK_KEY_VALUE_STOPMSG		"Value_Stopmsg"			//
#define HK_KEY_VALUE_STOPONLINE		"Value_Stoponline"		//
#define HK_KEY_VALUE_SENDMSG		"Value_Sendmsg"

//会话设置
#define HK_KEY_VALUE_ALLOWWHO		"Value_Allowwho"		//

//热键
#define HK_KEY_VALUE_SENDMSG_H		"Value_Sendmsg_h"
#define HK_KEY_VALUE_SENDMSG_E		"Value_Sendmsg_e"
#define HK_KEY_VALUE_FINDUSER_H		"Value_Finduser_h"
#define HK_KEY_VALUE_FINDUSER_E		"Value_Finduser_e"
#define HK_KEY_VALUE_POPCLOSEWND_H	"Value_Popclosewnd_h"
#define HK_KEY_VALUE_POPCLOSEWND_E	"Value_Popclosewnd_e"
#define HK_KEY_VALUE_REFUSECALL_H	"Value_Refusecall_h"
#define HK_KEY_VALUE_REFUSECALL_E	"Value_Refusecall_e"
#define HK_KEY_VALUE_IGNORECALL_H	"Value_Ignorecall_h"
#define HK_KEY_VALUE_IGNORECALL_E	"Value_Ignorecall_e"
#define HK_KEY_VALUE_RESPONSION_H	"Value_Responsion_h"
#define HK_KEY_VALUE_RESPONSION_E	"Value_Responsion_e"
#define HK_KEY_VALUE_HOTENABLE		"Value_Hotenable"

//提示
#define HK_KEY_VALUE_REMINDONLINE_P			"Value_Remindonline_p"
#define HK_KEY_VALUE_REMINDTEXTSESSION_P	"Value_Remindtextsession_p"
#define HK_KEY_VALUE_REMINDSENDFILE_P		"Value_Remindsendfile_p"
#define HK_KEY_VALUE_REMINDBIRTHDAY_P		"Value_Remindbirthday_p"
#define HK_KEY_VALUE_REMINDCONTACTINFO_P	"Value_Remindcontactinfo_p"
#define HK_KEY_VALUE_REMINDVIDEO_P			"Value_Remindvideo_p"
#define HK_KEY_VALUE_REMINDRING_P			"Value_Remindring_p"
#define HK_KEY_VALUE_REMINDONLINE_E			"Value_Remindonline_e"
#define HK_KEY_VALUE_REMINDTEXTSESSION_E	"Value_Remindtextsession_e"
#define HK_KEY_VALUE_REMINDSENDFILE_E		"Value_Remindsendfile_e"
#define HK_KEY_VALUE_REMINDBIRTHDAY_E		"Value_Remindbirthday_e"
#define HK_KEY_VALUE_REMINDCONTACTINFO_E	"Value_Remindcontactinfo_e"
#define HK_KEY_VALUE_REMINDVIDEO_E			"Value_Remindvideo_e"
#define HK_KEY_VALUE_REMINDRING_E			"Value_Remindring_e"

//会话样式
#define HK_KEY_VALUE_STYLEFACE		"Value_Styleface"			//显示表情
#define HK_KEY_VALUE_STYLEACTFACE	"Value_Styleactface"		//显示动表情
#define HK_KEY_VALUE_STYLETIME		"Value_Styletime"			//在及时消息中显示时间

//呼叫
#define HK_KEY_VALUE_CALLALLOWWHO		"Value_Callallowwho"		//允许何人呼叫
#define HK_KEY_VALUE_CALLAUTORESPONSE	"Value_Callautoresponse"	//自动应答呼叫
#define HK_KEY_VALUE_CALLAUTOVIDEO		"Value_Callautovideo"		//自动打开我的视频
#define HK_KEY_VALUE_CALLAUTOVIDEOED	"Value_Callautovideoed"		//自动接受用户的视频请求

//连接
#define HK_KEY_CONNECT_TYPE		"Connect_Type"
#define HK_KEY_CONNECT_UDP_IP	"Connect_Udp_Ip"
#define HK_KEY_CONNECT_UDP_PORT	"Connect_Udp_Port"
#define HK_KEY_CONNECT_TCP_IP	"Connect_TCP_Ip"
#define HK_KEY_CONNECT_TCP_PORT	"Connect_TCP_Port"
#define HK_KEY_CONNECT_CURRENT	"Connect_Current"

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

//联系信息
#define HK_KEY_CONTACT_COUNTRY		"Value_Contactcountry"		//国家
#define HK_KEY_CONTACT_PROVINCE		"Value_Contactprovince"		//省
#define HK_KEY_CONTACT_CITY			"Value_Contactcity"			//城市
#define HK_KEY_CONTACT_POST			"Value_Contactpost"			//邮编
#define HK_KEY_CONTACT_ADDR			"Value_Contactaddr"			//地址
#define HK_KEY_CONTACT_TEL			"Value_Contacttel"			//电话
#define HK_KEY_CONTACT_MOBILE		"Value_Contactmobile"		//手机
#define HK_KEY_CONTACT_EMAIL		"Value_Contactemail"		//邮箱

//字体属性
#define HK_KEY_VALUE_FONTSIZE		"Value_Fontsize"
#define HK_KEY_VALUE_FONTMASK		"Value_Fontmask"
#define HK_KEY_VALUE_FONTEFFECTS	"Value_Fonteffects"
#define HK_KEY_VALUE_FONTHEIGHT		"Value_Fontheight"
#define HK_KEY_VALUE_FONTOFFSET		"Value_Fontoffset"
#define HK_KEY_VALUE_FONTCOLOR		"Value_Fontcolor"
#define HK_KEY_VALUE_FONTCHARSET	"Value_Fontcharset"
#define HK_KEY_VALUE_FONTPF			"Value_Fontpf"
#define HK_KEY_VALUE_FONTFACENAME	"Value_Fontfacename"

//隐私
#define HK_KEY_PRIVACY_TIMELONG		"Value_Privacytimelong"			//会话信息保存时间
#define HK_KEY_PRIVACY_SHOWMYSTATE	"Value_Privacyshowmystate"		//是否显示我的状态

//系统图标的名称
#define HK_SYSIMAGE_ERROR		"sys_image_error"		//没有接收到图片时显示的图片的名字
#define HK_SYSIMAGE_WAIT		"sys_image_wait"		//正在接收图片时显示的等待图片的名字
