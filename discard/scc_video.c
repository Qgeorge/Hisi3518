
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>

#include "ipc_hk.h"

#define VIDEO_STREAM_TIME   0
enum { videobuflen = 200*1024 };
//static char *g_videobuf=NULL;
//static char *g_videoSubbuf=NULL;

VideoDataRecord *hostVideoDataP = NULL;//master starem //*mVideoDataBuffer;
VideoDataRecord *slaveVideoDataP = NULL; //slave starem
VideoDataRecord *slaveAudioDataP = NULL; //audio
//static pthread_mutex_t db = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_MutexLock[3] = { PTHREAD_MUTEX_INITIALIZER };

static int sccPushVideoDataSlave(int pStreamType,char *pData,unsigned int nSize,short IFrame, int iResType, int iStreamType );


int sccPushStream(int pIPCInst, int pStreamInst, char *poDataBuf, int poDataLen, int iPofType, int iResType, int iStreamType)
{
    sccPushVideoDataSlave( pStreamInst, poDataBuf, poDataLen, iPofType, iResType, iStreamType );
}

static int sccPushVideoData_(int pStreamType, char *pData, unsigned int nSize, short IFrame, int iResType, int iStreamType, VideoDataRecord *mVideoDataBuffer)
{
    if ((mVideoDataBuffer == NULL) || (nSize <= 0))
        return 0;

    if ( ((mVideoDataBuffer->g_allDataSize + nSize) > (pStreamType ? MAX_VIDEODATA_SLAVETREAM : MAX_VIDEODATA_HOSTSTREAM)) \
            || (nSize > 190000) \
            || (abs(mVideoDataBuffer->g_writeIndex - mVideoDataBuffer->g_readIndex) > (POOLSIZE-1)) )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr, "again again \r\n");
        usleep(10);
        return 0;
    }

    if( IFrame < 0 )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr, "no I P Frame**********************\n\n");
        return 0;
    }

    if( mVideoDataBuffer->g_bLost && IFrame != 1 )
    {
        printf( "Push Video lost--ccccccc\n" );
        return 0;
    }


    if( mVideoDataBuffer->g_writeIndex == POOLSIZE ) mVideoDataBuffer->g_writeIndex = 0;

#if VIDEO_STREAM_TIME
    nSize += 4;
    static unsigned int timeC = 0;
    if(g_PushVideo_hostFrameOnec)
    {
        if(g_PushVideo_hostFrame == 0)   g_PushVideo_hostFrame = 25;
        timeC +=  1000/g_PushVideo_hostFrame;
    }
    else
    {
        timeC = HKMSGetTick();
        g_PushVideo_hostFrameOnec = 1;
    }
#endif

    mVideoDataBuffer->g_bLost = 0;
    if( mVideoDataBuffer->g_bAgain )
    {
        if( mVideoDataBuffer->g_CurPos + nSize < mVideoDataBuffer->g_readPos )
        {
        #if VIDEO_STREAM_TIME
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_CurPos, &timeC,4 );
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_CurPos+4, pData,nSize-4 );
        #else
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_CurPos ,pData,nSize );
        #endif
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos = mVideoDataBuffer->g_CurPos;
            //printf( "again[%d][%d-%d]\n",g_writeIndex,g_CurPos,nSize );
            mVideoDataBuffer->g_CurPos += nSize;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt++;
        }
        else
        {
            mVideoDataBuffer->g_bLost = 1;
            printf( "lost--a\n" );
            usleep(10);
            return 0;
        }
    }
    else
    {
        if( mVideoDataBuffer->g_writePos + nSize < (pStreamType?MAX_VIDEODATA_SLAVETREAM:MAX_VIDEODATA_HOSTSTREAM) )
        {
        #if VIDEO_STREAM_TIME
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_writePos,&timeC,4 );
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_writePos+4,pData,nSize-4 );
        #else
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_writePos,pData,nSize );
        #endif
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = mVideoDataBuffer->g_writePos;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_writePos += nSize;
            mVideoDataBuffer->g_CurPos += nSize;//mVideoDataBuffer->g_writePos;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt ++;
        }
        else if( nSize < mVideoDataBuffer->g_readPos )
        {
            mVideoDataBuffer->g_CurPos = 0;
            mVideoDataBuffer->g_bAgain = 1;
        #if VIDEO_STREAM_TIME
            memcpy( mVideoDataBuffer->g_videoBuf, &timeC,4 );
            memcpy( mVideoDataBuffer->g_videoBuf+4,pData,nSize-4 );
        #else
            memcpy( mVideoDataBuffer->g_videoBuf,pData,nSize );
        #endif
            //printf( "push2[%d][%d-%d]\n",g_writeIndex,g_writePos,nSize );
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = 0;
            mVideoDataBuffer->g_CurPos += nSize;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt ++;
        }
        else
        {
            mVideoDataBuffer->g_bLost = 1;
            printf( "lost--b\n" );
            usleep(10);
            return 0;
        }
    }

    if( mVideoDataBuffer->g_haveFrameCnt >= POOLSIZE )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr,"lost--dd\r\n\r\n");
        usleep(10);
    }
    return 0;
}

static int sccPushAudioData(int pStreamType,char *pData,unsigned int nSize,short IFrame,int iResType,int iStreamType,VideoDataRecord  *mVideoDataBuffer)
{
    if(mVideoDataBuffer == NULL || nSize <= 0)
        return 0;

    //fprintf(stderr,"nSize = %d\r\n",nSize);
    if ( ((mVideoDataBuffer->g_allDataSize + nSize) > (pStreamType ? MAX_VIDEODATA_SLAVETREAM : MAX_VIDEODATA_HOSTSTREAM)) \
            || (nSize > 199600) \
            || (abs(mVideoDataBuffer->g_writeIndex - mVideoDataBuffer->g_readIndex) > (POOLSIZE - 1)) )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr,"again again \r\n");
        usleep(100);
        return 0;
    }

    if( IFrame < 0 )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr,"no I P Frame**********************\r\n\r\n\r\n");
        return 0;
    }

    if( mVideoDataBuffer->g_bLost && IFrame != 1 )
    {
        printf( "Push Audio Stream lost--ccccccc\n" );
        return 0;
    }

    if( mVideoDataBuffer->g_writeIndex == POOLSIZE ) mVideoDataBuffer->g_writeIndex = 0;

    mVideoDataBuffer->g_bLost = 0;
    if( mVideoDataBuffer->g_bAgain )
    {
        if( mVideoDataBuffer->g_CurPos + nSize < mVideoDataBuffer->g_readPos )
        {
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_CurPos ,pData,nSize );
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos = mVideoDataBuffer->g_CurPos;
            //printf( "again[%d][%d-%d]\n",g_writeIndex,g_CurPos,nSize );
            mVideoDataBuffer->g_CurPos += nSize;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt++;
        }
        else
        {
            mVideoDataBuffer->g_bLost = 1;
            printf( "lost--a\n" );
            usleep(10);
            return 0;
        }
    }
    else
    {
        if( mVideoDataBuffer->g_writePos + nSize < (pStreamType?MAX_VIDEODATA_SLAVETREAM:MAX_VIDEODATA_HOSTSTREAM) )
        {
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_writePos,pData,nSize );
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = mVideoDataBuffer->g_writePos;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_writePos += nSize;
            mVideoDataBuffer->g_CurPos += nSize;//mVideoDataBuffer->g_writePos;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt ++;
        }
        else if( nSize < mVideoDataBuffer->g_readPos )
        {
            mVideoDataBuffer->g_CurPos = 0;
            mVideoDataBuffer->g_bAgain = 1;
            memcpy( mVideoDataBuffer->g_videoBuf,pData,nSize );

            //printf( "push2[%d][%d-%d]\n",g_writeIndex,g_writePos,nSize );
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = 0;
            mVideoDataBuffer->g_CurPos += nSize;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt ++;
        }
        else
        {
            mVideoDataBuffer->g_bLost = 1;
            printf( "lost--b\n" );
            usleep(10);
            return 0;
        }
    }

    if( mVideoDataBuffer->g_haveFrameCnt >= POOLSIZE )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr, "lost--dd\r\n\r\n");
        usleep(10);
    }
    return 0;
     // fprintf(stderr,"g_allDataSize:%d   g_writeIndex=%d   nsize=%d\r\n",mVideoDataBuffer->g_allDataSize,mVideoDataBuffer->g_writeIndex,nSize);
}


//sun-server.........
static int sccPushVideoDataSlave_(int pStreamType,char *pData,unsigned int nSize,short IFrame,int iResType,int iStreamType,VideoDataRecord  *mVideoDataBuffer)
{
    if ((mVideoDataBuffer == NULL) || (nSize <= 0))
        return 0;

    if ( ((mVideoDataBuffer->g_allDataSize + nSize) > (pStreamType ? MAX_VIDEODATA_SLAVETREAM : MAX_VIDEODATA_HOSTSTREAM)) \
            || (nSize > 199600) \
            || (abs(mVideoDataBuffer->g_writeIndex - mVideoDataBuffer->g_readIndex) > (POOLSIZE-1)) )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr, "again again \n");
        usleep(100);
        return 0;
    }

    if( IFrame < 0 )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr, "no I P Frame**********************\n\n");
        return 0;
    }

    if( mVideoDataBuffer->g_bLost && IFrame != 1 )
    {
        printf( "Push Sub Stream Video lost--ccccccc\n" );
        return 0;
    }


    if( mVideoDataBuffer->g_writeIndex == POOLSIZE ) mVideoDataBuffer->g_writeIndex = 0;

    mVideoDataBuffer->g_bLost = 0;
    if( mVideoDataBuffer->g_bAgain )
    {
        if( mVideoDataBuffer->g_CurPos + nSize < mVideoDataBuffer->g_readPos )
        {
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_CurPos ,pData,nSize );

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos = mVideoDataBuffer->g_CurPos;
            //printf( "again[%d][%d-%d]\n",g_writeIndex,g_CurPos,nSize );
            mVideoDataBuffer->g_CurPos += nSize;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt++;
        }
        else
        {
            mVideoDataBuffer->g_bLost = 1;
            printf( "lost--a\n" );
            usleep(10);
            return 0;
        }
    }
    else
    {
        if( mVideoDataBuffer->g_writePos + nSize < (pStreamType?MAX_VIDEODATA_SLAVETREAM:MAX_VIDEODATA_HOSTSTREAM) )
        {
            memcpy( mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_writePos,pData,nSize );

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = mVideoDataBuffer->g_writePos;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_writePos += nSize;
            mVideoDataBuffer->g_CurPos += nSize;//mVideoDataBuffer->g_writePos;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt ++;
        }
        else if( nSize < mVideoDataBuffer->g_readPos )
        {
            mVideoDataBuffer->g_CurPos = 0;
            mVideoDataBuffer->g_bAgain = 1;
            memcpy( mVideoDataBuffer->g_videoBuf,pData,nSize );

            //printf( "push2[%d][%d-%d]\n",g_writeIndex,g_writePos,nSize );
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].IFrame = IFrame;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].iEnc = iResType;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].StreamType = iStreamType;

            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nPos  = 0;
            mVideoDataBuffer->g_CurPos += nSize;
            mVideoDataBuffer->g_allDataSize += nSize;
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_writeIndex].nSize = nSize;
            mVideoDataBuffer->g_writeIndex ++ ;
            mVideoDataBuffer->g_haveFrameCnt ++;
        }
        else
        {
            mVideoDataBuffer->g_bLost = 1;
            printf( "lost--b\n" );
            usleep(10);
            return 0;
        }
    }

    if( mVideoDataBuffer->g_haveFrameCnt >= POOLSIZE )
    {
        mVideoDataBuffer->g_bLost = 1;
        fprintf(stderr,"lost--dd\r\n\r\n");
        usleep(10);
    }
    return 0;
     // fprintf(stderr,"g_allDataSize:%d   g_writeIndex=%d   nsize=%d\r\n",mVideoDataBuffer->g_allDataSize,mVideoDataBuffer->g_writeIndex,nSize);
}


static int sccPushVideoDataSlave(int pStreamType, char *pData, unsigned int nSize, short IFrame, int iResType, int iStreamType)
{
    //pthread_mutex_lock( &db );
    pthread_mutex_lock( &g_MutexLock[pStreamType] );

    if (pStreamType == 0 && hostVideoDataP != NULL)//stream
    {
        sccPushVideoData_(pStreamType, pData, nSize, IFrame, iResType, iStreamType, hostVideoDataP);
    }
    else if (pStreamType == 1 && slaveVideoDataP != NULL)//sub stream
    {
        sccPushVideoDataSlave_(pStreamType, pData, nSize, IFrame, iResType, iStreamType, slaveVideoDataP);
    }
    else if (pStreamType == 2 && slaveAudioDataP != NULL)
    {
        sccPushAudioData(pStreamType, pData, nSize, IFrame, iResType, iStreamType, slaveAudioDataP);
    }

    //pthread_mutex_unlock( &db );
    pthread_mutex_unlock( &g_MutexLock[pStreamType] );
    return nSize; 
}

//---------------------------------Get stream-----------------------------

static int sccGetVideoData_(int pStreamType, char *pVideoData,unsigned int *nSize,unsigned int *iFream ,int *iResType, int *iStreamType,VideoDataRecord  *mVideoDataBuffer)
{
    if(mVideoDataBuffer == NULL)
        return 0;

    *nSize = 0;

    if ((mVideoDataBuffer->g_allDataSize == 0) || (mVideoDataBuffer->g_haveFrameCnt == 0) || (mVideoDataBuffer->g_readIndex == mVideoDataBuffer->g_writeIndex))
    {
        mVideoDataBuffer->g_bAgain = 0;
        mVideoDataBuffer->g_writePos = 0;
        mVideoDataBuffer->g_readPos = 0;
        mVideoDataBuffer->g_CurPos = 0;
        mVideoDataBuffer->g_allDataSize= 0;
        mVideoDataBuffer->g_haveFrameCnt = 0;
        mVideoDataBuffer->g_writeIndex = 0;
        mVideoDataBuffer->g_readIndex = 0;
        //fprintf(stderr,"====so=====GET 1 NO DATA \r\n");

        return 0;
    }
    
    if( mVideoDataBuffer->g_readIndex == POOLSIZE ) mVideoDataBuffer->g_readIndex = 0;

    if( mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize == 0  )
    {
        //printf( "have some err!\n" );
        //mVideoDataBuffer->g_readIndex ++;

        return 0;
    }
    memcpy( pVideoData,mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos,
            mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize  );
    //*pVideoData =   mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
    *nSize = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize;
    *iFream = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame;
    *iResType =mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc; 
    *iStreamType = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType;
    //if(mVideoDataBuffer->g_readPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
    if(mVideoDataBuffer->g_bAgain == 1 && mVideoDataBuffer->g_CurPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
    {
        mVideoDataBuffer->g_writePos = mVideoDataBuffer->g_CurPos;
        mVideoDataBuffer->g_bAgain = 0;

    }

    mVideoDataBuffer->g_haveFrameCnt --;
    mVideoDataBuffer->g_allDataSize -= mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize ;
    mVideoDataBuffer->g_readPos = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType = 0;

    mVideoDataBuffer->g_readIndex ++ ;

    return *nSize;
}

//audio
static int sccGetAudioData(int pStreamType, char *pVideoData,unsigned int *nSize,unsigned int *iFream ,int *iResType, int *iStreamType,VideoDataRecord  *mVideoDataBuffer)
{
    *nSize = 0;
    if(mVideoDataBuffer == NULL)
        return 0;
   // printf( "get  data \r\n" );

    if ( (mVideoDataBuffer->g_allDataSize == 0) || (mVideoDataBuffer->g_haveFrameCnt == 0) || (mVideoDataBuffer->g_readIndex == mVideoDataBuffer->g_writeIndex) )
    {
        mVideoDataBuffer->g_bAgain = 0;
        mVideoDataBuffer->g_writePos = 0;
        mVideoDataBuffer->g_readPos = 0;
        mVideoDataBuffer->g_CurPos = 0;
        mVideoDataBuffer->g_allDataSize= 0;
        mVideoDataBuffer->g_haveFrameCnt = 0;
        mVideoDataBuffer->g_writeIndex = 0;
        mVideoDataBuffer->g_readIndex = 0;
       // printf("scc Audio====so=====GET 1 NO DATA \r\n");
        return 0;
    }
    
    if( mVideoDataBuffer->g_readIndex == POOLSIZE ) mVideoDataBuffer->g_readIndex = 0;

    if( mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize == 0  )
    {
        printf( "scc..Audio.have some err!\n" );
        //mVideoDataBuffer->g_readIndex ++;
        return 0;
    }
    memcpy( pVideoData,mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos,
           mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize  );
    //*pVideoData =   mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
    *nSize = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize;
    *iFream = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame;
    *iResType =mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc; 
    *iStreamType = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType;

    //if( mVideoDataBuffer->g_readPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
    if(mVideoDataBuffer->g_bAgain == 1 && mVideoDataBuffer->g_CurPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
    {
        mVideoDataBuffer->g_writePos = mVideoDataBuffer->g_CurPos;
        mVideoDataBuffer->g_bAgain = 0;

    }

    mVideoDataBuffer->g_haveFrameCnt --;
    mVideoDataBuffer->g_allDataSize -= mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize ;
    mVideoDataBuffer->g_readPos = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType = 0;
    mVideoDataBuffer->g_readIndex ++ ;

    return *nSize;
}


static int sccGetVideoDataSlave_(int pStreamType, char *pVideoData,unsigned int *nSize,unsigned int *iFream ,int *iResType, int *iStreamType,VideoDataRecord  *mVideoDataBuffer)
{
    *nSize = 0;
    if(mVideoDataBuffer == NULL)
        return 0;
   // printf( "get  data \r\n" );

    if ( (mVideoDataBuffer->g_allDataSize == 0) || (mVideoDataBuffer->g_haveFrameCnt == 0) || (mVideoDataBuffer->g_readIndex == mVideoDataBuffer->g_writeIndex))
    {
        mVideoDataBuffer->g_bAgain = 0;
        mVideoDataBuffer->g_writePos = 0;
        mVideoDataBuffer->g_readPos = 0;
        mVideoDataBuffer->g_CurPos = 0;
        mVideoDataBuffer->g_allDataSize= 0;
        mVideoDataBuffer->g_haveFrameCnt = 0;
        mVideoDataBuffer->g_writeIndex = 0;
        mVideoDataBuffer->g_readIndex = 0;
        //fprintf(stderr,"====so=====GET 1 NO DATA \r\n");

        return 0;
    }
    
    if( mVideoDataBuffer->g_readIndex == POOLSIZE ) mVideoDataBuffer->g_readIndex = 0;

    if( mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize == 0  )
    {

        //printf( "have some err!\n" );
        //mVideoDataBuffer->g_readIndex ++;
        return 0;
    }
    memcpy( pVideoData,mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos,
           mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize  );
    //*pVideoData =   mVideoDataBuffer->g_videoBuf + mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
    *nSize = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize;
    *iFream = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame;
    *iResType =mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc; 
    *iStreamType = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType;

    //if( mVideoDataBuffer->g_readPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
    if(mVideoDataBuffer->g_bAgain == 1 && mVideoDataBuffer->g_CurPos > mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos )
    {
        mVideoDataBuffer->g_writePos = mVideoDataBuffer->g_CurPos;
        mVideoDataBuffer->g_bAgain = 0;

    }

    mVideoDataBuffer->g_haveFrameCnt --;
    mVideoDataBuffer->g_allDataSize -= mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize ;
    mVideoDataBuffer->g_readPos = mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nSize = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].nPos = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].IFrame = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].iEnc = 0;
    mVideoDataBuffer->g_VideoData[mVideoDataBuffer->g_readIndex].StreamType = 0;
    mVideoDataBuffer->g_readIndex ++ ;

    return *nSize;
}


int sccGetVideoDataSlave(int pStreamType, char *pVideoData, unsigned int *nSize, unsigned int *iFream, int *iResType, int *iStreamType)
{
    int ret = 0;
    //pthread_mutex_lock( &db);
    pthread_mutex_lock( &g_MutexLock[pStreamType]);

    if (pStreamType == 0 && hostVideoDataP != NULL)
    {
        ret = sccGetVideoData_(pStreamType, pVideoData, nSize, iFream, iResType, iStreamType, hostVideoDataP);
    }
    else if (pStreamType == 1 && slaveVideoDataP != NULL)
    {
        ret = sccGetVideoDataSlave_(pStreamType, pVideoData, nSize, iFream, iResType, iStreamType, slaveVideoDataP);
    }
    else if (pStreamType == 2 && slaveAudioDataP != NULL)
    {
        ret = sccGetAudioData(pStreamType, pVideoData, nSize, iFream, iResType, iStreamType, slaveAudioDataP);
    }

    //pthread_mutex_unlock(&db);
    pthread_mutex_unlock(&g_MutexLock[pStreamType]);
    return ret;
}

//-------------------Init stream list ------------------------
int sccInitVideoData(int pStreamType)
{
    //HK_DEBUG_PRT("...... pStreamType: %d ......\n", pStreamType);
    if ((0 == pStreamType) && (NULL == hostVideoDataP))
    {
        hostVideoDataP = calloc(1, sizeof(VideoDataRecord));
        if (NULL == hostVideoDataP)
        {
            HK_DEBUG_PRT("calloc hostVideoDataP failed, %d, %s\n", errno, strerror(errno));
            hostVideoDataP = NULL;
            return -1;
        }

        hostVideoDataP->g_videoBuf = calloc(1, MAX_VIDEODATA_HOSTSTREAM);
        if (NULL == hostVideoDataP->g_videoBuf)
        {
            HK_DEBUG_PRT("calloc g_videoBuf failed, %d, %s\n", errno, strerror(errno));
            if (hostVideoDataP) free(hostVideoDataP);
            hostVideoDataP->g_videoBuf = NULL;
            hostVideoDataP = NULL;
            return -1;
        }
        return 0;
    }
    else if ((1 == pStreamType) && (NULL == slaveVideoDataP))
    {
        slaveVideoDataP = calloc(1, sizeof(VideoDataRecord));
        if (NULL == slaveVideoDataP)
        {
            HK_DEBUG_PRT("calloc slaveVideoDataP failed, %d, %s\n", errno, strerror(errno));
            slaveVideoDataP == NULL;
            return -1;
        }

        slaveVideoDataP->g_videoBuf = calloc(1, MAX_VIDEODATA_SLAVETREAM);
        if (NULL == slaveVideoDataP->g_videoBuf)
        {
            HK_DEBUG_PRT("calloc g_videoBuf failed, %d, %s\n", errno, strerror(errno));
            if (slaveVideoDataP) free(slaveVideoDataP);
            slaveVideoDataP->g_videoBuf = NULL;
            slaveVideoDataP = NULL;
            return -1;
        }
        return 0;
    }
    else if ((2 == pStreamType) && (NULL == slaveAudioDataP))
    {
        slaveAudioDataP = calloc(1, sizeof(VideoDataRecord));
        if (NULL == slaveAudioDataP)
        {
            HK_DEBUG_PRT("calloc slaveAudioDataP failed, %d, %s\n", errno, strerror(errno));
            slaveAudioDataP = NULL;
            return -1;
        }

        slaveAudioDataP->g_videoBuf = calloc(1,MAX_VIDEODATA_SLAVETREAM);
        if (NULL == slaveAudioDataP->g_videoBuf)
        {
            HK_DEBUG_PRT("calloc g_videoBuf failed, %d, %s\n", errno, strerror(errno));
            if (slaveAudioDataP) free(slaveAudioDataP);
            slaveAudioDataP->g_videoBuf = NULL;
            slaveAudioDataP = NULL;
            return -1;
        }
        return 0;
    }
    return 0;
}

void sccResetVideData(int pStreamType, VideoDataRecord *mVideoDataBuffer)
{
    HK_DEBUG_PRT("...... pStreamType: %d ......\n", pStreamType);
    //pthread_mutex_lock( &db );
    pthread_mutex_lock( &g_MutexLock[pStreamType] );

    mVideoDataBuffer->g_writePos = 0;
    mVideoDataBuffer->g_readPos = 0;
    mVideoDataBuffer->g_allDataSize = 0;
    mVideoDataBuffer->g_bLost = 0;
    mVideoDataBuffer->g_bAgain = 0;
    mVideoDataBuffer->g_CurPos = 0;
    mVideoDataBuffer->g_readIndex = 0;
    mVideoDataBuffer->g_writeIndex = 0;
    mVideoDataBuffer->g_haveFrameCnt = 0;
    memset(mVideoDataBuffer->g_videoBuf, 0, sizeof(mVideoDataBuffer->g_videoBuf));

    //pthread_mutex_unlock( &db );
    pthread_mutex_unlock( &g_MutexLock[pStreamType] );
}

