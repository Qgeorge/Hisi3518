#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <netdb.h>

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>

#include "ipc_email.h"

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define CHARSET  "gbk"
//#define CHARSET "utf-8"
//#define CHARSET "sjis"
#define BOUNDARY "---sinamailerqwertyuiop---"

#define MIME            "MIME-Version: 1.0"
#define CONTENT_TYPE    "Content-Type: multipart/mixed; boundary=\"" BOUNDARY "\""        
#define FIRST_BODY      "--" BOUNDARY "\r\nContent-Type: text/plain;charset=" CHARSET \
                                        "\r\nContent-Transfer-Encoding: 7bit\r\n\r\n"

#define mail_error(x) do { printf("%s:%d ERR:%s\n" , __func__ , __LINE__ , (x));} while(0) 
#define printf(args...) printf(args)

extern int imgSize[MAXIMGNUM];
extern char g_JpegBuffer[MAXIMGNUM][ALLIMGBUF];
extern HKEMAIL_T hk_email;

static char* GetTimeCharRand(char* buf)
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep);
	srand(time(NULL));
    sprintf(buf,"%d%02d%02d%02d%02d%02d%d",(1900+p->tm_year),(1+p->tm_mon),p->tm_mday, p->tm_hour,p->tm_min,p->tm_sec,rand()%1000);
    return buf;
}

static void sccInitPicCount(int iCount )
{
    hk_email.mcount = iCount;
}

extern void InitEmailInfo(int isOpen,char *mailserver, char *sendEmail, char *recvEmail, char *smtpUser, char *smtpPaswd, int emailPort, int count,int secType)
{
	//memset(&hk_email, 0, sizeof(hk_email));
	if( mailserver == NULL || sendEmail == NULL || recvEmail == NULL ||smtpUser == NULL || smtpPaswd == NULL )
	{
		printf( "EMail config err \n" );
        hk_email.isOpen = 0;
		return;
	}

    sccInitPicCount( count );
	hk_email.isOpen = isOpen;
	if(strlen(mailserver)>0)
    {
        strcpy(hk_email.mailserver, mailserver);
    }
    if(strlen(sendEmail)>0)
    {
        strcpy(hk_email.mailfrom, sendEmail);
    }
    if(strlen(recvEmail)>0)
    {
        strcpy(hk_email.mailto, recvEmail);
    }
    if(strlen(smtpUser)>0 )
    {
        strcpy(hk_email.username, smtpUser);
    }
    if(strlen(smtpPaswd)>0)
    {
        strcpy(hk_email.passwd, smtpPaswd);
    }
    hk_email.port = emailPort;
    hk_email.secType = secType;

    if ((strlen(mailserver) > 0) && (strlen(sendEmail) > 0) && (strlen(recvEmail) > 0) 
            && (strlen(smtpUser) > 0) && (strlen(smtpPaswd) > 0) && (emailPort > 0))
    {
        printf("....confing email success....\n");
    }
    else
    {
        printf("....confing email failed, please check your email settings....\n");
        hk_email.isOpen = 0;
    }
}




/**************************************/

//////////////////////////////////////////////////////////
static size_t encrypt_b64(char* dest,char* src,size_t size)
{
	int i, tmp = 0, b64_tmp = 0;
	
	if ( dest == NULL || src == NULL )
		return -1;
	
	while ((size - b64_tmp) >= 3) 
    {
		// every 3 bytes of source change to 4 bytes of destination, 4*6 = 3*8
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = ((src[b64_tmp]<<4) & 0x30) | ((src[b64_tmp+1]>>4) & 0x0F);
		dest[tmp+2] = ((src[b64_tmp+1]<<2) & 0x3C) | ((src[b64_tmp+2]>>6) & 0x03);
		dest[tmp+3] = 0x3F & src[b64_tmp+2];
		for (i=0; i<=3; i++)
        {
			if ( (dest[tmp+i] <= 25) )
				dest[tmp+i] += 'A';
			else	
				if (dest[tmp+i] <= 51)
					dest[tmp+i] += 'a' - 26;	       
				else
					if (dest[tmp+i] <= 61)
						dest[tmp+i] += '0' - 52;
			if (dest[tmp+i] == 62)
				dest[tmp+i] = '+';
			if (dest[tmp+i] == 63)
				dest[tmp+i] = '/';
		}
		
		tmp += 4;
		b64_tmp += 3;
	} //end while	

	if (b64_tmp == size)
    {
		dest[tmp] = '\0';
		return tmp;
	}
	if ((size - b64_tmp) == 1)
    {    //one
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = (src[b64_tmp]<<4) & 0x30;
		dest[tmp+2] = '=';
	}
	else 
    {    //two
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = ((src[b64_tmp]<<4) & 0x30) | ((src[b64_tmp+1]>>4) & 0x0F);
		dest[tmp+2] = (src[b64_tmp+1]<<2) & 0x3F;	
	}

	for (i=0; i<=(size - b64_tmp); i++)
    {
		if  (dest[tmp+i] <= 25)
			dest[tmp+i] += 'A';
		else	
			if (dest[tmp+i] <= 51)
				dest[tmp+i] += 'a' - 26;	       
			else
				if (dest[tmp+i] <= 61)
					dest[tmp+i] += '0' - 52;	//end if
		if (dest[tmp+i] == 62)
			dest[tmp+i] = '+';
		if (dest[tmp+i] == 63)
			dest[tmp+i] = '/';
	}
	dest[tmp+3] = '=';
	dest[tmp+4] = '\0';

	return tmp+4;
}

//////////////////////////////////////////////////////////
static int connect_nonblock(const char *ip,int port,int sec) 
{
	int	sockfd, flags, n;
	int sock_error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;
	struct sockaddr_in servaddr;	
	struct hostent *hent;
		
	if (NULL == ip)
		return 0;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		return 0;	
	hent = gethostbyname(ip);
	if (NULL == hent) 
	{
		close(sockfd);
		return 0;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr = *(struct in_addr *)hent->h_addr;

	flags = fcntl(sockfd, F_GETFL, 0);
	if (-1 == flags) 
	{
		close(sockfd);
		return 0;
	}
	if (-1 == fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
	{
		close(sockfd);
		return 0;
	}

	if ((n = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0))
	    if (errno != EINPROGRESS)
	    {
	    	close(sockfd);
	    	return 0;
	    }

	/* Do whatever we want while the connect is taking place. */
	if ( 0 == n)
		return sockfd;	/* connect completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = sec;
	tval.tv_usec = 0;

	if ( select(sockfd+1, &rset, &wset, NULL,sec ? &tval : NULL) == 0 ) 
	{
		close(sockfd);		/* timeout */		
		return 0;
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) 
	{		
		len = sizeof(sock_error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sock_error, &len) < 0)
		{  
			close(sockfd);			
			return 0;
		}
		if (sock_error)
		{
			errno = sock_error;
			close(sockfd);
			return 0;
		}		
	} else {
		//printf("select error: sockfd not set");
		close(sockfd);
		return 0;
	}
	/* restore file status flags */
	if ( fcntl(sockfd, F_SETFL, flags) < 0 )
	{
		close(sockfd);
		return 0;
	}
	return sockfd;
	
}

////////////////////////////////////////////////////////////
// if success , return the number of bytes read
// if error, return -1;
static ssize_t readline2(int fd, void *vptr, size_t maxlen) 
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	*ptr = 0;
	for (n = 1; n < maxlen; n++) {
again:
		if ( (rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n') 
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;	/* EOF, some data was read */
		} else {
			if (errno == EINTR)
				goto again;
			return(-1);		/* error, errno set by read() */
		}
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}


//////////////////////////////////////////////////////////
static int encrypt_b64_file_to_buf(char* en64_buf, char *emailBuffer, int size )
{
	//int r_size = 100*1024;
	//int r_size = 500*1024;
	char *ptr ;
	int count = size;
    if( en64_buf == NULL || emailBuffer == NULL )
    {
        return 0;
    }
    HK_DEBUG("Start encrypt... Count:%d\n", count);
	ptr = en64_buf;
    //memcpy(ptr, en64_buf, strlen(en64_buf));
    //do {
    int i=size;
    while (i >= 17*3) 
    {
        //HK_DEBUG("1---%d,%d\n",i,count);
        encrypt_b64(ptr,(emailBuffer+count-i),17*4);
        ptr += 17*4;
        ptr += sprintf(ptr,"\n");			
        i -= 17*3;
        //HK_DEBUG("2---%d,%d\n",i,count);
    }
    if (i > 0) {
        ptr += encrypt_b64(ptr,(emailBuffer+count-i),i);
        ptr += sprintf(ptr,"\n");			
    }
    HK_DEBUG("3---%d,%d\n",i,count);
    //if (count < r_size)
    //		break;
    //} while (count == r_size);
    return (ptr-en64_buf);
}

///////////////////////////////////////////////

//char en64_buf[ALLIMGBUF]={0};

static int write_image_buf(int socketfd)
{
	unsigned char buffer[1024]={0};
    char en64_buf[ALLIMGBUF+ALLIMGBUF/2]={0};

	int i;
	char imgName[15]={0};
	//for(i=0; i < MAXIMGNUM; i++)
	for(i=0; i < hk_email.mcount; i++)
	{
		memset(en64_buf, 0, sizeof(en64_buf) );
        memset(buffer, 0, sizeof(buffer));
        sprintf(imgName, "notice%d.jpg", i+1);

		sprintf(buffer,"\r\n%s\r\n","--" BOUNDARY);
        HK_DEBUG("Reday Send:%s Size:%d\n", imgName, imgSize[i]);
		write(socketfd,buffer,strlen(buffer));
		sprintf(buffer,"Content-Type: image/jpeg; name=%s\r\n",imgName);
		if (write(socketfd,buffer,strlen(buffer)) < 0)
			return 0;
		
		sprintf(buffer,"Content-disposition: attachment; filename=%s\r\nContent-Transfer-Encoding: base64\r\n\r\n", imgName);
		if (write(socketfd,buffer,strlen(buffer)) < 0)
			return 0;	

		int len = encrypt_b64_file_to_buf(en64_buf, g_JpegBuffer[i], imgSize[i]);
		if( len < 0 )
		{
			HK_DEBUG("%s: encrypt_b64 error\n", __func__);
			continue;
		}
        HK_DEBUG("[%s,%d]...encrypt base64 OK, imgSize[%d]:%d len:%d...\n", __func__, __LINE__, i, imgSize[i], len);

        if(write(socketfd, (void *)en64_buf, len) <= 0 )
		{
			printf("%s: write image error: %s\n", __func__, strerror(errno));
            return 0;
		}
	}
	return 1;
}

extern int send_mail_to_user(char *smtp_server,char *passwd, char *send_from,const
		char *send_to, char *smtp_user, char *body, int iType)
{
	if(NULL == smtp_server || NULL == passwd || NULL == send_from || NULL ==
			send_to || NULL == smtp_user)
	{
		printf("email confif is empyt!\n");
		return -1;
	}
	struct timeval tv;
	int sockfd;
	int cnt,rt;
	int auth_base64_flag = 0;
	unsigned char buffer[1024] = {0},*ptr;
	//unsigned char username[MAXLINE];
	int first_auth = FALSE;
	struct hostent *hent;
	
    if ((strlen(smtp_server)==0) || (strlen(send_from)==0) ||
			(strlen(body)==0) || ( strlen(passwd)==0 ) || (strlen(send_to)==0)
			|| (strlen(smtp_user)==0))
	{
		fprintf(stderr, "smtp_server=%s\n send_from=%s\n body=%s\n passwd=%s\n send_to=%s\n", 
				smtp_server,send_from,body,passwd,send_to);

		return -1;
	}
	
#if 0
	//get the username. eg: test@test.com ---> "test"
	strncpy(username,send_from,MAXLINE);
    ptr = username;
	while ( *ptr != '@' && *ptr != '\0' )
		ptr++;
	if ( *ptr == '\0' )
		return -1;	 	
	else if ( *ptr == '@' ) {
		*ptr = '\0';
	} else 
		return -1;
#endif

	sockfd = connect_nonblock(smtp_server, hk_email.port,60);
	if (sockfd <= 0) {
		mail_error("open socket");
		goto error;
	}
	
	tv.tv_sec = 10;
	tv.tv_usec = 0;	
	if ( setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		mail_error("setsockopt");
		goto error;
	}	
	
	readline2(sockfd,buffer,MAXLINE);
	if ( strncmp(buffer,"220",3) ) {		
		mail_error("not smtp server");
        goto error;
    }

    sprintf(buffer,"EHLO %s\r\n",smtp_server);
    if (write(sockfd,buffer,strlen(buffer)) < 0){
        mail_error("EHLO");
        goto error;
    }

    auth_base64_flag = 0;
    do {
        rt = readline2(sockfd,buffer,MAXLINE);		
        if (rt <= 0){
            mail_error("readline2");
            goto error;
        }
        if ( !strncmp("250-AUTH",buffer,8) && strstr(buffer,"LOGIN") )
            auth_base64_flag = 1;
    } while (strncmp(buffer,"250 ",4) != 0) ;

    if (auth_base64_flag != 0)
    {
        if ( write(sockfd,"AUTH LOGIN\r\n",12)  < 0 )
            goto error;
        if ( readline2(sockfd,buffer,MAXLINE) < 0)
            goto error;
        if ( 0 != strncmp("334 ",buffer,4) )
            goto error;
        //write the base64 crypted username
        encrypt_b64(buffer, smtp_user, strlen(smtp_user) );
        strcat(buffer,"\r\n");
        if ( write(sockfd,buffer,strlen(buffer)) < 0)
            goto error;

        if ( readline2(sockfd,buffer,MAXLINE) < 0)
            goto error;
        if ( 0 != strncmp("334 ",buffer,4) )
        {
            first_auth = FALSE;
        }else{
            first_auth = TRUE;
        } 

        //write the base64 crypted passwd
        encrypt_b64(buffer,passwd,strlen(passwd) );
        strcat(buffer,"\r\n");
        // finished!
        if ( write(sockfd,buffer,strlen(buffer)) < 0)
            goto error;
        if ( readline2(sockfd,buffer,MAXLINE) < 0)
            goto error;
        if ( 0 != strncmp("235 ",buffer,4) )
        {
            first_auth = FALSE;
        }else{
            first_auth = TRUE;
        }
        // auth ok!
    }

    // the first auth failed , try use the send_from as the email account
    if ( auth_base64_flag != 0  && first_auth == FALSE )
    {
        if ( write(sockfd,"AUTH LOGIN\r\n",12)  < 0 )
        {
            goto error;
		}
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
		{
			goto error;
		}
		if ( 0 != strncmp("334 ",buffer,4) )
		{
			goto error;
		}
		// Write the base64 crypted username
		encrypt_b64(buffer,send_from,strlen(send_from) );
		strcat(buffer,"\r\n");
		if ( write(sockfd,buffer,strlen(buffer)) < 0)
		{
	    	goto error;
		}
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
		{
			goto error;
		}
		if ( 0 != strncmp("334 ",buffer,4) )
		{
			goto error;
		}
		
		//Write the base64 crypted password
		encrypt_b64(buffer,passwd,strlen(passwd) );
		strcat(buffer,"\r\n");
		if (write(sockfd,buffer,strlen(buffer)) < 0)
		{
			goto error;
		}
		if (readline2(sockfd, buffer, MAXLINE) < 0)
		{
            HK_DEBUG("Check Your Password!\n");
			goto error;
		}
		if ( 0 != strncmp("235 ",buffer,4) ) 
		{
            HK_DEBUG("Check Your Password!\n");
			goto error;
		}
		// auth ok!
	}
	
	sprintf(buffer,"MAIL FROM: <%s>\r\n",send_from);
	write(sockfd,buffer,strlen(buffer));
	readline2(sockfd,buffer,MAXLINE);
	if(strncmp(buffer,"250",3)) 
    {
		printf("The smtp server didn't accept sender name(%s) \n",send_from);
		goto error;
	}
    //HK_SUCCESS("Send mailfrom success!%d\n", __LINE__);

	sprintf(buffer,"RCPT TO: <%s>\r\n",send_to);
	write(sockfd,buffer,strlen(buffer));
	readline2(sockfd,buffer,MAXLINE);
	//HK_DEBUG("We get from smtp server: %s\n",buffer);
	if (strncmp(buffer,"250",3)) {
		printf("The smtp server didn't accept the send_to(%s) ret=%s\n",send_to,buffer);
		goto error;
	}

	write(sockfd,"DATA\r\n",6);
	readline2(sockfd,buffer,MAXLINE);
	if (strncmp(buffer,"354",3)) {
		printf("send \"DATA\" command error:%s\n",buffer);	 
		goto error; 
	}	 

	//next we will write the header
	//To: example@hotmail.com
	//From: in4s@in4s.com
	//Subject: e-Home Security
	//MIME-Version: 1.0
	//Content-Type: multipart/mixed; boundary="PtErOdAcTyL2285248"
	//#define MIME		"MIME-Version: 1.0"
	//#define CONTENT_TYPE	"Content-Type: multipart/mixed"		  
	
	sprintf(buffer,"From: %s\r\n",send_from);
	if(write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;

	sprintf(buffer,"To: %s\r\n",send_to);
	if(write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;

	sprintf(buffer,"%s" "\r\n",MIME);
	if(write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;
	
//	sprintf(buffer,"Subject: %s %s\r\n", getenv("USER"), MAIL_SUBJECT); 
	char timeBufs[64] = {0};
	GetTimeCharRand(timeBufs);
	sprintf(buffer,"Subject: %s %s-%s\r\n", getenv("USER"), MAIL_SUBJECT,timeBufs);  // 2014.7.28 增加时间戳 主题
	if(write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;	
	
	sprintf(buffer,"%s\r\n",CONTENT_TYPE);
	if(write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;
	
	sprintf(buffer,"\r\n%s\r\n", "--" BOUNDARY);
	write(sockfd,buffer,strlen(buffer));
	
	sprintf(buffer,"%s",FIRST_BODY);
	write(sockfd,buffer,strlen(buffer)); 

/*
	if(write(sockfd,body,strlen(body)) < 0)
		goto error;
		*/
	memset(buffer,0,sizeof(buffer));	// 2014.7.28增加时间戳  内容
	//memset(timeBufs,0,sizeof(timeBufs));
	//GetTimeCharRand(timeBufs);
	time_t timePtr;
	struct tm *timeStr;
	timePtr = time(NULL);
	timeStr = localtime(&timePtr);
	sprintf(buffer,"%s  Time:%s",body,asctime(timeStr));
	if(write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;
	
    if(hk_email.mcount > 0 && iType==1)
    {
	    if(!write_image_buf(sockfd))
	    	goto error;
    }
	//printf("image complete\n");

	sprintf(buffer,"\r\n%s\r\n","--" BOUNDARY);
	write(sockfd,buffer,strlen(buffer));
	write(sockfd,"\r\n.\r\n",5);
	write(sockfd,"quit\r\n",6);
	close(sockfd);
	//HK_DEBUG("Send Mail to list successfully\n");
	return 0;	

error:
	close(sockfd);
	return -1;	
}
