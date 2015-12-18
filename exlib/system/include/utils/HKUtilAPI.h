#pragma  once
#include <pthread.h>
#include "HKCmdPacket.h"
void* conf_set(const char* cf, const char* nm, const char* val);
void* conf_set_int(const char* cf, const char* nm, int val);

char* conf_get(const char* cf, const char* nm, char val[], size_t len);
int   conf_get_int(const char* cf, const char* nm);

char *EncodeStr( char *pBuf );
char *DecodeStr( char *pBuf );

char *EncodeBuf( char *pBuf,int nSize,unsigned int nMask,unsigned short nVer );
char *DecodeBuf( char *pBuf,int nSize,unsigned int nMask,unsigned short nVer );

unsigned long HKMSGetTick();
void HKGetNow( char *pNow );

void  HKInitRWLock( pthread_rwlock_t *prwlock_ );
void  HKUNInitRWLock( pthread_rwlock_t *prwlock_ );

void  HKRLock( pthread_rwlock_t *prwlock_ );
void  HKWLock( pthread_rwlock_t *prwlock_ );

void  HKXUNLock( pthread_rwlock_t *prwlock_ );

void  HKInitLock( pthread_mutex_t *plock_mutex );
void  HKUNInitLock( pthread_mutex_t *plock_mutex );
void  HKLock( pthread_mutex_t *plock_mutex );
void  HKUNLock( pthread_mutex_t *plock_mutex );

long  HKGetFileSize( char *filename );

typedef void ( *wifiCALLBACK )( unsigned short nWifiState,char *param );
char    *GetWifiDBCastAddr();
unsigned int   HKMakeTokenID( unsigned short HPid,unsigned short LPid );
unsigned short HKGetHPid( unsigned int SID );
unsigned short HKGetLPid( unsigned int SID );


int StartWifiAP( HKFrameHead* hf,wifiCALLBACK wificb );
int ScanWifiAP( HKFrameHead* hf );
void StopWifi();
int GetWifiStatus();
int InitWifiAP();
enum
{
    WIFIST_NONE = 0,
    WIFIST_FAIL,
    WIFIST_OK,
};
enum
{
	INFACE_NONE = 0,
	INFACE_RAUSB0 = 2,
	INFACE_RA0  = 3
};
