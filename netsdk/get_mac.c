#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>
#include <stdlib.h>
#include "ipc_sd.h"
#include "zlog.h"
#include "ipc_param.h"
int get_device_id(char *DeviceId)
{
	char manufacturerid[20] = {0}, productid[20] = {0};
	struct ifreq ifreq;
	int sock;

	get_manufacturer_id(manufacturerid);
	get_product_id(productid);

	if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		return -1;
	}
	strcpy(ifreq.ifr_name,"ra0");
	if(ioctl(sock,SIOCGIFHWADDR,&ifreq)<0)
	{
		perror("ioctl");
		return -1;
	}
#if 0
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
				(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);

	sprintf(DeviceId, "%02x:%02x:%02x:%02x:%02x:%02x",	
				(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
#endif	
	sprintf(DeviceId, "%s%s%02x%02x%02x%02x%02x%02x",
				manufacturerid,
				productid,
				(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
	//	ZLOG_INFO(zc,"the deviceid:%s\n",DeviceId);
	//	printf("the deviceid:%s\n",DeviceId);
	return 1;
}
#if 0
int main()
{
	char buf[100] = {0};
	get_device_id(buf);

}
#endif
