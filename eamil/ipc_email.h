#ifndef __HK_EMAIL_H__
#define __HK_EMAIL_H__

#define MSGLENGTH  64
#define MAXLINE    256 
#define MAXIMGNUM  3 //snapshot <= 3 pictures.
#define ALLIMGBUF  350*1024 //total image buffer size.
//#define BUFLENGTH  1024


typedef struct emailInfo
{
    char mailserver[128];
    char mailfrom[128];
    char mailto[128];
    char username[128];
    char passwd[128];
    int  port;
    int  isOpen;
    int  mcount;
    int  secType;
}HKEMAIL_T;

#define SDMSG         "Micro SD ERROR!"
#define MSG           "Your monitor is abnormal!"
#define MAIL_SUBJECT  "Home Security Message"

#define HK_DEBUG(args...)   printf(args)
#define HK_NOTICE(args...)  printf(args)
#define HK_SUCCESS(args...) printf(args)


void InitEmailInfo(int isOpen,char *mailserver, char *sendEmail, char *recvEmail, char *smtpUser, char *smtpPaswd, int emailPort, int count,int secType);

int send_mail_to_user(char *smtp_server,char *passwd, char *send_from,const char *send_to, char *smtp_user, char *body, int iType);

#endif /*__HK_EMAIL_H__ */
