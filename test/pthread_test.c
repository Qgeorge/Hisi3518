/*
 * =====================================================================================
 *
 *       Filename:  pthread_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/15/2015 02:43:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
int play(void *args)
{
	printf("****play****%d\n", *((int *)args));
	while(1);

}
int main()
{
	pthread_t id;
	int ret;
	int args = 100;
	ret = pthread_create(&id, NULL, (void *)play, &args);
	if(ret != 0)
	{
		return -1;
	}
	pthread_join(id, NULL);
}

