#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "flvenc.h"
#include "flvdec.h"
#include "record.h"
#define min(x, y)    ((x) < (y) ? (x) : (y))

static char g_mount_point[1024] = {0};

static FlvEnc *enc = NULL;  //该结构体对应一个flv文件,一个flv格式的文件包含有h.264的视频流和alaw的音频流
static struct tm prev_tm;

int av_record_init(char *mount_point)
{
    strncpy(g_mount_point, mount_point, sizeof(g_mount_point) - 1);
    return 0;
}

int av_record_quit(void)
{
    if (enc) {
        flvenc_close(enc);
        enc = NULL;
    }
    return 0;
}

extern int CheckSDStatus();

static void get_dirname(int year, int month, int day, int hour, char *dirname, int len)
{
    snprintf(dirname, len, "%s/%04d-%02d-%02d/%d", g_mount_point, year, month, day, hour);
}


//获得视频路径列表里最早的视频存放路径,找到则返回1 否则返回0
static int find_oldest_dir(char *dirname, int len)
{
    
	int found = 0;
    struct dirent **namelist = NULL;
	
	//读取挂载点下视频存储目录,并读取里面的文件并排序,结果存储到namelist中
    int n = scandir(g_mount_point, &namelist, 0, alphasort);
    if (n <= 0)
        return 0;   /* no record */

    int i;
    for (i = 0; i < n; i++) {
        int year, month, day;
        if (sscanf(namelist[i]->d_name, "%04d-%02d-%02d", &year, &month, &day) == 3) {
            snprintf(dirname, len, "%s/%s", g_mount_point, namelist[i]->d_name);
            found = 1;
            break; //排序后找到最老的视频
        }
    }

    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return found;
}
//获得视频列表里最早的视频，找到返回1 否则返回0
static int find_oldest_file(char *dirname, char *filepath, int len)
{
    int found = 0;
    struct dirent **namelist = NULL;

    int n = scandir(dirname, &namelist, 0, alphasort);
    if (n <= 0)
        return 0;   /* no record */

    int i;
    for (i = 0; i < n; i++) {
        int year, month, day, hour, minute;
        if (sscanf(namelist[i]->d_name, "%04d%02d%02dT%02d%02d.flv", &year, &month, &day, &hour, &minute) == 5) {
            snprintf(filepath, len, "%s/%s", dirname, namelist[i]->d_name);
            found = 1;
            break;
        }
    }

    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return found;
}

//查询sd卡的剩余空间
static int64_t freesize(void)
{
    struct statfs sfs; //这个结构体屌爆了，能记录文件系统的信息
    if (statfs(g_mount_point, &sfs) < 0) //查询文件系统的相关信息
        return -1;
    return (int64_t)sfs.f_bsize * (int64_t)sfs.f_bavail;//传输块大小*块数 
}

//获得文件路径
static int get_filepath(int year, int month, int day, int hour, int minute, char *filepath, int len)
{
    snprintf(filepath, len, "%s/%04d-%02d-%02d/%d/%04d%02d%02dT%02d%02d.flv",
            g_mount_point, year, month, day, hour, year, month, day, hour, minute);
    return 0;
}
//判断是否需要转换另外一个文件
static int needswitch(struct tm tm)
{
	//判断是否需要转换另一个文件的时间条件精确到1min，即一分钟存储一个视频文件
    if (tm.tm_year == prev_tm.tm_year && tm.tm_mon  == prev_tm.tm_mon  &&
        tm.tm_mday == prev_tm.tm_mday && tm.tm_hour == prev_tm.tm_hour &&
        tm.tm_min  == prev_tm.tm_min)
        return 0;
    return 1;
}

#define SIZE_1MB    (1024 * 1024)


//当记录好一个flv文件后,想在记录另一个flv文件,则需调用该函数
static int prepare_recording(int keyframe)
{
    char dirname[1024];
    char filepath[1024];

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);  //将time获得的秒数转换成本地时间,该函数线程安全

    int year   = tm.tm_year + 1900;
    int month  = tm.tm_mon + 1;
    int day    = tm.tm_mday;
    int hour   = tm.tm_hour;
    int minute = tm.tm_min;

	//要想转换文件,需存在flv编码结构体,满足转换时间和转换文件后的第一个帧为I帧
    if (enc && !(needswitch(tm) && keyframe))
        return 0;

    /*
     * switch to next file
     */

    if (enc) {
		//关闭上一个flv文件
        flvenc_close(enc); /* close prevous FLV file */
        enc = NULL;
    }

    get_dirname(year, month, day, hour, dirname, sizeof(dirname));

    if (access(dirname, R_OK | W_OK | X_OK) < 0) {
#if 0
        if (mkdir(dirname, 0777) < 0) {
            perror("mkdir");
            return -1;
        }
#else
        char cmd[1024];
		//若不存在该文件路劲,则创建
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", dirname);
        system(cmd);
#endif
    }

    while (freesize() < 100LL * SIZE_1MB) {
        if (find_oldest_dir(dirname, sizeof(dirname)) != 1)
            return -1;

        if (find_oldest_file(dirname, filepath, sizeof(filepath)) != 1)
            return -1;
		//若sd卡的空间剩余不足100M时,则开始删除最早录制的文件
        unlink(filepath);   /* delete oldest file to earn some space */
    }

    get_filepath(year, month, day, hour, minute, filepath, sizeof(filepath));

    MediaDesc desc[2] = { //媒体编码类型结构体
        { .media = MediaType_Video, .codec = CodecType_H264 },
        { .media = MediaType_Audio, .codec = CodecType_ALAW },
    };

	//让enc指向新的flv文件
    enc = flvenc_open(filepath, desc, 2); //打开flv文件,该文件存储的的视频是h264编码的,音频是ALAW编码
    if (!enc) {
        perror("flvenc_open");
        return -1;
    }

    prev_tm = tm; 

    return 0;
}

//将数据写入flv文件中
int av_record_write(int codec, void *buf, int len, int64_t time_ms, int keyframe)
{
	if(CheckSDStatus()!=1)
	  return -2;
   
	//每次写入前,都要prepare下----->prepare_recording主要是检测文件有没写满1min,是否需要转换文件和
	//sd卡的内存剩余多少需不需要释放里面的空间
	if (prepare_recording(keyframe))  //为0继续写入,为1则退出
        return -1;

    av_packet_t pkt; //该结构体对应一个写入flv文件的一个数据包
    memset(&pkt, 0, sizeof(av_packet_t));

    switch (codec) {
    case 0:
        pkt.media = MediaType_Video;
        pkt.codec = CodecType_H264;
        break;
    case 1:
        pkt.media = MediaType_Audio;
        pkt.codec = CodecType_ALAW;
        break;
    case 2:
        pkt.media = MediaType_Audio;
        pkt.codec = CodecType_MULAW;
        break;
    default:
        printf("unknown codec type %d\n", codec);
        return -1;
    }

    pkt.keyframe = keyframe;
    memcpy(pkt.data, buf, len);
    pkt.size     = len;
    pkt.time_ms  = time_ms; //注意把当前时间也写进入了

    if (flvenc_write(enc, &pkt) < 0)
        return -1;

    return 0;
}

struct av_record_s {
    FlvDec *dec;
};

//打开某个flv文件
av_record_t *av_record_open(int year, int month, int day, int hour, int minute)
{
	//打开一个flv文件可能在时光倒流的时候需要被外部调用，因此需要先检查sd卡是否存在
	if(CheckSDStatus()!=1)
	  return NULL;
    
	char filepath[1024];
    get_filepath(year, month, day, hour, minute, filepath, sizeof(filepath));

    av_record_t *rec = malloc(sizeof(av_record_t));
    if (!rec)
        return NULL;
    memset(rec, 0, sizeof(av_record_t));

    rec->dec = flvdec_open(filepath);
    if (!rec->dec) {
        free(rec);
        return NULL;
    }

    return rec;
}

int av_record_close(av_record_t *rec)
{
    flvdec_close(rec->dec);
    free(rec);
    return 0;
}

//在一个文件里读取一帧的数据,连续调用的话即播放该flv文件的视频,里面存在文件指针
int av_record_read(av_record_t *rec, av_frame_t *frame)
{
	if(CheckSDStatus()!=1)
	  return -2;
    
	av_packet_t pkt; //该结构体对应一帧的数据
    if (flvdec_read(rec->dec, &pkt) < 0)
        return -1;

    int size = min(pkt.size, sizeof(frame->data));

    frame->codec    = pkt.codec;
    frame->keyframe = pkt.keyframe;
    frame->time_ms  = pkt.time_ms;

    memcpy(frame->data, pkt.data, size);
    frame->size = size;

    return 0;
}


static int customFilter(const struct dirent *pDir)
{
	const char *ptr = NULL;
	int name_len = 0, count = 0;  //程序存储的文件名
	ptr = pDir->d_name;
	name_len = strlen(ptr);
		printf("line:%d func:%s name:%s\n",__LINE__, __FUNCTION__,ptr);
		if(name_len == 18 || name_len == 2 || name_len == 10)  //正确的文件名有可能是这三个长度
		{
			if(name_len == 18)   
			{
				for(count=0;count<9;count++)  //若文件名的长度为18的话，那么对应的就是具体flv的文件名,前9个字符必须为数字，第10个字符必须为T  
				{
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
					if(!isdigit(*(ptr+count)))  //若该字符是在'0'-'9'之间的话isdigit会返回1 否则返回0
					  return 0;
				}
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
				if(!strncmp(ptr+count,"T",1))
				  return 1;
				else
				  return 0;
			}

			else if(name_len == 2)   //若文件名的长度为2的话，那么对应的就是某个小时的文件夹，文件名全部应由数字组成
			{
				if(isdigit(*(ptr+count)))
				{
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
					count++;
					if(isdigit(*(ptr+count)))
					  return 1;
					else
					  return 0;
				}
				else
				{
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
					return 0;
				}
			}
			
			else if(name_len == 10) //若文件名的长度为10的话，那么对应的就是具体某一天的文件夹
			{
				for(count=0;count<4;count++)
				{
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
					if(!isdigit(*(ptr+count)))
					  return 0;
				}
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
				if(strncmp(ptr+count,"-",1))
				  return 0;
			
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
				count++;

				for(;count<7;count++)
				{
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
					if(!isdigit(*(ptr+count)))
					  return 0;
				}

					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
				if(strncmp(ptr+count,"-",1))
				  return 0;
				
				count++;
				
				for(;count<10;count++)
				{
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));
					if(!isdigit(*(ptr+count)))
					  return 0;
				}
					printf("line:%d func:%s char:%c\n",__LINE__, __FUNCTION__,*(ptr+count));

				return 1;
			}
	}
	else
	{
		return 0;
	}
}
/*
static int customFilter(const struct dirent *pDir)
{
	//将隐藏的.和..目录过滤掉
	if (strcmp(pDir->d_name, ".") && strcmp(pDir->d_name, "..")) 
	{
		return 1; //返回1即可将此目录结构存到namelist中
	}
	return 0;  //返回0的话表示不想将此目录结构存到namelist中
}
*/

//timeinfos包含指定一天nmemb个视频的时间索引信息
int av_record_search(int year, int month, int day, int *timeinfos, int nmemb)
{
	if(CheckSDStatus()!=1)
	  return -2;

    int count = 0;
    int hour;
	char chartime[20]={0};

	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
	memset(tmp_time, 0, sizeof(tmp_time));

    for (hour = 0; hour < 24; hour++) {
        char dirname[1024];//文件信息结构体
        get_dirname(year, month, day, hour, dirname, sizeof(dirname));
        struct dirent **namelist = NULL;
        int n = scandir(dirname, &namelist, customFilter, alphasort); //将特定目录下的文件信息存储到namelist中
        if (n <= 0)
            continue;
	int count = 0;
	for(count=0;count<n;count++)
	{
		printf("the %d file name:%s\n",count,*(namelist+count));
	}
        int i;
        for (i = 0; i < n; i++) {
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
            record_info_t infos; //该结构体对应一个具体时间(精确到min)
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
            record_info_t *info = &infos;
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
            memset(info, 0, sizeof(record_info_t));
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
			//if(strcmp())
            int ret = sscanf(namelist[i]->d_name, "%04d%02d%02dT%02d%02d.flv",
                    &info->year, &info->month, &info->day, &info->hour, &info->minute);
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
            if (ret != 5)
                continue;
			sprintf(chartime, "%04d-%02d-%02d-%02d-%02d", info->year, info->month, info->day, info->hour, info->minute);
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
			strptime(chartime, "%Y-%m-%d-%H-%M", tmp_time);
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
			timeinfos[count] = mktime(tmp_time); //mktime将时间转换成秒数,方便存储和索引
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
            count++;
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
            if (count >= nmemb)
                break;
        }   
		
        for (i = 0; i < n; i++)
            free(namelist[i]);//scandir 是在堆分配的空间,因此在使用后需要释放掉
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
        free(namelist);
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
    }
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
	free(tmp_time);
    return count;
}
