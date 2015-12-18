#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SMT_GET_INFO "iwpriv ra0 elian result"

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
int main()
{
	FILE *fp;
	printf("the status is %d\n", Check_WPACLI_Status(1));
	char info[4][100] = {0};
	char ssid[100] = {0};
	char pass[100] = {0};
	char castom[100] = {0};
	char buffer[1000] = {0};

	get_smt_info(info);
	sscanf(info[0], "ssid=%s", ssid);
	sscanf(info[1], "pwd=%s", pass);
	sscanf(info[2], "cust_data=%s", castom);
	printf("ssid is %s\n", ssid);
	printf("pwd is %s\n", pass);
	printf("cust_data is %s\n", castom);
#if 0
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
}\n", "huawei","biaobiao521");
	printf("the buffer is %s\n", buffer);
	fp = fopen("/root/wpa.conf","wr");
	fwrite(buffer, strlen(buffer), 1, fp);
	fclose(fp);
#endif
}
