#include "P2Pserver.h"
#include "hi_comm_aio.h"
#include "hi_type.h"

#define  MAX_SIZE  (512*1024)
#define ENABLE_P2P
extern void OnCmdPtz(int ev);

static CHAR g_cSubFilebuf[MAX_SIZE]; 
static INT32  g_nSubCurfilebufsize = 0;
static CHAR g_SubFrameBuf[MAX_FRAME_SIZE]; 
static INT32  g_SubFrameLen = 0; 
static UINT16 gLPort =0;
static CHAR g_cServerID[MAX_ID_LEN];
static CHAR g_cP2PIP[32]= {0};

static volatile INT32 g_iTalkStat =0;   //对讲状态
static PEER_INFO * g_TalkPeer =NULL;    //正在对讲的客户端

typedef struct nalu_s
{
	unsigned char  nalu_type;
	unsigned char  sz_startcode;
	char  *nalubuf;
	int   nalusize;
}nalu_t;

static int h264_get_nalu(nalu_t *nalu,char *srcbuf,int srcsize)
{
	int  index=0;
	int  i=0;
	while(i < srcsize - 3)
	{
		if(srcbuf[i+2] <= 1) 
		{
			if(srcbuf[i] == 0 && srcbuf[i+1] == 0 && srcbuf[i+2] == 1)
			{
				if(i > 0 && srcbuf[i-1] == 0)
				{
					nalu->sz_startcode = 4;
					nalu->nalubuf = &srcbuf[i-1];
					index = i-1;
				}
				else
				{
					nalu->sz_startcode = 3;
					nalu->nalubuf = &srcbuf[i];
					index = i;
				}
				break;
			}
			i++;
		}
		else
		{
			i += 3;
		}
	}

	if(i >= srcsize-3)
	{
		dbgmsg("##########%s  i:%d srcsize:%d\n", __FUNCTION__,i,srcsize);  
		return  -1;
	}

	i += nalu->sz_startcode; //跳过第一个NALU起始码
	while(i < srcsize - 3)
	{
		if(srcbuf[i+2] <= 1) 
		{
			if(srcbuf[i] == 0 && srcbuf[i+1] == 0 && srcbuf[i+2] == 1)
			{
				if(srcbuf[i-1] == 0)
				{
					nalu->nalusize = i-1 - index;
				}
				else
				{
					nalu->nalusize = i - index;
				}
				break;
			}
			i++;
		}
		else
		{
			i += 3;
		}
	}

	if(i >= srcsize-3)
	{
		nalu->nalusize = srcsize - index;
	}
	nalu->nalu_type = nalu->nalubuf[nalu->sz_startcode];

	return  0;
}
//用录像文件模拟发送视频数据
INT32 PthreadStream(void *_uThis) 
{
	INT32 ret = 0;
	INT32 MainSq = 0;
	INT32 SubSq = 0;
	INT32 MainLen = 0;
	INT32 SubLen = 0;
	FILE *FpSub = NULL;
	nalu_t nalu;
	INT32 nType =0;
open_lab:
	FpSub = fopen("sub.h264", "rb");
	while(1)
	{
		if(FpSub==NULL)
		{
			XSleep(1,40*1000);
			goto open_lab;
		}
loop:
		ret = fread(g_cSubFilebuf+g_nSubCurfilebufsize,1,MAX_SIZE - g_nSubCurfilebufsize,FpSub);
		if(ret<=0)
		{

			fseek(FpSub,0,0);
			goto loop;
		}    
		else
		{
			g_nSubCurfilebufsize+= ret;
		}
		if(h264_get_nalu(&nalu,g_cSubFilebuf,g_nSubCurfilebufsize) ==0)
		{
			memcpy(g_SubFrameBuf+SubLen,nalu.nalubuf,nalu.nalusize);
			SubLen+=nalu.nalusize;

			//   dbgmsg("##########%s nalu.nalu_type %02x\n", __FUNCTION__,nalu.nalu_type);  
			if((nalu.nalu_type & 0x1f) != 0x07 && (nalu.nalu_type & 0x1f) != 0x08  &&(nalu.nalu_type & 0x1f)!=0x06)
			{
				nType = (nalu.nalu_type & 0x1f) == 0x05 ? 0 : 1;
				P2PNetServerChannelDataSndToLink(0,0,g_SubFrameBuf,SubLen,nType,0);
				P2PNetServerChannelDataSndToLink(0,1,g_SubFrameBuf,SubLen,nType,0);
				SubLen = 0;		
				XSleep(0,40*1000);

			}
			g_nSubCurfilebufsize-= nalu.nalusize;
			memmove(g_cSubFilebuf, nalu.nalubuf + nalu.nalusize, g_nSubCurfilebufsize);
		}      
		else
		{
			dbgmsg("##########%s g_pSubMedia error \n", __FUNCTION__);  
		}
	}
	return 0;
}


//收到客户端回调登陆时的回调函数
static INT32 NetLogin(PEER_INFO* pPeerInfo,CHAR* _u8Buf,INT32 _iBufLength)
{
	INT32 result = -1;
	char cSndBuf[sizeof(CMDHEADP2P) + sizeof(LogInRsp)];
	LogInReq *pLogInReq = (LogInReq *)_u8Buf;
	PCMDHEADP2P pNetCmd = (PCMDHEADP2P)cSndBuf;
	LogInRsp *pLogInRsp = (LogInRsp *)(cSndBuf+sizeof(CMDHEADP2P));
	dbgmsg("Receive  logon message \n");
	//biaobiao
	int version[2];
	version[0] = 1;
	version[0] = 100;
	P2PNetServerSndMsgToLink(pPeerInfo,(char *)version, sizeof(version));
	if ((strcmp(pLogInReq->szUserName,"admin") == 0) && (strcmp(pLogInReq->szPassword,"admin") == 0))
	{
		result = 0; //模拟为用户名admin 密码1234 才是唯一可登录的用户
	}
	else
	{
		result = -1; //非0即可，意味着用户信息不对，不允许登录
	}
	if(0==result)
	{
		pLogInRsp->m_iLogInRes = htonl(0);
		pLogInRsp->m_Capabilities.m_iChannel= htons(1);
		pLogInRsp->m_Capabilities.m_iStream= htons(2);
		pLogInRsp->m_Capabilities.m_iAlarmNum= htons(1);
		pLogInRsp->m_ChannelIfor[0][0].Width = htonl(1920);
		pLogInRsp->m_ChannelIfor[0][0].Height = htonl(1080);
		pLogInRsp->m_ChannelIfor[0][0].BitRat = htonl(4096);
		pLogInRsp->m_ChannelIfor[0][0].FrameRate = htonl(25);
		pLogInRsp->m_ChannelIfor[0][0].IFrameRate = htonl(100);
		pLogInRsp->m_ChannelIfor[0][0].Quity = htonl(80);
		pLogInRsp->m_ChannelIfor[0][0].AudioType = htonl(AUDIO_PCM);//PCM
		pLogInRsp->m_ChannelIfor[0][0].VideoType = htonl(VIDEO_H264);//H264

		pLogInRsp->m_ChannelIfor[0][1].Width = htonl(720);
		pLogInRsp->m_ChannelIfor[0][1].Height = htonl(576);
		pLogInRsp->m_ChannelIfor[0][1].BitRat = htonl(1024);
		pLogInRsp->m_ChannelIfor[0][1].FrameRate = htonl(25);
		pLogInRsp->m_ChannelIfor[0][1].IFrameRate = htonl(100);
		pLogInRsp->m_ChannelIfor[0][1].Quity = htonl(80);
		pLogInRsp->m_ChannelIfor[0][1].AudioType = htonl(0x05);//PCM
		pLogInRsp->m_ChannelIfor[0][1].VideoType = htonl(1);//H264

		pLogInRsp->m_VideoPara[0].Brightness =htonl(50) ; 
		pLogInRsp->m_VideoPara[0].Saturation =htonl(50) ; 
		pLogInRsp->m_VideoPara[0].Contrast =htonl(50) ; 
		pLogInRsp->m_VideoPara[0].Hue =htonl(50); 

	}
	else
	{
		pLogInRsp->m_iLogInRes = htonl(1);
	}
	pNetCmd->m_iMsgType= htonl(REQ_LOGIN); 
	pNetCmd->m_u32TotalLen = htonl(sizeof(LogInRsp));   
	P2PNetServerSndCmd2Link(pPeerInfo,cSndBuf,sizeof(cSndBuf));
	return result;
}


//收到客户端登出时的回调函数
static INT32 NetLogOut(PEER_INFO* pPeerInfo)
{
	INT32 i=0;
	return 0;
}

//收到客户端设置通道参数时的回调函数
INT32 NetSetChannelInfor(PEER_INFO* pPeerInfo,CHAR* _u8Buf,INT32 _iBufLength)
{
	SetChannelIforReq *pReq = (SetChannelIforReq *)_u8Buf;
	dbgmsg("##########  rcv %s  message \n", __FUNCTION__);
	return 0;
}


//收到客户端设置图像参数时的回调函数
static INT32 NetSetVideoPara(PEER_INFO* pPeerInfo,CHAR* _u8Buf,INT32 _iBufLength)
{
	SetVideoParaReq *pReq = (SetVideoParaReq *)_u8Buf;
	dbgmsg("##########  rcv %s  message \n", __FUNCTION__);
	return 0;
}

//收到客户端PTZ控制命令时的回调函数
static INT32 NetPtzControl(void * pThis,CHAR* _u8Buf,INT32 _iBufLength)
{
	PtzControlReq *pReq = (PtzControlReq *)_u8Buf;
	dbgmsg("##########  rcv %s  message \n", __FUNCTION__);
	OnCmdPtz(pReq->mPtzData.iAction);
	return 0;
}

/*
   接受对讲数据的线程
   当收到客户端对讲请求的时候创建
   注意对P2PNetServerGetTalkData返回值的判断
   P2PNetServerGetTalkData 返回值小于0时说明连接由于某种原因已经断开
 */
#ifdef _WIN32
DWORD WINAPI TskRcvTalkData(LPVOID pArgs) 
#else
void *TskRcvTalkData(void *pArgs) 
#endif
{
	CHAR cTalkData[32*1024];
	INT32 nLen = 0;
	INT32 nRet =0;
	AUDIO_STREAM_S stAudioStream;
	INT32 s32AdecChn;

	int threq = 0;
	scc_AudioOpen("audio.vbAudio.In", NULL, &threq);
	dbgmsg("##########TskRcvTalkData############\n");  
	while(g_iTalkStat)
	{   
#if 1
		nRet = P2PNetServerGetTalkData(g_TalkPeer,cTalkData,&nLen,1);
		if(0==nRet)
		{
			printf("########################################getdata#######################\n");
			dbgmsg("Rcv talk data  :%d \n",nLen);
			//add by biaobiao
			P2P_Write(cTalkData, nLen, NULL);
		}
		//对讲连接中断清楚对讲信息
		else if(-1==nRet)
		{
			g_iTalkStat = 0;
			g_TalkPeer = NULL;
		}
#else
		XSleep(1, 0);
#endif
	}
	//退出对讲的一些操作比如释放资源等可以在此处处理
#ifdef _WIN32
	return 0;
#else
	Audio_Close(2);
	return NULL;
#endif
}

//收到客户端请求对讲 的回调函数
static INT32 NetTalkReq(void * pThis)
{   
#ifdef _WIN32
	HANDLE hThread1;
#else
	pthread_t          	Tid;  
	pthread_attr_t 	g_pThreadAttr;
	pthread_attr_init(&g_pThreadAttr);
	pthread_attr_setdetachstate(&g_pThreadAttr,PTHREAD_CREATE_DETACHED);
#endif
	char cSndBuf[sizeof(CMDHEADP2P) + sizeof(P2pTalkMsg)];
	PCMDHEADP2P pNetCmd = (PCMDHEADP2P)cSndBuf;
	P2pTalkMsg *pTalk = (PtzControlReq *)(cSndBuf+sizeof(CMDHEADP2P));
	dbgmsg("##########  rcv %s  message \n", __FUNCTION__);    
	pNetCmd->m_iMsgType= htonl(REQ_TALK); 
	pNetCmd->m_u32TotalLen = htonl(sizeof(P2pTalkMsg));   
	pTalk->nAudioTYpe = g_iTalkStat == 0 ? htons(AUDIO_PCM): htons(-1);
	pTalk->nAudioBit = htons(16);
	pTalk->nAudioSample = htons(8000);
	pTalk->nAudioChannel= htons(1);
	P2PNetServerSndCmd2Link(pThis,cSndBuf,sizeof(cSndBuf));
	if(g_iTalkStat==0)
	{
		g_TalkPeer = pThis;
		g_iTalkStat = 1;
		//创建获取对讲数据的线程   
#ifdef _WIN32
		hThread1=CreateThread(NULL,0,TskRcvTalkData,NULL,0,NULL); 
		CloseHandle(hThread1);
#else
		dbgmsg("##########%s  NetCheckHeatThread\n", __FUNCTION__); 
		pthread_create(&Tid, &g_pThreadAttr, TskRcvTalkData,NULL);
#endif

	}
	return 0;
}


//收到客户端请求对讲 的回调函数
static INT32 NetTalkClose(void * pThis)
{
	if(g_TalkPeer==pThis)
	{
		g_iTalkStat = 0;
		g_TalkPeer = NULL;
	}
	return 0;
}



//处理客户端透明传输的数据，用于自己扩展协议
//客户端调用SndMsgToServer接口发送的数据在此处处理
static INT32 NetMsgProc(void * pThis,CHAR* _u8Buf,INT32 _iBufLength)
{
#if 1
	dbgmsg("%s :%s \n", __FUNCTION__,_u8Buf);
	int cmd;
	cmd = (int)(*_u8Buf);
	printf("*************************the cmd is %d\n", cmd);
	switch(cmd)
	{
		case 1:
			net_get_upgrade();
			break;
		default:
			printf("the protocal is no sense\n");
	}
#endif
	return 0;
}


//收到客户端命令时的回调函数
INT32 NetReadCallback(PEER_INFO* pPeerInfo, CHAR* _u8Buf, INT32 length)
{
	INT32 nRet = 0;
	CMDHEADP2P NetCmd;      
	memcpy(&NetCmd,_u8Buf,sizeof(CMDHEADP2P));
	switch(ntohl(NetCmd.m_iMsgType))
	{
		case REQ_LOGIN:
			nRet = NetLogin(pPeerInfo,_u8Buf+sizeof(CMDHEADP2P),length-sizeof(CMDHEADP2P));
			printf("***************login is %d\n", nRet);
			break;         
		case REQ_LOGOUT:
			nRet = NetLogOut(pPeerInfo);
			break;         
		case REQ_SETCHANNELINFOR:
			nRet = NetSetChannelInfor(pPeerInfo,_u8Buf+sizeof(CMDHEADP2P),length-sizeof(CMDHEADP2P));
			break;   
		case REQ_SETVIDEOPARA:
			nRet =NetSetVideoPara(pPeerInfo,_u8Buf+sizeof(CMDHEADP2P),length-sizeof(CMDHEADP2P));
			break;  
		case REQ_PTZCONTRIL:
			nRet =NetPtzControl(pPeerInfo,_u8Buf+sizeof(CMDHEADP2P),length-sizeof(CMDHEADP2P));
			break;

		case REQ_TALK:
			nRet =NetTalkReq(pPeerInfo);
			break;        
		case REQ_TALKCLOSE:
			nRet =NetTalkClose(pPeerInfo);
			break;

		case REQ_PLAYSTREAM :
			//ChannelDataSndToLink(0,0,g_SubFrameBuf,SubLen,0,0);
			//   ChannelDataSndToLink(0,1,g_SubFrameBuf,SubLen,0,0);
			break; 

			//客户端透明传输的数据，用于自己扩展协议
		case MSG_TRANS:
			NetMsgProc(pPeerInfo,_u8Buf+sizeof(CMDHEADP2P),length-sizeof(CMDHEADP2P)); 
			break; 

		default:    
			break;
	}
	return nRet;
}

//INT32 p2p_server(INT32 argc, char **argv)
INT32 p2p_server_f()
{
	
        char argv[4][100] ={"4","server","115.28.168.140","HM-003"};
	int argc = atoi(argv[0]);
	CHAR cServerID[MAX_ID_LEN+1] = {0};
	CHAR cRole[16] = {0};
#ifdef _WIN32
	WSADATA Wsadata;
	WSAStartup(MAKEWORD(2,2), &Wsadata);
#endif

	if (argc < 3)
	{
		dbgmsg("For Sample server-\n");
		dbgmsg("Use: Sample server <P2PIP> <Server ID>\n");
		dbgmsg("Example: Sample server 221.176.34.55 MyIPCServer\n");
		return -1;
	}
	if (strcmp(argv[1], "server") != 0) 
	{
		dbgmsg("Please check the parameter 1. It must be server!\n");
		return -1;
	}
	gLPort =7234;
	strcpy(cRole, argv[1]);
	strcpy(g_cP2PIP,argv[2]);
	strcpy(cServerID, argv[3]);
	printf("cRole is %s g_cP2PIP is %s cServerID is %s \n", cRole, g_cP2PIP, cServerID);
	printf("The Test has started  if have anything  Please contact \n"); 
	//初始化SDK 
//	P2PNetServerSdkInit(cServerID,gLPort,g_cP2PIP,NetReadCallback,256,256);
	P2PNetServerSdkInit(cServerID,gLPort,g_cP2PIP,0,NetReadCallback,256,256);
	while(1);
	return 0;
}
