/*
 * libghttp_get.c
 *  http get test
 *  Created on: 2013年10月25日
 *      Author: elesos.com
 */

#include <stdio.h>
#include <string.h>
#include "ghttp.h"
#include "protocol_josn.h"
#include "log.h"


int http_comm_request(char *URL, char *str_request, char *ret_buffer)
{
	char *uri = URL;
	ghttp_request *request = NULL;
	ghttp_status status;
	char *buf = NULL;
	int bytes_read = 0;
	int size = 0;


//	pFile = fopen ( "elesos.html" , "wb" );
	//新建一个http请求
	request = ghttp_request_new();

	//设置请求的url
	if(ghttp_set_uri(request, uri) == -1)
		return -1;
	
	ghttp_set_header(request, http_hdr_Connection, "close");


	//设置请求的类型
	if(ghttp_set_type(request, ghttp_type_post) == -1)//get
		return -1;

	status = ghttp_set_body(request, str_request, strlen(str_request));
	if(status == ghttp_error)
	{
		printf("ghttp_set_body error\n");	
		return -1;
	}

	//准备请求
	ghttp_prepare(request);

	//处理请求
	status = ghttp_process(request);
	if(status == ghttp_error)
		return -1;

	//得到请求的返回码
	printf("Status code -> %d\n", ghttp_status_code(request));

	//得到请求的体
	buf = ghttp_get_body(request);
	printf("*********************%s\n", buf);

	//得到请求体的长度
	bytes_read = ghttp_get_body_len(request);

	//返回return buffer
	memcpy(ret_buffer, buf, bytes_read);

	size = strlen(buf);//size == bytes_read
//	fwrite (buf , 1 ,size , pFile );
//	fclose(pFile);
//	ghttp_close(request);
	ghttp_request_destroy(request);
	return 0;
}
#if 0
int get_key()
{
	char buffer[200] = {0};
	int errorCode;
	int obj;
	http_request(GET_KEY_URI, GetP2PKey_Str, buffer);
	printf("%s\n", buffer);
	printf("%s\n",Rsp_GetP2PKey_Str);
	sscanf(buffer, Rsp_GetP2PKey_Str, &obj, &errorCode);
	if(errorCode == 0)
	{
		printf("get key is good\n");
		printf("obj is %d\n", obj);
		printf("errorCode is %d\n", errorCode);
	}
	return obj;
}
int bind_device()
#endif

int http_request(request_st *RequeSt)
{
	char buffer[500] = {0};
	int errorCode = 0;
	int obj = 0;
	printf("RequeSt uri is %s\n", RequeSt->uri);
	if (http_comm_request(RequeSt->uri, RequeSt->requethttp, buffer) != 0)
	{
		printf("http_comm_request is error\n");
		LOGERROR("catfile","http_request faild!\n");
		return -1;
	}
	printf("the return buffer is %s \n", buffer);
	sscanf(buffer, RequeSt->rsqrequeshttp, &obj, &errorCode);
	if(errorCode == 0)
	{
		printf("get key is good\n");
		printf("obj is %d\n", obj);
		printf("errorCode is %d\n", errorCode);
	}
	return obj;
}
#if 0
int main()
{
	char GetP2PKey_Str_tmp[200];
	sprintf(GetP2PKey_Str_tmp, GetP2PKey_Str, "1001");
	printf("%s\n", GetP2PKey_Str_tmp);

	request_st stGetKey = {GET_KEY_URI, GetP2PKey_Str_tmp, Rsp_GetP2PKey_Str};

	request_st stCreatDev = {CREATE_DEV_URI, Creat_Str, Rsp_Creat_Str};

	request_st stBind = {BIND_DEV_URI, BindDevice_Str, Rsp_BindDevice_Str};

	http_request(&stGetKey);
}
#endif
