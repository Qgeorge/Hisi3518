/*
 * =====================================================================================
 *
 *       Filename:  net_http.h
 *
 *    Description:  http的接口文件
 *
 *        Version:  1.0
 *        Created:  09/14/2015 06:14:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbiaobiao (biaobiao), wang_shengbiao@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __NET_HTTP_H__
#define __NEI_HTTP_H__

#ifdef _cplusplus
extern "C" {
}
#endif

/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_bind_device
 *  Description:	用户设备绑定  
 * =====================================================================================
 */
int net_bind_device (char *UserId, char *DeviceId);

/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_create_device
 *  Description:   设备添加注册
 * =====================================================================================
 */
int net_create_device (char *DeviceId);

/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_get_key
 *  Description:  获取P2P 密钥
 * =====================================================================================
 */
int net_get_key (char *DeviceId, int *key);
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_get_key
 *  Description: 获取设备id
 * =====================================================================================
 */
int get_device_id(char *DeviceId);
/* 
 * ===  FUNCTION  ======================================================================
 *  Name:  net_create_device
 *  Description:   设备修改
 * =====================================================================================
 */
int net_modify_device (char *DeviceId);

#ifdef _cplusplus
}
#endif

#endif
