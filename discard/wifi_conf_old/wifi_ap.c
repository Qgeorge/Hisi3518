/*
 * =====================================================================================
 *
 *       Filename:  wifi_ap.c
 *
 *    Description:  设置AP模式
 *
 *        Version:  1.0
 *        Created:  09/26/2015 04:26:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


void set_ap_mode(void)
{
    printf("......start wifi AP mode......\n"); 
#if 1
    int ret = 0;
    ret = system("killall -q udhcpc");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    ret = system("killall -q udhcpd");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    //ret = system("ifconfig ra0 down");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    //ret = system("killall -q wpa_supplicant");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    ret = system("ifconfig ra0 down");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    ret = system("rmmod mt7601Usta");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    ret = system("insmod /tmpfs/komod/mt7601Uap.ko");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    //system("ifconfig eth0 0.0.0.0"); //switch off eth0 connection.
	#if 0
    if (0 == g_DevWireless)
    {
        ret = system("ifconfig eth0 0.0.0.0"); //switch off eth0 connection.
        //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 
    }
	#endif
    //signal(SIGCHLD, SIG_DFL);
    ret = system("ifconfig ra0 192.168.100.2 broadcast 192.168.100.255 netmask 255.255.255.0 up");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    ret = system("udhcpd /mnt/sif/udhcpd.conf &");
    ret = system("route add -net 224.0.0.0 netmask 224.0.0.0 ra0");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 
    sleep(1);
#endif
}

void set_sta_mode(void)
{
    printf("......start wifi sta mode......\n");
#if 1
    int ret = 0;
    ret = system("killall -q udhcpc");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    ret = system("killall -q udhcpd");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    //ret = system("killall -q wpa_supplicant");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    ret = system("ifconfig ra0 down");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    ret = system("rmmod mt7601Uap");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    ret = system("/komod/load_wifi_driver");
    //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 

    sleep(1);
    //system("ifconfig eth0 0.0.0.0"); //switch off eth0 connection.
	#if 0
    if (0 == g_DevWireless)
    {
        ret = system("ifconfig eth0 0.0.0.0"); //switch off eth0 connection.
        //printf("[%s, %d] system ret:%d, errno:%d, %s\n", __func__, __LINE__, ret, errno, strerror(errno)); 
    }
	#endif
    //signal(SIGCHLD, SIG_DFL);

#endif
}
