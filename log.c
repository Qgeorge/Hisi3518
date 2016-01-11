
#include "log.h"

const char *format = "[%10s][%4d][%10s()]: ";

log4c_category_t* catfile   = NULL ;
log4c_category_t* catstdout = NULL ;

int log_init()
{
	if (log4c_init() == 1)
	{
		return -1;
	}

	catfile = log4c_category_get(FILE_CATEGORY_NAME);
	catstdout = log4c_category_get(STDOUT_CATEGORY_NAME);
	return 0 ;
}

void log_message(log4c_category_t *mycat, char* file, int line, int priority, const char* func,const char* a_format, ...) 
{
	char *file_info;
	char *new_format;
	size_t info_len;
	size_t new_format_len;
	va_list va;
	
	info_len = strlen(format) + 50;
	file_info = (char *) malloc(info_len);
	sprintf(file_info, format,file, line,func );
	
	new_format_len = strlen(file_info) + strlen(a_format) + 2;
	new_format = (char *) malloc(new_format_len);
	sprintf(new_format, "%s%s", file_info, a_format);
	
	va_start(va, a_format);
	log4c_category_vlog(mycat, priority, new_format, va);
	va_end(va);
	
	free(file_info);
	free(new_format);
}

int log_fini()
{
	return(log4c_fini());
}
