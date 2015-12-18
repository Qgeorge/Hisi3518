#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int detect_process(char * process_name)  
{  
        FILE *ptr;  
        char buff[512] = {0}; 
        char ps[128];  
        sprintf(ps,"ps  | grep -c %s",process_name);
		printf("the process_name is %s\n", ps);
		if((ptr=popen(ps, "r")) != NULL)
        {  
                while (fgets(buff, 512, ptr) != NULL)  
                {
					int a = atoi(buff);
					printf("the a is %d\n", a);
					if(a >= 2)  
					{  
						pclose(ptr);  
						return 0;  
					}  
				}  
		}  
		pclose(ptr);  
		return -1;  
}
int main()
{
	int a;
	a = detect_process("wpa");
	printf("%d\n", a);
}
