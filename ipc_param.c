#include "ipc_param.h"

HK_SD_MSG_T hk_net_msg;

void init_param_conf()
{
	int ret;
	/*add for test----->sd to connect route*/
	memset(&hk_net_msg,0,sizeof(hk_net_msg));
	
	conf_get("/etc/configure","productid",hk_net_msg.productid,20);	
	conf_get("/etc/configure","manufacturerid",hk_net_msg.manufacturerid,20);	
	conf_get("/etc/configure","gateway",hk_net_msg.gw,20);	
	conf_get("/etc/configure","ip",hk_net_msg.ip,20);	
	ret = conf_get_int("/etc/configure","istestmode");
	if(ret == 1)
	{
		hk_net_msg.isTestMode = true;
	}else if(ret == 0){
		hk_net_msg.isTestMode = false;
	}
	
	printf("productid:%s\nmanufacturerid:%s\ngateway:%s\nip:%s\n",
				hk_net_msg.productid,hk_net_msg.manufacturerid,hk_net_msg.gw,hk_net_msg.ip);	
}

void get_manufacturer_id(uint8 *str)
{
	memset(str,0,strlen(str));
	strncpy(str,hk_net_msg.manufacturerid,strlen(hk_net_msg.manufacturerid));
}

void get_product_id(uint8 *str)
{
	memset(str,0,strlen(str));
	strncpy(str,hk_net_msg.productid,strlen(hk_net_msg.productid));
}

void get_gateway(uint8 *str)
{
	memset(str,0,strlen(str));
	strncpy(str,hk_net_msg.gw,strlen(hk_net_msg.gw));
}

void get_ipaddr(uint8 *str)
{
	memset(str,0,strlen(str));
	strncpy(str,hk_net_msg.ip,strlen(hk_net_msg.ip));
}

void set_testmode(bool param)
{
	hk_net_msg.isTestMode = param;
}
bool get_isTestMode()
{
	return hk_net_msg.isTestMode;
}

