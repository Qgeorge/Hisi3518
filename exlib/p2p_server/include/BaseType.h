
#ifndef _BASE_TYPE_H_
#define _BASE_TYPE_H_
#include <stdarg.h>

#ifdef _WIN32
#if !defined(_WINSOCKAPI_) && !defined(_WINSOCK2API_)
#include<winsock2.h>
#include<MSWSock.h>
#endif
#include<sys/stat.h>
#include<io.h>
#include<time.h>
#include<sys/timeb.h>
#include <mstcpip.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>  
#include <sys/timeb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>             /* ioctl command */
#include <netinet/if_ether.h>      /* ethhdr struct */
#include <net/if.h>                /* ifreq struct */
#include <netinet/in.h>             /* in_addr structure */
#include <netinet/ip.h>             /* iphdr struct */
#include <netinet/udp.h>            /* udphdr struct */
#include <netinet/tcp.h>            /*tcphdr struct */
#endif

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#define UDP_CONNECT
#ifdef UDP_CONNECT
#define UDP_IPADDR   0x7f000001   
#define UDP_PORT     0x5555     
#endif

#ifdef _WIN32  
#define DLL_DECLARE __declspec(dllexport)  
#else  
#define DLL_DECLARE  
#endif 


#ifndef FAST
#define FAST 
#endif


typedef char					CHAR;
typedef unsigned char			UCHAR;
typedef signed char				INT8;
typedef unsigned char			UINT8;
typedef signed short			INT16;
typedef unsigned short			UINT16;
typedef signed int				INT32;
typedef unsigned int			UINT32;

#ifndef _WIN32
typedef unsigned long long		UINT64;
typedef long long		INT64;
#else
typedef unsigned __int64      UINT64;
typedef __int64           INT64;
#endif

typedef intptr_t        int_t;
typedef uintptr_t       uint_t;


#ifdef _WIN32
   typedef HANDLE pthread_t;
   typedef HANDLE pthread_mutex_t;
   typedef HANDLE pthread_cond_t;
#endif

#if 1
#ifndef P2P_DBG
#define P2P_DBG
#endif
#endif

#ifdef P2P_DBG
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h> 
#define LOG_TAG "P2PLib"
#define dbgmsg(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define dbgmsg		cr_printf
#endif
#else
#define dbgmsg      //cr_printf
#endif

#define  TEST_COMPANY  0
#define  VERSION_NUM  6

//#define  TRACE
#define		CRE 	"^M^[[K"                                                                    
#define 	NORMAL	"[0;39m"                                                               
#define		RED		"[1;31m"                                                                  
#define		GREEN	"[1;32m"                                                                
#define		YELLOW	"[1;33m"                                                               
#define		BLUE	"[1;34m"                                                                 
#define		MAGENTA	"[1;35m"                                                              
#define		CYAN	"[1;36m"                                                                 
#define		WHITE	"[1;37m"



#ifndef _WIN32
#ifdef P2P_DBG
#define pthread_info()\
	do{\
		time_t tUniqueName = time(NULL);\
		printf(BLUE"%s"NORMAL, ctime(&tUniqueName));\
		printf(BLUE"PID = %d, PPID = %d,Thread Name: %s\r\n"NORMAL, getpid(),getppid(), __func__);\
		printf(BLUE"Created at line %d, file %s\r\n"NORMAL, __LINE__, __FILE__);\
		printf(BLUE"=================================================================================\r\n"NORMAL);\
	}while(0)
#else
#define pthread_info()
#endif
#endif




#ifndef _WIN32
#define SOCKET			  INT32
#define INVALID_SOCKET    -1
#endif
#ifdef _WIN32
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINAPIV     __cdecl
#define APIENTRY    WINAPI
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#else
#define CALLBACK    
#define WINAPI      
#define WINAPIV     
#define APIENTRY    
#define APIPRIVATE  
#define PASCAL      
#endif


#define MAX_ID_LEN      31
#define MAX_DEVNAME_LEN      63
#define MAX_SERVERNAME_LEN      63
#define MAX_FILENAME_LEN      127
#define thread_volatile   volatile
#define true            		1
#define false           		0

#ifdef _WIN32
#define P2PLock 			    CRITICAL_SECTION
#define InitP2PLock(lck)		InitializeCriticalSection(&(lck))
#define LockP2PLock(lck)		EnterCriticalSection(&(lck))
#define TryLockP2PLock(lck)		TryEnterCriticalSection(&(lck))
#define UnLockP2PLock(lck)	    LeaveCriticalSection(&(lck))
#else
#define P2PLock 			    pthread_mutex_t
#define InitP2PLock(lck)		pthread_mutex_init(&(lck), NULL)
#define LockP2PLock(lck)		pthread_mutex_lock(&(lck))
#define TryLockP2PLock(lck)		pthread_mutex_trylock(&(lck))
#define UnLockP2PLock(lck)	    pthread_mutex_unlock(&(lck))
#endif


#define MAX(a, b)           (((a) > (b)) ? (a) : (b))
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#define IPPORT(ip, port) ((ip) >> 0) & 0xFF, ((ip) >> 8) & 0xFF, ((ip) >> 16) & 0xFF, ((ip) >> 24) & 0xFF, ntohs((port))
#define IP(ip) ((ip) >> 0) & 0xFF, ((ip) >> 8) & 0xFF, ((ip) >> 16) & 0xFF, ((ip) >> 24) & 0xFF
//#define repeat(a)   a; a; do { } while(0)
#define repeat(a)   a;
#define repeat2(a)   a; a; do { } while(0)

#ifdef _WIN32
#define write2(s,b,l)           send((s),((char *)b),l,0)
#define read2(s,b,l)            recv((s),((char *)b),l,0)
#define close2(s)               closesocket((s)); (s) = (SOCKET) -1; do { } while(0)
#else
#define write2(s,b,l)           write((s),((char *)b),l)
#define read2(s,b,l)            read((s),((char *)b),l)
#define close2(s)               close((s)); (s) = (SOCKET) -1; do { } while(0)
#endif
#ifdef _WIN32
#define snprintf _snprintf
#endif
#define MAX_CLIENTS     4096
#define MAX_BUF_LEN     64
#define MAX_IP_STR_LEN	23

#define STREAMSIZE 1350
//#define STREAMSIZE 500

#define SERVER_REPLY_NUM  4

#define TEST_DEBUG	0

#ifdef _WIN32
#define act_on_error(x, s, a)                                                               \
    errno = 0;                                                                              \
    if ((x) != 0) {                                                                        \
		INT32 __act_errno;																	\
	    char *__act_error;																	\
		__act_errno = errno == 0 ? WSAGetLastError() : errno;								\
		__act_error = strerror(__act_errno);												\
        dbgmsg("%s:%d: %s ... ERROR(%d)\n", __FILE__, __LINE__, (s), __act_errno);	            \
        if (__act_errno != 0) {                                                             \
            dbgmsg("%s:%d: ERROR(%d): %s\n", __FILE__, __LINE__, __act_errno, __act_error);		\
        }                                                                                   \
        a;                                                                                  \
    } else if (TEST_DEBUG) {                                                                 \
        dbgmsg("%s:%d: %s ... ok\n", __FILE__, __LINE__, (s));					\
    }                                                                                       \
    do {                                                                                    \
    } while(false)
#else
#define act_on_error(x, s, a)																\
	if ((x) != 0) {                                                                        \
        dbgmsg("%s:%d: %s ... ERROR(%d)\n",__FILE__, __LINE__, (s), errno);	            \
        if (errno != 0) {                                                             \
            dbgmsg("%s:%d: ERROR(%d)\n", __FILE__, __LINE__, errno);		\
        }                                                                                   \
        a;                                                                                  \
    } else if (TEST_DEBUG) {                                                                 \
        dbgmsg("%s:%d: %s ... ok\n", __FILE__, __LINE__, (s));					\
    }                                                                                       \
    do {                                                                                    \
    } while(false)
#endif

#define readtimeout(s, socks, t, g)     \
    do {                                \
        Timeout.tv_sec = (t);           \
        Timeout.tv_usec = 0;            \
        FD_ZERO(&(socks));              \
        FD_SET((s), &(socks));          \
        if (select(1+(INT32)(s), &(socks), NULL, NULL, &Timeout) <= 0) goto g;    \
    } while(false)



typedef struct 
{
	CHAR m_cNatType[32];   //NATÀàÐÍ
	CHAR m_cGloabIP[32];    //¹«ÍøIP
	CHAR m_cLocalIP[32];    // ¾ÖÓòÍøIP    
	//CHAR m_cPeerID[32];    // ½ÚµãID  
}peer_infor;

static INT32 CheckIP(char *pchIP) 
{
    UINT32 i = 0,nCount = 0;
    for(i = 0; i < strlen(pchIP); i++) 
    {
        if (pchIP[i] == '.') 
        	nCount++;
    }
    if (nCount != 3) 
    	return -1;
    #ifdef _WIN32
    if (inet_addr(pchIP) == INADDR_NONE) 
    	return -1;
    #else
    if (inet_aton(pchIP, NULL) == 0) 
    	return -1;
    #endif
    return 0;
}

static void XUSleep(UINT64 nMSec)
{ 
    #if 0
    #ifdef _WIN32
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval tv;
	fd_set rdset;
    FD_ZERO(&rdset);
    FD_SET(sock, &rdset);
	tv.tv_sec = nMSec/1000000;
	tv.tv_usec = nMSec%1000000;
	select(0, &rdset, NULL, NULL, &tv);
	close2(sock);
	#else
    struct timeval tv;
	tv.tv_sec = nMSec/1000000;
	tv.tv_usec = nMSec%1000000;
	select(0, NULL, NULL, NULL, &tv);
	#endif
	#endif    
	#ifdef _WIN32
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval tv;
	fd_set rdset;
    FD_ZERO(&rdset);
    FD_SET(sock, &rdset);
	tv.tv_sec = 0;
	tv.tv_usec = nMSec;
	select(0, &rdset, NULL, NULL, &tv);
	close2(sock);
	#else
    struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = nMSec;
	select(0, NULL, NULL, NULL, &tv);
	#endif

}


static void XSleep(INT32 nSec, INT32 nUSec)
{
    #ifdef _WIN32
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct timeval tv;
	fd_set rdset;
    FD_ZERO(&rdset);
    FD_SET(sock, &rdset);
	tv.tv_sec = nSec;
	tv.tv_usec = nUSec;
	select(0, &rdset, NULL, NULL, &tv);
	close2(sock);
	#else    
	struct timeval tv;
	tv.tv_sec = nSec;
	tv.tv_usec = nUSec;
	select(0, NULL, NULL, NULL, &tv);
	#endif
}

static INT32 XGetErrno(void)
{
#ifdef _WIN32
	return (WSAGetLastError());
#else
	return errno;
#endif
}

static void CTimerRdtsc(UINT64 *x) 
{
   #ifdef _WIN32
   struct timeval t;
      if (!QueryPerformanceCounter((LARGE_INTEGER *)x))
      {
         gettimeofday(&t, 0);
         *x = t.tv_sec * 1000000 + t.tv_usec;
      }
   #else
      // use system call to read time clock for other archs
      struct timeval t;
      gettimeofday(&t, 0);
      *x = t.tv_sec * 1000000 + t.tv_usec;
   #endif
   //TODO: add machine instrcutions for different archs
}

static UINT64 CTimerSec(void) 
{
    UINT64 x=0;
   #ifdef _WIN32
   struct timeval t;
      if (!QueryPerformanceCounter((LARGE_INTEGER *)&x))
      {
         gettimeofday(&t, 0);
         x = t.tv_sec;
      }
   #else
      // use system call to read time clock for other archs
      struct timeval t;
      gettimeofday(&t, 0);
      x = t.tv_sec;
   #endif
   return x;
   //TODO: add machine instrcutions for different archs
}
static INT32 cr_printf(char* pszfmt, ...)
{
        va_list struAp;
        time_t now;

        struct tm tm_now;

        char tmp[64];   

        int ret = 0;
        char format[512];
    
        if(pszfmt == NULL) 
        {
            return -1;
        }

		if((strlen(pszfmt)+3)>512)
		{
			return -2;
		}
		memset(format,0,sizeof(format)); 
        strcpy(format,pszfmt);
        if(format[strlen(pszfmt)-1]=='\n')
        {
            format[strlen(pszfmt)] = format[strlen(pszfmt)-2]=='\r' ?'\0':'\r';
        }
        else if(format[strlen(pszfmt)-1]=='\r')
        {
            format[strlen(pszfmt)] = format[strlen(pszfmt)-2]=='\n' ?'\0':'\n';
        }
        else
        {
            format[strlen(pszfmt)] ='\r';
            format[strlen(pszfmt)+1] ='\n';
        }

        
        #ifdef _WIN32
        now= time(0);   
        strftime( tmp, sizeof(tmp), "[%Y/%m/%d %X]",localtime(&now)); 
        printf("%s",tmp);
        #else
        now=time(&now);
		localtime_r(&now, &tm_now); 
        printf("[%04d/%02d/%02d %02d:%02d:%02d]",
            tm_now.tm_year+1900,
            tm_now.tm_mon+1,
            tm_now.tm_mday,
            (tm_now.tm_hour)%24,
            tm_now.tm_min,
            tm_now.tm_sec);

        #endif

        va_start(struAp, pszfmt);
        ret = vprintf(format, struAp);
        va_end(struAp);
        
        return ret;
}


#ifdef _WIN32
static INT32 gettimeofday(struct timeval *tv, void * pData)
{
    unsigned __int64 ft;
    LARGE_INTEGER ccf;
    if (QueryPerformanceFrequency(&ccf))
    {
         LARGE_INTEGER cc;
         QueryPerformanceCounter(&cc);
         tv->tv_sec = cc.QuadPart / ccf.QuadPart;
         tv->tv_usec = (cc.QuadPart % ccf.QuadPart) / (ccf.QuadPart / 1000000);
    }
    else
    {
         GetSystemTimeAsFileTime((FILETIME *)&ft);
         tv->tv_sec = ft / 10000000;
         tv->tv_usec = (ft % 10000000) / 10;
    }
    return 0;
}

static int writev(SOCKET s, const struct  iovec* vector, int count)
{
  INT32 ssize = 0;

  WSASend(s, (LPWSABUF)vector, count, &ssize, 0, NULL, NULL);

  return ssize;
}



#endif

static INT32 XInitSockAddr(struct sockaddr_in *pSockAddr, INT16 wFamily, const CHAR * pchAddr, UINT16 uwPort, UINT32 unConvertedAddr, UINT16 uwConvertedPort)
{
	if (pchAddr != NULL)
	{
		pSockAddr->sin_family = wFamily;
		pSockAddr->sin_addr.s_addr = inet_addr(pchAddr);
		pSockAddr->sin_port = htons(uwPort);
	}
	else
	{
		pSockAddr->sin_family = wFamily;
		pSockAddr->sin_addr.s_addr = unConvertedAddr;
		pSockAddr->sin_port = uwConvertedPort;
	}
	return 0;
}

#endif


