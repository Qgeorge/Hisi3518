/*
 * =====================================================================================
 *
 *       Filename:  net_http.c
 *
 *    Description:  管理服务器通信
 *
 *        Version:  1.0
 *        Created:  09/03/2015 05:35:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "protocol_josn.h"

#define DEVICE_TYPE 1
#define SUB_TYPE 1
#define DEVICE_VERSION "V1.0"
#define DEVICE_VERSION_NUM "00220150908"
#define DEVICE_PRODUCE_NUM "1111334455"

extern int get_device_id(char *DeviceId);
extern int http_request(request_st *RequeSt);

/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_bind_device
 *  Description:	用户设备绑定  
 * =====================================================================================
 */
int net_bind_device (char *UserId)
{
	char DeviceId[50];
	get_device_id(DeviceId);
	char BindDevice_Str_tmp[200];
	sprintf(BindDevice_Str_tmp, BindDevice_Str, DeviceId, UserId);
	printf("%s\n", BindDevice_Str_tmp);

	request_st stBind = {BIND_DEV_URI, BindDevice_Str_tmp, Rsp_BindDevice_Str};
	http_request(&stBind);
	return 0;
}
/* -----  end of function net_bind_device  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_create_device
 *  Description:   设备添加注册
 * =====================================================================================
 */
int net_create_device (char *DeviceId)
{
	char Create_str_tmp[500]={0};
	//sprintf(Create_str_tmp, GetP2PKey_Str, DeviceId);
	sprintf(Create_str_tmp, Creat_Str,DeviceId, DEVICE_TYPE, SUB_TYPE, DEVICE_VERSION, DEVICE_VERSION_NUM, DEVICE_PRODUCE_NUM);
	printf("%s\n", Create_str_tmp);
	request_st stCreatDev = {CREATE_DEV_URI, Create_str_tmp, Rsp_Creat_Str};
	http_request(&stCreatDev);
	printf("net_create_device is finaly\n");
	return 0;
}
/* -----  end of function net_create_device  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_get_key
 *  Description:  获取P2P 密钥
 * =====================================================================================
 */
int net_get_key (char *DeviceId, int *key)
{
	char GetP2PKey_Str_tmp[200] = {0};
	sprintf(GetP2PKey_Str_tmp, GetP2PKey_Str, DeviceId);
	printf("%s\n", GetP2PKey_Str_tmp);
	request_st stGetKey = {GET_KEY_URI, GetP2PKey_Str_tmp, Rsp_GetP2PKey_Str};
	*key = http_request(&stGetKey);
	return 0;
}
/* -----  end of function net_get_key  ----- */
