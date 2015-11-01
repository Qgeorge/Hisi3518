#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define ADHOC_SSID "uu_smarthome"
/*
 *连接没有密码的wifi
 */
int connect_open_ap(char *ssid)
{
	char cmd_str[500] = {0};
	sprintf(cmd_str, "iwpriv ra0 set NetworkType=Infra;\
		iwpriv ra0 set AuthMode=WPAPSK;\
		iwpriv ra0 set EncrypType=OPEN;\
		iwpriv ra0 set SSID=%s", ssid);
	system(cmd_str);
}
/*
 *连接加密的wifi
 */
int connect_enc_ap(char *encmod, char *ssid, char * password)
{
	char cmd_str[500] = {0};
	sprintf(cmd_str, "iwpriv ra0 set NetworkType=Infra;\
		iwpriv ra0 set AuthMode=WPAPSK;\
		iwpriv ra0 set EncrypType=%s;\
		iwpriv ra0 set SSID=%s;\
		iwpriv ra0 set WPAPSK=%s;\
		iwpriv ra0 set SSID=%s", encmod, ssid, password, ssid);

	printf("str is %s\n", cmd_str);
	system(cmd_str);
	sleep(5);
	//system(cmd_str);
}
/*
 *设置为adhoc模式
 */
int set_adhoc_mode()
{
	char cmd_str[500] = {0};
	sprintf(cmd_str, "iwpriv ra0 set NetworkType=Adhoc;\
		iwpriv ra0 set AuthMode=OPEN;\
		iwpriv ra0 set EncrypType=NULL;\
		iwpriv ra0 set SSID=%s", ADHOC_SSID);
	printf("str is %s\n", cmd_str);
	system(cmd_str);
	sleep(3);

}

/*
 *连接wifi
 */
int connect_ap(int authmode, char *ssid, char * password)
{
	switch(authmode)
	{
		case 0:
		connect_open_ap(ssid);break;
		case 1:
		connect_enc_ap("AES", ssid, password);break;
		case 2:
		connect_enc_ap("AES", ssid, password);break;
		case 3:
		connect_enc_ap("AES", ssid, password);break;
		case 4:
		connect_enc_ap("AES", ssid, password);break;
		case 5:
		connect_enc_ap("AES", ssid, password);break;
		case 6:
		connect_enc_ap("AES", ssid, password);break;
		case 7:
		connect_enc_ap("AES", ssid, password);break;
		case 8:
		connect_enc_ap("AES", ssid, password);break;
		case 9:
		connect_enc_ap("AES", ssid, password);break;
	}
}
