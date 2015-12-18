/*
 * =====================================================================================
 *
 *       Filename:  test_main.c
 *
 *    Description:  idfsafds
 *
 *        Version:  1.0
 *        Created:  10/25/2015 01:31:53 PM
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
#include <string.h>

int main()
{
	int num[10] = {2, 3, 4, 7, 9, 10, 11, 19, 20,21};
	int tmp_num[10] = {0};
	int ext_num[10] = {0};
	int preval = -1;
	int afterval;
	int i = 0;
	int j = 0;
	int k = 0;
	for(i = 0; i < 10; i++)
	{
		if((i+1) == 10)
		{
			afterval = -1;
		}
		afterval = num[i+1];
		if((num[i] != (preval +1)) || (num[i] != (afterval -1)))
		{
			printf("%d\n", num[i]);

		}
		if((num[i] != (preval +1)) && (num[i] != (afterval -1)))
		{
			printf("%d\n", num[i]);
		}
		preval = num[i];
	}
	return 0;
}

