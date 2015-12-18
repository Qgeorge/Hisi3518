/******************************************************************************
 * Copyright 2014-2015  (By younger)
 *
 * FileName: smartconfig.c
 *
 * Description: smartconfig demo on sniffer
 *
 * Modification history:
 *     2015/1/24, v1.0 create this file.
 *******************************************************************************/
#include "smartconfig.h"

uint8 smart_lock_flag=0,smart_mac[6],smart_channel=-1,current_channel;
uint16 channel_bits = 0;
SMARTSTATUS smart_status = SMART_CH_INIT;
router_p router_list;
bool START_FLAG = false;
sem_t sem;
#if DEBUG
int smartfd;
#endif
uint8 calcrc_1byte(uint8 onebyte)    
{    
	uint8 i,crc_1byte;     
	crc_1byte=0;               
	for(i = 0; i < 8; i++)    
	{    
		if(((crc_1byte^onebyte)&0x01)) 
		{    
			crc_1byte^=0x18;     
			crc_1byte>>=1;    
			crc_1byte|=0x80;    
		}else{
			crc_1byte>>=1; 
		}     

		onebyte>>=1;          
	}   
	return crc_1byte;   
}   

uint8 calcrc_bytes(uint8 *p,uint8 len)  
{  
	uint8 crc=0;  
	while(len--)
	{  
		crc=calcrc_1byte(crc^(*p));
		p++;
	}  
	printf("[smart] calcrc_bytes: %02x \n",crc);
	return crc;
}

/******************************************************************************
 * FunctionName : smart_config_decode
 * Description  :decode the packet.
 * Parameters   :  pOneByte -- the packet buf.
 * Returns      : none
 *******************************************************************************/
int smart_config_decode(uint8* pOneByte)
{
	int i = 0,count;
	uint8 pos=0,val=0;
	uint8 n0,n1;
	uint8 tmp[100];
	static uint8 smart_recvbuf_len=0;    
	static uint8 smart_recvbuf[RECEVEBUF_MAXLEN];

	if( !(0==pOneByte[0]&&0==pOneByte[1]) ) return -9;

	n0 = (pOneByte[2]-1);
	n1 = (pOneByte[3]-1);
	if( (n1+n0) != 15 )
	{
#if DEBUG	
		if(write(smartfd,"-1\n",3)<0){
			perror("write");
			exit(1);
		}
		memset(tmp,0,strlen(tmp));
		sprintf(tmp,"pos:smart_config_decode_84:n0+n1=%hhd\n",n0+n1);
		if(write(smartfd,tmp,strlen(tmp))<0){
			perror("write");
			exit(1);
		}
#endif
		return -1;
	}
	pos = (n0&0xf);
	n0 = (pOneByte[4]-1);
	n1 = (pOneByte[5]-1);
	if( (n1+n0) != 15 ){
#if DEBUG
		if(write(smartfd,"-2\n",3)<0){
			perror("write");
			exit(1);
		}
		memset(tmp,0,strlen(tmp));
		sprintf(tmp,"n0+n1=%hhd\n",n0+n1);
		if(write(smartfd,tmp,strlen(tmp))<0){
			perror("write");
			exit(1);
		}
#endif
		return -2;
	}

	pos |= (n0&0xf)<<4;
	n0 = (pOneByte[6]-1);
	n1 = (pOneByte[7]-1);
	if( (n1+n0) != 15 ){
#if DEBUG
		if(write(smartfd,"-3\n",3)<0){
			perror("write");
			exit(1);
		}
		memset(tmp,0,strlen(tmp));
		sprintf(tmp,"n0+n1=%hhd\n",n0+n1);
		if(write(smartfd,tmp,strlen(tmp))<0){
			perror("write");
			exit(1);
		}
#endif
		return -3;
	}

	val = (n0&0xf);
	n0 = (pOneByte[8]-1);
	n1 = (pOneByte[9]-1);
	if( (n1+n0) != 15 ){
#if DEBUG
		if(write(smartfd,"-4\n",3)<0){
			perror("write");
			exit(1);
		}
		memset(tmp,0,strlen(tmp));
		sprintf(tmp,"n0+n1=%hhd\n",n0+n1);
		if(write(smartfd,tmp,strlen(tmp))<0){
			perror("write");
			exit(1);
		}
#endif
		return -4;
	}

	val |= (n0&0xf)<<4;
	if( (int)pos >= (int)RECEVEBUF_MAXLEN ){ 
#if DEBUG	
		if(write(smartfd,"-5\n",3)<0){
			perror("write");
			exit(1);
		}
		memset(tmp,0,strlen(tmp));
		sprintf(tmp,"n0+n1=%hhd\n",n0+n1);
		if(write(smartfd,tmp,strlen(tmp))<0){
			perror("write");
			exit(1);
		}
#endif
		return -5;
	}

	if( smart_recvbuf[pos] == val && smart_recvbuf_len>2 ){//begin crc8
		if(calcrc_bytes(smart_recvbuf,smart_recvbuf_len) == smart_recvbuf[smart_recvbuf_len] ){
			// smart_recvbuf[smart_recvbuf_len+1]='\0';
			smart_recvbuf[smart_recvbuf_len]='\0';
			printf("%s\n",smart_recvbuf);
#if DEBUG
			memset(tmp,0,strlen(tmp));
			sprintf(tmp,"pos:%hhd,val:%c\n",pos,val);
			if(write(smartfd,tmp,strlen(tmp))<0){
				perror("write");
				exit(1);
			}
#endif
			exit(1);

			//smartconfig_end(smart_recvbuf);
			//return 0;
		}

		printf("[smart]:%s\n",smart_recvbuf);
	}else{

		smart_recvbuf[pos] = val;
#if DEBUG
		memset(tmp,0,strlen(tmp));
		sprintf(tmp,"pos:%hhd,val:%c\n",pos,val);

		if(write(smartfd,tmp,strlen(tmp))<0){
			perror("write");
			exit(1);
		}
#endif
		printf("[%hhd]=%hhd\n",pos,val);
		if( pos > smart_recvbuf_len ){
			smart_recvbuf_len = pos;
		}
		return SMART_CH_LOCKED;
	}

	return 99;
}

/******************************************************************************
 * FunctionName : smart_analyze
 * Description  : smart_analyze the packet.
 * Parameters   :  buf -- the packet buf.
 len -- the packet len.
 channel -- the packet channel.
 * Returns      : none
 *******************************************************************************/
void smart_analyze(uint8* buf,int len,int channel,int fd)
{
	int funret;
	uint8 tmp[100];
	static uint32 smart_lock_time=0;
	static uint8  smart_onbyte_Idx=0;
	static uint8  smart_onbyte[10];
	//1.th the packet len is under 16.
	if( len > 16 )
		return;

	if(SMART_CH_INIT == smart_status && len == 0)
	{
		printf("[smart] sync  into SMART_CH_LOCKING\n");
#if 1
		if(smart_lock_flag == 0)
		{
			memcpy(smart_mac,buf,6);
		}
		smart_channel=channel;
#endif

		smart_onbyte_Idx = 0;
		smart_onbyte[smart_onbyte_Idx++]=len;
		smart_status = SMART_CH_LOCKING;

	}else if( SMART_CH_LOCKING == smart_status ){

		if( memcmp(smart_mac,buf,6) == 0 ) {
			if( len == 0){
				smart_onbyte[smart_onbyte_Idx++]=len;
				smart_status = SMART_CH_LOCKED;
			}else{
				smart_status =SMART_CH_INIT; 
				smart_onbyte_Idx=0;
			}
		}

	}else if( SMART_CH_LOCKED == smart_status ){

		if( memcmp(smart_mac,buf,6) == 0 ){
			if( len == 0 ){
				if(smart_lock_flag == 0){
					memcpy(smart_mac,buf,6);
				}
				smart_channel=channel;
				smart_status = SMART_CH_LOCKING;
				smart_onbyte_Idx = 0;
				smart_onbyte[smart_onbyte_Idx++]=len;
				printf("[smart] resync  into SMART_CH_LOCKING\n");
			}else{
				smart_onbyte[smart_onbyte_Idx++]=len; 
			}
		}

	}

	if(smart_onbyte_Idx>=10)
	{
		int index;
#if DEBUG	
		for(index = 0;index<smart_onbyte_Idx;index++){
			memset(tmp,0,strlen(tmp));
			sprintf(tmp,"smart_onbyte[%d]:%hhd\n",index,smart_onbyte[index]);
			
			if(write(fd,tmp,strlen(tmp))<0){
				perror("write");
				exit(1);
			}
		}

		if(write(fd,"------------------\n",19)<0){
			perror("write");
			exit(1);
		}
#endif
		funret = smart_config_decode(smart_onbyte);

		printf("[smart] Decode ret=%d\n",funret);
		if( SMART_CH_LOCKED == funret )
		{
			smart_status = SMART_CH_LOCKED ;
			smart_lock_flag = 1;
		}
		smart_status = SMART_CH_INIT; 
		smart_onbyte_Idx=0;
	}
}
void sniffer_hopping(int current_channel){
	char cmdstr[100];
	sprintf(cmdstr,"iwconfig ra0 channel %d",current_channel);
	system(cmdstr);
}


int smartlink_start() {
	uint32 sock,n, packetCount = 0,ret,len;
	uint8 buffer[2048],tmp[100];
	struct ifreq ethreq;
	struct list_head *pos;
	router_p router_tmp,info = NULL;

	ret = sem_init (&sem, 0, 1);
	if(ret != 0){
		perror("sem_init");
	    return 0;
	}
#if DEBUG
	uint32 fd,dfd;
	if((fd=open("./packet.txt",O_RDONLY|O_CREAT|O_WRONLY,0644))<0){
		perror("open");
		exit(1);
	}

	if((dfd=open("./decode.txt",O_RDONLY|O_CREAT|O_WRONLY,0644))<0){
		perror("open");
		exit(1);
	}

	if((smartfd=open("./smartrecv.txt",O_RDONLY|O_CREAT|O_WRONLY,0644))<0){
		perror("open");
		exit(1);
	}
#endif
	if((sock=socket(PF_PACKET, SOCK_RAW,htons(ETH_P_ALL)))<0) {
		perror("socket");
		exit(1);
	}

	router_discover();

	/* Set the network card in promiscuos mode */
	strncpy(ethreq.ifr_name,"ra0",IFNAMSIZ);
	if (ioctl(sock,SIOCGIFFLAGS, &ethreq)==-1) {
		perror("ioctl");
		close(sock);
		exit(1);
	}

	ethreq.ifr_flags|=IFF_PROMISC;
	if (ioctl(sock,SIOCSIFFLAGS, &ethreq)==-1) {
		perror("ioctl");
		close(sock);
		exit(1);
	}

	printf( "ra0  promisc..\n" );

	//Bind to device
	if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, "ra0", strlen("ra0")) == -1)
	{
		printf("SO_BINDTODEVICE");
		close(sock);
		exit(EXIT_FAILURE);
	}

	system("iwconfig ra0 mode monitor");
	printf("mode monitor..\n");

	//  sniffer_hopping(11);
	while(!START_FLAG);
		set_channel_timer();	

	printf("------------start recv data...\n");
	while (1) {
		info = NULL;
		n = recvfrom(sock,buffer,2048,0,NULL,NULL);

		/* Check to see if the packet contains at least
		 * complete Ethernet (14), IP (20) and TCP/UDP
		 * (8) headers.
		 */

		if (n<42) {
			perror("recvfrom():");
			printf("Incomplete packet (errno is %d)\n",
					errno);
			close(sock);
			exit(0);
		}

		sem_wait(&sem);
		if(((buffer[145]&0x02)==0x02)&&buffer[144]==0x08){
			list_for_each(pos,&(router_list->list)){
				router_tmp = list_entry(pos,router_t,list);
				if(strncmp(router_tmp->bssid,&buffer[154],6)==0){
				printf("----------smartbuf!\n");
					info=router_tmp;				
					break;
				}
			}	
		}
		sem_post(&sem);
	
	if(info==NULL)
		continue;

	len = n-144;

	/******************************************************/

	/************判断是否为要接收的广播帧*******************/

	/******************************************************/	
	printf("%d\n",packetCount);

	len -= 24;  //fix me.....


	if (info->encryption_mode == ENCRY_NONE) {
		len -= 40;
	} else if (info->encryption_mode == ENCRY_WEP){
		len = len - 40 - 4 - 4;
	} else if (info->encryption_mode == ENCRY_TKIP) {
		len = len - 40 - 12 - 8;
	} else if (info->encryption_mode == ENCRY_CCMP) {
		len = len - 40 - 8 - 8;
	}
#if DEBUG
	smart_analyze(buffer+144+10,len,info->channel,dfd);
#else
	smart_analyze(buffer+144+10,len,info->channel,0);
#endif


	/******************************************************/

	/**********将收到的数据打印到指定文件上****************/

	/******************************************************/	
#if DEBUG
	sprintf(tmp,"-------------------------packagelenght:%d--number:%d--lenght:%d\n",n-144,packetCount++,len);
	ret = write(fd,tmp,strlen(tmp));
	if(ret < 0 ){
		perror("write");
		exit(1);
	}
	int count = 144;
	while(1){
		memset(tmp,0,sizeof(tmp));
		if(count == n){
			//	printf("\n");
			ret = write(fd,"\n",1);
			if(ret < 0){
				perror("write");
				exit(1);
			}

			break;
		}
		sprintf(tmp,"%02x",*(buffer+count));
		count++;
		ret = write(fd,tmp,2);
		if(ret < 0){
			perror("write");
			exit(1);
		}
		if(count%16 == 0){
			//	printf("\n");
			ret = write(fd,"\n",1);
			if(ret<0){
				perror("write");
				exit(1);
			}
		}
	}
#endif
}
#if DEBUG
close(fd);
#endif
}

void router_discover(){
	pthread_t tid;
	system("iwconfig ra0 mode managed");
	system("./iwlist ra0 scanning");

	router_scanf(&router_list);
	pthread_create (&tid, NULL, thread_routine, NULL);
}

void router_scanf(router_p *router_info_head){

	u8 buf[BUFSIZ],pairmethod1[BUFSIZ],pairmethod2[BUFSIZ];
	u8 pairnumber = -1;
	u8 router_number;
	router_p router_head;
	int ret = -1;
	FILE *fp;
	router_p router_node;

	if(((*router_info_head) = (router_p)malloc(sizeof(router_t)))==NULL){
		perror("malloc");
		exit(1);
	}

	router_head = (*router_info_head);

	//initialize list head
	INIT_LIST_HEAD(&router_head->list);

	memset(buf,0,strlen(buf));
	//打开文件
	if((fp = fopen("router_fifo.txt","r")) == NULL){
		perror("fopen");
		exit(1);
	}
	while(1){
		if((router_node = (router_p)malloc(sizeof(router_t)))==NULL){
			perror("malloc");
			exit(1);
		}

		if(fgets(buf,BUFSIZ,fp) != NULL){
			while(ret!=7){ 
				ret = sscanf(buf,"Cell %hhd - Address:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&router_number
						,&router_node->bssid[0],&router_node->bssid[1],
						&router_node->bssid[2],&router_node->bssid[3],
						&router_node->bssid[4],&router_node->bssid[5]);
				if(ret==7){
					ret = -1;
					break;
				}		 
				memset(buf,0,strlen(buf));
				if(fgets(buf,BUFSIZ,fp)==NULL)
					break;
			}
		}else{
			free(router_node);
			break;
		}

		if(fgets(buf,BUFSIZ,fp) != NULL){
			sscanf(buf,"ESSID:%s",router_node->essid);
			memset(buf,0,strlen(buf));		
		}else{
			free(router_node);
			break;
		}

		if(fgets(buf,BUFSIZ,fp) != NULL){
			sscanf(buf,"channel:%hhd",&router_node->channel);
			memset(buf,0,strlen(buf));
		}else{
			free(router_node);
			break;
		}	
		if(fgets(buf,BUFSIZ,fp) != NULL){

			while(ret!=1){
				ret = sscanf(buf,"Pairwise Ciphers(%hhd)",&pairnumber);
				if(ret == 1){
					ret = -1;
					break;
				}
				memset(buf,0,strlen(buf));
				if(fgets(buf,BUFSIZ,fp)==NULL)
					break;
			}

			if(pairnumber == 1){
				sscanf(buf,"Pairwise Ciphers(%hhd): %s",&pairnumber,pairmethod1);
				if(strncmp(pairmethod1,"WEP",3)==0){
					router_node->encryption_mode = 2;
				}else if(strncmp(pairmethod1,"TKIP",4)==0){
					router_node->encryption_mode = 3;
				}else if(strncmp(pairmethod1,"CCMP",4)==0){
					router_node->encryption_mode = 4;
				}
			}else{	
				sscanf(buf,"Pairwise Ciphers(%hhd): %s %s",&pairnumber,pairmethod1,pairmethod2);
				if((strncmp(pairmethod1,"CCMP",4)==0) || (strncmp(pairmethod2,"CCMP",4)==0)){
					router_node->encryption_mode = 4;
				}
			}

			memset(buf,0,strlen(buf));		
		}else{
			free(router_node);
			break;
		}

		// add router node
		sem_wait(&sem);
		list_add(&(router_node->list),&(router_head->list));
		sem_post(&sem);

		channel_bits |= 1<<(router_node->channel);

		if(START_FLAG==false){
			START_FLAG = true;
			sniffer_hopping(router_node->channel);
		}
	}
	fclose(fp);
}

void *thread_routine (void *arg)
{
	router_scanf(&router_list);
	return NULL;
}

void timer_handler (int signum)
{
	printf("hello---timer_handler\n");
	if(signum == SIGALRM){    
		uint8 i;
		//1.th If find one channel send smartdata,lock on this.
		if( smart_channel > 0 && smart_lock_flag == 1)
		{
			struct itimerval timer;
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = 0;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = 0;

			setitimer (ITIMER_REAL, &timer, NULL);
	
			sniffer_hopping(smart_channel);
			printf("[smart] locked Smartlink channel=%d\n",smart_channel);
			return;
		}

		//2.th scan channel by timer 
		for (i = current_channel; i < 14; i++) {
			if ((channel_bits & (1 << i)) != 0) {
				current_channel = i + 1;
				sniffer_hopping(i);
				printf("[smart] current channel %d\n", i);
				break;
			}
		}

		if (i == 14) {
			current_channel = 1;
			for (i = current_channel; i < 14; i++) {
				if ((channel_bits & (1 << i)) != 0) {
					current_channel = i + 1;
					sniffer_hopping(i);
					printf("[smart] current channel %d\n", i);
					break;
				}
			}
		}
	}
}


void set_channel_timer(){

	struct sigaction sa;
	struct itimerval timer;
	memset (&sa, 0, sizeof (sa));
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = &timer_handler;
	sigaction(SIGALRM, &sa, NULL);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 500000;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 500000;

	printf("set_channel_timer\n");
	setitimer(ITIMER_REAL, &timer, NULL);
}

