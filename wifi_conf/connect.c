#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "smartconfig.h"
#include "zlog.h"
#include "ipc_param.h"
#define SMT_CONF_START "echo 'start' > /proc/elian"
#define SMT_CONF_STOP "echo 'stop' > /proc/elian"
#define SMT_CONF_CLEAR "echo 'clear' > /proc/elian"
#define SMT_GET_INFO "iwpriv ra0 elian result"
#define WIFI_CONFIG "/etc/wifiConf/wpa_supplicant.conf"

int detect_process(char * process_name)  
{  
	FILE *ptr;  
	char buff[512];  
	char ps[128];  
	sprintf(ps,"ps | grep -c %s",process_name);  
	strcpy(buff,"ABNORMAL");  
	if((ptr=popen(ps, "r")) != NULL)  
	{  
		while (fgets(buff, 512, ptr) != NULL)  
		{  
			if(atoi(buff) == 3)  
			{  
				pclose(ptr);  
				return 0;  
			}
		}  
	}  
	if(strcmp(buff,"ABNORMAL")==0)  /*ps command error*/  
	  return -1;          
	pclose(ptr);  
	return -1;
}
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
int get_smt_info(char (*info)[100]) 
{
	int i = 0;
	char slip[10] = ", ";
	char tmp[1000]; //设置一个合适的长度，以存储每一行输出

	FILE *pp = popen(SMT_GET_INFO, "r"); //建立管道
	if (!pp) 
	{
		printf("popen is error\n");
		return -1;
	}
	printf("get smt info *****************\n");
	fgets(tmp, sizeof(tmp), pp);
	if (tmp[strlen(tmp) - 1] == '\n')
	{
		tmp[strlen(tmp) - 1] = '\0'; //去除换行符
	}
	printf("*************tmp is %s\n", tmp);

	printf("the info 0 is %s\n", strtok(tmp,slip));
	strtok(NULL,slip);
	strcpy(info[0],strtok(NULL,slip));
	strcpy(info[1],strtok(NULL,slip));
	strtok(NULL,slip);
	strcpy(info[2],strtok(NULL,slip));
	strcpy(info[3],strtok(NULL,slip));
	printf("get smt info *****************\n");
	printf("the info is %s\n", info[0]);
	printf("the info is %s\n", info[1]);
	printf("the info is %s\n", info[2]);
	printf("the info is %s\n", info[3]);
	pclose(pp); //关闭管道
	return 0;
}
int save_wifi_info(char (*info)[100])
{
	FILE *fp = NULL;
	char ssid[100] = {0};
	char pass[100] = {0};
	char buffer[1000] = {0};


	sscanf(info[0], "ssid=%s", ssid);
	sscanf(info[1], "pwd=%s", pass);

	printf("ssid is %s\n", ssid);
	printf("pwd is %s\n", pass);
	sleep(1);
	if(strlen(pass) > 1)
	{
		sprintf(buffer, "ctrl_interface=/var/run/wpa_supplicant\nnetwork={\nssid=\"%s\"\n\
					psk=\"%s\"\n\
					scan_ssid=1\n\
					key_mgmt=WPA-EAP WPA-PSK IEEE8021X NONE\n\
					pairwise=TKIP CCMP\n\
					group=CCMP TKIP WEP104 WEP40\n\
					eap=TTLS PEAP TLS\n\
					proto=WPA RSN\n\
					frequency=2414\n\
					scan_freq=2412\n\
					}\n", ssid,pass);

	fp = fopen(WIFI_CONFIG, "w");
	if(fp != NULL)
	{
		fwrite(buffer, strlen(buffer), 1, fp);
		fclose(fp);
	}
	system("sync");
	return 0;
	}
	return 1;
}

int get_wifi_info(char (*info)[100])
{
	FILE *fp = NULL;
	fp = fopen(WIFI_CONFIG, "r");
	if(fp == NULL)
	{
		printf("open the wifi error\n");
		return -1;
	}
	int i;
	for(i = 0; i < 4 ;i++)
	{
		fgets(info[i],sizeof(info[i]),fp);
	}
	fclose(fp);
	return 0;
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
		//		if(j >= 2)
		{
			temp = strtol(str[j], NULL, 16);
			printf("%d\n", temp);
			//con_str[j-2] = (char)temp;
			con_str[j] = (char)temp;
		}
		j++;
	}
	strcpy(usrid, con_str);
	printf("%s\n", con_str);
}
/*判断wifi的连接状态*/
int Check_WPACLI_Status(int interval)
{
	int ConnectCnt = 0; //count for checking wifi connection status.
	char cmdbuf[32] = {0};   //popen result buffer.
	char wpaState_Key[16] = {0}; //wifi connection status: Key.
	char wpaState_Value[32] = {0}; //wifi connection status: Value.
	FILE *pfp = NULL;

	printf(".........check time interval: %d.........\n", interval);
	for (ConnectCnt = 0; ConnectCnt < interval; ConnectCnt++)
	{
		printf(".........connect count: %d.........\n", ConnectCnt);
		pfp = popen("wpa_cli status", "r");
		if (NULL == pfp)
		{

			fprintf(stderr, "popen failed with: %d, %s\n", errno, strerror(errno));
			return -1;
		}
		/*parse status result to find if remote wifi is usable*/
		fgets(cmdbuf, sizeof(cmdbuf), pfp); //first line, invalid.
		while (fgets(cmdbuf, sizeof(cmdbuf), pfp))
		{
			cmdbuf[strlen(cmdbuf) - 1] = '\0'; //skip '\n' at the end of line.
			memset(wpaState_Key, '\0', strlen(wpaState_Key));
			memset(wpaState_Value, '\0', strlen(wpaState_Value));
			sscanf(cmdbuf, "%[^=]=%[^'\n']", wpaState_Key, wpaState_Value);
			//fprintf(stderr, "===> cmdbuf:%s, wpaState_Key:%s, wpaState_Value:%s\n", cmdbuf, wpaState_Key, wpaState_Value);

			if ( (!strcmp(wpaState_Key, "wpa_state")) && (!strcmp(wpaState_Value, "COMPLETED")) )
			{
				return 1; 
			}
		}
		if (pfp)  
		{
			pclose(pfp);
			pfp = NULL;
		}
		//sleep(3);
	}
	return 0;
}
/*连接ap热点*/
int connect_the_ap()
{
	//get_smt_info(smt_info);
	int flag = 1;
	int i = 0;
	system("/usr/bin/pkill wpa_supplicant");
	system("/usr/bin/pkill udhcpc");
	system("ifconfig ra0 down");
	system("ifconfig ra0 up");
	sleep(1);
	system("wpa_supplicant -Dwext -ira0 -c/etc/wifiConf/wpa_supplicant.conf &");
	//sleep(5);
	while(flag)
	{
		if(Check_WPACLI_Status(1) == 1)
		{
			flag = 0;
		}
		sleep(1);
		i++;
		if(i == 30)
		{
			break;
			return -1;
		}
	}
	//if(Check_WPACLI_Status(1) == 1)
	{
		//if(detect_process("udhcpc") != 0)
		{
			system("/sbin/udhcpc -b -i ra0 -s /mnt/sif/udhcpc.script");
			printf("udhcpc already runing\n");
			sleep(3);
		}
#if 0
		if(test_network("s1.uuioe.net") == 0)
		{
			printf("connect success\n");
			return 0;
		}
		else
		{
			printf("connect failed\n");
			return -1;
		}
#endif
		return 0;
	}
}

/*连接ap热点*/
int connect_smt_ap()
{
	//get_smt_info(smt_info);
	int flag = 1;
	int i = 0;
	static int wpa_flag = 0;

	//	system("wpa_supplicant -Dwext -ira0 -c/etc/wifiConf/wpa_supplicant.conf &");
	system("wpa_supplicant -Dwext -ira0 -c/etc/wifiConf/wpa_supplicant.conf &");

	//sleep(5);
	while(flag)
	{
		if(Check_WPACLI_Status(1) == 1)
		{
			flag = 0;
			system("/usr/bin/pkill udhcpc");
		}
		sleep(1);
		i++;
		if(i == 60)
		{
			ZLOG_INFO(zc,"connect faild\n");
			system("/usr/bin/pkill wpa_supplicant");
			system("/usr/bin/pkill udhcpc");
			return -1;
		}
	}

	//if(Check_WPACLI_Status(1) == 1)
	{
		//if(detect_process("udhcpc") != 0)
		{
			//不使用udhcpc看能不能ping通s1.uuioe.net,或者使用了udhcpc后在静态修改id行不行
			system("/sbin/udhcpc -b -i ra0 -s /mnt/sif/udhcpc.script");
			printf("udhcpc already runing\n");
			sleep(3);
		}
		if(test_network("s1.uuioe.net") == 0)
		{
			ZLOG_INFO(zc,"connect success\n");
			return 0;
		}
		else
		{
			ZLOG_INFO(zc,"connect faild\n");
			system("/usr/bin/pkill wpa_supplicant");
			system("/usr/bin/pkill udhcpc");
			return -1;
		}
	}

}

int connect_ap_for_test(){

	uint8 tmp[100]={0}, i=0, flag=1;
	uint8 ipaddr[20]={0}, gateway[20]={0};
	static uint32 wpa_flag = 0;
	
	get_ipaddr(ipaddr);
	get_gateway(gateway);

	system("/usr/bin/pkill wpa_supplicant");
	system("/usr/bin/pkill udhcpc");
	system("/usr/bin/pkill udhcpd");
	system("ifconfig ra0 down");
	system("rmmod mt7601Usta.ko");
	system("rmmod mt7601Uap.ko");
	sleep(1);

	system("insmod /opt/wifi_driver/mt7601Usta.ko");
	sleep(1);
	
	system("wpa_supplicant -Dwext -ira0 -c/etc/wifiConf/wpa_supplicant.conf &");

	//sleep(5);
	while(flag)
	{
		if(Check_WPACLI_Status(1) == 1)
		{
			flag = 0;
			system("/usr/bin/pkill udhcpc");
		}
		sleep(1);
		i++;
		if(i == 60)
		{
			ZLOG_INFO(zc,"connect faild\n");
			system("/usr/bin/pkill wpa_supplicant");
			system("/usr/bin/pkill udhcpc");
			return -1;
		}
	}

	sprintf(tmp,"ifconfig ra0 %s\n",ipaddr);
	system(tmp);

	memset(tmp,0,strlen(tmp));
	sprintf(tmp,"route add default gw %s\n",gateway);
	system(tmp);
	system("route add -net 224.0.0.0 netmask 224.0.0.0 ra0");
	set_testmode(false);
	
}
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  smart_config
 *  Description:  
 * =====================================================================================
 */
int smart_config(char *userid)
{
	char smt_info[4][100] = {0};
	char ssid[100] = {0};
	char password[100] = {0};
	char cust_data_len[100] = {0};
	char cust_data[100] = {0};
	char authmode[50] = {0};
	char tlv_hex[50] ={0};
	int len;
	char auth = '1';
	router_login_info_p router_msg = NULL;

	system("/usr/bin/pkill wpa_supplicant");
	system("/usr/bin/pkill udhcpc");
	system("ifconfig ra0 down");
	system("ifconfig ra0 up");
	sleep(1);

	smartlink_start(&router_msg);
	//strcpy(smt_info[0],router_msg->usrname);
	//strcpy(smt_info[1],router_msg->passwd);
	//strcpy(smt_info[2],&auth);

	memset(userid,0,strlen(userid));
	sprintf(smt_info[0],"ssid=%s",router_msg->usrname);
	sprintf(smt_info[1],"pwd=%s",router_msg->passwd);
	strcpy(userid,router_msg->userid);

	printf("smt_info[0]:%s\n",smt_info[0]);
	printf("smt_info[1]:%s\n",smt_info[1]);

	sleep(1);

	save_wifi_info(smt_info);

	return 0;
#if 0
	start_smart_conf();
	while(1)
	{
		get_smt_info(smt_info);
		printf("smt_info 0 is %s\n", smt_info[0]);
		printf("smt_info 1 is %s\n", smt_info[1]);
		printf("smt_info 2 is %s\n", smt_info[2]);
		printf("smt_info 3 is %s\n", smt_info[3]);

		sscanf(smt_info[0], "ssid=%s", ssid);
		sscanf(smt_info[1], "pwd=%s", password);
		sscanf(smt_info[2], "cust_data_len=%s", cust_data_len);
		sscanf(smt_info[3], "cust_data=%s", cust_data);

		printf("ssid is %s\n", ssid);
		printf("password is %s\n", password);
		if(strlen(password) > 1)
		{
			stop_smart_conf();
			len = atoi(cust_data_len);
			cust_data[len] = 0;
			//获取用户ID
			strcpy(userid,cust_data);
			//保存配置
			if(save_wifi_info(smt_info) == 0)
			{
				//有密码
				return 0;
			}else
			{
				//无密码
				return 1;
			}
		}
		sleep(1);
	}
#endif

}
