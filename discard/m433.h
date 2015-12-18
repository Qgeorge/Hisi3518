#ifndef __M433_H__
#define __M433_H__
/*
初始化信号处理函数
*/
void init_sighandler();

/*
初始化射频模块
*/
int init_m433(pid_t pid);

/*
获得系统信息
参数reserve1 reserve2:	保留参数
参数iCmd：命令
参数iValues：值
*/
int sccGetSysInfo(int reserve1,int reserve2,int iCmd,int *iValues);

/*
设定系统信息
参数reserve1 reserve2:	保留参数
参数iCmd：命令
参数iValues：值
*/
int sccSetSysInfo(int reserve1, int reserve2,int iCmd,int iValues);
#endif //__M433_H__
