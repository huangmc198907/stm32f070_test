#include "iot_protocol.h"
#include "string.h"
#include "dev_memery.h"

#define STX					(0x02)		//报文开始
#define ETX					(0x03)		//报文结束
#define DLE					(0x10)		//转义字符
#define NAK					(0x15)		//检测连接状态时使用
#define ACK					(0x06)		//ACK

#define	DEV_TYPE							(0x0001)			//设备类型 污水控制器

#define CMD_GET(cmd)					((cmd) & 0x7f)	//服务器到设备为 0x80 | cmd
#define CMD_DIRECTION(cmd)		((cmd) & 0x80)	//获取命令发起方向
#define CMD_FROME_SERVER			(0x00)					//命令为服务器发送
#define CMD_BACK_FROME_SERVER	(0x80)					//命令为服务器回复命令
#define CMD_BACK(cmd)					((cmd) | 0x80)	//生成回复命令

#define CMD_REGISTER			(0x01)				//设备注册命令
#define CMD_HEART					(0x02)				//设备心跳
#define CMD_RESTART				(0x13)				//设备重启
#define CMD_READ					(0x14)				//读取数据命令
#define CMD_WRITE					(0x15)				//写入数据命令
#define CMD_REQUEST				(0x06)				//请求数据命令

#define NET_2G						(0x01)				//设备使用2G网络
#define NET_WIFI					(0x02)				//设备使用WIFI
#define NET_4G						(0x04)				//设备使用4G网络
#define NET_5G						(0x08)				//设备使用5G网络
#define NET_LINE					(0x10)				//设备使用有线网络

static uint8_t cmd_buf[128];
static uint8_t rx_buf[128];

__weak void protocol_send(void *buf, uint16_t len)
{
}

static int8_t send_cmd_packet(uint8_t *cmd, uint16_t len)
{
	uint8_t *p = cmd_buf;
	uint16_t i = 0;
	uint16_t send_len = len;
	uint8_t bcc = 0;
	
	if(NULL == cmd || len == 0)
		return 0;
	
	*p++ = STX;
	send_len++;
	for(i = 0; i < len; i++){
		// 控制字符需要进行转义
		if(cmd[i] == STX || cmd[i] == ETX || cmd[i] == DLE ||
			cmd[i] == NAK || cmd[i] == ACK){
				*p++ = DLE;
				send_len++;
			}
			*p++ = cmd[i];
			// 只对数据部分进行异或运算
			bcc ^= cmd[i];
	}
	*p++ = ETX;
	send_len++;
	*p++ = bcc;
	send_len++;
	
	// 发送命令数据包
	protocol_send(cmd_buf, send_len);
	
	return send_len;
}

static uint8_t rx_cmd_len = 0;
static uint8_t rx_index = 0;
static uint8_t rx_bcc = 0;
static uint8_t cmd_flag = 0;
static uint8_t rx_step = 0;
static int8_t copy_protocol_data(void *data, uint16_t len)
{
	uint8_t *p = (uint8_t *)data;
	uint8_t tmp = 0;
	uint8_t i = 0;
	while(i < len){
		i++;
		switch(rx_step){
			case 0:{
				// 定位命令起始字符
				if(*p++ == STX){
					rx_step++;
					rx_bcc = 0;
				}
			}break;
			case 1:{
				tmp = *p++;
				// 判断转义字符
				if(tmp == DLE){
					rx_step++;
				// 定位结束字符	
				}else if(tmp == ETX){
					rx_step += 2;
				}else{
					// 保留有效数据
					rx_buf[rx_index++] = tmp;
					// 数据异或
					rx_bcc ^= tmp; 
				}
			}break;
			case 2:{
				tmp = *p++;
				// 被转义的有效数据
				if(tmp == STX || tmp == ETX || tmp == DLE ||
						tmp == NAK || tmp == ACK){
					rx_buf[rx_index++] = tmp;
					// 数据异或
					rx_bcc ^= tmp;
					// 返回到第一步
					rx_step--;
				// 数据错误
				}else{
					rx_step = 0;
					rx_index = 0;
					rx_bcc = 0;
				}
			}break;
			case 3:{
				tmp = *p++;
				rx_cmd_len = rx_index;
				rx_step = 0;
				rx_index = 0;
				// 判断数据校验
				if(tmp == rx_bcc){
					cmd_flag = 1;
					return i;
				}
			}break;
			default:{
				rx_step = 0;
				rx_index = 0;
				rx_bcc = 0;
			}break;
		}
	}
	
	return i;
}

static void build_cmd_and_send(uint8_t cmd, uint8_t *data, uint16_t len)
{
	uint8_t *p = rx_buf;
	// 填充设备类型
	*p++ = DEV_TYPE & 0xff;
	*p++ = (DEV_TYPE >> 8) & 0xff;
	// 填充命令
	*p++ = cmd;
	// 填充数据长度
	*p++ = len & 0xff;
	*p++ = (len >> 8) & 0xff;
	// 填充数据
	memcpy(p, data, len);
	// 发送数据
	send_cmd_packet(rx_buf, (p - rx_buf) + len);
}

static void reply_server_cmd(uint8_t cmd, uint8_t *data, uint16_t len)
{
	uint32_t time = 0;
	uint8_t sn_len = dev_get_sn_len();
	uint8_t id = 0;
	// 判断数据长度是否合法
	if(len < (sn_len + 4)){
		return;
	}
	// 判断sn是否一致
	if(0 != dev_sn_cmp(data)){
		return;
	}
	time = data[sn_len++];
	time |= data[sn_len++] << 8;
	time |= data[sn_len++] << 16;
	time |= data[sn_len++] << 24;
	id = sn_len;
	switch(cmd){
		case CMD_RESTART:{
			build_cmd_and_send(CMD_BACK(cmd), data, len);
			dev_restart(time);
		}break;
		case CMD_READ:{
			int32_t ret = 0;
			// 读取32位地址
			uint32_t addr = data[id++];
			uint32_t data_len = 0;
			addr |= data[id++] << 8;
			addr |= data[id++] << 16;
			addr |= data[id++] << 24;
			// 读取需要读取的数据长度
			data_len = data[id++];
			data_len |= data[id++] << 8;
			data_len |= data[id++] << 16;
			data_len |= data[id++] << 24;
			// 返回数据不填充长度信息。
			id -= 4;
			// 读取数据
			ret = dev_mem_read(addr, data_len, data + id);
			// 返回设备sn与时间戳相同
			if(ret > 0){
				// 返回读取数据长度
				id += ret;
			}else{
				id = sn_len;
				// 填充写入失败错误码
				data[id++] = ret & 0xff;
				data[id++] = (ret >> 8) & 0xff;
				data[id++] = (ret >> 16) & 0xff;
				data[id++] = (ret >> 24) & 0xff;
			}
			// 回复数据
			build_cmd_and_send(CMD_BACK(cmd), data, id);
		}break;
		case CMD_WRITE:{
			uint16_t ret = 0;
			// 读取32位地址
			uint32_t addr = data[id++];
			uint32_t data_len = 0;
			addr |= data[id++] << 8;
			addr |= data[id++] << 16;
			addr |= data[id++] << 24;
			// 读取写入数据长度
			data_len = data[id++];
			data_len |= data[id++] << 8;
			data_len |= data[id++] << 16;
			data_len |= data[id++] << 24;
			// 写入数据
			ret = dev_mem_write(addr, data_len, data + id);
			// 返回设备sn与时间戳相同
			id = sn_len;
			if(ret > 0){
				// 返回地址相同
				id += 4;
				// 填充成功写入的数据长度
				data[id++] = ret & 0xff;
				data[id++] = (ret >> 8) & 0xff;
				data[id++] = (ret >> 16) & 0xff;
				data[id++] = (ret >> 24) & 0xff;
			}else{
				// 填充写入失败错误码
				data[id++] = ret & 0xff;
				data[id++] = (ret >> 8) & 0xff;
				data[id++] = (ret >> 16) & 0xff;
				data[id++] = (ret >> 24) & 0xff;
			}
			// 回复数据
			build_cmd_and_send(CMD_BACK(cmd), data, id);
		}break;
		default:{
		}break;
	}
}

static void parse_server_reply(uint8_t cmd, uint8_t *data, uint16_t len)
{
	uint8_t id = 0;
	switch(cmd){
		case CMD_REGISTER:{
			uint8_t net_type = data[id++];
			// 判断设备id号
			if(0 == dev_sn_cmp(data + id)){
				uint32_t board_ver = 0;
				uint32_t soft_ver = 0;
				id += dev_get_sn_len();
				// 读取硬件版本号
				board_ver = data[id++];
				board_ver |= data[id++] << 8;
				board_ver |= data[id++] << 16;
				board_ver |= data[id++] << 24;
				// 读取软件版本号
				soft_ver = data[id++];
				soft_ver |= data[id++] << 8;
				soft_ver |= data[id++] << 16;
				soft_ver |= data[id++] << 24;
				dev_check_version(board_ver, soft_ver);
			}
		}break;
		case CMD_HEART:{
			// 判断设备id号
			if(0 == dev_sn_cmp(data)){
				uint32_t seq = 0;
				id += dev_get_sn_len();
				// 心跳包序列号
				seq = data[id++];
				seq |= data[id++] << 8;
				seq |= data[id++] << 16;
				seq |= data[id++] << 24;
				dev_check_heart_seq(seq);
			}
		}break;
		case CMD_REQUEST:{
			// 判断设备id号
			if(0 == dev_sn_cmp(data)){
				uint32_t time = 0;
				uint32_t addr = 0;
				uint32_t len = 0;
				id += dev_get_sn_len();
				// 获取时间戳
				time = data[id++];
				time |= data[id++] << 8;
				time |= data[id++] << 16;
				time |= data[id++] << 24;
				// 获取数据地址
				addr = data[id++];
				addr |= data[id++] << 8;
				addr |= data[id++] << 16;
				addr |= data[id++] << 24;
				// 获取数据长度
				len = data[id++];
				len |= data[id++] << 8;
				len |= data[id++] << 16;
				len |= data[id++] << 24;
				dev_request_data_done(time, addr, len, data+id);
			}
		}break;
		default:{
		}break;
	}
}

static void parse_cmd(uint8_t *data, uint16_t len)
{
	uint8_t *p = data;
	if(data && len > 0){
		// 读取设备类型
		uint16_t dev_type = *p++;
		dev_type |= *p++ << 8;
		if(dev_type == DEV_TYPE){
			// 读取命令
			uint8_t cmd = *p++;
			// 读取命令长度
			uint16_t cmd_len = *p++;
			cmd_len |= *p++ << 8;
			if(len == (p - data + cmd_len)){
				// 判断命令方向
				uint8_t direction = CMD_DIRECTION(cmd);
				if(direction == CMD_FROME_SERVER){
					// 命令由服务器发起，需要回复
					reply_server_cmd(CMD_GET(cmd), p, cmd_len);
				}else if(direction == CMD_BACK_FROME_SERVER){
					// 命令由设备发起，判断命令执行情况
					parse_server_reply(CMD_GET(cmd), p, cmd_len);
				}
			}
		}
	}
}

void parse_protocol(void *data, uint16_t len)
{
	int16_t ret;
	uint8_t *p = (uint8_t *)data;
PARSE_PROTOCOL_AGAIN:	
	if(NULL != data && len > 0){
		// 拷贝出一条完整的命令
		ret = copy_protocol_data(p, len);
		// 得到一条完整命令并处理
		if(ret > 0 && ret <= len && cmd_flag == 1){
			cmd_flag = 0;
			// 处理命令
			parse_cmd(rx_buf, rx_cmd_len);
			// 此条数据包中包含多个命令，再进行一次拷贝命令
			if(ret < len){
				p += ret;
				len -= ret;
 				goto PARSE_PROTOCOL_AGAIN;
			}
		}
	}
}

int device_register_request(void)
{
	uint8_t *p = rx_buf + 20;
	uint8_t id = 0;
	uint32_t board_ver = dev_get_board_version();
	uint32_t soft_ver = dev_get_soft_version();
	uint32_t heart_addr = dev_get_heart_addr();
	uint32_t heart_len = dev_get_heart_len();
	// 填充网络类型
	p[id++] = NET_2G;
	// 填充设备sn
	dev_get_sn(p+id);
	id += dev_get_sn_len();
	// 填充硬件版本号
	p[id++] = board_ver & 0xff;
	p[id++] = (board_ver >> 8) & 0xff;
	p[id++] = (board_ver >> 16) & 0xff;
	p[id++] = (board_ver >> 24) & 0xff;
	// 填充软件版本号
	p[id++] = soft_ver & 0xff;
	p[id++] = (soft_ver >> 8) & 0xff;
	p[id++] = (soft_ver >> 16) & 0xff;
	p[id++] = (soft_ver >> 24) & 0xff;
	// 填充心跳数据地址
	p[id++] = heart_addr & 0xff;
	p[id++] = (heart_addr >> 8) & 0xff;
	p[id++] = (heart_addr >> 16) & 0xff;
	p[id++] = (heart_addr >> 24) & 0xff;
	// 填充心跳数据长度
	p[id++] = heart_len & 0xff;
	p[id++] = (heart_len >> 8) & 0xff;
	p[id++] = (heart_len >> 16) & 0xff;
	p[id++] = (heart_len >> 24) & 0xff;
	
	build_cmd_and_send(CMD_REGISTER, p, id);
	return 1;
}

int device_heart_update(void)
{
	uint8_t *p = rx_buf + 20;
	uint8_t id = 0;
	uint32_t heart_seq = dev_get_heart_seq();
	uint32_t heart_addr = dev_get_heart_addr();
	// 填充设备sn
	dev_get_sn(p+id);
	id += dev_get_sn_len();
	// 填充心跳序列号
	p[id++] = heart_seq & 0xff;
	p[id++] = (heart_seq >> 8) & 0xff;
	p[id++] = (heart_seq >> 16) & 0xff;
	p[id++] = (heart_seq >> 24) & 0xff;
	// 填充心跳数据地址
	p[id++] = heart_addr & 0xff;
	p[id++] = (heart_addr >> 8) & 0xff;
	p[id++] = (heart_addr >> 16) & 0xff;
	p[id++] = (heart_addr >> 24) & 0xff;
	// 填充心跳数据
	id += dev_get_heart_data(p+id);
	
	build_cmd_and_send(CMD_HEART, p, id);
	return 1;
}

int device_data_request(uint32_t addr, uint32_t len)
{
	uint8_t *p = rx_buf + 20;
	uint8_t id = 0;
	uint32_t time = dev_get_time();
	// 填充设备sn
	dev_get_sn(p+id);
	id += dev_get_sn_len();
	// 填充时间戳
	p[id++] = time & 0xff;
	p[id++] = (time >> 8) & 0xff;
	p[id++] = (time >> 16) & 0xff;
	p[id++] = (time >> 24) & 0xff;
	// 填充请求数据地址
	p[id++] = addr & 0xff;
	p[id++] = (addr >> 8) & 0xff;
	p[id++] = (addr >> 16) & 0xff;
	p[id++] = (addr >> 24) & 0xff;
	// 填充请求数据长度
	p[id++] = len & 0xff;
	p[id++] = (len >> 8) & 0xff;
	p[id++] = (len >> 16) & 0xff;
	p[id++] = (len >> 24) & 0xff;
	
	build_cmd_and_send(CMD_REQUEST, p, id);
	return 1;
}
