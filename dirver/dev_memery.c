#include "dev_memery.h"
#include "string.h"
#include "iot_protocol.h"

#define REG_CTRL_MASK							(0xff000000)
#define REG_GET_CTRL(addr)				((addr) & REG_CTRL_MASK)
#define REG_GET_OFFSET(addr)			((addr) & 0xff)
#define REG_READ_ONLY							(0x00000000)	//只读寄存器
#define REG_READ_AND_WRIT					(0x01000000)	// 可写寄存器
#define REG_SET										(0x02000000)	// 写1置1寄存器
#define REG_CLR										(0x03000000)	// 写1清零寄存器

// 只读寄存器地址
#define DEV_SN_RO_REG							(0x00000000) // 设备ID寄存器地址
#define DEV_HARD_WARE_RO_REG			(0x00000008) //	硬件版本号地址
#define DEV_SOFT_WARE_RO_REG			(0x0000000c) //	软件版本号地址
#define DEV_NET_TYPE_RO_REG				(0x00000010) //	网络连接方式
#define DEV_RTC_TIME_RO_REG				(0x00000014) //	RTC时间戳

// 可写寄存器
#define DEV_RTC_TIME_RW_REG				(0x01000014) // RTC时间戳
#define DEV_FTP_USER_RW_REG				(0x01000100) // 固件FTP账号
#define DEV_FTP_PWD_RW_REG				(0x01000110) // 固件FTP密码
#define DEV_FTP_PORT_RW_REG				(0x01000120) // 固件FTP端口
#define DEV_FTP_HOST_RW_REG				(0x01000122) // 固件FTP地址

// 设备运行信息只读寄存器
#define DEV_RELAY_STATE_RO_REG			(0x00010000) // 0-3bit 4路继电器器状态，0-断开，1-闭合
#define DEV_LEVEL_STATE_RO_REG			(0x00010001) // 0-3bit 低液位浮球，0-未达到，1-液位过低，6-7bit 高液位浮球，0-未达到，1-液位过高
#define DEV_FAN_CURRENT_RO_REG			(0x00010002) // 风机电流，			单位mA
#define DEV_CURRENT_1_RO_REG				(0x00010004) // 模拟输入1采集值，5-20mA对应 5000 ~ 20000
#define DEV_CURRENT_2_RO_REG				(0x00010006) // 模拟输入2采集值，5-20mA对应 5000 ~ 20000

// 继电器控制可写
#define DEV_RELAY1_RW_REG						(0x01010000) // 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
#define DEV_RELAY2_RW_REG						(0x01010002) // 2继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
#define DEV_RELAY3_RW_REG						(0x01010004) // 3继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
#define DEV_RELAY4_RW_REG						(0x01010006) // 4继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒

// 心跳地址固定为 0x00010002
#define HEART_ADDR							(0x00010000)
#define HEART_LEN								(0x08)

#define DEV_SN_LEN								(8)
#define FTP_USER_NAME_LEN					(16)
#define FTP_PWD_LEN								(16)
#define FTP_HOST_NAME_LEN					(62)

#define DEV_NET_2G						(0x01)				//设备使用2G网络

#define PARAM_INIT_FLAG				(0xebea7fcc)

#define RELAY_ON							(0xffff)
#define RELAY_OFF							(0x0000)

#define MAX_RELAY_NUMBERS			(4)					//继电器个数

// 设备注册标志
#define DEV_REGISTER_FLAG			(0xa5)
static uint8_t register_flag = 0;

// 设备基本信息
typedef struct {
	uint8_t sn[DEV_SN_LEN];			// 设备id号
	uint32_t hard_ver;	// 设备硬件版本号
	uint32_t soft_ver;	// 设备软件版本号
	uint8_t net_type;		// 设备联网类型
} board_info_st;

// ftp配置信息
typedef struct {
	uint8_t user[FTP_USER_NAME_LEN];		// ftp用户名
	uint8_t pwd[FTP_PWD_LEN];		// ftp密码
	uint16_t port;			// ftp端口号
	uint8_t host[FTP_HOST_NAME_LEN];		// ftp主机地址
}ftp_info_st;

// 保存到flash的信息，上电时拷贝到内存，变化是保存到flash
typedef struct {
	uint32_t flag;
	uint32_t rtc;										//RTC时间戳
	ftp_info_st ftp_info;						//ftp配置信息
} board_param_st;

// 内存维护的运行状态
typedef struct{
	uint8_t relay_state;												//0-3bit 4路继电器器状态，0-断开，1-闭合
	uint8_t level_state;												//0-3bit 低液位浮球，0-未达到，1-液位过低，6-7bit 高液位浮球，0-未达到，1-液位过高
	uint16_t fan_current;												//风机电流，			单位mA
	uint16_t current_1;													//模拟输入1采集值，5-20mA对应 5000 ~ 20000
	uint16_t current_2;													//模拟输入2采集值，5-20mA对应 5000 ~ 20000
	uint16_t relay_ctrl[MAX_RELAY_NUMBERS]; 		//1到4号继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
}device_run_state_st;

const static board_info_st sboard_info = {
	.sn = "TEST-IOT",
	.hard_ver = 0xf0000001,
	.soft_ver = 0xf0000001,
	.net_type = DEV_NET_2G,
};

const static board_param_st sboard_param = {
	.flag = PARAM_INIT_FLAG,
	.rtc = 1583892299,				
	.ftp_info = {
		.user = "test",
		.pwd = "hCtx3aKMKhpXmwPn",
		.port = 8000,
		.host = "118.89.79.241",
	},
};

static board_info_st *p_board_info = (board_info_st *)&sboard_info;
static board_param_st board_param;
static device_run_state_st device_run_state;

__weak void get_board_param(void *param, uint32_t len)
{
	memcpy(param, &sboard_param, len);
}

__weak void save_board_param(void *param, uint32_t len)
{
	
}

static void init_param(char *ftp_user, char *ftp_pwd, uint16_t ftp_port, char *ftp_host)
{
	get_board_param(&board_param, sizeof(board_param));
	// 判断参数是否初始化
	if(board_param.flag != PARAM_INIT_FLAG){
		if(NULL == ftp_user || NULL == ftp_pwd || NULL == ftp_port || NULL == ftp_host){
			memcpy(&board_param, &sboard_param, sizeof(board_param));
		}else{
			uint8_t user_len = strlen(ftp_user);
			uint8_t pwd_len = strlen(ftp_pwd);
			uint8_t host_len = strlen(ftp_host);
			
			// 传入参数错误使用默认参数
			if(user_len > FTP_USER_NAME_LEN || pwd_len > FTP_PWD_LEN || host_len > FTP_HOST_NAME_LEN){
				memcpy(&board_param, &sboard_param, sizeof(board_param));
			}else{
				// 传入参数争取初始化标志
				board_param.flag = PARAM_INIT_FLAG;
				// 获取时间戳
				board_param.rtc = get_system_time();
				// 设置ftp用户名
				memset(board_param.ftp_info.user, 0, FTP_USER_NAME_LEN);
				memcpy(board_param.ftp_info.user, ftp_user, user_len);
				// 设置ftp密码
				memset(board_param.ftp_info.pwd, 0, FTP_PWD_LEN);
				memcpy(board_param.ftp_info.pwd, ftp_pwd, pwd_len);
				// 设置fpt端口号
				board_param.ftp_info.port = ftp_port;
				// 设置ftp服务器地址
				memset(board_param.ftp_info.host, 0, FTP_HOST_NAME_LEN);
				memcpy(board_param.ftp_info.host, ftp_host, host_len);
				
			}
		}
		// 保存参数
		save_board_param(&board_param, sizeof(board_param));
	}
}

void device_init(void *addr, char *ftp_user, char *ftp_pwd, uint16_t ftp_port, char *ftp_host)
{
	p_board_info = (board_info_st *)addr;
	init_param(ftp_user, ftp_pwd, ftp_port, ftp_host);
	get_board_param(&board_param, sizeof(board_param));
	// 初始化运行状态
	memset(&device_run_state, 0, sizeof(device_run_state));
}

// 获取设备sn长度
uint8_t dev_get_sn_len(void)
{
	return DEV_SN_LEN;
}


// 比较sn与设备是否相同
int8_t dev_sn_cmp(void *sn)
{
	return memcmp(sn, p_board_info->sn, DEV_SN_LEN);
}

__weak void soft_restart(void)
{

}

// 设备重启
void dev_restart(uint32_t time)
{
	soft_restart();
}

int32_t copy_rtc_time(void *data, uint32_t len)
{
	uint32_t data_len = sizeof(board_param.rtc);
	uint8_t *p = NULL;
	uint8_t *d = (uint8_t *)data;
	p = (uint8_t *)&board_param.rtc;
	// 读取时间戳长度必须大于等于时间戳的长度
	if(len >= data_len){
		memcpy(d, p, data_len);
		return data_len;
	}else{
		return ERR_REG_READ_LEN;
	}
}

// 寄存器读取函数
int32_t dev_mem_read(uint32_t addr, uint32_t len, void *data)
{
	int32_t ret = 0;
	uint16_t offset = 0;
	uint32_t data_len = 0;
	uint8_t *p = NULL;
	uint8_t *d = (uint8_t *)data;
	if(REG_GET_CTRL(addr) == REG_READ_ONLY || REG_GET_CTRL(addr) == REG_READ_AND_WRIT){
		switch(addr){
			// 只读设备信息寄存器
			case DEV_SN_RO_REG:                  // 设备ID寄存器地址
			case DEV_HARD_WARE_RO_REG:           //	硬件版本号地址
			case DEV_SOFT_WARE_RO_REG:           //	软件版本号地址
			case DEV_NET_TYPE_RO_REG:{           //	网络连接方式
				// 寄存器地址偏移
				offset = REG_GET_OFFSET(addr);
				// 设备信息可读长度
				data_len = REG_GET_OFFSET(DEV_RTC_TIME_RO_REG) - offset;
				// 读取寄存器地址
				p = (uint8_t *)p_board_info + offset;
				if(len <= data_len){
					// 数据小于设备信息数据直接拷贝
					memcpy(d, p, len);
					return len;
				}else{
					// 拷贝设备信息数据
					memcpy(d, p, data_len);
					// 拷贝rtc数据
					ret = copy_rtc_time(d+data_len, len - data_len);
					if(ret > 0){
						return ret + data_len;
					}else{
						return ret;
					}
				}
			}//break;
			// 只读寄存器
			case DEV_RTC_TIME_RO_REG:						//	RTC时间戳
			// 可写寄存器
			case DEV_RTC_TIME_RW_REG:{					//	RTC时间戳
				return copy_rtc_time(data, len);
			}//break;
			// 可写ftp设置信息寄存器
			case DEV_FTP_USER_RW_REG:							// 固件FTP账号
			case DEV_FTP_PWD_RW_REG:							// 固件FTP密码
			case DEV_FTP_PORT_RW_REG:							// 固件FTP端口
			case DEV_FTP_HOST_RW_REG:{						// 固件FTP地址
				// 读取寄存器地址偏移
				offset = REG_GET_OFFSET(addr);
				// 可以读取的最大数据
				data_len = REG_GET_OFFSET(DEV_FTP_HOST_RW_REG) + FTP_HOST_NAME_LEN - offset;
				// 赋值拷贝寄存器地址
				p = (uint8_t *)&board_param.ftp_info + offset;
				// 有效可读取数据长度
				ret = len > data_len ? data_len : len;
				memcpy(data, p, ret);
				return ret;
			}//break;
			// 只读设备运行状态寄存器
			case DEV_RELAY_STATE_RO_REG:					// 0-3bit 4路继电器器状态，0-断开，1-闭合
			case DEV_LEVEL_STATE_RO_REG:					// 0-3bit 低液位浮球，0-未达到，1-液位过低，6-7bit 高液位浮球，0-未达到，1-液位过高
			case DEV_FAN_CURRENT_RO_REG:					// 风机电流，			单位mA
			case DEV_CURRENT_1_RO_REG:						// 模拟输入1采集值，5-20mA对应 5000 ~ 20000
			case DEV_CURRENT_2_RO_REG:{						// 模拟输入2采集值，5-20mA对应 5000 ~ 20000寄存器地址偏移
				offset = REG_GET_OFFSET(addr);
				// 设备信息可读长度
				data_len = REG_GET_OFFSET(DEV_CURRENT_2_RO_REG) + sizeof(device_run_state.current_2) - offset;
				// 读取寄存器地址
				p = (uint8_t *)&device_run_state + offset;
				len = len >= data_len ? data_len : len;
				data_len = sizeof(device_run_state.current_2);
				if(len >= data_len){
					// 数据信息直接拷贝
					memcpy(d, p, len);
					return len;
				}else{
					return ERR_REG_READ_LEN;
				}
			}//break;
			case DEV_RELAY1_RW_REG: 							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
			case DEV_RELAY2_RW_REG: 							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
			case DEV_RELAY3_RW_REG: 							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
			case DEV_RELAY4_RW_REG:{							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
				offset = REG_GET_OFFSET(addr);
				// 设备信息可读长度
				data_len = REG_GET_OFFSET(DEV_RELAY4_RW_REG) + sizeof(device_run_state.relay_ctrl[0]) - offset;
				// 读取寄存器地址
				p = (uint8_t *)&(device_run_state.relay_ctrl) + offset;
				len = len >= data_len ? data_len : len;
				data_len = sizeof(device_run_state.relay_ctrl[0]);
				if(len >= data_len){
					// 数据信息直接拷贝
					memcpy(d, p, len);
					return len;
				}else{
					return ERR_REG_READ_LEN;
				}
			}//break;
			default:{
				return len;
			}//break;
		}
	}
	return ret;
}

__weak void dev_relay_on(uint8_t index)
{

}

__weak void dev_relay_off(uint8_t index)
{

}

// 寄存器写入函数
int32_t dev_mem_write(uint32_t addr, uint32_t len, void *data)
{
	uint8_t *d = (uint8_t *)data;
	int32_t ret = 0;
	uint32_t data_len = 0;
	uint16_t offset = 0;
	uint8_t *p;
	if(REG_GET_CTRL(addr) == REG_READ_AND_WRIT || REG_GET_CTRL(addr) == REG_SET || REG_GET_CTRL(addr) == REG_CLR){
		switch(addr){
			case DEV_RTC_TIME_RW_REG:{					//	RTC时间戳
				ret = sizeof(board_param.rtc);
				if(len >= ret){
					board_param.rtc = d[0];
					board_param.rtc |= d[1] << 8;
					board_param.rtc |= d[2] << 16;
					board_param.rtc |= d[3] << 24;
					save_board_param(&board_param, sizeof(board_param));
					return ret;
				}else{
					return ERR_REG_WRITE_LEN;
				}
			}//break;
			// 可写ftp设置信息寄存器
			case DEV_FTP_USER_RW_REG:							// 固件FTP账号
			case DEV_FTP_PWD_RW_REG:							// 固件FTP密码
			case DEV_FTP_PORT_RW_REG:							// 固件FTP端口
			case DEV_FTP_HOST_RW_REG:{						// 固件FTP地址
				// 读取寄存器地址偏移
				offset = REG_GET_OFFSET(addr);
				// 可以写入的最大数据
				data_len = REG_GET_OFFSET(DEV_FTP_HOST_RW_REG) + FTP_HOST_NAME_LEN - offset;
				// 赋值拷贝寄存器地址
				p = (uint8_t *)&board_param.ftp_info + offset;
				// 有效可读取数据长度
				ret = len > data_len ? data_len : len;
				memcpy(p, data, ret);
				save_board_param(&board_param, sizeof(board_param));
				return ret;
			}//break;
			case DEV_RELAY1_RW_REG: 							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
			case DEV_RELAY2_RW_REG: 							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
			case DEV_RELAY3_RW_REG: 							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
			case DEV_RELAY4_RW_REG:{							// 1继电器控制 0x0000 – 断开 0xffff – 闭合不限时间 其它值，继电器闭合指定时间，单位为 秒
				int i = 0;
				offset = REG_GET_OFFSET(addr);
				// 设备信息可读长度
				data_len = REG_GET_OFFSET(DEV_RELAY4_RW_REG) + sizeof(device_run_state.relay_ctrl[0]) - offset;
				// 读取寄存器地址
				p = (uint8_t *)&(device_run_state.relay_ctrl) + offset;
				len = len >= data_len ? data_len : len;
				data_len = sizeof(device_run_state.relay_ctrl[0]);
				if(len >= data_len){
					uint8_t max = (len + offset) / data_len;
					// 数据信息直接拷贝
					memcpy(p, d, len);
					for(i = offset / data_len; i < max; i++){
						if(device_run_state.relay_ctrl[i] == RELAY_OFF){
							dev_relay_off(i);
							device_run_state.relay_state &= ~(0x1 << i);
						}else{
							dev_relay_on(i);
							device_run_state.relay_state |= 0x1 << i;
						}
					}
					return len;
				}else{
					return ERR_REG_WRITE_LEN;
				}
			}//break;
			default:{
			}break;
		}
	}
	return len;
}

__weak void update_software(void)
{
}

// 版本号比较函数
void dev_check_version(uint32_t board_ver, uint32_t soft_ver)
{
	if(p_board_info->hard_ver == board_ver){
		if(soft_ver > p_board_info->soft_ver){
			update_software();
		}
		register_flag = DEV_REGISTER_FLAG;
	}
}

static uint32_t heart_seq = 0;

// 心跳序号检查函数
void dev_check_heart_seq(uint32_t seq)
{
	
}

// 请求数据完成函数
void dev_request_data_done(uint32_t time, uint32_t addr, uint32_t len, void *data)
{

}

// sn号获取函数
void dev_get_sn(void *sn)
{
	memcpy(sn, p_board_info->sn, DEV_SN_LEN);
}

// 获取硬件版本号
uint32_t dev_get_board_version(void)
{
	return p_board_info->hard_ver;
}

// 获取软件版本号
uint32_t dev_get_soft_version(void)
{
	return p_board_info->soft_ver;
}


// 获取心跳地址
uint32_t dev_get_heart_addr(void)
{
	return HEART_ADDR;
}

// 获取心跳包长度
uint32_t dev_get_heart_len(void)
{
	return HEART_LEN;
}

// 获取心跳序号
uint32_t dev_get_heart_seq(void)
{
	return heart_seq;
}

// 获取心数据
uint32_t dev_get_heart_data(void *data)
{
	uint32_t len = dev_get_heart_len();
	memcpy(data, &device_run_state, len);
	return len;
}

static uint32_t process_id = 0;
__weak uint32_t get_system_time(void)
{
	process_id++;
	return process_id / 0xffff;
}

// 获取时间戳
uint32_t dev_get_time(void)
{
	return get_system_time();
}

static uint32_t relay_pre_time = 0;
static uint32_t heart_pre_time = 0;
static uint32_t register_pre_time = 0;
void device_process(void)
{
	uint32_t time = get_system_time();
	uint32_t tmp = 0;
	// 设备未注册成功尝试注册设备，注册成功发送心跳
	if(register_flag != DEV_REGISTER_FLAG){
		uint16_t interval = 5;
		// 处理时间变量溢出的情况
		tmp = time >= register_pre_time ? time - register_pre_time : time + register_pre_time;
		switch(register_flag){
			case 0:{
				register_pre_time = time;
				register_flag++;
				// 未注册成功，第一次什么也不做
				return;
			}//break;
			// 前3次5秒钟注册一次
			case 1:
			case 2:
			case 3:	{
				// 3秒钟注册一次
				interval = 5;
			}break;
			// 后3次30秒钟注册一次
			case 4:
			case 5:
			case 6:	{
				// 3秒钟注册一次
				interval = 30;
			}break;
			// 默认5分钟注册一次
			default:{
				interval = 300;
			}break;
		}
		if(interval <= tmp){
			register_pre_time = time;
			device_register_request();
			if(register_flag <= 6){
				register_flag++;
			}
		}
	}else{
		// 20秒上报一次心跳
		tmp = time >= heart_pre_time ? time - heart_pre_time : time + heart_pre_time;
		if(20 <= tmp){
			heart_pre_time = time;
			device_heart_update();
			heart_seq++;
			device_run_state.level_state = 0x1 << (heart_seq & 0x7);
			((uint16_t *)(&(device_run_state.fan_current)))[heart_seq & 0x3] = heart_seq;
		}
	}
	if(time > relay_pre_time || (relay_pre_time - time) > 1){
		int i = 0;
		relay_pre_time = time;
		for(i = 0; i < MAX_RELAY_NUMBERS; i++){
			if(device_run_state.relay_ctrl[i] != RELAY_OFF && device_run_state.relay_ctrl[i] != RELAY_ON){
				device_run_state.relay_ctrl[i]--;
				if(device_run_state.relay_ctrl[i] == RELAY_OFF){
					dev_relay_off(i);
					device_run_state.relay_state &= ~(0x1 << i);
				}
			}
		}
	}
}


