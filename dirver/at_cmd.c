#include "at_cmd.h"
#include "string.h"

#define AT_HEAD				"AT"
#define AT_END				"\r\n"

#define AT_CPIN_CMD						"CPIN"								// 识别sim卡
#define AT_CHECK_CPIN_CMD			"CPIN?"								// 查看sim卡状态

#define AT_CREG_CMD						"CREG" 								// CS域网络注册
#define AT_CHECK_CREG_CMD			"CREG?" 							// 查看CS域网络注册

#define AT_CGREG_CMD					"CGREG" 							// PS网络注册
#define AT_CHECK_CGREG_CMD		"CGREG?" 							// 查看PS网络注册

#define AT_CSQ_CMD						"CSQ"									// 查询当前信号质量

#define AT_QIREGAPP_CMD				"QIREGAPP"							
#define AT_SET_QIREGAPP_CMD		"QIREGAPP=\"CMNET\""		// 设置APN网络（只能使用单路连接）

#define AT_CGDCONT_CMD				"CGDCONT"							
#define AT_SET_CGDCONT_CMD		"CGDCONT=1,\"IP\",\"CMNET\""		// 设置APN网络（可开启多路连接）

#define AT_CGACT_CMD					"CGACT"
#define AT_SET_CGACT_CMD			"CGACT=1,1"						// 激活PDP

#define AT_IP_OPEN_CMD				"QIOPEN"							// 建立IP连接
#define AT_IP_CLOSE_CMD				"QICLOSE"							// 断开IP连接
#define AT_IP_SEND_CMD				"QISEND"							// IP数据发送命令
#define AT_SET_IP_HEAD_CMD		"QIHEAD"							// 设置返回数据带IP数据包
#define AT_IP_MUX_CMD					"QIMUX"								// 设置IP多路连接命令
#define AT_SET_IP_MUX_CMD			"QIMUX=1"							// 设置IP多路连接

#define AT_SET_FTP_USER_CMD		"QFTPUSER"						// 设置ftp连接用户名
#define AT_SET_FTP_PASSWORD_CMD	"QFTPPASS"					// 设置ftp链接密码。
#define AT_FTP_OPEN_CMD				"QFTPOPEN"						// 连接ftp服务器。
#define AT_FTP_CLOSE_CMD			"QFTPCLOSE"						// 断开ftp连接
#define AT_FTP_GET_FILE_CMD		"QFTPGET"							// ftp下载文件
#define AT_FTP_GET_FILE_SIZE_CMD "QFTPSIZE"					// 获取ftp文件长度
#define AT_FTP_CFG_CMD				"QFTPCFG"							// ftp参数配置命令
#define AT_FTP_SET_POSITION_CMD	"QFTPCFG=3,"				// ftp文件下载偏移设置

#define AT_DNS_GET_IP					"QIDNSGIP"						// DNS获取ip地址

#define AT_RECV_BUF_LEN				(128)
static char rbuf[AT_RECV_BUF_LEN];
static char rdbuf[AT_RECV_BUF_LEN];
static char *dbuf = rbuf;
static uint8_t dlen=0;;
static uint8_t dindex=0;

#define CMD_VALUE(l)		(1+(l)+2)

#define CMD_START_END		(0)
#define CMD_START				(1)
#define CMD_END					(2)
#define CMD_DATA				(3)

// +RECEIVE: 1, 128\r\n
#define IP_RECV_MINI_HEAD_LEN		(18)
// +RECEIVE: 1, 31
#define GET_IP_FD()			(dbuf[10]-'0')

const char *ip_connet_string = "CONNECT OK";
const char *ip_tcp_type_string = ",\"TCP\",\"";
const char *ip_close_string = "CLOSE OK";
const char *ip_send_string = "SEND OK";
const char *ip_receive_string = "RECEIVE: ";
const char *ftp_get_file_string = "CONNECT";

__weak void at_uart_puts(void *buf, uint32_t len)
{
}

__weak uint32_t at_uart_gets(void *buf, uint32_t max_len)
{
	return ERR_UNKNOWN;
}

__weak void at_delay_ms(uint16_t count)
{
}

#define MAX_IP_SOCKET_NUM		(2)

typedef struct {
	//char ip[16];
	//char port[6];
	IP_RX_CALLBACK_FUNC callback;
}ip_info_st;

static ip_info_st ip_st[MAX_IP_SOCKET_NUM];

//// 获取字符串长度
//static uint8_t strlen(char* s)
//{
//	uint8_t l = 0;
//	if(s){
//		while(*s){
//			l++;
//			s++;
//			if(l == 255)
//				return 0;
//		}
//	}
//	return l;
//}

//// 比较字符串
//static int8_t memcmp(char *s1, char *s2, uint8_t len)
//{
//	int8_t i = len;
//	if(s1 && s2 && len > 0){
//		while(i > 0){
//			if(!*s1 || !*s2 || *s1 != *s2){
//				return -1;
//			}
//			i--;
//		}
//		return 0;
//	}
//	return -1;
//}

static uint8_t cmd_len = 0;
static void at_send_cmd(char *cmd, uint8_t flag)
{
	int8_t i=0;
	uint8_t len = 0;
	if(flag == CMD_START_END || flag == CMD_START){
		// 清除串口缓存的无用数据。
		do{
			len = at_uart_gets(dbuf, AT_RECV_BUF_LEN);
		}while(len > 0);
		cmd_len = 0;
		// 发送“AT+”字符串
		if(cmd[0] != 'A' || cmd[1] != 'T'){
			len = strlen(AT_HEAD);
		//if(memcmp(cmd,AT_HEAD,strlen(AT_HEAD))){
			at_uart_puts(AT_HEAD, len);
			at_uart_puts("+", 1);
			cmd_len += len + 1;
		}
	}
	// 发送命令字符串
	len = strlen(cmd);
	at_uart_puts(cmd, len);
	cmd_len += len;
	if(flag == CMD_START_END || flag == CMD_END){
		// 发送”\r\n”字符串
		len = strlen(AT_END);
		at_uart_puts(AT_END, len);
		// 读取回显命令字符串
		len = 0;
		for(i = 0; i < 1000; i++){
			len += at_uart_gets(dbuf, cmd_len-len);
			if(len == cmd_len){
				break;
			}
			at_delay_ms(10);
		}
		cmd_len = 0;
	}
}

#define WAIT_UNTIL_END				(0)

// 等待接收AT命令返回数据
static int8_t at_wait_response(uint16_t len)
{
	int8_t i;
	uint8_t ret = 0;
	dlen=0;;
	dindex=0;
	dbuf = rbuf;
	// 在50ms内等待接收数据完成
	len = (len > AT_RECV_BUF_LEN) || (len == WAIT_UNTIL_END) ? AT_RECV_BUF_LEN : len;
	for(i = 0; i < 100; i++){
		ret = at_uart_gets(dbuf+dlen, len-dlen);
		if(ret > 0 && (ret + dlen) <= len){
			dlen += ret;
			while(*dbuf == '\r' && dlen > 0){
				if(*(dbuf + 1) == '\n'){
					dbuf += 2;
					dlen -= 2;
					dindex += 2;
				}else{
					break;
				}
			}
			if(dlen == len)
				return RET_OK;
			if(dlen > 0){
				if(dbuf[dlen - 1] == '\n' && dbuf[dlen - 2] == '\r'){
					return RET_OK;
				}
			}
		}
		at_delay_ms(10);
	}
	return ERR_UNKNOWN;
}

// 发送AT指令判断模块是否存在
static int8_t wait_at_cmd_response(void)
{
	uint8_t i = 0;
	at_send_cmd(AT_HEAD, CMD_START_END);
	// 进行5次命令发送测试
	for(i = 0; i < 5; i++){
		if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
			// 判断返回数据是否为”OK“
			if(dbuf[0]=='O' && dbuf[1]=='K')
				return RET_OK;
		}
		at_delay_ms(10);
		at_send_cmd(AT_HEAD, CMD_START_END);
	}
	
	return ERR_UNKNOWN;
}

// 判读 SIM 卡状态
static int8_t check_cpin_state(void)
{
	uint8_t cmd_len = strlen(AT_CPIN_CMD);
	at_send_cmd(AT_CHECK_CPIN_CMD, CMD_START_END);
	/* 	AT+CPIN? //返回 READY，表示 SIM 卡正常识别运行
	 *	+CPIN: READY
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		if(dlen > (CMD_VALUE(cmd_len) + 5) && 0 == memcmp(dbuf+1, AT_CPIN_CMD, cmd_len)){
			if(0 == memcmp(dbuf+CMD_VALUE(cmd_len), "READY", 5)){
				return RET_OK;
			}
		}
	}
	
	return ERR_UNKNOWN;
}

// 确认 CS 域网络注册成功
static int8_t check_creg_state(void)
{
	uint8_t cmd_len = strlen(AT_CREG_CMD);
	char c = 0;
	at_send_cmd(AT_CHECK_CREG_CMD, CMD_START_END);
	/*	AT+CREG?
	 *	+CREG: 0,1 //第二位 1 或 5 表示注册成功
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		// 0,1 (1 或 5 表示注册成功)
		if(dlen > (CMD_VALUE(cmd_len) + 2) && 0 == memcmp(dbuf+1, AT_CREG_CMD, cmd_len)){
			c = dbuf[CMD_VALUE(cmd_len) + 2];
			if(c == '1' || c == '5'){
				return RET_OK;
			}
		}
	}
	
	return ERR_UNKNOWN;
}

//确认 PS 域网络注册成功
static int8_t check_cgreg_state(void)
{
	uint8_t cmd_len = strlen(AT_CGREG_CMD);
	char c = 0; 
	at_send_cmd(AT_CHECK_CGREG_CMD, CMD_START_END);
	/*	AT+CGREG? //确认 PS 域网络注册成功
	 *	+CGREG: 0,1 //第二位 1 或 5 表示注册成功
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		// 0,1 (1 或 5 表示注册成功)
		if(dlen > (CMD_VALUE(cmd_len) + 2) && 0 == memcmp(dbuf+1, AT_CGREG_CMD, cmd_len)){
			c = dbuf[CMD_VALUE(cmd_len) + 2];
			if(c == '1' || c == '5'){
				return RET_OK;
			}
		}
	}
	
	return ERR_UNKNOWN;
}

// 获取当前信号质量，小于10信号比较弱。
static int8_t check_csq_state(void)
{
	uint8_t cmd_len = strlen(AT_CSQ_CMD);
	char c = 0; 
	at_send_cmd(AT_CSQ_CMD, CMD_START_END);
	/*	AT+CSQ //查询当前信号质量
	 *	+CSQ: 31,99 // 信号质量小于 10，表示当前网络环境信号比较弱
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		if(dlen > (CMD_VALUE(cmd_len) + 1) && 0 == memcmp(dbuf+1, AT_CSQ_CMD, cmd_len)){
			c = dbuf[CMD_VALUE(cmd_len) + 1];
			if(c != ','){
				return RET_OK;
			}
		}
	}
	
	return ERR_UNKNOWN;
}

//设置IP连接为多路连接
static int8_t set_ip_mux(void)
{
	at_send_cmd(AT_SET_IP_MUX_CMD, CMD_START_END);
	/*	AT+QIMUX=1 //设置多路连接模式
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		if(dlen > 2 && dbuf[0] == 'O' && dbuf[1] == 'K'){
			return RET_OK;
		}
	}
	
	return ERR_UNKNOWN;
}

//设置 GPRS 的 APN
static int8_t set_apn(void)
{
	#if 0
	// 只能设置为单路连接
	at_send_cmd(AT_SET_QIREGAPP_CMD, CMD_START_END);
	/*	AT+QIREGAPP="CMNET"
	 *	OK
	*/
	if(0 > at_wait_response()){
		if(dlen > 2 && dbuf[0] == 'O' && dbuf[1] == 'K'){
			return 1;
		}
	}
	#else
	// 设置为多路连接
	at_send_cmd(AT_SET_CGDCONT_CMD, CMD_START_END);
	/*	CGDCONT=1,"IP","CMNET"
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		if(dlen > 2 && dbuf[0] == 'O' && dbuf[1] == 'K'){
			return RET_OK;
		}
	}
	#endif
	return ERR_UNKNOWN;
}

//激活 PDP
static int8_t enable_pdp(void)
{
	at_send_cmd(AT_SET_CGACT_CMD, CMD_START_END);
	/*	AT+CGACT=1,1 //激活 PDP
	 *	(或 AT+QIACT)
	 *	OK
	*/
	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
		if(dlen > 2 && dbuf[0] == 'O' && dbuf[1] == 'K'){
			return RET_OK;
		}
	}
	
	return ERR_UNKNOWN;
}

//设置ip返回数据包携带ip包头
//static int8_t enable_ip_rx_head(void)
//{
//	at_send_cmd(AT_SET_IP_HEAD_CMD, CMD_START_END);
//	/*	AT+QIHEAD=1
//	 *	OK
//	*/
//	if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
//		if(dlen > 2 && dbuf[0] == 'O' && dbuf[1] == 'K'){
//			return RET_OK;
//		}
//	}
//	
//	return ERR_UNKNOWN;
//}

int8_t init_m6315(void)
{
	int8_t ret = 0;
	
	ret = wait_at_cmd_response();
	if(ret <= ERR_UNKNOWN)
		return ERR_NO_M6315;
	
	ret = check_cpin_state();
	if(ret <= ERR_UNKNOWN)
		return ERR_NO_SIM_CARD;
	
	ret = check_creg_state();
	if(ret <= ERR_UNKNOWN)
		return ERR_CS_NET_FAILED;
	
	ret = check_cgreg_state();
	if(ret <= ERR_UNKNOWN)
		return ERR_PS_NET_FAILED;
	
	ret = check_csq_state();
	if(ret <= ERR_UNKNOWN)
		return ERR_SIGNAL_WEAK;

	ret = set_apn();
	if(ret <= ERR_UNKNOWN)
		return ERR_SET_APN_FAILED;
	
	ret = enable_pdp();
	if(ret <= ERR_UNKNOWN)
		return ERR_ENABLE_PDP_FAILED;
	
	ret = set_ip_mux();
	if(ret <= ERR_UNKNOWN)
		return ERR_ENABLE_MUX;
//	ret = enable_ip_rx_head();
//	if(ret <= ERR_UNKNOWN)
//		return ERR_ENABLE_IP_HEAD_FAILED;
	
	return RET_OK;
}

int8_t m6315_dns_get_ip(char *host, char *ip)
{
	uint8_t len = 0;
	
	if(host && ip){
		at_send_cmd(AT_DNS_GET_IP, CMD_START);
		at_send_cmd("=\"", CMD_DATA);
		at_send_cmd(host,CMD_DATA);
		at_send_cmd("\"", CMD_END);
		/*	OK
		 *	103.46.128.49
		*/
		len = strlen("OK\r\n");
		if(RET_OK != at_wait_response(len)){
			return ERR_DNS_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_DNS_CMD;
		}
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_DNS_CMD;
		}
		if(dlen <= 17){
			memcpy(ip, dbuf, dlen - 2);
			return RET_OK;
		}else{
			return ERR_DNS_CMD;
		}
	}
	return ERR_PARAMETER;
}

int8_t m6315_socket_open(char *ip, char *port, IP_RX_CALLBACK_FUNC rx_callback)
{
	int8_t i = 0;
	uint16_t len = 0;
	char nowip[16] = {0};
	if(NULL == ip || NULL == port || NULL == rx_callback)
		return ERR_PARAMETER;
	
	if(RET_OK != m6315_dns_get_ip(ip, nowip)){
		return ERR_IP_OR_HOST;
	}
	
	for(i = 0; i < MAX_IP_SOCKET_NUM; i++){
		if(NULL == ip_st[i].callback){
			char fd[2]={0};
			fd[0] = i+'1';
			memset(&ip_st[i], 0, sizeof(ip_st[i]));
			//memcpy(ip_st[i].ip, ip, strlen(ip));
			//memcpy(ip_st[i].port, port, strlen(port));
			ip_st[i].callback = rx_callback;
			// 发送命令如 AT+QIOPEN=1,"TCP"，“183.230.40.150”，36000
			at_send_cmd(AT_IP_OPEN_CMD, CMD_START);
			at_send_cmd("=", CMD_DATA);
			at_send_cmd(fd, CMD_DATA);
			at_send_cmd((char *)ip_tcp_type_string, CMD_DATA);
			at_send_cmd(nowip, CMD_DATA);
			at_send_cmd("\",", CMD_DATA);
			at_send_cmd(port, CMD_END);
			/*	连接成功返回
			*		OK
			*		1, CONNECT OK
			*/
			len = strlen("OK\r\n");
			if(RET_OK	!= at_wait_response(len)){
				return ERR_IP_CMD;
			}
			if(dlen < len || dbuf[0] != 'O' || dbuf[1] != 'K'){
				// 1,ALREADY CONNECT
				// ERROR
				if(dbuf[0] == fd[0]){
					// READY CONNECT
					len = strlen("READY CONNECT\r\n");
					if(RET_OK	!= at_wait_response(len)){
						return ERR_IP_CMD;
					}
					if(dlen == len && 0 == memcmp(dbuf, "READY CONNECT\r\n", len)){
						// ERROR
						at_wait_response(WAIT_UNTIL_END);
						return i+1;
					}else{
						return ERR_IP_CONNECT;
					}
				}
				return ERR_IP_CMD;
			}
			len = strlen(ip_connet_string);
			if(RET_OK	!= at_wait_response(WAIT_UNTIL_END)){
				return ERR_IP_CMD;
			}
			if(dbuf[0] != i+'1'){
				return ERR_IP_CMD;
			}
			//			 3('1, ')
			if(dlen > len && 0 == memcmp(dbuf+3, ip_connet_string, len)){
				// 返回socket句柄{1，2}
				return i+1;
			}else{
				return ERR_IP_CONNECT;
			}
		}
	}
	
	return ERR_UNKNOWN;
}

int8_t m6315_socket_close(int8_t fd)
{
	char fdstring[2] = {0};
	uint8_t len;
	if(fd > 0 && fd <= MAX_IP_SOCKET_NUM){
		at_send_cmd(AT_IP_CLOSE_CMD, CMD_START);
		at_send_cmd("=", CMD_DATA);
		fdstring[0] = fd+'0';
		at_send_cmd(fdstring, CMD_END);
		if(RET_OK	== at_wait_response(WAIT_UNTIL_END)){
			len = strlen(ip_close_string);
			if(dlen > len && 0 == memcmp(dbuf, ip_close_string, len)){
				return RET_OK;
			}else{
				return ERR_IP_CMD;
			}
		}
	}
	
	return ERR_PARAMETER;
}

int8_t m6315_socket_close_all(void)
{
	int8_t i;
	for(i = 0; i < MAX_IP_SOCKET_NUM; i++){
		m6315_socket_close(i+1);
	}
	
	return RET_OK;
}

int8_t m6315_socket_send(int8_t fd, uint8_t *data, uint16_t len)
{
	char fdstring[2] = {0};
	char len_str[16] = {0};
	uint8_t retlen;
	uint8_t i;
	if(fd > 0 && fd <= MAX_IP_SOCKET_NUM && len > 0 && len < AT_RECV_BUF_LEN){
		at_send_cmd(AT_IP_SEND_CMD, CMD_START);
		at_send_cmd("=", CMD_DATA);
		fdstring[0] = fd+'0';
		at_send_cmd(fdstring, CMD_DATA);
		at_send_cmd(",", CMD_DATA);
		at_itoa(len, len_str);
		// 发送需要发送的数据长度
		at_send_cmd(len_str, CMD_END);
		for(i = 0; i < 10; i++){
			if(0 < at_uart_gets(fdstring,1)){
				if(fdstring[0] == '>'){
					break;
				}
			}
			at_delay_ms(1);
		}
		if(fdstring[0] == '>'){
			// 读出之后的空格字符
			at_uart_gets(fdstring,1);
			// 发送数据
			at_uart_puts(data, len);
			len+=2; // 读出回显数据和“\r\n”
			// 读取回显数据
			retlen = 0;
			do{
				retlen += at_uart_gets(dbuf,len-retlen);
				at_delay_ms(5);
			}while(retlen < len);
			// 发送<ctrl+z>
			//fdstring[0] = 0x1A;
			//at_send_cmd(fdstring, CMD_END);
			// 等待回复发送成功返回字符串
			len = strlen("SEND OK\r\n");
			if(RET_OK	== at_wait_response(len)){
				retlen = strlen(ip_send_string);
				if(dlen > retlen && 0 == memcmp(dbuf, ip_send_string, retlen)){
					return RET_OK;
				}else{
					return ERR_IP_CMD;
				}
			}
		}else{
			return ERR_IP_CMD;
		}
	}
	
	return ERR_PARAMETER;
}

//static uint8_t now_ip_recv_index = 0;
//// +RECEIVE: 1, 31
//int8_t get_ip_recv_len(void)
//{
//	int8_t i = 0;
//	uint16_t len = 0;
//	for(i = 13; i < dlen; i++){
//		if(dbuf[i] == '\r' && dbuf[i+1] == '\n'){
//			now_ip_recv_index = i+1;
//			return len;
//		}
//		len *= 10;
//		len += dbuf[i] - '0';
//	}
//	
//	return 0;
//}	

static uint8_t ip_rx_data_len=0;
static uint8_t ip_rx_fd=0;
static uint8_t process_step = 0;
void m6315_ip_process(void)
{
	uint8_t len = 0;
	// +RECEIVE: 1, 5
	// 12345
	switch(process_step){
		case 0:{
			len = at_uart_gets(rbuf, 1);
			if(len > 0 && *rbuf == '+'){
				process_step++;
				dlen = 0;
			}
		}break;
		case 1:{
			uint8_t ret = 0;
			len = strlen(ip_receive_string);
			ret = at_uart_gets(rbuf+dlen, len-dlen);
			if(ret > 0){
				dlen += ret;
			}
			// 判断是否为接收ip数据
			if(dlen == len){
				if(0 == memcmp(rbuf, ip_receive_string, len)){
					process_step++;
					dlen = 0;
				}else{
					process_step = 0;
				}
			}
		}break;
		case 2:{
			len = at_uart_gets(rbuf, 1);
			if(len > 0){
				// 获取ip句柄
				if(*rbuf >= '0' && *rbuf <= '9'){
					ip_rx_fd = *rbuf - '0';
					if(ip_rx_fd > MAX_IP_SOCKET_NUM){
						process_step = 0;
					}else{
						process_step++;
						dlen = 0;
					}
				}else{
					process_step = 0;
				}
			}
		}break;
		case 3:{
			len = at_uart_gets(rbuf+dlen, 1);
			if(len > 0){
				dlen += len;
			}
			if(dlen > 2 && rbuf[dlen - 2] == '\r' && rbuf[dlen - 1] == '\n'){
				if(dlen >= 5){
					// 设置数据长度字符串结束
					rbuf[dlen-2] = 0;
					// , 5
					// 获取ip数据包长度
					ip_rx_data_len = at_atoi(rbuf + 2);
					// 接收数据大于0同时小于缓冲区
					if(ip_rx_data_len > 0 && ip_rx_data_len < AT_RECV_BUF_LEN){
						process_step++;
						dlen = 0;
					}else{
						process_step = 0;
					}
				}else{
					process_step = 0;
				}
			}
		}break;
		case 4:{
			len = at_uart_gets(rdbuf+dlen, ip_rx_data_len - dlen);
			if(len > 0){
				dlen += len;
			}
			// 回调接收数据函数
			if(dlen == ip_rx_data_len){
				ip_st[ip_rx_fd - 1].callback(rdbuf, ip_rx_data_len);
				process_step = 0;
			}
		}break;
		default:{
			process_step = 0;
		}break;
	}
}

int8_t m6315_ftp_open(char *ip, char *port, char *user, char *passwd)
{
	char nowip[16] = {0};
	uint8_t len = 0;
	
	if(ip && port && user && passwd){
		if(RET_OK != m6315_dns_get_ip(ip, nowip)){
			return ERR_IP_OR_HOST;
		}
		
		// 设置用户名
		at_send_cmd(AT_SET_FTP_USER_CMD, CMD_START);
		at_send_cmd("=\"", CMD_DATA);
		at_send_cmd(user, CMD_DATA);
		at_send_cmd("\"", CMD_END);
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_FTP_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_FTP_CMD;
		}
		
		// 设置密码
		at_send_cmd(AT_SET_FTP_PASSWORD_CMD, CMD_START);
		at_send_cmd("=\"", CMD_DATA);
		at_send_cmd(passwd, CMD_DATA);
		at_send_cmd("\"", CMD_END);
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_FTP_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_FTP_CMD;
		}
		
		//连接ftp服务器
		at_send_cmd(AT_FTP_OPEN_CMD, CMD_START);
		at_send_cmd("=\"", CMD_DATA);
		at_send_cmd(nowip, CMD_DATA);
		at_send_cmd("\",", CMD_DATA);
		at_send_cmd(port, CMD_END);
		/*	OK
		 *	+QFTPOPEN:0
		*/
		len = strlen("OK\r\n");
		if(RET_OK != at_wait_response(len)){
			return ERR_FTP_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_FTP_CMD;
		}
		len = strlen(AT_FTP_OPEN_CMD);
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_FTP_CMD;
		}
		//			 1('+')
		if(0 == memcmp(dbuf + 1, AT_FTP_OPEN_CMD, len)){
			// 连接返回值为0则成功连接
			if(dbuf[1 + len + 2] == '0'){
				return RET_OK;
			}else{
				return ERR_FTP_OPEN;
			}
		}else{
			return ERR_FTP_CMD;
		}
	}
	return ERR_PARAMETER;
}

void at_itoa(uint32_t number, char *str)
{
	uint8_t num = 0;
	if(number <= 9){
		*str = number+'0';
	}else{
		num = number % 10;
		number /= 10;
		at_itoa(number, str);
		str++;
		*str = num+'0';
	}
}

uint32_t at_atoi(char *len_str)
{
	uint32_t len = 0;
	while(*len_str){
		if(*len_str < '0' || *len_str > '9')
			return len;
		len *= 10;
		len += *len_str-'0';
		len_str++;
		if(len > 0x7fffffff)
			return len;
	}
	return len;
}

static int8_t set_file_position(uint32_t pos)
{
	char pos_str[10] = {0};
	uint8_t len = 0;
	if(pos < 999999999){
		at_itoa(pos, pos_str);
		at_send_cmd(AT_FTP_SET_POSITION_CMD, CMD_START);
		at_send_cmd(pos_str, CMD_END);
		/*	OK
		 *	+QFTPCFG:0
		*/
		len = strlen("OK\r\n");
		if(RET_OK != at_wait_response(len)){
			return ERR_FTP_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_FTP_CMD;
		}
		len = strlen(AT_FTP_CFG_CMD);
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_FTP_CMD;
		}
		//			 1('+')
		if(0 == memcmp(dbuf + 1, AT_FTP_CFG_CMD, len)){
			// 连接返回值为0则设置成功
			if(dbuf[1 + len + 2] == '0'){
				return RET_OK;
			}else{
				return ERR_FTP_OPEN;
			}
		}else{
			return ERR_FTP_CMD;
		}
	}
	
	return ERR_PARAMETER;
}

int8_t m6315_ftp_get_file(char *file_name, uint32_t position, void *buf, uint16_t now_len)
{
	char len_str[10] = {0};
	uint32_t len = 0;
	// 设置下载文件偏移
	if(RET_OK != set_file_position(position))
		return ERR_FTP_CMD;
	
	if(file_name && buf){
		at_itoa(now_len, len_str);
		at_send_cmd(AT_FTP_GET_FILE_CMD, CMD_START);
		at_send_cmd("=\"", CMD_DATA);
		at_send_cmd(file_name, CMD_DATA);
		at_send_cmd("\",", CMD_DATA);
		at_send_cmd(len_str, CMD_END);
		/*	OK
		 *	CONECT
		 *	1234567
		 *	+QFTPGET:7
		*/
		len = strlen("OK\r\n");
		if(RET_OK != at_wait_response(len)){
			return ERR_FTP_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_FTP_CMD;
		}
		len = strlen(ftp_get_file_string);
		if(RET_OK != at_wait_response(len + 2)){
			return ERR_FTP_CMD;
		}
		if(0 != memcmp(dbuf, ftp_get_file_string, len)){
			return ERR_FTP_CMD;
		}
		// 读取ftp文件数据
		len = 0;
		do{
				len += at_uart_gets((char *)buf+len, now_len-len);
		}while(len < now_len);
		
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_FTP_CMD;
		}
		// 判断返回命令
		//			 1('+')
		if(0 == memcmp(dbuf + 1, AT_FTP_CFG_CMD, len)){
			// 判断返回数据长度
			//			 1('+')   len('QFTPGET')    1(':')
			if(0 == memcmp(dbuf + 1 + len + 1, len_str, strlen(len_str))){
				return RET_OK;
			}else{
				return ERR_FTP_OPEN;
			}
		}else{
			return ERR_FTP_CMD;
		}
	}
	
	return ERR_PARAMETER;
}

int32_t m6315_ftp_get_file_len(char *file_name)
{
	char len_str[10];
	uint8_t len = 0;
	if(file_name){
		at_send_cmd(AT_FTP_GET_FILE_SIZE_CMD, CMD_START);
		at_send_cmd("=\"", CMD_DATA);
		at_send_cmd(file_name, CMD_DATA);
		at_send_cmd("\"", CMD_END);
		/*	OK
		 *	+QFTPSIZE:10
		*/
		len = strlen("OK\r\n");
		if(RET_OK != at_wait_response(len)){
			return ERR_FTP_CMD;
		}
		if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
			return ERR_FTP_CMD;
		}
		len = strlen(AT_FTP_GET_FILE_SIZE_CMD);
		if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
			return ERR_FTP_CMD;
		}
		//			 1('+')
		if(0 != memcmp(dbuf + 1, AT_FTP_GET_FILE_SIZE_CMD, len)){
			return ERR_FTP_CMD;
		}
		//			 1('+')   len('QFTPSIZE')    1(':')
		if(dlen > (1+len+1+sizeof(len_str))){
			return ERR_FTP_FILE_SIZE;
		}
		//			 1('+')   len('QFTPSIZE')    1(':')	    2('\r\n')
		memcpy(len_str, dbuf + 1 + len + 1, dlen - (1 + len + 1 + 2));
		
		return at_atoi(len_str);
	}
	
	return ERR_PARAMETER;
}

int8_t m6315_ftp_close(void)
{
	at_send_cmd(AT_FTP_CLOSE_CMD, CMD_START_END);
	if(RET_OK != at_wait_response(WAIT_UNTIL_END)){
		return ERR_FTP_CMD;
	}
	if(dlen < 4 || dbuf[0] != 'O' || dbuf[1] != 'K'){
		return ERR_FTP_CMD;
	}
	return RET_OK;
}
