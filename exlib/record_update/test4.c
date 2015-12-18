#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#define MAX 1024

int get_file_count(char *root)
{
	DIR *dir;
	struct dirent * ptr;
	int total = 0;
	char path[MAX];
	dir = opendir(root); /* 打开目录*/
	if(dir == NULL)
	{
		perror("fail to open dir");
		exit(1);
	}

	errno = 0;
	while((ptr = readdir(dir)) != NULL)
	{
		//顺序读取每一个目录项；
		//跳过“..”和“.”两个目录
		if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0)
		{
			continue;
		}
		//printf("%s%s/n",root,ptr->d_name);
		//如果是目录，则递归调用 get_file_count函数

		if(ptr->d_type == DT_DIR)
		{
			sprintf(path,"%s%s/",root,ptr->d_name);
			//printf("%s/n",path);
			total += get_file_count(path);
		}

		if(ptr->d_type == DT_REG)
		{
			total++;
			//printf("%s%s/n",root,ptr->d_name);
		}
	}
	if(errno != 0)
	{
		printf("fail to read dir"); //失败则输出提示信息
		exit(1);
	}
	closedir(dir);
	return total;
}

int main(int argc, char * argv[])
{
	int total;
	if(argc != 2)
	{
		printf("wrong usage/n");
		exit(1);
	}
	total = get_file_count(argv[1]);
	printf("%s ha %d files/n",argv[1],total);
	return 0;
} 
