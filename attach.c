#include "ipc_email.h"
#include "ipc_ssl.h"

extern char SendBuf[BUFFER_SIZE];
extern int m_bAuthenticate;
extern int gmail_socket;

Command_Entry command_list[16]= 
{
    {command_INIT,          0,     5*60,  220, SERVER_NOT_RESPONDING},
    {command_EHLO,          5*60,  5*60,  250, COMMAND_EHLO},
    {command_AUTHPLAIN,     5*60,  5*60,  334, COMMAND_AUTH_PLAIN},
    {command_AUTHLOGIN,     5*60,  5*60,  334, COMMAND_AUTH_LOGIN},
    {command_AUTHCRAMMD5,   5*60,  5*60,  334, COMMAND_AUTH_CRAMMD5},
    {command_AUTHDIGESTMD5, 5*60,  5*60,  334, COMMAND_AUTH_DIGESTMD5},
    {command_DIGESTMD5,     5*60,  5*60,  335, COMMAND_DIGESTMD5},
    {command_USER,          5*60,  5*60,  334, UNDEF_XYZ_RESPONSE},
    {command_PASSWORD,      5*60,  5*60,  235, BAD_LOGIN_PASS},
    {command_MAILFROM,      5*60,  5*60,  250, COMMAND_MAIL_FROM},
    {command_RCPTTO,        5*60,  5*60,  250, COMMAND_RCPT_TO},
    {command_DATA,          5*60,  2*60,  354, COMMAND_DATA},
    {command_DATABLOCK,     3*60,  0,     0,   COMMAND_DATABLOCK},
    {command_DATAEND,       3*60,  10*60, 250, MSG_BODY_ERROR},
    {command_QUIT,          5*60,  5*60,  221, COMMAND_QUIT},
    {command_STARTTLS,      5*60,  5*60,  220, COMMAND_EHLO_STARTTLS}
};

Command_Entry* FindCommandEntry(int command)
{
    Command_Entry* pEntry = NULL;
    int i;

    for(i = 0; i < sizeof(command_list)/sizeof(command_list[0]); ++i)
    {
        if(command_list[i].command == command)
        {
            pEntry = &command_list[i];
            break;
        }
    }
    assert(pEntry != NULL);

    return pEntry;
}

static int SendData_SSL(SSL* ssl, Command_Entry* pEntry)
{
    int offset = 0;
    int res = 0;
    int nLeft = strlen(SendBuf);
    fd_set fdwrite;
    fd_set fdread;
    struct timeval time;

    int write_blocked_on_read = 0;

    time.tv_sec = pEntry->send_timeout;
    time.tv_usec = 0;

    assert(SendBuf);

    if(SendBuf == NULL ||  gmail_socket == 0 )
    {
        printf("SendBuf is null!\n"); //SENDBUF_IS_EMPTY
        return -1;
    }

    while(nLeft > 0)
    {
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdread);

        FD_SET(gmail_socket,&fdwrite);

        if(write_blocked_on_read)
        {
            FD_SET(gmail_socket, &fdread);
        }

        if((res = select(gmail_socket+1,&fdread,&fdwrite,NULL,&time)) == SOCKET_ERROR)
        {
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdread);
            printf("select error!\n"); //WSA_SELECT;
            return -1;
        }

        if(!res)
        {
            //timeout
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdread);
            printf("Time out!\n"); //SERVER_NOT_RESPONDING
            return -1;
        }

        if(FD_ISSET(gmail_socket,&fdwrite) || (write_blocked_on_read && FD_ISSET(gmail_socket, &fdread)) )
        {
            write_blocked_on_read=0;

            /* Try to write */
            res = SSL_write(ssl, SendBuf+offset, nLeft);

            switch(SSL_get_error(ssl,res))
            {
                /* We wrote something*/
                case SSL_ERROR_NONE:
                    nLeft -= res;
                    offset += res;
                    break;

                    /* We would have blocked */
                case SSL_ERROR_WANT_WRITE:
                    break;

                    /* We get a WANT_READ if we're
                     * trying to rehandshake and we block on
                     * write during the current connection.
                     * We need to wait on the socket to be readable
                     * but reinitiate our write when it is */
                case SSL_ERROR_WANT_READ:
                    write_blocked_on_read=1;
                    break;

                    /* Some other error */
                default:        
                    FD_ZERO(&fdread);
                    FD_ZERO(&fdwrite);
                    break;
            }

        }
    }

    FD_ZERO(&fdwrite);
    FD_ZERO(&fdread);
    return 0;
}


int SendData(Command_Entry* pEntry)
{
    if( gmail_socket == 0 )
    {
           return -1;
    }
    if(m_ssl != NULL)
    {
        SendData_SSL(m_ssl, pEntry);
        return -1;
    }
    int idx = 0,nLeft = strlen(SendBuf);
    int res = 0;
    fd_set fdwrite;
    struct timeval time;

    time.tv_sec = pEntry->send_timeout;
    time.tv_usec = 0;

    assert(SendBuf);

    if(SendBuf == NULL)
    {
        printf("SendBuf is null!\n");
        return -1;
    }

    while(nLeft > 0)
    {
        FD_ZERO(&fdwrite);

        FD_SET(gmail_socket,&fdwrite);

        if((res = select(gmail_socket+1,NULL,&fdwrite,NULL,&time)) == SOCKET_ERROR)
        {
            FD_CLR(gmail_socket,&fdwrite);
            printf("select error!\n");
            return -1;
        }

        if(!res)
        {
            //timeout
            FD_CLR(gmail_socket,&fdwrite);
            printf("timeout!\n");
            return -1;
        }

        //printf("gmail_socket:%d...return:%d\n", gmail_socket, FD_ISSET(gmail_socket, &fdwrite));
        if(res && FD_ISSET(gmail_socket,&fdwrite))
        {
            res = send(gmail_socket,&SendBuf[idx],nLeft,0);
            if(res == SOCKET_ERROR || res == 0)
            {
                FD_CLR(gmail_socket,&fdwrite);
                printf("send failed!\n");
                return -1;
            }
            nLeft -= res;
            idx += res;
        }
    }

    FD_CLR(gmail_socket,&fdwrite);

    return 0;
}



//std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) 
static char g_b64Code[128]={0};
char *base64_encode(unsigned char * bytes_to_encode,  int in_len) 
{
    //std::string ret;
    memset(g_b64Code, 0, sizeof(g_b64Code));
    int ret =0 ;
    int i = 0, j = 0;
    unsigned char char_array_3[3] = {0}, char_array_4[4] = {0};

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) 
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
            {
                g_b64Code[ret++] += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
        {
            g_b64Code[ret++] += base64_chars[char_array_4[j]];
        }
        while((i++ < 3))
            g_b64Code[ret++] += '=';

    }
    g_b64Code[ret]='\0';

    //printf("g_b64Code:%s\n", g_b64Code);
    return g_b64Code;
}

/*
#if 0
int send_attach(SSL *ssl)
{

    printf(".....func:%s...line:%d..\n", __func__, __LINE__);
    char SendBuf[1024] = {0};
    char *FileName = "notice.jpg";
    char FileBuf[55]={0}; 
    int FileSize = 0;
    int TotalSize = 0;
    int MsgPart = 0;

    int i, res;
    for(i=0;i<hk_email.mcount;i++)
    {
        //strcpy(FileName,Attachments[FileId].c_str());

        sprintf(SendBuf,"--%s\r\n",BOUNDARY_TEXT);
        strcat(SendBuf,"Content-Type: application/x-msdownload; name=\"");
        strcat(SendBuf,FileName);
        strcat(SendBuf,"\"\r\n");
        strcat(SendBuf,"Content-Transfer-Encoding: base64\r\n");
        strcat(SendBuf,"Content-Disposition: attachment; filename=\"");
        strcat(SendBuf,FileName);
        strcat(SendBuf,"\"\r\n");
        strcat(SendBuf,"\r\n");

        send_line(ssl, SendBuf);
        recv_line(ssl);

        printf(".....func:%s...line:%d..\n", __func__, __LINE__);
        // opening the file:
        //hFile = fopen(FileName,"rb");
        //if(hFile == NULL)
        //    printf("...FILE_NOT_EXIST\n");

        // checking file size:
        // sending the file:

        if(imgSize[i]/1024 > MSG_SIZE_IN_MB*1024)
            printf("...Image too big...\n");
        else
        {
            //fseek (hFile,0,SEEK_SET);
            for(i=0;i<FileSize/54+1;i++)
            {
                MsgPart = 0;
                //res = fread(FileBuf,sizeof(char),54,hFile);
                res = 54;

                MsgPart ? strcat(SendBuf,base64_encode(gJpegBuffer[i],res)) : strcpy(SendBuf,base64_encode(gJpegBuffer[i],res));
                strcat(SendBuf,"\r\n");
                MsgPart += res + 2;
                if(MsgPart >= BUFFER_SIZE/2)
                { // sending part of the message
                    MsgPart = 0;
                    //SendData(pEntry); 
                    printf("...size:...%d.....line:%d.\n", strlen(SendBuf), __LINE__);
                    send_line(ssl, SendBuf);
                    recv_line(ssl);
                }
            }
            if(MsgPart)
            {
                printf("...size:...%d.....line:%d.\n", strlen(SendBuf), __LINE__);
                //SendData(pEntry);
                send_line(ssl, SendBuf);
                recv_line(ssl);
            }
        }
    }

    if(hk_email.mcount)
    {
        sprintf(SendBuf,"\r\n--%s--\r\n",BOUNDARY_TEXT);
        //SendData(pEntry);
        send_line(ssl, SendBuf);
        recv_line(ssl);
    }

    // <CRLF> . <CRLF>
    strcpy(SendBuf, "\r\n.\r\n");
    //SendData(pEntry);
    //ReceiveResponse(pEntry);
    send_line(ssl, SendBuf);
    recv_line(ssl);

    ////char FileName[15] = {0};
    //char *FileName = "notice.jpg";
    //
    //
    ////strcpy(FileName,Attachments[FileId].c_str());

    //sprintf(SendBuf,"--%s\r\n",BOUNDARY_TEXT);
    //strcat(SendBuf,"Content-Type: application/x-msdownload; name=\"");
    //strcat(SendBuf,FileName);
    //strcat(SendBuf,"\"\r\n");
    //strcat(SendBuf,"Content-Transfer-Encoding: base64\r\n");
    //strcat(SendBuf,"Content-Disposition: attachment; filename=\"");
    //strcat(SendBuf,FileName);
    //strcat(SendBuf,"\"\r\n");
    //strcat(SendBuf,"\r\n");

    return 0;
}
#endif
*/

#if 0
int send_attach(SSL *ssl, int sockfd)
{
    //char SendBuf[1024];
    printf(".....func:%s...line:%d..\n", __func__, __LINE__);
    int MsgPart = 0;
    int i;
    int res;

    FILE* hFile = NULL;
    Command_Entry* pEntry = FindCommandEntry(command_DATABLOCK);

    printf(".....func:%s...line:%d..\n", __func__, __LINE__);
    char FileBuf[55] = {0};
    //char FileName[20] = {0};
    char *FilePath = "./test1.jpg";
    char *FileName = "test1.jpg";
    int TotalSize = 0;
    int FileId;

    printf(".....func:%s...line:%d..\n", __func__, __LINE__);
    for(FileId=0; FileId<hk_email.mcount; FileId++)
    {
        //strcpy(FileName,"test1.jpg");
        sprintf(SendBuf,"--%s\r\n",BOUNDARY_TEXT);
        strcat(SendBuf,"Content-Type: application/x-msdownload; name=\"");
        strcat(SendBuf,FileName);
        strcat(SendBuf,"\"\r\n");
        strcat(SendBuf,"Content-Transfer-Encoding: base64\r\n");
        strcat(SendBuf,"Content-Disposition: attachment; filename=\"");
        strcat(SendBuf,FileName);
        strcat(SendBuf,"\"\r\n");
        strcat(SendBuf,"\r\n");
        printf("..SendBuf%s\n", SendBuf);

        //SendData(pEntry, sockfd);

    printf("......error...%d\n", __LINE__); //MSG_TOO_BIG

        //opening the file:
        hFile = fopen(FilePath,"rb");
        if(hFile == NULL)
            printf("......error...%d\n", __LINE__); //FILE_NOT_EXIST

        //checking file size:
        int FileSize = 0;
        while(!feof(hFile))
        FileSize += fread(FileBuf,sizeof(char),54,hFile);
        TotalSize += FileSize;

        //sending the file:
        if(TotalSize/1024 > MSG_SIZE_IN_MB*1024)
            printf("......error...%d\n", __LINE__); //MSG_TOO_BIG
        else
        {
            fseek (hFile,0,SEEK_SET);

    printf("......error...%d\n", __LINE__); //MSG_TOO_BIG
            
            for(i=0;i<FileSize/54+1;i++)
            {
                res = fread(FileBuf,sizeof(char),54,hFile);
                MsgPart ? strcat(SendBuf,base64_encode(FileBuf,res))
                    : strcpy(SendBuf,base64_encode(FileBuf,res));
                strcat(SendBuf,"\r\n");
                MsgPart += res + 2;
                if(MsgPart >= BUFFER_SIZE/2)
                {  //sending part of the message
                    MsgPart = 0;
                    //SendData(pEntry, sockfd);  //FileBuf, FileName, fclose(hFile); 
                }
            }
            if(MsgPart)
            {
                //SendData(pEntry, sockfd);  //FileBuf, FileName, fclose(hFile);
            }
        }
        fclose(hFile);
    }

    return 0;
}
#endif

