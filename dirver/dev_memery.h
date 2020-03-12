#ifndef __DEV_MEMERY_H__
#define __DEV_MEMERY_H__

#include "stdint.h"

#define ERR_REG_UNKNOWN								(0)							//未知错误
#define ERR_REG_READ_LEN							(-1)						//读取数据长度错误
#define ERR_REG_WRITE_LEN							(-2)						//写入数据长度错误
#define ERR_REG_ADDR									(-3)						//寄存器地址错误

__weak void get_board_param(void *param, uint32_t len);
__weak void save_board_param(void *param, uint32_t len);
__weak void update_software(void);
__weak uint32_t get_system_time(void);
__weak void soft_restart(void);
__weak void dev_motor_on(uint8_t index);
__weak void dev_motor_off(uint8_t index);

// 设备状态维护函数
void device_process(void);

/* 获取设备sn长度
 *	返回值：sn长度
*/
uint8_t dev_get_sn_len(void);


/* 比较sn与设备是否相同
 *	sn：需要比较的sn数据
 *	返回值：(0 sn相同，不等于0 sn不同)
*/
int8_t dev_sn_cmp(void *sn);


/* 设备重启
 *	time：时间戳
 *	返回值：无
*/
void dev_restart(uint32_t time);


/* 寄存器独缺函数
 *	addr：寄存器地址
 *	len：读取数据长度
 *	data：读取数据缓冲区 
 *	返回值：<=0 失败，>0 读取的实际数据长度
*/
int32_t dev_mem_read(uint32_t addr, uint32_t len, void *data);


/* 寄存器写入函数
 *	addr：寄存器地址
 *	len：写入数据长度
 *	data：写入数据
 *	返回值：<=0 失败，>0 写入的实际数据长度
*/
int32_t dev_mem_write(uint32_t addr, uint32_t len, void *data);

/* 版本号比较函数
 *	board_ver：硬件版本
 *	soft_ver：软件版本
*/
void dev_check_version(uint32_t board_ver, uint32_t soft_ver);

/* 心跳序号检查函数
 *	seq：需要检查的序列号
*/
void dev_check_heart_seq(uint32_t seq);

/* 请求数据完成函数
 *	time：时间戳
 *	addr：请求数据地址
 *	len：请求数据长度
 *	data：返回的请求数据
*/
void dev_request_data_done(uint32_t time, uint32_t addr, uint32_t len, void *data);

/* sn号获取函数
 *	sn：sn缓冲区
*/
void dev_get_sn(void *sn);

/* 获取硬件版本号
 *	返回值：硬件版本号
*/
uint32_t dev_get_board_version(void);

/* 获取软件版本号
 *	返回值：软件版本号
*/
uint32_t dev_get_soft_version(void);


/* 获取心跳地址
 *	返回值：心跳地址
*/
uint32_t dev_get_heart_addr(void);

/* 获取心跳包长度
 *	返回值：心跳长度
*/
uint32_t dev_get_heart_len(void);

/* 获取心跳序号
 *	返回值：心跳序号
*/
uint32_t dev_get_heart_seq(void);

/* 获取心数据
 *	data：心跳数据缓冲区
 *	返回值：心跳数据长度
*/
uint32_t dev_get_heart_data(void *data);

/* 获取时间戳
 *	返回值：当前时间戳
*/
uint32_t dev_get_time(void);

#endif /* __DEV_MEMERY_H__ */

