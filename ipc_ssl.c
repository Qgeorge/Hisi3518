#include "ipc_email.h"
#include "ipc_ssl.h"
#include <ctype.h>
extern Command_Entry* FindCommandEntry(int command);
extern char *base64_encode(const char * bytes_to_encode,  int in_len);

static int OpenSSLConnect()
{
	printf("func:%s..line:%d\n", __func__, __LINE__);
	if(m_ctx == NULL)
	{
		printf("m_ctx is null!\n");   //SSL_PROBLEM
		return -1;
	}

	m_ssl = SSL_new (m_ctx);   

	if(m_ssl == NULL)
	{
		printf("m_ssl is null!\n");   //SSL_PROBLEM
		return -1;
	}

	SSL_set_fd (m_ssl, (int)gmail_socket);
	SSL_set_mode(m_ssl, SSL_MODE_AUTO_RETRY);

	int res = 0;
	fd_set fdwrite;
	fd_set fdread;
	int write_blocked = 0;
	int read_blocked = 0;

	struct timeval time;
	time.tv_sec = TIME_IN_SEC;
	time.tv_usec = 0;

	while(1)
	{
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdread);

		if(write_blocked)
			FD_SET(gmail_socket, &fdwrite);
		if(read_blocked)
			FD_SET(gmail_socket, &fdread);

		if(write_blocked || read_blocked)
		{
			write_blocked = 0;
			read_blocked = 0;
			if((res = select(gmail_socket+1,&fdread,&fdwrite,NULL,&time)) == SOCKET_ERROR)
			{
				FD_ZERO(&fdwrite);
				FD_ZERO(&fdread);
				printf("slect error!\n");   //WSA_SELECT
				return -1;
			}
			if(!res)
			{
				//timeout
				FD_ZERO(&fdwrite);
				FD_ZERO(&fdread);
				printf("server not responding!\n");   //SERVER_NOT_RESPONDING
				return -1;
			}
		}
		res = SSL_connect(m_ssl);

		switch(SSL_get_error(m_ssl, res))
		{ 
			case SSL_ERROR_NONE:
				FD_ZERO(&fdwrite);
				FD_ZERO(&fdread);
				return 0;  
				break;
			case SSL_ERROR_WANT_WRITE:  
				write_blocked = 1;   
				break;   
			case SSL_ERROR_WANT_READ:  
				read_blocked = 1;  
				break;   
			default:  
				FD_ZERO(&fdwrite);
				FD_ZERO(&fdread);
				printf("ssl problem! line%d -- %d\n", __LINE__,res);  //SSL_PROBLEM
				return -1;
		}
	}
	return 0;
}

static void FormatHeader(char* header)
{
	char month[][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	size_t i;
	char to[64];
	//char* cc;

	time_t rawtime;
	struct tm* timeinfo;

	// date/time check
	if(time(&rawtime) > 0)
		timeinfo = localtime(&rawtime);
	else
		printf("time error!\n");   //TIME_ERROR

	sprintf(to, "<%s>", hk_email.mailto);

	// Date: <SP> <dd> <SP> <mon> <SP> <yy> <SP> <hh> ":" <mm> ":" <ss> <SP> <zone> <CRLF>
	sprintf(header,"Date: %d %s %d %d:%d:%d\r\n",   timeinfo->tm_mday,
			month[timeinfo->tm_mon],
			timeinfo->tm_year+1900,
			timeinfo->tm_hour,
			timeinfo->tm_min,
			timeinfo->tm_sec); 

	// From: <SP> <sender>  <SP> "<" <sender-email> ">" <CRLF>
	if(strlen(hk_email.mailfrom) <= 0) 
		printf("undef mail from!\n");   //UNDEF_MAIL_FROM

	strcat(header,"From: ");
	if(strlen(hk_email.mailfrom)) 
		strcat(header, hk_email.mailfrom);

	strcat(header," <");
	strcat(header,hk_email.mailfrom);
	strcat(header, ">\r\n");

	// To: <SP> <remote-user-mail> <CRLF>
	strcat(header,"To: ");
	strcat(header, to);
	strcat(header, "\r\n");

	char m_Subject[64] = {0};
	sprintf(m_Subject, "%s %s", getenv("USER"), MAIL_SUBJECT );
	strcat(header, "Subject: ");
	strcat(header, m_Subject);
	strcat(header, "\r\n");
	//strcat(header, "Subject: Home Security Message!\r\n");

	// MIME-Version: <SP> 1.0 <CRLF>
	strcat(header, "MIME-Version: 1.0\r\n");
	strcat(header, "Content-Type: multipart/mixed; boundary=\"");
	strcat(header,BOUNDARY_TEXT);
	strcat(header, "\"\r\n");
	strcat(header, "\r\n");

	// first goes text message
	strcat(SendBuf, "--");
	strcat(SendBuf,BOUNDARY_TEXT);
	strcat(SendBuf, "\r\n");

	strcat(SendBuf,"Content-type: text/plain; charset=");
	strcat(header, "US-ASCII");
	strcat(header, "\r\n");

	strcat(SendBuf, "Content-Transfer-Encoding: 7bit\r\n");
	strcat(SendBuf, "\r\n");
}

static int ReceiveData_SSL(SSL* ssl, Command_Entry* pEntry)
{
	int res = 0;
	int offset = 0;
	fd_set fdread;
	fd_set fdwrite;
	struct timeval time;

	int read_blocked_on_write = 0;

	time.tv_sec = pEntry->recv_timeout;
	time.tv_usec = 0;

	assert(RecvBuf);

	if(RecvBuf == NULL)
		printf("Recvbuf is empty!\n");   //RECVBUF_IS_EMPTY

	int bFinish = 0;

	while(!bFinish)
	{
		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);

		FD_SET(gmail_socket,&fdread);

		if(read_blocked_on_write)
		{
			FD_SET(gmail_socket, &fdwrite);
		}

		if((res = select(gmail_socket+1, &fdread, &fdwrite, NULL, &time)) == SOCKET_ERROR)
		{
			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			printf("select error!\n");   //WSA_SELECT
			return -1;
		}

		if(!res)
		{
			//timeout
			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			printf("server not responding !\n");   //SERVER_NOT_RESPONDING
			return -1;
		}

		if(FD_ISSET(gmail_socket,&fdread) || (read_blocked_on_write && FD_ISSET(gmail_socket,&fdwrite)) )
		{
			while(1)  
			{
				read_blocked_on_write=0;
				const int buff_len = 1024;     
				char buff[buff_len];  
				res = SSL_read(ssl, buff, buff_len);    
				int ssl_err = SSL_get_error(ssl, res);    
				if(ssl_err == SSL_ERROR_NONE)     
				{     
					if(offset + res > BUFFER_SIZE - 1)
					{
						FD_ZERO(&fdread);
						FD_ZERO(&fdwrite);
						printf("lack of memory1\n");   //LACK_OF_MEMORY
						return -1;
					}
					memcpy(RecvBuf + offset, buff, res);
					offset += res;
					if(SSL_pending(ssl))
					{
						continue;
					}
					else
					{
						bFinish = 1;
						break;
					}
				}              
				else if(ssl_err == SSL_ERROR_ZERO_RETURN)   
				{        
					bFinish = 1;
					break;            
				}
				else if(ssl_err == SSL_ERROR_WANT_READ)        
				{     
					break;    
				}       
				else if(ssl_err == SSL_ERROR_WANT_WRITE)      
				{      
					read_blocked_on_write=1;   
					break;   
				}       
				else  
				{    
					FD_ZERO(&fdread);
					FD_ZERO(&fdwrite);
					printf("ssl problem ! line:%d\n", __LINE__);   //SSL_PROBLEM
					return -1;
				}    
			}
		}
	}

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	RecvBuf[offset] = 0;
	if(offset == 0)
	{
		printf("connection closed!%d\n", __LINE__);   //CONNECTION_CLOSED
		return -1;
	}

	return 0;
}

static int ReceiveData(Command_Entry* pEntry)
{
	int ret = 0;
	if(m_ssl != NULL)
	{
		//printf("Ready to receive ssl data!\n");
		ret = ReceiveData_SSL(m_ssl, pEntry);
		if(ret != 0)
			return -1;
		else
			return 0;
	}
	int res = 0;
	fd_set fdread;
	struct timeval time;

	time.tv_sec = pEntry->recv_timeout;
	time.tv_usec = 0;

	//printf(".....func:%s...line:%d\n", __func__, __LINE__);

	assert(RecvBuf);

	if(RecvBuf == NULL)
		printf("recvbuf is empty!\n");   //RECVBUF_IS_EMPTY

	FD_ZERO(&fdread);

	FD_SET(gmail_socket,&fdread);

	if((res = select(gmail_socket+1, &fdread, NULL, NULL, &time)) == SOCKET_ERROR)
	{
		FD_CLR(gmail_socket,&fdread);
		printf("select error!\n");   //WSA_SELECT
		return -1;
	}

	if(!res)
	{
		//timeout
		FD_CLR(gmail_socket,&fdread);
		printf("server not responding!\n");   //SERVER_NOT_RESPONDING
		return -1;
	}

	if(FD_ISSET(gmail_socket,&fdread))
	{
		res = recv(gmail_socket,RecvBuf,BUFFER_SIZE,0);
		if(res == SOCKET_ERROR)
		{
			FD_CLR(gmail_socket,&fdread);
			printf("recv error!\n");   //WSA_RECV
			return -1;
		}
	}

	FD_CLR(gmail_socket,&fdread);
	RecvBuf[res] = 0;
	if(res == 0)
	{
		printf("connection closed!:%d\n", __LINE__);   //CONNECTION_CLOSED
		return -1;
	}

	return 0;
	//printf("===== %s Success! ======\n", __func__);
}

static int ReceiveResponse(Command_Entry* pEntry)
{
	//std::string line;
	char line[2048] = {0};
	int reply_code = 0;
	int bFinish = 0;
	int ret = 0;
	while(!bFinish)
	{
		ret = ReceiveData(pEntry);
		if(ret != 0)
		{
			printf("Receive Data failed!\n");
			return -1;
		}

		//line.append(RecvBuf);
		strcat(line, RecvBuf);
		int len = strlen(line);
		size_t begin = 0;
		size_t offset = 0;

		while(1) // loop for all lines
		{
			while(offset + 1 < len)
			{
				if(line[offset] == '\r' && line[offset+1] == '\n')
					break;
				++offset;
			}
			if(offset + 1 < len) // we found a line
			{
				// see if this is the last line
				// the last line must match the pattern: XYZ<SP>*<CRLF> or XYZ<CRLF> where XYZ is a string of 3 digits 
				offset += 2; // skip <CRLF>
				if(offset - begin >= 5)
				{
					if(isdigit(line[begin]) && isdigit(line[begin+1]) && isdigit(line[begin+2]))
					{
						// this is the last line
						if(offset - begin == 5 || line[begin+3] == ' ')
						{
							reply_code = (line[begin]-'0')*100 + (line[begin+1]-'0')*10 + line[begin+2]-'0';
							bFinish = 1;
							break;
						}
					}
				}
				begin = offset; // try to find next line
			}
			else // we haven't received the last line, so we need to receive more data 
			{
				break;
			}
		}
	}
	////strcpy_s(RecvBuf, BUFFER_SIZE, line.c_str());
	strcpy(RecvBuf, line);
	//OutputDebugStringA(RecvBuf);
	if(reply_code != pEntry->valid_reply_code)
	{
		printf("reply_code:%d\n", reply_code); 
		printf("valid_reply_code:%d\n", pEntry->valid_reply_code);
		return -1;
	}

	return 0;
}

static void CleanupOpenSSL()
{   
	if(m_ssl != NULL)
	{
		SSL_shutdown (m_ssl);  /* send SSL/TLS close_notify */
		SSL_free (m_ssl);
		m_ssl = NULL;
	}

	if(m_ctx != NULL)
	{
		SSL_CTX_free (m_ctx);   
		m_ctx = NULL;
		ERR_free_strings();
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
	}
}

static void DisconnectRemoteServer()
{
	if(m_bConnected) 
		SayQuit();

	if(gmail_socket)
	{
		shutdown(gmail_socket, SHUT_RDWR);
		close(gmail_socket);
		gmail_socket = 0;
	}
	CleanupOpenSSL();
}


static int StartTls()
{
	int ret = 0;
	Command_Entry* pEntry = FindCommandEntry(command_STARTTLS);
	strcpy(SendBuf, "STARTTLS\r\n");
	SendData(pEntry);
	ret = ReceiveResponse(pEntry);
	if(ret!= 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	ret = OpenSSLConnect();
	if(ret < 0)
		return -1;

	return 0;
}

static int SayHello()
{
	Command_Entry* pEntry = FindCommandEntry(command_EHLO);
	sprintf(SendBuf, "EHLO %s\r\n", hk_email.mailserver);

	SendData(pEntry);
	if(ReceiveResponse(pEntry) != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	m_bConnected = 1;
	return 0;
}

int SayQuit()
{
	// ***** CLOSING CONNECTION *****
	Command_Entry* pEntry = FindCommandEntry(command_QUIT);
	strcpy(SendBuf, "QUIT\r\n");
	SendData(pEntry);
	if(ReceiveResponse(pEntry) != 0)
	{
		return -1;
	}
	m_bConnected = 0; 
	return 0;
}

static int InitOpenSSL()
{
	SSL_library_init();
	SSL_load_error_strings();
	m_ctx = SSL_CTX_new (SSLv23_client_method());
	if(m_ctx == NULL)
	{
		printf("m_ctx is null!\n");
		return -1;
	}
	return 0;
}

static int ConnectRemoteServer(const char* szServer, const unsigned short mPort, 
		int securityType, const char* login, const char* password)
{
	unsigned short nPort = 0;
	struct sockaddr_in sockAddr;
	unsigned long ul = 1;
	fd_set fdwrite,fdexcept;
	struct timeval timeout;
	int res = 0;

	timeout.tv_sec = TIME_IN_SEC;
	timeout.tv_usec = 0;

	gmail_socket = INVALID_SOCKET;

	if ((gmail_socket = socket(AF_INET, SOCK_STREAM,0)) == INVALID_SOCKET)
	{
		printf("invalid socket!\n");   //WSA_INVALID_SOCKET
		return -1;
	}

	if (mPort != 0)
		nPort = htons(mPort);
	else
		nPort = htons(25);

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = nPort;

	if ((sockAddr.sin_addr.s_addr = inet_addr(szServer)) == INADDR_NONE)
	{
		struct hostent* host;

		host = gethostbyname(szServer);

		if (host)
			memcpy(&sockAddr.sin_addr,host->h_addr_list[0],host->h_length);
		else
		{
			shutdown(gmail_socket, SHUT_RDWR);
			close(gmail_socket);
			printf("Gethostbyname address error!\n");  //WSA_GETHOSTBY_NAME_ADDR
			return -1;
		}               
	}

	if (ioctl(gmail_socket, FIONBIO, (unsigned long*)&ul) == SOCKET_ERROR)
	{
		shutdown(gmail_socket, SHUT_RDWR);
		close(gmail_socket);
		printf("ioctl socket error!\n");   //WSA_IOCTLSOCKET
		return -1;
	}

	if (connect(gmail_socket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		if (errno != EINPROGRESS)
		{
			shutdown(gmail_socket, SHUT_RDWR);
			close(gmail_socket);
			printf("connect error!\n");   //WSA_CONNECT
			return -1;
		}
	}
	else
		return 0;

	while(1)
	{
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdexcept);

		FD_SET(gmail_socket, &fdwrite);
		FD_SET(gmail_socket, &fdexcept);

		if ((res = select(gmail_socket+1, NULL, &fdwrite, &fdexcept, &timeout)) == SOCKET_ERROR)
		{
			shutdown(gmail_socket, SHUT_RDWR);
			close(gmail_socket);
			printf("select error!\n");   //WSA_SELECT
			return -1;
		}

		if (!res)
		{
			shutdown(gmail_socket, SHUT_RDWR);
			close(gmail_socket);
			printf("select timeout error!\n");   //SELECT_TIMEOUT
			return -1;
		}
		if (res && FD_ISSET(gmail_socket, &fdwrite))
			break;

		if (res && FD_ISSET(gmail_socket, &fdexcept))
		{
			shutdown(gmail_socket, SHUT_RDWR);
			close(gmail_socket);
			printf("select error!\n");   //WSA_SELECT
			return -1;
		}
	} //end while

	FD_CLR(gmail_socket, &fdwrite);
	FD_CLR(gmail_socket, &fdexcept);

	if (securityType != DO_NOT_SET)
		m_type = securityType;

	if ((m_type == USE_TLS) || (m_type == USE_SSL))
	{

		res = InitOpenSSL();

		if (res != 0)
		{
			printf("Init OpenSSL failed!\n");
			shutdown(gmail_socket, SHUT_RDWR);
			close(gmail_socket);
			return -1;
		}

		if (m_type == USE_SSL)
		{
			printf("SSL Open SSL Connection! \n");
			res = OpenSSLConnect();

			if(res != 0)
			{
				CleanupOpenSSL();
				shutdown(gmail_socket, SHUT_RDWR);
				close(gmail_socket);
				printf("Open SSL Connect failed!\n");
				return -1;
			}
		}
	}

	Command_Entry* pEntry = FindCommandEntry(command_INIT);

	if (ReceiveResponse(pEntry) != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	if (SayHello() < 0)
	{
		CleanupOpenSSL();
		return -1;
	}

	if (m_type == USE_TLS)
	{

		if (StartTls() < 0)
		{
			CleanupOpenSSL();
			return -1;
		}

		if (SayHello() < 0)
		{
			CleanupOpenSSL();
			return -1;
		}

	}

	pEntry = FindCommandEntry(command_AUTHLOGIN);

	strcpy(SendBuf, "AUTH LOGIN\r\n");
	SendData(pEntry);
	if (ReceiveResponse(pEntry) != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	// send login:
	char *encoded_login = base64_encode(login, strlen(login));
	pEntry = FindCommandEntry(command_USER);

	sprintf(SendBuf, "%s\r\n", encoded_login);
	SendData(pEntry);
	if (ReceiveResponse(pEntry) != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	// send password:
	char *encoded_password = base64_encode(password, strlen(password));
	pEntry = FindCommandEntry(command_PASSWORD);
	sprintf(SendBuf, "%s\r\n", encoded_password);
	SendData(pEntry);
	if (ReceiveResponse(pEntry) != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	return 0;
}

static void InitAllVariable()
{
	gmail_socket = 0;
	m_bConnected = 0;
	m_type = NO_SECURITY;
	m_ctx = NULL;
	m_ssl = NULL;

	memset(SendBuf, 0, sizeof(SendBuf));
	memset(RecvBuf, 0, sizeof(RecvBuf));
}


/*************************************************************************
 * func: sending email in ssl security.
 ************************************************************************/
int send_ssl_email(char *smtp_server, char *passwd, char *send_from, const char *send_to,
		char *smtp_user, char *body, int iType, int mport, int secType)
{
	printf("Start Send ssl EMail!\n");

	unsigned int i = 0, rcpt_count = 0, res = 0, FileId = 0;
	char *FileBuf[55] = {0}; 
	char FileName[16] = {0};
	FILE* hFile = NULL;
	int retValue = 0;
	unsigned long int FileSize = 0, TotalSize = 0, MsgPart = 0;
	//char username[MSGLENGTH];
	//char *ptr;

	InitAllVariable();

#if 0
	strcpy(username, send_from);
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

	// ***** CONNECTING TO SMTP SERVER *****
	// connecting to remote host if not already connected:


	if ( secType == 2 ) // 587
	{
		secType = USE_TLS;
		//mport = 25;
		printf("mport:%d secType:USE_TLS\n", mport);
	}
	else if ( secType == 1) // 465
	{
		//mport = 25;
		secType = USE_SSL;
		printf("mport:%d secType:USE_SSL\n", mport);
	}
	else
	{
		printf("mport error!\n");
		return -1;
	}

	if (gmail_socket == 0)
	{
		if (ConnectRemoteServer(smtp_server, mport, secType, smtp_user, passwd) != 0)
		{
			m_bConnected = 0; 
			DisconnectRemoteServer();
			printf("Connect Romote Server error!\n");  //WSA_INVALID_SOCKET
			return -1;
		}
	}

	if (strlen(send_from) <= 0)
	{
		printf("undef mail from!\n");   //UNDEF_MAIL_FROM
		DisconnectRemoteServer();
		return -1;
	}

	Command_Entry* pEntry = FindCommandEntry(command_MAILFROM);
	sprintf(SendBuf, "MAIL FROM:<%s>\r\n", send_from);
	SendData(pEntry);
	retValue = ReceiveResponse(pEntry);
	if (retValue != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	// RCPT <SP> TO:<forward-path> <CRLF>
	if (!(rcpt_count = strlen(send_to)))
	{
		printf("undef recipients!\n");   //UNDEF_RECIPIENTS
		return -1;
	}

	pEntry = FindCommandEntry(command_RCPTTO);
	sprintf(SendBuf, "RCPT TO:<%s>\r\n", send_to);
	SendData(pEntry);

	retValue = ReceiveResponse(pEntry);
	if (retValue != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	pEntry = FindCommandEntry(command_DATA);
	strcpy(SendBuf, "DATA\r\n");
	SendData(pEntry);
	retValue = ReceiveResponse(pEntry);
	if (retValue != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}

	pEntry = FindCommandEntry(command_DATABLOCK);
	// send header(s)
	FormatHeader(SendBuf);
	SendData(pEntry);

	sprintf(SendBuf, "%s\r\n", body);
	SendData(pEntry);

	// next goes attachments (if they are)

	for (FileId = 0; (iType == 1) && (FileId < hk_email.mcount); FileId++)
	{
		printf("Ready to send picture:%d\n", FileId + 1);
		sprintf(FileName, "notice%d.jpg", FileId+1);

		sprintf(SendBuf, "--%s\r\n", BOUNDARY_TEXT);
		//strcat(SendBuf, "Content-Type: application/x-msdownload; name=\"");
		strcat(SendBuf,"Content-Type: image/jpeg; name=\"");
		strcat(SendBuf, FileName);
		strcat(SendBuf, "\"\r\n");
		strcat(SendBuf, "Content-Transfer-Encoding: base64\r\n");
		strcat(SendBuf, "Content-Disposition: attachment; filename=\"");
		strcat(SendBuf, FileName);
		strcat(SendBuf, "\"\r\n");
		strcat(SendBuf, "\r\n");

		SendData(pEntry);

		//// opening the file:
		//hFile = fopen("/mnt/sif/bin/test1.jpg","r");
		//if(hFile == NULL)
		//    printf("file not exist1\n");   //FILE_NOT_EXIST

		//// checking file size:
		//FileSize = 0;
		//while(!feof(hFile))
		//    FileSize += fread(FileBuf,sizeof(char),54,hFile);
		//TotalSize += FileSize;
		//printf("TotalSize:%d\n", TotalSize);

		//// sending the file:
		//if(TotalSize/1024 > MSG_SIZE_IN_MB*1024)
		//    printf("msg too big!\n");   //MSG_TOO_BIG
		//if(imgSize[i] > ALLIMGBUF)
		//    printf("Image is too big!\n");
		//else
		//{
		//    //fseek (hFile,0,SEEK_SET);


		MsgPart = 0;
		char *pData = g_JpegBuffer[FileId];
		for (i = 0; i < imgSize[FileId]/54+1; i++)
		{
			if (i == imgSize[FileId]/54)
				res = imgSize[FileId] - i * 54;
			else
				res = 54;

			MsgPart ? strcat(SendBuf,base64_encode(pData,res)) : strcpy(SendBuf,base64_encode(pData, res)); 
			pData += res;
			strcat(SendBuf, "\r\n");
			MsgPart += res + 2;
			if (MsgPart >= BUFFER_SIZE/2)
			{ // sending part of the message
				MsgPart = 0;
				SendData(pEntry); // FileBuf, FileName, fclose(hFile);
			}
		}

		if (MsgPart)
		{
			SendData(pEntry); // FileBuf, FileName, fclose(hFile);
		}
		//}
		//fclose(hFile);
	} 

	sprintf(SendBuf, "\r\n--%s--\r\n", BOUNDARY_TEXT);
	SendData(pEntry);

	pEntry = FindCommandEntry(command_DATAEND);
	strcpy(SendBuf, "\r\n.\r\n");
	SendData(pEntry);
	retValue = ReceiveResponse(pEntry);
	if (retValue != 0)
	{
		DisconnectRemoteServer();
		return -1;
	}


	if (m_bConnected == 1)
	{
		DisconnectRemoteServer();
	}
	CleanupOpenSSL();

	return 0;
}
