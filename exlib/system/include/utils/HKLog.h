#pragma  once

#define L_ERR    0
#define L_WARN   1
#define L_INFO   2
#define L_DBG    3


	/* log facility (see syslog(3)) */
#define L_FAC  LOG_DAEMON
	/* priority at which we log */
#define DPRINT_PRIO LOG_DEBUG

#define LOG_NAME "HKLog"
#define AFX_EXT_CLASS 

#ifdef __cplusplus
extern "C"
{
#endif

AFX_EXT_CLASS void LOG_Init( const char *name );
AFX_EXT_CLASS void LOG_UNInit();

AFX_EXT_CLASS void LOG_SetLevel( unsigned short nLogLevel );
AFX_EXT_CLASS unsigned short LOG_GetLevel();
AFX_EXT_CLASS int  LOG_IsStderr();
AFX_EXT_CLASS void LOG_Foreground();
AFX_EXT_CLASS void LOG_Background();
AFX_EXT_CLASS void LOG_Print( int level,const char *fn,const char *fu,int nLine,const char* fmt,... );

#ifndef WIN32	

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

AFX_EXT_CLASS void dprint (int level, const char* fct, char* file, int line, char* fmt, ...);

AFX_EXT_CLASS void vlog( int level, const char * fmt,... );

AFX_EXT_CLASS  void LOG_InitBroadCast( int level );

AFX_EXT_CLASS  void LOG_UnInitBroadCast();
#ifdef _DEBUG
#define LOG_PRINT(level, args... ) dprint( level, __FUNCTION__, __FILE__, __LINE__, ##args )
#else
#define LOG_PRINT(level,fmt, ... ) log_print( level ,fmt, __VA_ARGS__ )
#endif


#define HKLOG(level, fmt, args...)  LOG_Print( level,__FILE__, __FUNCTION__, __LINE__,fmt, ##args );
////if((level)<=LOG_GetLevel()) {\
//	if(LOG_IsStderr())\
//	  printf( fmt, ##args );\
//	  else {\
//	  switch(level){\
//		  case L_ERR:\
//		  syslog(LOG_ERR | L_FAC, "Error: (%s)(%s)(%i): ", __FILE__, __FUNCTION__, __LINE__);\
//		  syslog(LOG_ERR | L_FAC,fmt, ##args);\
//		  break;\
//		  case L_WARN:\
//		  syslog(LOG_WARNING | L_FAC, "Warning: (%s)(%s)(%i): ", __FILE__, __FUNCTION__, __LINE__);\
//		  syslog(LOG_WARNING | L_FAC,fmt, ##args);\
//		  break;\
//		  case L_INFO:\
//		  syslog(LOG_INFO | L_FAC, "Info: (%s)(%s)(%i):", __FILE__, __FUNCTION__, __LINE__);\
//		  syslog(LOG_INFO | L_FAC,fmt, ##args);\
//		  break;\
//		  case L_DBG:\
//		  syslog(LOG_DEBUG | L_FAC, "Debug: (%s)(%s)(%i):", __FILE__, __FUNCTION__, __LINE__);\
//		  syslog(LOG_DEBUG | L_FAC,fmt, ##args);\
//		  break;\
//	  }\
//	  }\
//}
#else
//#define HKLOG LOG_Print
#define HKLOG( level, fmt, ... ) LOG_Print( level,__FILE__, __FUNCTION__, __LINE__,fmt, __VA_ARGS__);
#endif

#ifdef __cplusplus
}
#endif
