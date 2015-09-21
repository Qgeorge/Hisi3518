#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>

int get_device_id(char *DeviceId)
{

        struct ifreq ifreq;
        int sock;
        if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
        {
                perror("socket");
                return 2;
        }
        strcpy(ifreq.ifr_name,"ra0");
        if(ioctl(sock,SIOCGIFHWADDR,&ifreq)<0)
        {
                perror("ioctl");
                return 3;
        }
		#if 0
        printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                        (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
                        (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
                        (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
                        (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
                        (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
                        (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
		#endif	

		sprintf(DeviceId, "%02x:%02x:%02x:%02x:%02x:%02x",	
				(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
				(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);

		printf("%s\n", DeviceId);
		
        return 0;
}
