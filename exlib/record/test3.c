/*
   scandir函数
   */
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int customFilter(const struct dirent *pDir)
{
	if (strcmp(pDir->d_name, ".") && strcmp(pDir->d_name, ".."))
	{
		return 1;
	}
	return 0;
}

inline int get_times(char *cstr)
{
	printf("the cstring is %s\n", cstr);	
	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
	memset(tmp_time, 0, sizeof(struct tm));
	strptime(cstr,"%Y-%m-%d",tmp_time);
	//  strptime("2014-11-04","%Y-%m-%d",tmp_time);
	printf("the year is %d\n", tmp_time->tm_year);
	printf("the day is %d\n", tmp_time->tm_mday);
	printf("the mon is %d\n", tmp_time->tm_mon);
	time_t t = mktime(tmp_time);
	printf("%ld\n",t);
	free(tmp_time);
	return t;

}

int get_time_line(int *timeline)
{
	struct dirent **namelist;

	int n;
	int i;
	char stime[20] = {0};
	char etime[20] = {0};

	n = scandir("/mnt/mmc", &namelist, customFilter, alphasort);

	if (n < 0)
	{
		perror("scandir");
	}
	else
	{
		timeline[0] = get_times(namelist[0]->d_name);
		timeline[1] = get_times(namelist[n-1]->d_name);

		for (i = 0; i < n; i++)
		{
			printf("%s\n", namelist[i]->d_name);
			free(namelist[i]);
		}
		free(namelist);
	}
}
/*
   int main()
   {

   get_time_line();
   }
   */
