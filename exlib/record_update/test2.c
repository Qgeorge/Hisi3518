#include <stdio.h>
#include <time.h>

int main(int argc, const char * argv[])
{
    struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
    strptime("2015-11-09","%Y-%m-%d",tmp_time);
	printf("the year is %d\n", tmp_time->tm_year);
	printf("the day is %d\n", tmp_time->tm_mday);
	printf("the mon is %d\n", tmp_time->tm_mon);
    time_t t = mktime(tmp_time);
    printf("%ld\n",t);
    free(tmp_time);
    return 0;
}
