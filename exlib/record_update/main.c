#include <stdio.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include <time.h>
#include "record.h"

void get_dirname(int year, int month, int day, int hour, char *dirname, int len)
{
	    snprintf(dirname, len, "%s/%04d-%02d-%02d/%d", "/mnt/mmc/uusmt", year, month, day, hour);
		printf("the dir name is %s\n", dirname);
}

int av_record_search(int year, int month, int day, int *timeinfos, int nmemb)
{
    int count = 0;
    int hour;
	char chartime[40]={0};

	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
	memset(tmp_time, 0, sizeof(tmp_time));
    for (hour = 0; hour < 24; hour++) {
        char dirname[1024];
        get_dirname(year, month, day, hour, dirname, sizeof(dirname));
        struct dirent **namelist = NULL;
        int n = scandir(dirname, &namelist, 0, alphasort);
		//printf("the number is %d\n", n);
        if (n <= 0)
            continue;

        int i;
        for (i = 0; i < n; i++) {
            record_info_t infos;
            record_info_t *info = &infos;
            memset(info, 0, sizeof(record_info_t));
			//if(strcmp())
            int ret = sscanf(namelist[i]->d_name, "%04d%02d%02dT%02d%02d.flv",
                    &info->year, &info->month, &info->day, &info->hour, &info->minute);
            if (ret != 5)
                continue;
			sprintf(chartime, "%04d-%02d-%02d-%02d-%02d", info->year, info->month, info->day, info->hour, info->minute);
			//printf("char time is ****************%s\n", chartime);
			strptime(chartime, "%Y-%m-%d-%H-%M", tmp_time);
			timeinfos[count] = mktime(tmp_time);
			//printf("make time is %d\n", timeinfos[count]);
			//printf("the dirname is %s\n", namelist[i]->d_name);
			//printf("the count is %d\n", count);
            count++;
            if (count >= nmemb)
                break;
        }   
		
        for (i = 0; i < n; i++)
            free(namelist[i]);
        free(namelist);
    }
	free(tmp_time);
    return count;
}

#if 0
int main()
{
	int count = 0;

	typedef struct info                                                                                                                                                     {       int cmdid;
		    int pinfo[0];
	}info;
	while(1)
	{
		info *sendinfo = (info *)malloc(sizeof(info)+sizeof(int)*60*24);

		//	av_record_init("/mnt/mmc");
		count = av_record_search(2014, 4, 11, sendinfo->pinfo, 24*60);


		int i;
		for(i = 0; i < count; i++)
		{
			printf("the min is %d\n", sendinfo->pinfo[i]);
		}
		free(sendinfo);
	}
	printf("the count is %d\n", count);
	return 0;
}
#endif
