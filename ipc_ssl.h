#ifndef __IPC_SSL_H__
#define __IPC_SSL_H__

#define INVALID_SOCKET  (unsigned int)(~0)
#define SOCKET_ERROR -1
#define TLSPORT   587
#define SSLPORT   465

#define MSG_SIZE_IN_MB 5
#define BUFFER_SIZE 10240
#define TIME_IN_SEC  3*60
#define BOUNDARY_TEXT "__MESSAGE__ID__54yg6f6h6y456345"

enum
{
    command_INIT,
    command_EHLO,
    command_AUTHPLAIN,
    command_AUTHLOGIN,
    command_AUTHCRAMMD5,
    command_AUTHDIGESTMD5,
    command_DIGESTMD5,
    command_USER,
    command_PASSWORD,
    command_MAILFROM,
    command_RCPTTO,
    command_DATA,
    command_DATABLOCK,
    command_DATAEND,
    command_QUIT,
    command_STARTTLS
};

enum CSmtpError
{
    CSMTP_NO_ERROR = 0,
    WSA_STARTUP = 100, // WSAGetLastError()
    WSA_VER,
    WSA_SEND,
    WSA_RECV,
    WSA_CONNECT,
    WSA_GETHOSTBY_NAME_ADDR,
    WSA_INVALID_SOCKET,
    WSA_HOSTNAME,
    WSA_IOCTLSOCKET,
    WSA_SELECT,
    BAD_IPV4_ADDR,
    UNDEF_MSG_HEADER = 200,
    UNDEF_MAIL_FROM,
    UNDEF_SUBJECT,
    UNDEF_RECIPIENTS,
    UNDEF_LOGIN,
    UNDEF_PASSWORD,
    BAD_LOGIN_PASSWORD,
    BAD_DIGEST_RESPONSE,
    BAD_SERVER_NAME,
    UNDEF_RECIPIENT_MAIL,
    COMMAND_MAIL_FROM = 300,
    COMMAND_EHLO,
    COMMAND_AUTH_PLAIN,
    COMMAND_AUTH_LOGIN,
    COMMAND_AUTH_CRAMMD5,
    COMMAND_AUTH_DIGESTMD5,
    COMMAND_DIGESTMD5,
    COMMAND_DATA,
    COMMAND_QUIT,
    COMMAND_RCPT_TO,
    MSG_BODY_ERROR,
    CONNECTION_CLOSED = 400, // by server
    SERVER_NOT_READY, // remote server
    SERVER_NOT_RESPONDING,
    SELECT_TIMEOUT,
    FILE_NOT_EXIST,
    MSG_TOO_BIG,
    BAD_LOGIN_PASS,
    UNDEF_XYZ_RESPONSE,
    LACK_OF_MEMORY,
    TIME_ERROR,
    RECVBUF_IS_EMPTY,
    SENDBUF_IS_EMPTY,
    OUT_OF_MSG_RANGE,
    COMMAND_EHLO_STARTTLS,
    SSL_PROBLEM,
    COMMAND_DATABLOCK,
    STARTTLS_NOT_SUPPORTED,
    LOGIN_NOT_SUPPORTED
};

typedef struct tagCommand_Entry
{
    int command;
    int send_timeout;     // 0 means no send is required
    int recv_timeout;     // 0 means no recv is required
    int valid_reply_code; // 0 means no recv is required, so no reply code
    int error;
}__attribute__((packed)) Command_Entry;


SSL_CTX* m_ctx;
SSL* m_ssl;

static unsigned char base64_chars[] ={"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

/*************************************************************************
 * func: sending email in ssl security.
 ************************************************************************/
int send_ssl_email(char *smtp_server, char *passwd, char *send_from, const char *send_to,
                   char *smtp_user, char *body, int iType, int mport, int secType);
#endif
