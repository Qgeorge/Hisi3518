/*
 * =====================================================================================
 *
 *       Filename:  test_main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/16/2015 03:14:50 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
typedef struct time_st
{
	int year;
	int month;
	int day;
}time_st;

//将字符串的日期时间转换成整形
int convert_str(time_st * tm, char *stime)
{
	char *delim = "-";
	char *p;
	printf("the time : %s\n",stime);
	tm->year = atoi(strtok(stime, delim)); 
	printf("the year : %d\n",tm->year);
	tm->month = atoi(strtok(NULL, delim));
	printf("the month : %d\n",tm->month);
	tm->day = atoi(strtok(NULL, delim));
	printf("%d %d %d\n", tm->year, tm->month, tm->day);
	return 0;
}

int filter_numer(int num[], int len, int get_num[])
{

//	int num[10] = {2, 3, 4, 7, 9, 10, 11, 19, 20,21};
//	int tmp_num[10] = {0};
	int step = 60;  //单位是秒
//	int ext_num[10] = {0};
	int preval = -1;
	int afterval;
	int i = 0;
	int j = 0;
	int k = 0;
	for(i = 0; i < len; i++)
	{
		if((i+1) == len)
		{
			afterval = -1;
		}
		afterval = num[i+1];
		if((num[i] != (preval + step)) || (num[i] != (afterval - step)))
		{
			printf("##############%d\n", num[i]);
			get_num[j] = num[i];
			j++;
		}
		if((num[i] != (preval + step)) && (num[i] != (afterval - step)))
		{
			printf("###############%d\n", num[i]);
			get_num[j] = num[i];
			j++;
		}
		preval = num[i];
	}
	return j;

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
			printf("line:%d func:%s\n",__LINE__, __FUNCTION__);
		return 0;
	}
}


int record_list(int get_num[], int len)
{
	struct dirent **namelist;
	int n;
	int year;
	int month;
	int day;
	int *p = NULL;
	int num[60*24] = {0}; //60*24表示一天里的每一分钟
	//int get_num[1000];
	int get_num_len = 0;
	int all_len = 0;

	time_st tmp_time_st;
	int number = 0;
	p = get_num;
	int i = 0;
	n = scandir("/mnt/mmc/uusmt", &namelist, customFilter, alphasort);
	int count = 0;
	for(count=0;count<n;count++)
	{
		printf("the %d file name:%s\n",count,*(namelist+count));
	}

	if (n < 0)
	{
		perror("not found\n");
	}
	else
	{
		printf("*******************%d\n", time(NULL));
		while(n--)
		{
			convert_str(&tmp_time_st, namelist[n]->d_name);  //fix me ...
			//sprintf(namelist[n]->d_name,"%d-%d-%d",year, month, day);
			//printf("the %d %d %d\n", year, month, day);
			
			//查找某天的所有视频
			number = av_record_search(tmp_time_st.year, tmp_time_st.month, tmp_time_st.day, num, 60*24);
			#if 0 
			for(i = 0; i < number; i++)
			{
				printf("******%d\n", num[i]);
			}
			#endif
			printf("%s\n", namelist[n]->d_name);
			get_num_len = filter_numer(num, number, p);
			p = get_num_len + p + 2;
			all_len = get_num_len + all_len + 2;
			get_num[all_len] = 0;
			get_num[all_len+1] = 0;

			free(namelist[n]);
		}
		printf("*******************%d\n", time(NULL));
		printf("the all_len is %d\n", all_len);
		for(i=0; i < all_len; i++)
		{
			printf("@@@@@@@@@@@@@@@@@@@@@%d\n", get_num[i]);
		}
		free(namelist);
	}
	return all_len;
}

