#include <stdio.h>
#include <dirent.h>
#include <time.h>
int main()
{
	struct tm tmp_time;
	char chartime[100] = {0};
	int time;

    struct dirent **namelist;
    int n;
    n = scandir("/mnt/mmc", &namelist, 0, alphasort);
    if (n < 0)
        perror("scandir");
    else
    {
        while(n--)
        {
            printf("%s\n", namelist[n]->d_name);
            free(namelist[n]);
        }
        free(namelist);
    }

#if 0
	sprintf(chartime, "%04d-%02d-%02d-%02d-%02d", 2014, 4,11,0,0);
	printf("the time is %s\n", chartime);
	strptime(chartime, "%Y-%m-%d-%H-%M", &tmp_time);
	time = mktime(&tmp_time);
	printf("the time i %d\n", time);
#endif

}
