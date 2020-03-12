#include "dev_memery.h"
#include "string.h"
#include "iot_protocol.h"

#define REG_CTRL_MASK							(0xff000000)
#define REG_GET_CTRL(addr)				((addr) & REG_CTRL_MASK)
#define REG_GET_OFFSET(addr)			((addr) & 0xfff)
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
#define DEV_FTP_USER_RW_REG				(0x01001000) // 固件FTP账号
#define DEV_FTP_PWD_RW_REG				(0x01001010) // 固件FTP密码
#define DEV_FTP_PORT_RW_REG				(0x01001020) // 固件FTP端口
#define DEV_FTP_HOST_RW_REG				(0x01001022) // 固件FTP地址

// 设备运行信息只读寄存器
#define DEV_MOTOR_STATE_RO_REG			(0x00010000) // 0-15号电机运行状态寄存器，0-停止，1-运行
#define DEV_MOTOR_ERR_RO_REG				(0x00010002) // 0-15号电机故障状态，			0-无故障，1-有故障
#define DEV_FLOATER_HIGH_RO_REG			(0x00010004) // 0-7号浮球高液位状态，			0-正常，1-液位高
#define DEV_FLOATER_LOW_RO_REG			(0x00010005) // 0-7号浮球低液位状态，			0-正常，1-液位低
#define DEV_MOTOR_0_CURRENT_RO_REG	(0x00010006) // 0号电机电流								单位mA
#define DEV_MOTOR_0_ERR_NO_RO_REG		(0x00010008) // 0号电机故障码。
#define DEV_MOTOR_15_CURRENT_RO_REG	(0x00010042) // 15号电机电流								单位mA
#define DEV_MOTOR_15_ERR_NO_RO_REG	(0x00010044) // 15号电机故障码。
#define DEV_MOTOR_0_RUN_TIME_RO_REG	(0x00010046) // 0号电机最长持续工作时间，单位秒，0表示不限制
#define DEV_MOTOR_15_RUN_TIME_RO_REG (0x00010082) // 15号电机最长持续工作时间，单位秒，0表示不限制
#define DEV_VOICE_LAST_PLAY_RO_REG 	(0x00010086) // 设备声音播放剩余次数

// 设备运行信息可写
#define DEV_MOTOR_0_RUN_TIME_RW_REG	(0x01010046) // 0号电机最长持续工作时间，单位秒，0表示不限制
#define DEV_MOTOR_15_RUN_TIME_RW_REG (0x01010082) // 15号电机最长持续工作时间，单位秒，0表示不限制
#define DEV_VOICE_LAST_PLAY_RW_REG 	(0x01010086) // 设备声音播放剩余次数

// 设备运行信息（写1置1）
#define DEV_MOTOR_ON_SET_WO_REG					(0x02010000) // 0-15号电机开启控制，1-开启

// 设备运行信息（写1清0）
#define DEV_MOTOR_OFF_SET_WO_REG				(0x03010000) // 0-15号电机开启控制，1-停止
#define DEV_MOTOR_ERR_CLR_WO_REG				(0x03010002) // 0-15号电机故障清除，1-清除

// 心跳地址固定为 0x00010002
#define HEART_ADDR							(0x00010002)

// 设备支持的最大控制电机数量
#define MAX_MOTOR_NUMBER					(15)

// 当前设备控制的电机数量
#define NOW_MAX_MOTOR_NUMBER			(5)

#define DEV_SN_LEN								(8)
#define FTP_USER_NAME_LEN					(16)
#define FTP_PWD_LEN								(16)
#define FTP_HOST_NAME_LEN					(62)

#define DEV_NET_2G						(0x01)				//设备使用2G网络

#define PARAM_INIT_FLAG				(0xebea7fcc)
#define VOICE_PLAY_TIMES			(15)

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

// 需要保存到flash的运行信息
typedef struct {
	uint32_t motor_run_time[NOW_MAX_MOTOR_NUMBER];	//电机运行最大时间
	uint16_t voice_last_play_times;									//设备声音播放剩余次数
}device_param_st;

// 保存到flash的信息，上电时拷贝到内存，变化是保存到flash
typedef struct {
	uint32_t flag;
	uint32_t rtc;										//RTC时间戳
	ftp_info_st ftp_info;						//ftp配置信息
	device_param_st dev_param;			//设备参数信息,0-当前最大电机运行时间限制值和音量播放剩余次数
} board_param_st;

// 电机运行信息
typedef struct {
	uint16_t motor_current;					//电机工作电流
	uint16_t motor_err_no;					//电机故障码
}motor_info_st;

// 内存维护的运行状态
typedef struct{
	uint16_t motor_run_state;				//0-15号电机运行状态，1-运行，0-停止
	uint16_t motor_err_state;				//0-15号电机故障状态，1-故障，2-无
	uint8_t floater_high_state;			//0-7号浮球高液位状态，1-高于等于，0-低于
	uint8_t floater_low_state;			//0-7号浮球低液位状态，1-低于等于，0-高于
	motor_info_st motor_info[NOW_MAX_MOTOR_NUMBER];		//0-当前最大号电机电流和错误码。
	device_param_st device_param;
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
	.dev_param = {
		.motor_run_time = {0},
		.voice_last_play_times = VOICE_PLAY_TIMES,
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
				// 初始化电机运行时间时间设置
				memset(board_param.dev_param.motor_run_time, 0, sizeof(board_param.dev_param.motor_run_time));
				// 初始化声音播放次数
				board_param.dev_param.voice_last_play_times = VOICE_PLAY_TIMES;
				// 保存参数
			}
		}
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

int32_t copy_voice_times(void *data, uint32_t len)
{
	uint32_t data_len = sizeof(board_param.dev_param.voice_last_play_times);
	uint8_t *p = NULL;
	uint8_t *d = (uint8_t *)data;
	p = (uint8_t *)&board_param.dev_param.voice_last_play_times;
	// 读取声音播放次数必须大于寄存器值长度
	if(len >= data_len){
		memcpy(d, p, data_len);
		return data_len;
	}else{
		return ERR_REG_READ_LEN;
	}
}

int32_t copy_motor_run_time(uint32_t offset, void *data, uint32_t len)
{
	int32_t ret = 0;
	uint32_t data_len = 0;
	uint8_t *p = NULL;
	uint8_t *d = (uint8_t *)data;
	uint32_t tmp_len = 0;
	
	if(offset & sizeof(board_param.dev_param.motor_run_time[0]))
		return ERR_REG_ADDR;
	
	// 当前设备支持的最大电机运行时间设置
	data_len = sizeof(board_param.dev_param.motor_run_time);
	p = (uint8_t *)&(board_param.dev_param.motor_run_time) + offset;
	if(len <= data_len){
		memcpy(d, p, len);
		return len;
	}else{
		// 拷贝当前设备支持的最大电机运行时间设置
		memcpy(d, p, data_len);
		d += data_len;
		ret += data_len;
		len -= data_len;
		// 此设备不支持的电机运行时间设置但在此类型设备支持的电机运行时间设置内
		data_len = sizeof(board_param.dev_param.motor_run_time[0]) * MAX_MOTOR_NUMBER;
		data_len -= sizeof(board_param.dev_param.motor_run_time);
		if(len <= data_len){
			// 超出此设备设备最大电机运行时间设置，将超出的置零
			memset(d, 0, len);
			return ret + len;
		}else{
			// 将超出此设备设备最大电机数量与此类型设备最大电机之间的运行时间设置置零
			memset(d, 0, data_len);
			ret += data_len;
			tmp_len = ret;
			// 拷贝电机运行时间设置寄存器值
			ret = copy_voice_times(d + data_len, len - data_len);
			if(ret > 0){
				return ret + tmp_len;
			}else{
				return ret;
			}
		}
	}
}

int32_t copy_motor_current_and_err(uint32_t offset, void *data, uint32_t len)
{
	int32_t ret = 0;
	uint32_t data_len = 0;
	uint8_t *p = NULL;
	uint8_t *d = (uint8_t *)data;
	uint32_t tmp_len = 0;
	
	if(offset & sizeof(motor_info_st))
		return ERR_REG_ADDR;
	
	// 当前设备支持的最大电机电流和错误信息
	data_len = REG_GET_OFFSET(DEV_MOTOR_0_CURRENT_RO_REG);
	data_len += (NOW_MAX_MOTOR_NUMBER * sizeof(motor_info_st));
	data_len -= offset;
	p = (uint8_t *)&(device_run_state.motor_info) + offset;
	if(len <= data_len){
		memcpy(d, p, len);
		return len;
	}else{
		// 拷贝当前设备支持的最大电机电流和错误信息
		memcpy(d, p, data_len);
		d += data_len;
		ret += data_len;
		len -= data_len;
		// 此设备不支持的电机电流和错误但在此类型设备支持的电机控制范围内
		data_len = REG_GET_OFFSET(DEV_MOTOR_0_RUN_TIME_RO_REG);
		data_len -= REG_GET_OFFSET(DEV_MOTOR_0_CURRENT_RO_REG);
		data_len -= (NOW_MAX_MOTOR_NUMBER * sizeof(motor_info_st));
		if(len <= data_len){
			// 超出此设备设备最大电机数量，将超出的状态置零
			memset(d, 0, len);
			return ret + len;
		}else{
			// 将超出此设备设备最大电机数量与此类型设备最大电机之间的电流和错误置零
			memset(d, 0, data_len);
			ret += data_len;
			tmp_len = ret;
			// 拷贝电机运行时间设置寄存器值
			ret = copy_motor_run_time(0, d + data_len, len - data_len);
			if(ret > 0){
				return ret + tmp_len;
			}else{
				return ret;
			}
		}
	}
}

// 寄存器独缺函数
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
			case DEV_MOTOR_STATE_RO_REG:                                             // 0-15号电机运行状态寄存器，0-停止，1-运行
			case DEV_MOTOR_ERR_RO_REG:                                               // 0-15号电机故障状态，			0-无故障，1-有故障
			case DEV_FLOATER_HIGH_RO_REG:                                            // 0-7号浮球高液位状态，			0-正常，1-液位高
			case DEV_FLOATER_LOW_RO_REG:{                                             // 0-7号浮球低液位状态，			0-正常，1-液位低
				// 寄存器地址偏移
				offset = REG_GET_OFFSET(addr);
				// 设备信息可读长度
				data_len = REG_GET_OFFSET(DEV_MOTOR_0_CURRENT_RO_REG) - offset;
				// 读取寄存器地址
				p = (uint8_t *)&device_run_state + offset;
				if(len <= data_len){
					// 数据小于设备信息数据直接拷贝
					memcpy(d, p, len);
					return len;
				}else{
					// 拷贝设备信息数据
					memcpy(d, p, data_len);
					// 拷贝电机电流和错误码
					ret = copy_motor_current_and_err(0, d+data_len, len - data_len);
					if(ret > 0){
						return ret + data_len;
					}else{
						return ret;
					}
				}
			}//break;
			// 只读寄存器
			case DEV_VOICE_LAST_PLAY_RO_REG:																		// 设备声音播放剩余次数
			// 可写寄存器
			case DEV_VOICE_LAST_PLAY_RW_REG:{																		// 设备声音播放剩余次数
				return copy_voice_times(data, len);
			}//break;
			default:{
				// 0-15号电机电流(单位mA)和错误码
				if(addr >= DEV_MOTOR_0_CURRENT_RO_REG && addr <= DEV_MOTOR_15_ERR_NO_RO_REG){
					return copy_motor_current_and_err(REG_GET_OFFSET(addr) - REG_GET_OFFSET(DEV_MOTOR_0_CURRENT_RO_REG), data, len);
					// 只读和可写电机运行时间设置寄存器
				}else if((addr >= DEV_MOTOR_0_RUN_TIME_RO_REG && addr <= DEV_MOTOR_15_RUN_TIME_RO_REG) ||
										(addr >= DEV_MOTOR_0_RUN_TIME_RW_REG && addr <= DEV_MOTOR_15_RUN_TIME_RW_REG)){
					return copy_motor_run_time(REG_GET_OFFSET(addr)- REG_GET_OFFSET(DEV_MOTOR_0_RUN_TIME_RO_REG), data, len);					
				}
			}break;
		}
	}
	return ret;
}

__weak void dev_motor_on(uint8_t index)
{

}

__weak void dev_motor_off(uint8_t index)
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
			case DEV_VOICE_LAST_PLAY_RW_REG:{																		// 设备声音播放剩余次数
				uint16_t *tmp = &(board_param.dev_param.voice_last_play_times);
				ret = sizeof(board_param.dev_param.voice_last_play_times);
				if(len >= ret){
					*tmp = d[0];
					*tmp |= d[1] << 8;
					*tmp |= d[2] << 16;
					*tmp |= d[3] << 24;
					save_board_param(&board_param, sizeof(board_param));
					return ret;
				}else{
					return ERR_REG_WRITE_LEN;
				}
			}//break;
			// 设备运行信息（写1置1）
			case DEV_MOTOR_ON_SET_WO_REG:{						// 0-15号电机开启控制，1-开启
				data_len = sizeof(device_run_state.motor_run_state);
				if(len >= data_len){
					uint8_t i = 0;
					uint16_t ctrl = d[0];
					ctrl |= d[1] << 8;
					ctrl |= d[2] << 16;
					ctrl |= d[3] << 24;
					for(i = 0; i < NOW_MAX_MOTOR_NUMBER; i++){
						if(((device_run_state.motor_run_state & (0x1 << i)) == 0) && ((ctrl & (0x1 << i)) != 0)){
							dev_motor_on(i);
							if(board_param.dev_param.motor_run_time[i] != 0){
								device_run_state.device_param.motor_run_time[i] = get_system_time() + board_param.dev_param.motor_run_time[i];
							}
							device_run_state.motor_run_state |= 0x1 << i;
						}
					}
					return data_len;
				}else{
					return ERR_REG_WRITE_LEN;
				}
			}//break;
			// 设备运行信息（写1清0）
			case DEV_MOTOR_OFF_SET_WO_REG:{							// 0-15号电机开启控制，1-停止
				data_len = sizeof(device_run_state.motor_run_state);
				if(len >= data_len){
					uint8_t i = 0;
					uint16_t ctrl = d[0];
					ctrl |= d[1] << 8;
					ctrl |= d[2] << 16;
					ctrl |= d[3] << 24;
					for(i = 0; i < NOW_MAX_MOTOR_NUMBER; i++){
						if(((device_run_state.motor_run_state & (0x1 << i)) != 0) && ((ctrl & (0x1 << i)) != 0)){
							dev_motor_off(i);
							device_run_state.motor_run_state &= ~(0x1 << i);
						}
					}
					return data_len;
				}else{
					return ERR_REG_WRITE_LEN;
				}
			}//break;                                                                                                                     
			case DEV_MOTOR_ERR_CLR_WO_REG:{							// 0-15号电机故障清除，1-清除
				data_len = sizeof(device_run_state.motor_err_state);
				if(len >= data_len){
					uint8_t i = 0;
					uint16_t ctrl = d[0];
					ctrl |= d[1] << 8;
					ctrl |= d[2] << 16;
					ctrl |= d[3] << 24;
					for(i = 0; i < NOW_MAX_MOTOR_NUMBER; i++){
						if(((device_run_state.motor_err_state & (0x1 << i)) != 0) && ((ctrl & (0x1 << i)) != 0)){
							dev_motor_off(i);
							device_run_state.motor_err_state &= ~(0x1 << i);
						}
					}
					return data_len;
				}else{
					return ERR_REG_WRITE_LEN;
				}
			}//break;
			default:{
				if(addr >= DEV_MOTOR_0_RUN_TIME_RW_REG && addr <= DEV_MOTOR_15_RUN_TIME_RW_REG){
					// 读取寄存器地址偏移
					offset = REG_GET_OFFSET(addr);
					if(offset & sizeof(board_param.dev_param.motor_run_time[0])){
						return ERR_REG_ADDR;
					}
					// 可以写入的最大数据
					data_len = sizeof(board_param.dev_param.motor_run_time) - offset;
					// 赋值拷贝寄存器地址
					p = (uint8_t *)&board_param.dev_param.motor_run_time + offset;
					// 有效可读取数据长度
					ret = len > data_len ? data_len : len;
					memcpy(p, data, ret);
					save_board_param(&board_param, sizeof(board_param));
					return ret;				
				}
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
	return sizeof(device_run_state) - sizeof(device_run_state.device_param);
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

__weak uint32_t get_system_time(void)
{
	return 0;
}

// 获取时间戳
uint32_t dev_get_time(void)
{
	return get_system_time();
}

static uint32_t process_id = 0;
void device_process(void)
{
	process_id++;
	if((process_id & 0xfffff) == 0){
		device_heart_update();
		heart_seq++;
	}
}


