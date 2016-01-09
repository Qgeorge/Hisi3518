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
char text[] = "{\"timestamp\":\"2013-11-19T08:50:11\",\"value\":1}";  
int main (int argc, const char * argv[])  
{  
    cJSON *json , *json_value , *json_timestamp;  
    // 解析数据包  
    json = cJSON_Parse(text);  
    if (!json)  
    {  
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());  
    }  
    else  
    {  
        // 解析开关值  
        json_value = cJSON_GetObjectItem( json , "value");  
        if( json_value->type == cJSON_Number )  
        {  
            // 从valueint中获得结果  
            printf("value:%d\r\n",json_value->valueint);  
        }  
        // 解析时间戳  
        json_timestamp = cJSON_GetObjectItem( json , "timestamp");  
        if( json_timestamp->type == cJSON_String )  
        {  
            // valuestring中获得结果  
            printf("%s\r\n",json_timestamp->valuestring);  
        }  
        // 释放内存空间  
        cJSON_Delete(json);  
    }  
    return 0;  
}
