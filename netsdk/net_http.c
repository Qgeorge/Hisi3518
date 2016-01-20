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
#include "cJSON.h"
#include "protocol_josn.h"
#include "zlog.h"
#define DEVICE_TYPE 2
#define HOME_DIR "/mnt/sif"

extern int get_device_id(char *DeviceId);
extern int http_request(request_st *RequeSt);

/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_bind_device
 *  Description:	用户设备绑定  
 * =====================================================================================
 */
int net_bind_device (char *UserId, char *DeviceId)
{
	get_device_id(DeviceId);
	char BindDevice_Str_tmp[200];
	sprintf(BindDevice_Str_tmp, BindDevice_Str, DeviceId, UserId);
	printf("%s\n", BindDevice_Str_tmp);

	request_st stBind = {BIND_DEV_URI, BindDevice_Str_tmp, Rsp_BindDevice_Str};
	http_request(&stBind);
	return 0;
}
/* -----  end of function net_bind_device  ----- */
void int2str(int n, char *str)
{
	sprintf(str, "v%d.%d.%d",n/100, (n-(n/100*100))/10, n%10);
	printf("the v is %s\n", str);
}
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_create_device
 *  Description:   设备添加注册
 * =====================================================================================
 */
int net_create_device (char *DeviceId)
{
	char Create_str_tmp[500]={0};
	int device_version_num;
	int  device_produce_num;
	char device_version[12] = {0};
	int devicetype;
	int sub_type;

	get_device_id(DeviceId);
	device_version_num = conf_get_int(HOME_DIR"/hkipc.conf", "DEVICE_VERSION");
	device_produce_num = conf_get_int("etc/devtype.conf", "DEVICE_PRODUCE_NUM");
	devicetype = conf_get_int("etc/devtype.conf", "DEVICE_TYPE");
	sub_type = conf_get_int(HOME_DIR"/hkipc.conf", "SUB_TYPE");
	int2str(device_version_num, device_version);
	//sprintf(Create_str_tmp, GetP2PKey_Str, DeviceId);
	sprintf(Create_str_tmp, Creat_Str,DeviceId, devicetype, sub_type, device_version, device_version_num, device_produce_num);
	printf("%s\n", Create_str_tmp);
	request_st stCreatDev = {CREATE_DEV_URI, Create_str_tmp, Rsp_Creat_Str};
	http_request(&stCreatDev);
	printf("net_create_device is finaly\n");
	return 0;
}
/* -----  end of function net_create_device  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_create_device
 *  Description:   设备修改
 * =====================================================================================
 */
int net_modify_device (char *DeviceId)
{
	char Modify_str_tmp[500]={0};
	int device_version_num;
	int  device_produce_num;
	char device_version[12] = {0};
	int sub_type;

	get_device_id(DeviceId);
	device_version_num = conf_get_int(HOME_DIR"/hkipc.conf", "DEVICE_VERSION");
	device_produce_num = conf_get_int(HOME_DIR"/hkipc.conf", "DEVICE_PRODUCE_NUM");
	sub_type = conf_get_int(HOME_DIR"/hkipc.conf", "SUB_TYPE");
	int2str(device_version_num, device_version);
	//sprintf(Create_str_tmp, GetP2PKey_Str, DeviceId);
	sprintf(Modify_str_tmp, Modify_Str,DeviceId, DEVICE_TYPE, sub_type, device_version, device_version_num, device_produce_num);
	printf("%s\n", Modify_str_tmp);
	request_st stCreatDev = {MODIFY_DEV_URI, Modify_str_tmp, Rsp_Modify_Str};
	http_request(&stCreatDev);
	printf("net_create_device is finaly\n");
	return 0;
}
/* -----  end of function net_create_device  ----- */
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

#if 0
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_get_update_uri
 *  Description:  
 * =====================================================================================
 */
int net_get_update_uri ()
{
	char Upgrade_Firware_Str_tmp[300] = {0};
	strcpy(Upgrade_Firware_Str_tmp, Upgrade_Firware_Str);
	request_st stGetKey = {GET_KEY_URI, Upgrade_Firware_Str_tmp, Rsp_GetP2PKey_Str};
	http_request(&stGetKey);
	return 0;
}
/* -----  end of function net_get_update_uri  ----- */
#endif
extern int g_Video_Thread;
extern int g_SubVideo_Thread;
extern int g_Audio_Thread;
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_get_upgrade
 *  Description:请求升级
 * =====================================================================================
 */
int net_get_upgrade()
{
	char buf[500] = {0};
	char cmd[200] = {0};
	cJSON *root, *obj;
	int errorCode;
	char *url;
	int ret;

	g_Video_Thread = 0;
	g_SubVideo_Thread = 0;
	g_Audio_Thread = 0;

	ret = http_comm_request(UPGRADE_FIRWARE_URI,Upgrade_Firware_Str, buf);
	if(ret < 0)
	{
		printf("http_comm_request is error\n");
		return -1;
	}
	root = cJSON_Parse(buf);
	if(!root)
	{
		printf("Error before [%s]\n", cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
		errorCode = cJSON_GetObjectItem(root,"errorCode")->valueint;
		printf("errorCode is %d\n", errorCode);
		obj = cJSON_GetObjectItem(root, "obj");
		url = cJSON_GetObjectItem(obj,"apk_url")->valuestring;
		//down the imge ....
		system("cd /opt/");
		sprintf(cmd, "/usr/bin/wget %s%s -O %s", DOWNLOAD_FIRWARE_URI, url, UPGRADE_IMAGE);
		printf("cmd is ****%s\n", cmd);
		system(cmd);
		system("sync");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd, "/bin/tar xvf %s%s -C %s", UPGRADE_IMAGE_DIR, UPGRADE_IMAGE, "/tmpfs/");
		system(cmd);
		printf("the url is %s\n", url);
		exit(0);
	}
	return 0;
}
/* -----  end of function net_get_upgrade  ----- */

#if 0
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  parse_json_grade
 *  Description:  
 * =====================================================================================
 */
int parse_json_grade (char *txt, struct *reply_st)
{
	cJSON *root, *obj;
	root = cJSON_Parse(txt);
	if(!root)
	{
		printf("Error before [%s]\n", cJSON_GetErrorPtr());
	}
	else
	{
		reply_st->errorCode = cJSON_GetObjectItem(root,"errorCode")->valueint;
		obj = cJSON_GetObjectItem(root, "obj");
		reply_st->uri = cJSON_GetObjectItem(root,"apk_url")->valuestring;
	}
	return;
}
/* -----  end of function parse_json_grade  ----- */
#endif
