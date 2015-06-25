#ifndef __HK_EMAIL_H__
#define __HK_EMAIL_H__

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

#define MSGLENGTH  64
#define MAXLINE    256 
#define MAXIMGNUM  3 //snapshot <= 3 pictures.
#define ALLIMGBUF  350*1024 //total image buffer size.
#define BUFLENGTH  1024


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


//#define MAILSERVER   "smtp.163.com"
//#define FROM         "hkemail_test@163.com"
//#define FROM         "hkmail_test@163.com"
//#define PASSWD       "hkemail*test"

/*
#define MAILSERVER   "mail.hekaikeji.com"
#define FROM         "hebaohu@hekaikeji.com"
#define TO           "tiger@hekaikeji.com"
#define USER         "hebaohu"
#define PASSWD       "123456"
#define PORT         25
*/

#define SDMSG         "Micro SD ERROR!"
#define MSG           "Your monitor is abnormal!"
#define MAIL_SUBJECT  "Home Security Message"

#define HK_DEBUG(args...)   printf(args)
#define HK_NOTICE(args...)  printf(args)
#define HK_SUCCESS(args...) printf(args)

//extern void getEmailInfo(void);
//#define   CHK_NULL(x)   if   ((x)==NULL)   exit   (1)
//#define   CHK_ERR(err,s)   if   ((err)==-1)   {   perror(s);   exit(1);   }
//#define   CHK_SSL(err)   if   ((err)==-1)   {   ERR_print_errors_fp(stderr);   exit(2);   }

void InitEmailInfo(int isOpen,char *mailserver, char *sendEmail, char *recvEmail, char *smtpUser, char *smtpPaswd, int emailPort, int count,int secType);

int send_mail_to_user(char *smtp_server,char *passwd, char *send_from,const char *send_to, char *smtp_user, char *body, int iType);

#endif /*__HK_EMAIL_H__ */
