#pragma once

enum _HK_AAA_OPERATION_CODE
{
	HK_SYS_UNREGISTER=33, //注销
	HK_SYS_DISCONNECT,    //由于网络，SYS退出
	
};

#define HK_KEY_NUMBER		"Number"
//#define HK_KEY_ASCODE     "AsCode"		//服务器内码
//#define HK_KEY_ASTYPE     "AsType"		//服务器；类型
//#define HK_KEY_ASNAME     "AsName"		//服务器名称
//#define HK_KEY_ASPASSWORD "AsPassword"	//服务器密码
#define HK_KEY_INNERCODE	"InnerCode"   
#define HK_KEY_RESULT       "Result"
#define HK_KEY_SERVERIP     "ServerIP"		//MCU的IP
#define HK_KEY_SERVERPORT   "ServerPort"	//MCU的PORT
#define HK_KEY_MCUNUM       "McuNum"
#define HK_KEY_MCUIPN       "McuIpN"
#define HK_KEY_MCUPORTN     "McuPortN"

#define HK_KEY_CAUSE        "cause"			 //登陆失败的原因

#define HK_KEY_MCUPOTON     "McuPotoN"