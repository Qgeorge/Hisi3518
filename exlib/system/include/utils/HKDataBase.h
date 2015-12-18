#pragma once
#include "RTSHead.h" 
#define MAXVCHAR 		255
#define MAXFIND 		100
#define HK_MAXPOOL_BUF    	65535

#ifndef _HKIPC
#include <stdlib.h>
#include <string.h>
#endif

#ifndef WIN32
#ifdef  _HKIPC
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>
#include <linux/rtc.h>
#include <net/if.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>


typedef void ( *ReceiveDataBuf )( unsigned int nEPID,unsigned short nLevel,RTSHead *rtsHead,char *pData,unsigned short nLen );

typedef void ( *StatusUpdate )( unsigned int nDstID,unsigned int nEPID,unsigned short nStatus );

#define closesocket(s) shutdown(s,SHUT_RDWR);close(s)

struct _PipeStatus
{
		unsigned int nPipeID;
		bool  		 bLost;
		struct _PipeStatus *pNext;
};

struct _UdpData
{
	char               	szIP[MAXVCHAR];   // host
	unsigned short		nPort;
	char				*pBuf;
	unsigned short		nSize;	
	struct _UdpData 	*pNext;
	struct _UdpData		*pPre;
};

struct _sAckLostPt
{
        unsigned short flg;
        int            nTime;
};

struct _seqBuf
{
        char *pBuf;
        unsigned short nFlg;
        unsigned short nLen;
        unsigned short nSeq;
        unsigned int   nTmSap;
};


typedef struct _EPConState //Huqing 05-13 IP Conncet State
{
    unsigned short   nNETType;  //Type
    char             szIP[ MAXVCHAR ];  //IP
    unsigned short   nPort; //Port
    unsigned short   nProto; //proto
#ifdef __cplusplus
    _EPConState()
    {
        memset( szIP, 0, MAXVCHAR );
        nPort = 0;
        nProto = 0;
    }
#endif
}EPConState;

struct _InSeqBuf
{
    char *pBuf;
    unsigned short nSeq;
    HKRedBody mhead;
};

struct _InPacket
{
    char *pDataBuf;
    int  nPacketSize;
    unsigned short nPos;
};

struct _InSeqFlg
{
    unsigned short nFlg;
    unsigned short nSeq;
};

struct _SeqTimeInval 
{
    unsigned short nBType;
    unsigned short nEType;
    unsigned int   nBeginTime;
    unsigned int   nEndTime;
};

struct _NStreamIn
{
    char                    ip[MAXVCHAR];
    unsigned short          nPort;
    struct _InSeqFlg        arySeqFlg[BACK_SIZE];
    struct _InPacket        inPacket[HK_DATAPROI_LEVEL];
    struct _SeqTimeInval    seqTimeInval[BACK_SIZE];
    struct _InSeqBuf        inSeqBuf[BACK_SIZE];    
    ReceiveDataBuf          cb_NStreamIn;
    StatusUpdate            cb_StatusUpdate;
	bool					bWAN;	

    unsigned short      nMaxPacketSize;
    unsigned short      nSeqNum;
    int                 nBreakPos;
    unsigned short      nProi;
    unsigned int        nEPID;
    unsigned int        nHKID; 
    unsigned int        nHKDstID; 
    SOCKET              hSocket;
	int                 nThreadID;
    //TCP
	unsigned short      nPoto;
	unsigned int		nlParam;
};

struct  _NStreamOut
{
	char			    ip[MAXVCHAR];
	unsigned short		nPort;
	unsigned int		gMaxBufSize;
	char 			    *pDataBuf[ HK_DATAPROI_LEVEL ];
	pthread_mutex_t     lock_mutex[HK_DATAPROI_LEVEL];
    pthread_mutex_t		muxlock;
	StatusUpdate        cb_StatusUpdate;

	unsigned int 		nCurrentPos[HK_DATAPROI_LEVEL];
	unsigned int 		nCutPos[HK_DATAPROI_LEVEL];
    unsigned short		nwndSize[HK_DATAPROI_LEVEL];

	unsigned int 		nBufSize[HK_DATAPROI_LEVEL];
	unsigned int 		nDataSize[HK_DATAPROI_LEVEL];
	unsigned int 		nRealBufSize[HK_DATAPROI_LEVEL];

	bool         		bAgain[HK_DATAPROI_LEVEL];
	bool         		bWait[HK_DATAPROI_LEVEL];
	bool         		bReadWait[HK_DATAPROI_LEVEL];

	//lost ----
	bool				bLostPacket;
	
	int					nBTSeq;
	int					nProi;
	int                 nGuardSeq;
	short			    nNetCapLevel;
	unsigned short		nHKSeq;	
	unsigned short		nACKSeq;
	short				nWaitTime;		
	short				nLoopTime;	
    SOCKET          	hSocket;
    bool            	bRun;
	struct _sAckLostPt	sAckLostFlg[BACK_SIZE];	
	struct _seqBuf		stcBackSeqBuf[BACK_SIZE];

	bool  				bEvnet;
	bool				bBlock;
	int					nThreadID;
	unsigned int        nEPID;
    unsigned int        nHKID;
    unsigned int        nHKDstID;		
	int 				nLostBoat[HK_DATAPROI_LEVEL];
	unsigned int 		nWaitCnt;
	// nack
	//----- TCP
	unsigned short      nPoto;
	unsigned int 		nlParam;
};

struct _RTSParam
{
    bool				bWAN;
	char                ip[MAXVCHAR];
    unsigned short      nPort;
    SOCKET              hSocket;
    unsigned short      nPoto;
    unsigned int        nHKID;
    unsigned int        nEPID;
    unsigned int        nDstID;
    unsigned short      nInPori;
    unsigned short      nOutPori;
    unsigned short      nInSize;
    unsigned short      nOutSize;
	bool				bWan;
	unsigned int		nlParam;	
	
	ReceiveDataBuf          cb_receivedata;
    StatusUpdate            cb_statusupdate;
	
};

struct _RTSSession
{
        bool                bRun;
        SOCKET                  hSocket;
        struct  _NStreamOut *pNStreamOut;
        struct  _NStreamIn  *pNStreamIn;
		pthread_rwlock_t    rwlock_t;
		unsigned int 		nlParam;
		struct _PipeStatus  *pPipeStatus;	
};

struct _epNetInfo
{
		char 				ip[MAXVCHAR];
		char				mac[MAXVCHAR];
		unsigned short 		nPort;
		unsigned short 		nBHCnt;
		unsigned int    	nHKID;
		bool 				bCenter;
		bool				bLanEP;
		unsigned short		nValidateID;
		unsigned short 		nStatus;
		
		unsigned short 		nLeqCmd;
		unsigned short		nLeqStatus;
		unsigned short      nLeqCnt;
		unsigned short 		nLeqMaxCnt;
		char				*pLeqBuf;
		unsigned short      nLeqSize;	

		long            	_ThreadID;	
		bool				bRun;

    	struct _RTSSession  *pRTSSession;
		struct _epNetInfo	*pNextNetInfo;
};

struct  _CLINetInfo
{
        char                    ip[MAXVCHAR];
        unsigned short          nPort;
        
        char                    natsrvip[MAXVCHAR];  
	    unsigned short          natPort;

        unsigned short          nstatus;
        SOCKET                  hksocket;
        unsigned int            nEPID;              // my allcol
        unsigned int            nMySelfID;
        unsigned int            nDstID; //
        unsigned short          nPoto;
		bool					bWAN;
        bool                    bRun;
    	short					nBHCnt;
	    bool					bActive;
	
        ReceiveDataBuf          cb_receivedata;
        StatusUpdate            cb_statusupdate;

	    struct  _epNetInfo	*pEpNetInfo;
    	struct _RTSSession  	*pRTSSession;
	    long			lbhtID;
	    long 			ltID;
		pthread_rwlock_t    rwlock_t;
		pthread_mutex_t	    mutex_t;
		struct _UdpData *pUDPListHead;
		struct _UdpData *pUDPListEnd;
		//---------------TCP
		char            *pReceiveBuf;
		int				nReceSize;
		unsigned short  nFreameSize;
		bool				bStun;
		bool                bSysCon;
};
#endif
#endif
typedef void ( *HKNETCALLBACK )( unsigned int nPipe,unsigned short nType, unsigned short size,char *pdata );               
typedef void ( *HKLANNETCALLBACK )( unsigned short nDstID,unsigned short nPipe,unsigned short nType, unsigned short size,char *pdata );   

enum 	_HKNETS_STATUS
{
	HKNETS_NULL = 0,
	HKNETS_TEST = 4,
	HKNETS_CONNECT = 5,               // 正在连接
	HKNETS_FAIL,					  // 连接失败
	HKNETS_DISCONNECT,                // 断开    
    HKNETS_CONOUT,                    // quit  
	HKNETS_SUCCESS,                   // 连接成功
    HKNETS_CONNECTED,
	HKNETS_CONNECTEDGOOD,             // 连接稳定正常
	HKNETS_CONNECTWARING,             // 不稳定，
	HKPIPE_START,
    HKPIPE_STOP,
	HKNETS_TIMEOUT = 15,                         //Huqing 06-12 Ping Connect TimeOut
	HKNETS_BHEAD,
};

enum _HK_CALLBCAKTYPE
{
	HKTP_OCMD			= 3,
	HKTP_RTP			= 4,
	HKTP_INBAND			= 5,
	HKTP_CONFAIL		= 6,
	HKTP_CONDISCONNECT	= 7,
	HKTP_CONSUCCESS		= 8,
	HKTP_MULCAST		= 20,   //?鲥
};

enum _HKMONPIPE_MSG
{
	HKMONPIPE_SUCCESS = 1,
	HKMONPIPE_FAIL = 2,
	HKMONPIPE_CHANGE = 3,
};
enum _HK_PROTO_TYPE
{
	HK_PROTO_NULL = 0,
	HKCLI_PROTO_DBCAST = 1,
	HKCLI_PROTO_UDP = 2,
	HKCLI_PROTO_TCP = 3,
	HKCLI_PROTO_HTTP = 4,
	HKCLI_PROTO_HTTPS = 5,
	HKCLI_PROTO_SOCK4 = 6,
	HKCLI_PROTO_SOCK5 = 7,

	HKSRV_PROTO_UDP = 8,
	HKSRV_PROTO_TCP = 9,
	HKSRV_PROTO_HTTP = 10,
	HKSRV_PROTO_HTTPS = 11,
	HKSRV_PROTO_SOCK4 = 12,
	HKSRV_PROTO_SOCK5 = 13,
	HKSRV_PROTO_HKUDP = 14,
	HKSRV_PROTO_MULCAST = 15,
	HKSRV_PROTO_HKUDPLAN = 16,
};

typedef struct _HKPacketInfo
{
	unsigned short nSeq;             
	unsigned short nBoatType;        
	unsigned int   nCHID;			
	unsigned short nPacketType;       
    unsigned short nLevel;
}HKPacketInfo;

typedef enum
{
	HK_BOAT_NLOST = 0,
	HK_BOAT_YLOST,
	HK_BOAT_IFREAM,
	HK_BOAT_PFREAM,
}HK_BOAT_TYPE;

typedef enum
{
	HK_LEVEL_1 = 0,
	HK_LEVEL_2,
	HK_LEVEL_3,
	HK_LEVEL_4,
	HK_LEVEL_MAX,
}HK_LEVEL_TYPE;

typedef struct _HKEPInfo
{
	char               szIP[MAXVCHAR];   // host
	unsigned short     port;
	unsigned short     hProto;  // TCP,UDP,HTTP,SOCKET5,
	unsigned short     lProto;  // code
#ifdef __cplusplus
	_HKEPInfo()
	{
		memset( szIP,0,MAXVCHAR );
		port    = 0;
		hProto  = HKSRV_PROTO_HKUDP;
		lProto  = 0;
	}
	_HKEPInfo( const _HKEPInfo &pHKEPInfo )
	{
		port = pHKEPInfo.port;
		hProto = pHKEPInfo.hProto;
		lProto = pHKEPInfo.lProto;
		strcpy( szIP, pHKEPInfo.szIP );
	}
	_HKEPInfo & operator = ( const _HKEPInfo & pHKEPInfo )
	{
		if( this == &pHKEPInfo )
		{
			return ( *this );
		}
		port = pHKEPInfo.port;
		hProto = pHKEPInfo.hProto;
		lProto = pHKEPInfo.lProto;
		strcpy( szIP, pHKEPInfo.szIP );
		return ( *this );
	}
#endif
}HKEPInfo;

typedef struct _HKHOSTInfo // 传输包时需要发送方的一下数据信息！
{
	HKEPInfo           sHostInfo;
	char               szUserName[MAXVCHAR];
	char               szPassword[MAXVCHAR];
#ifdef __cplusplus
	_HKHOSTInfo()
	{
		memset( szUserName,0,MAXVCHAR );
		memset( szPassword,0,MAXVCHAR );
	}
	_HKHOSTInfo( const _HKHOSTInfo &pHKHostInfo )
	{
		sHostInfo = pHKHostInfo.sHostInfo;
		strcpy( szUserName,pHKHostInfo.szUserName );
		strcpy( szPassword,pHKHostInfo.szPassword );
	}
	_HKHOSTInfo & operator = ( const _HKHOSTInfo & pHKHostInfo )
	{
		if( this == &pHKHostInfo )
		{
			return ( *this );
		}
		sHostInfo = pHKHostInfo.sHostInfo;
		strcpy( szUserName,pHKHostInfo.szUserName );
		strcpy( szPassword,pHKHostInfo.szPassword );
		return ( *this );
	}
#endif
}HKHOSTInfo;


