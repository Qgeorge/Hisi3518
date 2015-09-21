#ifndef __PROTOCOL_JSON_H__
#define __PROTOCOL_JSON_H__

typedef struct request_st
{
	char *uri;
	char *requethttp;
	char *rsqrequeshttp;
}request_st;

#define Creat_Str "{\
\"token\":\"token\",\
\"client_type\":\"device\",\
\"obj\":{\
\"device_id\":\"%s\",\
\"device_type\":%d,\
\"sub_type\":%d,\
\"device_verison\":\"%s\",\
\"device_version_num\":\"%s\",\
\"device_produce_num\":\"%s\"\
}\
}"

#define  Rsp_Creat_Str "{ \
\"obj\":\"%d\",\
\"errorCode\":%d\
}"

#define BindDevice_Str "{ \
\"token\":\"token\", \
\"client_type\":\"device\", \
\"obj\":{ \
\"device_id\":\"%s\", \
\"user_name\":\"%s\" \
} \
}"

#define Rsp_BindDevice_Str "{ \
\"obj\":\"%d\",\
\"errorCode\":%d\
}"


#define GetP2PKey_Str "{ \
\"token\":\"token\", \
\"client_type\":\"device\", \
\"obj\":\"%s\" \
}"

#define Rsp_GetP2PKey_Str "{\
\"obj\":\"%d\",\
\"errorCode\":%d\
}"

#define GET_KEY_URI  "http://120.25.216.218:8080/UUSmartHome/device/Device!GetP2PKey.action"
#define BIND_DEV_URI  "http:// 120.25.216.218:8080/UUSmartHome/device/Device!BindDevice.action"
#define CREATE_DEV_URI  "http://120.25.216.218:8080/UUSmartHome/device/Device!Create.action"

#endif
