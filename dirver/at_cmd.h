#ifndef __AT_CMD_H_
#define __AT_CMD_H_

#include "stdint.h"

#define RET_OK										(1)
#define ERR_UNKNOWN								(0)
#define ERR_NO_M6315							(-1)
#define ERR_NO_SIM_CARD						(-2)
#define ERR_CS_NET_FAILED					(-3)
#define ERR_PS_NET_FAILED					(-4)
#define ERR_SIGNAL_WEAK						(-5)
#define ERR_SET_APN_FAILED				(-6)
#define ERR_ENABLE_PDP_FAILED			(-7)
#define ERR_ENABLE_IP_HEAD_FAILED	(-8)
#define ERR_IP_CMD								(-9)
#define ERR_IP_CONNECT						(-10)
#define ERR_FTP_CMD								(-11)
#define ERR_FTP_CONNECT						(-12)
#define ERR_PARAMETER							(-13)
#define ERR_IP_OR_HOST						(-14)
#define ERR_FTP_OPEN							(-15)
#define ERR_DNS_CMD								(-16)
#define ERR_FTP_FILE_SIZE					(-17)
#define ERR_ENABLE_MUX						(-18)

typedef int8_t(*IP_RX_CALLBACK_FUNC)(void *data, uint16_t len);

// 初始化m6315模块
int8_t init_m6315(void);
// 通过dns获取ip地址
int8_t m6315_dns_get_ip(char *host, char *ip);
/* 建立ip连接 
 *	ip：IP地址或域名
 *	port：端口号
 *	rx_callback；ip数据接收回调函数
 *	返回值：socket句柄（小于等于0 失败，大于0成功）
*/
int8_t m6315_socket_open(char *ip, char *port, IP_RX_CALLBACK_FUNC rx_callback);
/* 关闭ip连接 
 *	fd：ip连接socket句柄
 *	返回值：（<=0失败，RET_OK 成功）
*/
int8_t m6315_socket_close(int8_t fd);
/* 关闭所有打开的ip连接 
 *	返回值：（<=0失败，RET_OK 成功）
*/
int8_t m6315_socket_close_all(void);
/* 发送ip数据包
 *	fd：创建的ip连接句柄
 *	data：发送的ip数据
 *	len：发送的ip数据长度
 *	返回值：（<=0失败，RET_OK 成功）
*/
int8_t m6315_socket_send(int8_t fd, uint8_t *data, uint16_t len);
/* ip连接接收数据处理进程
*/
void m6315_ip_process(void);
/* 建立ftp连接
 *	ip：ftp服务器ip地址或域名地址
 *	port：ftp服务器端口
 *	user：ftp用户名
 *	passwd：ftp用户密码
 *	返回值：（<=0失败，RET_OK 成功）
*/
int8_t m6315_ftp_open(char *ip, char *port, char *user, char *passwd);
/* 数字转字符串
*/
void at_itoa(uint32_t position, char *str);
/* 字符串转数字
*/
uint32_t at_atoi(char *len_str);
/* ftp文件下载
 *	file_name：文件名
 *	position：下载文件偏移位置
 *	buf：文件数据缓冲区
 *	now_len：当前需要下载的文件长度
 *	返回值：（<=0失败，RET_OK 成功）
*/
int8_t m6315_ftp_get_file(char *file_name, uint32_t position, void *buf, uint16_t now_len);
/* ftp获取文件长度
 *	file_name：文件名
 *	返回值：（<=0失败，>0 文件长度）
*/
int32_t m6315_ftp_get_file_len(char *file_name);
/* 关闭ftp连接
 *	返回值：（<=0失败，>0 文件长度）
*/
int8_t m6315_ftp_close(void);

#endif /* __AT_CMD_H_ */
