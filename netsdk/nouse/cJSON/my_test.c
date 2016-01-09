/*
 * =====================================================================================
 *
 *       Filename:  my.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/22/2015 07:38:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>  
#include <stdlib.h>  
#include "cJSON.h"  
// 被解析的JSON数据包  
//char text[] = "{\"timestamp\":\"2013-11-19T08:50:11\",\"value\":1}";  
//char text[] = "{\"errorCode\":0,\"obj\":{\"company_id\":0,\"apk_url\":\"www.uucam.com\"}";
char text[] = "{\"errorCode\":0,\"obj\":{\"company_id\":100, \"url\":\"wwww.uucam.com\"}}";

int main (int argc, const char * argv[])  
{  
    cJSON *json , *json_value , *json_timestamp;
	int id;
	char *uri;
    // 解析数据包  
    json = cJSON_Parse(text);  
    if (!json)  
    {  
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());  
    }  
    else  
    {  
        // 解析开关值  
        json_value = cJSON_GetObjectItem(json , "obj");
        uri = cJSON_GetObjectItem(json_value , "url")->valuestring;
		printf("the id is %s\n", uri);

        cJSON_Delete(json);  
    }  
    return 0;  
}
