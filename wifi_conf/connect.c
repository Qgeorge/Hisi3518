#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define SMT_CONF_START "echo 'start' > /proc/smart_connection"
#define SMT_CONF_STOP "echo 'stop' > /proc/smart_connection"
#define SMT_CONF_CLEAR "echo 'clear' > /proc/smart_connection"
#define SMT_GET_INFO "cat /proc/smart_connection  | awk \'{if ($1 == \"SSID\"){print \"SSID=\"$3;}else if($1 == \"PASSWORD\"){print \"PASSWORD=\"$3}else if($1 == \"AUTHMODE\"){print \"AUTHMODE=\"$3}else if($1 == \"TLV_HEX\"){print \"TLV_HEX=\"$3}}\'"
#define WIFI_CONFIG "/mnt/sif/wifi_info"


int start_smart_conf()
{
	system(SMT_CONF_START);
	return 0;
}

int stop_smart_conf()
{
	system(SMT_CONF_STOP);
	return 0;
}

int clear_smart_conf()
{
	system(SMT_CONF_CLEAR);
	return 0;
}
int get_smt_info(char (*info)[50]) 
{
	int i = 0;
	printf("get smt info *****************\n");
	FILE *pp = popen(SMT_GET_INFO, "r"); //建立管道
	if (!pp) 
	{
		printf("popen is error\n");
		return -1;
	}
	printf("get smt info *****************\n");
	char tmp[100]; //设置一个合适的长度，以存储每一行输出
	while (fgets(tmp, sizeof(tmp), pp) != NULL)
	{
		if (tmp[strlen(tmp) - 1] == '\n')
		{
			tmp[strlen(tmp) - 1] = '\0'; //去除换行符
		}
		printf("*************tmp is %s\n", tmp);
		strcpy(info[i],tmp);
		i++;
	}
	pclose(pp); //关闭管道
	return 0;
}
int save_wifi_info(char (*info)[50])
{
	FILE *fp = NULL;
	fp = fopen(WIFI_CONFIG, 'w');
	int i = 0;
	for(i = 0; i < 4; i++)
	{
		fputs(info[i], fp);
	}
	fclose(fp);
}
int get_wifi_info(char (*info)[50])
{
	FILE *fp = NULL;
	fp = fopen(WIFI_CONFIG, 'r');
	int i;
	for(i = 0; i < 4 ;i++)
	{
		fgets(info[i],sizeof(info[i]),fp);
	}
	fclose(fp);	
}
int tlv_hex_str(char *buffer, char *usrid)
{
	//char buffer[100] = "01-0F-54-4C-56-20-31-20-43-4F-4E-54-45-4E-54";
	char str[100][3] ={0};
	char con_str[100] = {0};
	int i = 0, j = 0;
	int temp;	
	for(i = 0; i <= strlen(buffer); i = i + 3)
	{
		strncpy(str[j], buffer+i, 2);
		if(j >= 2)
		{
			temp = strtol(str[j], NULL, 16);
			printf("%d\n", temp);
			con_str[j-2] = (char)temp;
		}
		j++;
	}
	strcpy(usrid, con_str);
	printf("%s\n", con_str);
}
int connect_the_ap()
{
	char smt_info[4][50] = {0};
	char ssid[50] = {0};
	char password[50] = {0};
	char authmode[50] = {0};
	char tlv_hex[50] ={0};
	int auth;
	get_smt_info(smt_info);
	sscanf(smt_info[0], "SSID=%s", ssid);
	sscanf(smt_info[1], "PASSWORD=%s", password);
	sscanf(smt_info[2], "AUTHMODE=%s", authmode);
	sscanf(smt_info[3], "TLV_HEX=%s", tlv_hex);
	auth = (int)authmode[0] - 48;		
	connect_ap(auth, ssid, password);

	if(test_network("www.baidu.com") == 0)
	{
		printf("connect success\n");
		return 0;
	}
	else
	{
		printf("connect failed");
		return -1;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  smart_config
 *  Description:  
 * =====================================================================================
 */
int smart_config(char *userid)
{
	char smt_info[4][50] = {0};
	char ssid[50] = {0};
	char password[50] = {0};
	char authmode[50] = {0};
	char tlv_hex[50] ={0};
	int auth;
	int num;
	start_smart_conf();
	while(1)
	{
		get_smt_info(smt_info);
		printf("smt_info 1 is %s\n", smt_info[0]);
		printf("smt_info 2 is %s\n", smt_info[1]);
		printf("smt_info 3 is %s\n", smt_info[2]);
		sscanf(smt_info[0], "SSID=%s", ssid);
		sscanf(smt_info[1], "PASSWORD=%s", password);
		sscanf(smt_info[2], "AUTHMODE=%s", authmode);
		sscanf(smt_info[3], "TLV_HEX=%s", tlv_hex);
		printf("ssid is %s\n", ssid);
		printf("password is %s\n", password);
		auth = (int)authmode[0] - 48;		
		if(strlen(ssid) > 1)
		{
			//connect_ap();
			printf("********ssid is %s\n", ssid);
			printf("********password is %s\n", password);
			tlv_hex_str(tlv_hex, userid);
			memset(tlv_hex, 0, sizeof(tlv_hex));
			strcpy(tlv_hex, userid);
			//connect_ap(auth, ssid, password);
			save_wifi_info(smt_info);
#if 0
			if(test_network("www.baidu.com") == 0)
			{
				save_wifi_info(smt_info);
				printf("*******************good good\n");
				return 0;
			}
			else
			{
				stop_smart_conf();
				clear_smart_conf();
				start_smart_conf();
			}
#endif
		}
		sleep(1);
	}
}
