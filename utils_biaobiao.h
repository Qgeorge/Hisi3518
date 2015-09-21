/*
*/
#ifndef __UTILS_BIAOBIAO_H__
#define __UTILS_BIAOBIAO_H__

void* conf_set_int(const char* cf, const char* nm, int val);
int conf_get_int(const char* cf, const char* nm);

void conf_set(const char* cf, const char* nm, const char *val);
char* conf_get(const char* cf, const char* nm, char val[], size_t len);

void conf_set_space(const char* cf, const char* nm, const char *val);

char * EncodeStr( char *codeStr);
char * DecodeStr( char *codeStr);

#endif
