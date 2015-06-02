/**
 * 这段代码的目的是用来演示：设备向手Q发送视频流
 *
 * 首先确保手Q与设备已经绑定，在on_online_status回调函数中判断设备是否登录成功，
 * 在登录成功后调用tx_start_av_service启动SDK的音视频服务，这个函数会设置一些回调，
 * 例如：开始采集视频数据，开始采集音频数据，停止采集视频数据等。
 * 然后SDK内部会启动一个线程处理这些逻辑，这个线程会一直运行直到进程结束。
 * 在手Q向设备请求视频数据时：SDK收到服务器下发的信令，将调用test_start_camera回调函数，
 * 开发者在这个函数中向SDK塞入视频数据，SDK将通过服务器中转（或者打洞直连）的方式将视频数据发送给手Q
 *
 * 为了方便演示，这个例子的视频流从视频文件中获取，不断的从文件中读取数据，通过tx_set_video_data函数发送给对应的手Q。
 * 当然您也可以同时在test_start_mic中通过tx_set_audio_data函数发送音频数据。
 *
 * 期望的结果是：
 * 手Q上有设备发送过来的视频画面
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#include "TXSDKCommonDef.h"
#include "TXDeviceSDK.h"
#include "TXIPCAM.h"

/**
 * 为了简单表示，这里只测试一个视频文件
 */
/********************************************************************/
extern int g_Video_Thread;
extern int sccGetVideoThread();
extern void OnCmdPtz(int ev );


static pthread_t thread_enc_id = 0;


void * myscc()
{
	while(g_Video_Thread)
	{
		printf("hello my god\n");
		sleep(1);
	}
}


/**
 * 开始采集视频的回调
 * 通知摄像头视频链路已经建立，可以通过 tx_set_video_data接口向 AV SDK 填充采集到的视频数据
 * 为了测试，这里是启动了一个线程从文件中读取数据并发送，实际应用中可以启动一个线程去实时采集
 */
bool test_start_camera() {
	printf("###### test_start_camera ###################################### \n");
	g_Video_Thread = 1;
	int ret = pthread_create(&thread_enc_id, NULL, (void *)sccGetVideoThread, NULL);
	//int ret = pthread_create(&thread_enc_id, NULL, (void *)myscc, NULL);
	
	if (ret || !thread_enc_id) {
		g_Video_Thread = 0;
		return false;
	}

	return true;
}

/**
 * 停止采集视频的回调
 * 通知摄像头视频链路已经断开，可以不用再继续采集视频数据。
 */
bool test_stop_camera() {
	printf("###### test_stop_camera ###################################### \n");
	g_Video_Thread = 0;
	if(thread_enc_id !=0) {
		pthread_join(thread_enc_id,NULL);
		thread_enc_id = 0;
	}

	return true;
}

/**
 * 视频码率意味着1s产生多少数据，这个参数跟网络带宽的使用直接相关
 * AV SDK 会根据当前的网络情况和Qos信息，给出建议的bitrate，
 * 上层应用可以根据这个建议值设置Camera的各项参数，如帧率、分辨率，量化参数等，从而获得合适的码率
 */
bool test_set_bitrate(int bit_rate) {
	printf("###### test_set_bitrate  ##################################### %d \n", bit_rate);
	return true;
}

/**
 * 如果I帧丢了，那么发再多的P帧也没有多大意义，AV SDK 会在发现这种情况发生的时候主动通知上层重新启动一个I帧
 */
bool test_restart_gop() {
	printf("###### test_restart_gop ###################################### \n");
	return true;
}

/**
 * 开始采集音频的回调
 * 通知麦克风音频链路已经建立，可以通过 tx_set_audio_data 接口向 AV SDK 填充采集到的音频数据
 * 这里只测试视频，所以这里是空实现
 */
bool test_start_mic() {
	printf("###### test_start_mic ###################################### \n");
	return true;
}

/**
 * 停止采集音频的回调
 * 通知摄像头音频链路已经断开，可以不用再继续采集音频数据
 * 这里只测试视频，所以这里是空实现
 */
bool test_stop_mic() {
	printf("###### test_stop_mic ######################################\n");
	return true;
}

/********************************************************************/

// 标记是否已经启动音视频服务
static bool g_start_av_service = false;

/**
 * 登录完成的通知，errcode为0表示登录成功，其余请参考全局的错误码表
 */
void on_login_complete(int errcode) {
	printf("on_login_complete | code[%d]\n", errcode);
}

/**
 * 在线状态变化通知， 状态（status）取值为 11 表示 在线， 取值为 21 表示  离线
 * old是前一个状态，new是变化后的状态（当前）
 */
void on_online_status(int old, int new) {
	printf("online status: %s\n", 11 == new ? "true" : "false");

	// 上线成功，启动音视频服务
	if(11 == new && !g_start_av_service) {
		// 视频通知：手Q发送请求视频信令，SDK收到后将调用 on_start_camera 函数
		// 这里只测试视频数据
		tx_av_callback avcallback = {0};
		avcallback.on_start_camera = test_start_camera;
		avcallback.on_stop_camera  = test_stop_camera;
		avcallback.on_set_bitrate  = test_set_bitrate;
		avcallback.on_restart_gop  = test_restart_gop;
		avcallback.on_start_mic    = test_start_mic;
		avcallback.on_stop_mic     = test_stop_mic;

		int ret = tx_start_av_service(&avcallback);
		if (err_null == ret) {
			printf(" >>> tx_start_av_service successed\n");
		}
		else {
			printf(" >>> tx_start_av_service failed [%d]\n", ret);
		}

		g_start_av_service = true;
	}
}

/**
 * 辅助函数: 从文件读取buffer
 * 这里用于读取 license 和 guid
 * 这样做的好处是不用频繁修改代码就可以更新license和guid
 */
bool readBufferFromFile(char *pPath, unsigned char *pBuffer, int nInSize, int *pSizeUsed) {
	if (!pPath || !pBuffer) {
		return false;
	}

	int uLen = 0;
	FILE * file = fopen(pPath, "rb");
	if (!file) {
		return false;
	}

	fseek(file, 0L, SEEK_END);
	uLen = ftell(file);
	fseek(file, 0L, SEEK_SET);

	if (0 == uLen || nInSize < uLen) {
		printf("invalide file or buffer size is too small...\n");
		return false;
	}

	*pSizeUsed = fread(pBuffer, 1, uLen, file);

	fclose(file);
	return true;
}

/**
 * 辅助函数：SDK的log输出回调
 * SDK内部调用改log输出函数，有助于开发者调试程序
 */
void log_func(int level, const char* module, int line, const char* message)
{
	//printf("%s\n", message);
}
void OnCmdPtzQQ(int rotate_direction, int rotate_degree)
{
	OnCmdPtz(rotate_degree);
}
/**
 * SDK初始化
 * 例如：
 * （1）填写设备基本信息
 * （2）打算监听哪些事件，事件监听的原理实际上就是设置各类消息的回调函数，
 * 	例如设置CC消息通知回调：
 * 	开发者应该定义如下的 my_on_receive_ccmsg 函数，将其赋值tx_msg_notify对象中对应的函数指针，并初始化：
 *
 * 			tx_msg_notify msgNotify = {0};
 * 			msgNotify.on_receive_ccmsg = my_on_receive_ccmsg;
 * 			tx_init_msg(&msgNotify);
 *
 * 	那么当SDK内部的一个线程收到对方发过来的CC消息后（通过服务器转发），将同步调用 msgNotify.on_receive_ccmsg 
 */
bool initDevice() {
	// 读取 license
	unsigned char license[256] = {0};
	int nLicenseSize = 0;
	if (!readBufferFromFile("/root/qq/licence.sign.file.txt", license, sizeof(license), &nLicenseSize)) {
		printf("[error]get license from file failed...\n");
		return false;
	}

	// 读取guid
	unsigned char guid[32] = {0};
	int nGUIDSize = 0;
	if(!readBufferFromFile("/root/qq/GUID_file.txt", guid, sizeof(guid), &nGUIDSize)) {
		printf("[error]get guid from file failed...\n");
		return false;
	}

	char svrPubkey[256] = {0};
	int nPubkeySize = 0;
	if (!readBufferFromFile("/root/qq/1000000501.pem", svrPubkey, sizeof(svrPubkey), &nPubkeySize))
	{
		printf("[error]get svrPubkey from file failed...\n");
		return NULL;
	}

	// 设备的基本信息
	tx_device_info info = {0};
	info.os_platform            = "Linux";

	info.device_name            = "demo1";
	info.device_serial_number   = guid;
	info.device_license         = license;
	info.product_version        = 1;

	info.product_id             = 1000000501;
	info.server_pub_key         = svrPubkey;

	// 设备登录、在线状态、消息等相关的事件通知
	// 注意事项：
	// 如下的这些notify回调函数，都是来自硬件SDK内部的一个线程，所以在这些回调函数内部的代码一定要注意线程安全问题
	// 比如在on_login_complete操作某个全局变量时，一定要考虑是不是您自己的线程也有可能操作这个变量
	tx_device_notify notify      = {0};
	notify.on_login_complete     = on_login_complete;
	notify.on_online_status      = on_online_status;

	// SDK初始化目录，写入配置、Log输出等信息
	// 为了了解设备的运行状况，存在上传异常错误日志 到 服务器的必要
	// system_path：SDK会在该目录下写入保证正常运行必需的配置信息
	// system_path_capicity：是允许SDK在该目录下最多写入多少字节的数据（最小大小：10K，建议大小：100K）
	// app_path：用于保存运行中产生的log或者crash堆栈
	// app_path_capicity：同上，（最小大小：300K，建议大小：1M）
	// temp_path：可能会在该目录下写入临时文件
	// temp_path_capicity：这个参数实际没有用的，可以忽略
	tx_init_path init_path = {0};
	init_path.system_path = "/tmp/";
	init_path.system_path_capicity = 10 * 1024;
	init_path.app_path = "/tmp/";
	init_path.app_path_capicity = 300* 1024;
	init_path.temp_path = "/tmp/";
	init_path.temp_path_capicity = 10 * 1024;

	// 设置log输出函数，如果不想打印log，则无需设置。
	// 建议开发在开发调试阶段开启log，在产品发布的时候禁用log。
	tx_set_log_func(log_func);

	// 初始化SDK，若初始化成功，则内部会启动一个线程去执行相关逻辑，该线程会持续运行，直到收到 exit 调用
	int ret = tx_init_device(&info, &notify, &init_path);
	if (err_null == ret) {
		printf(" >>> tx_init_device success\n");
	}
	else {
		printf(" >>> tx_init_device failed [%d]\n", ret);
		return false;
	}
	tx_ipcamera_notify ipcamera_notify;
	ipcamera_notify.on_control_rotate = OnCmdPtzQQ;
	ret = ipcamera_set_callback(&ipcamera_notify);
	if (err_null == ret) {
		printf(" >>> set_callback ipc success\n");
        }
        else {
                printf(" >>> set_callbak ipc failed [%d]\n", ret);
                return false;
        }

	return true;
}

