#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ipc_file_sd.h"
#include "ipc_vbVideo.h"
#include "utils_biaobiao.h"

#define GPIO_BIT        21
#define REFERENCE_MAX   12

#define START		10
#define PAUSE		11
#define PROBE		12
#define PWSETUP         13

#define SETALARM        0x20
#define M433SIG         0x21
#define SETCON          0x22
#define SETUNCON        0x23

#define CLEARTABLE      0x2D
#define FILLTABLE       0x2E
#define GETSTATE        0x30
#define SETSTATE        0x31


#define NORMAL_MODE     0x32
#define LEARN_MODE      0x33
#define TEST_MODE       0x34                

#define FLAG_ALARM      0x35
#define FLAG_UNALARM    0x36
#define FLAG_TEST       0x37
#define FLAG_LEARN      0x38

#define DELMODULE       0x40
#define TELECONTROL     0x41
#define DELCODE         0x42


#define MAXINDEX    	64
#define M433_PATH 	"/mnt/sif/m433.conf"
#define M433_CODE 	"code"

struct M433DEV {
    unsigned int accept_ad;
    unsigned int short_pulse;
    unsigned int long_pulse;
    int normal_mode;
    int telecontrol;
    unsigned int timeout;
};

struct m433_data_t {
    int keep_run;
    int current_ad;
    int alarm_flag;
    int test_flag;
    int reference_table[REFERENCE_MAX];
};

struct transe_t {
    int code;
    int flag;
    int mode;
};


int m433_fd;
unsigned int m433_adval;

static struct M433DEV m433dev;
static int m433_state = 0;
struct itimerval m433_tick;
struct transe_t tx;

struct m433_data_t m433_data = {
	.keep_run = 1,
	.current_ad = 0,
	.alarm_flag = 0,
	.test_flag = 0,
};


static int m433_save_param()
{
	int i = 0;
	FILE *fp = NULL;

	fp = fopen(M433_PATH,"w");
	if(fp == NULL)
	{
		fprintf(stderr,"fopen w %s failed\r\n",M433_PATH);
		return -1;
	}
	for(i = 0;i < REFERENCE_MAX;i++)
		fprintf(fp,M433_CODE"%d=0x%x\r\n",i,m433_data.reference_table[i]);
	fclose(fp);
	return 0;
}

static int m433_get_param()
{
	int i = 0;
	FILE *fp = NULL;

	fp = fopen(M433_PATH,"r");
	if(fp == NULL)
	{
		fprintf(stderr,"fopen r %s failed\r\n",M433_PATH);
		m433_data.reference_table[0] = 0x0;
		m433_data.reference_table[1] = 0x0;
		m433_data.reference_table[2] = 0x0;
		m433_data.reference_table[3] = 0x0;
		return 0;
	}

	for(i = 0;i < REFERENCE_MAX;i++)
		fscanf(fp,M433_CODE"%d=0x%x\r\n",&i,&m433_data.reference_table[i]);

	fclose(fp);

	return 0;
}

static int m433_set_code()
{
	ioctl(m433_fd, FILLTABLE,m433_data.reference_table);
	return 0;
}

void arbitration(unsigned int code)
{
	static char DevRoll = 4;
	static char TeleConRoll = 0;
	static char MatchDevCode = 0;
	int ret = 0;
	int i = 0;
	int arg = 0;
	static int cnt = 0;   

	m433_data.current_ad = m433_adval;
	m433_adval = 0;

	if(m433_data.current_ad == m433_data.reference_table[2]) 
	{
		m433_data.alarm_flag = 0;
		m433dev.normal_mode = 0;
	}

	printf("--- mode = %d, alarm flag = %d ---\n", m433dev.normal_mode, m433_data.alarm_flag);

	if(m433dev.normal_mode == 0)
	{
		printf("--- learn mode ---\n");
		// m433_data.alarm_flag = 0;

		for(i = 0; i < REFERENCE_MAX; i++)
		{
			if(i < 4)
			{
				if((m433dev.telecontrol >> i) & 0x1) //遥控码控制标记为
				{
					printf("--- judge learn Telecontrol code ---\n");
					if(m433_data.reference_table[i] != 0)
					{
						if(m433_data.reference_table[i] == m433_data.current_ad)
						{
							printf("=== Telecontrol had learned ===\n");
							break;
						}
						else
						{
							if(i == 3)
							{
								printf("--- full reset telecontrol[%d] = 0x%x\n", TeleConRoll, m433_data.current_ad);
								m433_data.reference_table[TeleConRoll++] == m433_data.current_ad;
								if(TeleConRoll == 4)
									TeleConRoll = 0;
								break;
								//alarm_handler();//接收到遥控码学习完成
							}
							else
							{
								continue;
							}
						}
					}
					else //if(m433_data.reference_table[i] == 0 )
					{
						printf("--- temp set telecontrol[%d] = 0x%x ---\n", i, m433_data.current_ad);
						m433_data.reference_table[i] = m433_data.current_ad;
						m433dev.normal_mode = 1;
						m433dev.telecontrol = 0;
						tx.code = m433_data.reference_table[i];
						tx.mode = LEARN_MODE;
						tx.flag = TELECONTROL; 
						//alarm_handler();//接收到遥控码学习完成
						break;
					}
				}
				else
				{
					if(i == 3)
						printf("--- not telecontrol code ---\n");
				}
			}
			else  //i >= 4
			{
				printf("--- dev code ---\n");
				if(m433_data.current_ad == m433_data.reference_table[i])
				{
					printf("=== had learned ===\n");
					tx.code = m433_data.reference_table[i];
					tx.mode = LEARN_MODE;
					tx.flag = FLAG_LEARN; 
					//alarm_handler();//接收到设备码学习完成
					m433dev.normal_mode = 1;
					sccOnAlarm(0,100,m433_data.current_ad);
					m433_save_param();
					break;
				}
				else
				{
					if(m433_data.reference_table[i] == 0)
					{
						m433_data.reference_table[i] = m433_data.current_ad;
						printf("--- temp set learn dev code table[%d] = 0x%x ---\n", i, m433_data.reference_table[i]);
						m433dev.normal_mode = 1;
						tx.code = m433_data.reference_table[i];
						tx.mode = LEARN_MODE;
						tx.flag = FLAG_LEARN; 
						sccOnAlarm(0,100,m433_data.current_ad);
						m433_save_param();
						//alarm_handler();//接收到设备码学习完成
						break;
					}
					else
					{
						if(i == REFERENCE_MAX - 1)
						{
							printf("--- full reset table[%d] = 0x%x ---\n", DevRoll, m433_data.current_ad);
							m433_data.reference_table[DevRoll++] = m433_data.current_ad;
							m433dev.normal_mode = 1;
							if(DevRoll == REFERENCE_MAX)
								DevRoll = 4;
							break;
						}
						else
						{
							continue;
						}
					}
				}
			}
		}
	}
	else //正常模式
	{
		printf("--- normal mode alarm flag = %d---\n", m433_data.alarm_flag);
		for(i = 0; i < REFERENCE_MAX; i++) //对比码表码值
		{
			if(m433_data.current_ad == m433_data.reference_table[i]) //如果有和接收码值一致时
			{
				printf("--- match code ---\n");
				if(i == 0)
				{
					m433_data.alarm_flag = 1;
					printf("--- alarm set ---\r\n");
					MatchDevCode = 1;
					break;
				}
				else if(i == 1)
				{
					m433_data.alarm_flag = 0;
					printf("--- alarm unset ---\r\n"); 
					MatchDevCode = 1;
					break;
				}
				else if(i == 3 || m433_data.test_flag == 1)
				{
					tx.code = m433_data.reference_table[i];
					tx.mode = TEST_MODE;
					tx.flag = FLAG_TEST; 
					m433_data.test_flag = 0;
					MatchDevCode = 1;
					//               sccOnAlarm(0,6,m433_data.current_ad);
					printf("test alarming!\r\n"); 
					break;
				}
				else if(i > 3)
				{
					if(m433_data.alarm_flag == 1)
					{
						tx.code = m433_data.reference_table[i];
						tx.mode = NORMAL_MODE;
						tx.flag = FLAG_ALARM; 
						sccOnAlarm(0,6,m433_data.current_ad);
						printf("alarming!\r\n");
						MatchDevCode = 1;
						break;
					}
					else
					{
						MatchDevCode = 1;
						printf("--- match dev code ---\n");
						break;
					}

				}
			}
		}
		if(MatchDevCode == 0)
			printf("--- without match code ---\n");
		else
			MatchDevCode = 0;
	}
}

void sig_handler(int signo)
{
	printf("---catch signal ---\n");
	if(signo == SIGUSR2)
	{
		m433_tick.it_value.tv_sec = 0;
		m433_tick.it_value.tv_usec = 0;
		m433_tick.it_interval.tv_sec = 0;
		m433_tick.it_interval.tv_usec = 0;
		if(setitimer(ITIMER_REAL, &m433_tick, NULL) < 0)
			printf("Set timer failed!\n");
		read(m433_fd, &m433_adval, 1);
		printf("--- m433_adval = 0x%x ---\n", m433_adval);
		arbitration( m433_adval );
		m433_adval = 0;
		m433dev.normal_mode = 1;
	}
	else if(signo == SIGALRM)
	{
		m433dev.normal_mode = 1;
		printf("learn mode time out\n");
	}
	else if(signo == SIGSEGV)
	{
		printf("...zzzzzzzzzzzzzzzzz segment fault and will reboot zzzzzzzzzzzzzzzzzzzzzz...\n");
		system("sync");
		sd_record_stop();
		system("umount /mnt/mmc");
		system("reboot");
	}
}

void install_sighandler( void (*handler)(int) )
{
	signal(SIGUSR2,handler);
	signal(SIGALRM,handler);
	signal(SIGSEGV,handler);
}
void init_sighandler()
{
	install_sighandler(sig_handler);
}

int init_m433(pid_t pid)
{
	int ret = 0;
	printf("433  pid  == %d\n",pid);
	m433_fd = open("/dev/m433dev", O_RDWR); //打开文件节点
	if (m433_fd < 0)
	{
		printf("can't open!\n");
		return -1;
	}
	ioctl(m433_fd, M433SIG, &pid);//传入进程pid
	ioctl(m433_fd, PROBE, &ret); //探测设备
	printf("--- probe ret = %d ---\n", ret);
	if(ret != 1)
	{
		printf("===  HW detect faile ===\n");
		close(m433_fd);
		return -2;
	}
	m433_get_param();
	m433_set_code();
	ioctl(m433_fd, START, &ret);//开启设备
	printf("--- start ret = %d ---\n", ret);
	if(ret != 1)
	{
		printf("=== HW init faile ===\n");
		close(m433_fd);
		return -3;
	}
	//signal(SIGALRM,sig_handler);
	if((m433_state = conf_get_int("hkipc.conf","m433_state")) == 0)
	{
		m433dev.normal_mode = 1;
		m433_data.alarm_flag = 0;
		m433_state = 1;
	}
	memset(&m433_tick,0,sizeof(m433_tick));
	return 0;
}

int set_m433_state(int operation)
{
	int i = 0;
	switch(operation)
	{
		case FLAG_ALARM:
			printf("set operate : alarm mode\n");
			m433dev.normal_mode = 1;
			m433_data.alarm_flag = 1;
			conf_set_int("hkipc.conf","m433_state",2);
			m433_state = 2;
			break;

		case FLAG_UNALARM:
			printf("set operate : unalarm mode\n");
			m433dev.normal_mode = 1;
			m433_data.alarm_flag = 0;
			conf_set_int("hkipc.conf","m433_state",1);
			m433_state = 1;
			break;

		case FLAG_LEARN:
			printf("set operate : learn mode\n");
			m433dev.normal_mode = 0;
			m433_tick.it_value.tv_sec = 20;
			m433_tick.it_value.tv_usec = 0;
			m433_tick.it_interval.tv_sec = 0;
			m433_tick.it_interval.tv_usec = 0;
			if(setitimer(ITIMER_REAL, &m433_tick, NULL) < 0)
				printf("Set timer failed!\n");
			break;

		case FLAG_TEST:
			printf("set operate : test mode\n");
			//read(m433_fd, &m433_adval, 1);
			int i = 0;
			for(;i < REFERENCE_MAX;i++)
			{
				if(m433_data.reference_table[i] == m433_adval)
				{
					sccOnAlarm(0,6,m433_adval);
					printf("test alarming  0x%x\n",m433_adval);
				}
			}
			m433_adval = 0;
			break;

		case CLEARTABLE:
			for(i = 0; i < REFERENCE_MAX; i++)
			{
				m433_data.reference_table[i] = 0;
			}
			for(i = 0; i < REFERENCE_MAX; i++)
			{
				printf("[%4d] code vale 0x%x\n",i, m433_data.reference_table[i]);
			}
			break;
		default:
			break;
	}	

	return 0;
}

int sccSetSysInfo(int reserve1, int reserve2,int iCmd,int iValues)
{
	if(iCmd == 204)
	{
		set_m433_state(FLAG_LEARN);
	}
	else if(iCmd == 205)
	{
		set_m433_state(FLAG_ALARM);
	}
	else if(iCmd == 206)
	{
		set_m433_state(FLAG_UNALARM);
	}
	else if(iCmd == 207)
	{
		m433_adval = iValues;
		set_m433_state(FLAG_TEST);
	}
	return 0;
}

int sccGetSysInfo(int reserve1,int reserve2,int iCmd,int *iValues)
{
	printf("get m433 state  =  %d\n",m433_state);
	*iValues = m433_state;
	return 0;
}
