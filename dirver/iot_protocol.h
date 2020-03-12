#ifndef __IOT_PROTOCOL_H__
#define __IOT_PROTOCOL_H__

#include "stdint.h"

__weak void protocol_send(void *buf, uint16_t len);

/* 协议数据解析函数
 *	data：接收到的数据
 *	len：数据长度
 *	返回值：无
*/
void parse_protocol(void *data, uint16_t len);


/* 设备注册函数
 *	返回值：1成功，不等于1失败
*/
int device_register_request(void);


/* 心跳上报函数
 *	返回值：1成功，不等于1失败
*/
int device_heart_update(void);


/* 设备数据请求函数
 *	addr：请求数据地址
 *	len：请求数据长度
 *	返回值：1成功，不等于1失败
*/
int device_data_request(uint32_t addr, uint32_t len);


#endif /* __IOT_PROTOCOL_H__ */

