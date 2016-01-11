#ifndef _LOG_H_
#define _LOG_H_

#include <string.h>
#include <stdlib.h>
#include "log4c.h"
#define FILE_CATEGORY_NAME     "log4ctest"
#define STDOUT_CATEGORY_NAME   "cstdout"

extern log4c_category_t* catfile   ;
extern log4c_category_t* catstdout ;
//1.LOG4C_PRIORITY_ERROR
//2.LOG4C_PRIORITY_WARN
//3.LOG4C_PRIORITY_NOTICE
//4.LOG4C_PRIORITY_DEBUG
//5.LOG4C_PRIORITY_TRACE

extern int log_init();
extern void log_message(log4c_category_t * mycat,char* file, int line, int priority, const char* func,const char* a_format, ...);
extern int log_fini();

#define LOGWARN(cat,fmt,args...) log_message(cat,__FILE__, __LINE__,LOG4C_PRIORITY_WARN, __FUNCTION__,fmt ,## args);
#define LOGERROR(cat,fmt,args...) log_message(cat,__FILE__, __LINE__,LOG4C_PRIORITY_ERROR, __FUNCTION__,fmt ,## args);
#define LOGNOTICE(cat,fmt,args...) log_message(cat,__FILE__, __LINE__,LOG4C_PRIORITY_NOTICE, __FUNCTION__,fmt ,## args);
#define LOGDEBUG(cat,fmt,args...) log_message(cat,__FILE__, __LINE__,LOG4C_PRIORITY_DEBUG, __FUNCTION__,fmt ,## args);
#define LOGTRACE(cat,fmt,args...) log_message(cat,__FILE__, __LINE__,LOG4C_PRIORITY_TRACE, __FUNCTION__,fmt ,## args);

#endif
