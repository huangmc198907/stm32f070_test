#include "stm32f0xx_hal.h"
#include "at_cmd.h"
#include "string.h"


#define PinMode(v, pin)   ((v) << ((   pin > 7 ? pin-8 : pin  )*4))

#define LED_OFF()   do{ GPIOA->BRR = GPIO_PIN_5; }while(0)
#define LED_ON()   do{ GPIOA->BSRR = GPIO_PIN_5; }while(0)


#define PAGE_SIZE           1024

/* JEDEC Manufacturer ID */
#define MF_ID           (0xEF)

#define DUMMY                       (0xFF)

#define BOOT_ADDR   0x08000000

#define  app    ((const prog_rom_t*)SYS_ADDR)

uint32_t stm32_crc(const uint32_t* data, uint32_t len, int reset)
{
  if(reset){
    SET_BIT(CRC->CR,CRC_CR_RESET);
  }
  while(len){
    CRC->DR = *data;
    data++;
    len--;
  }
  return CRC->DR;
}

#define  RESET_REASON         (*(uint32_t*)0x20000000)
#define  JUMP_TO_SYSTEM       0xdeadbeef
#define  FORCE_UPDATE         0xfeedbeef

#define IS_JUMP_TO_SYSTEM()   (RESET_REASON == JUMP_TO_SYSTEM)
#define IS_FORCE_UPDATE()     (RESET_REASON == FORCE_UPDATE)

#define UART_RX_INT_BUF_LEN		(128)
typedef struct ring_buf{
	uint8_t buf[UART_RX_INT_BUF_LEN];
	uint32_t put_id;
	uint32_t get_id;
	uint32_t len;
}ring_buf_st;

static ring_buf_st uart1_rx_buf;

static void soft_reset(uint32_t reason)
{
  int i;
  __disable_irq();
  __HAL_RCC_AHB_FORCE_RESET();
  __HAL_RCC_APB1_FORCE_RESET();
  __HAL_RCC_APB2_FORCE_RESET();
  for(i=20;i>0;i--);
  __HAL_RCC_AHB_RELEASE_RESET();
  __HAL_RCC_APB1_RELEASE_RESET();
  __HAL_RCC_APB2_RELEASE_RESET();
  __enable_irq();
  RESET_REASON = reason;
  NVIC_SystemReset();
}

void force_system(void)
{
  soft_reset(JUMP_TO_SYSTEM);
}

void force_update(void)
{
  soft_reset(FORCE_UPDATE);
}

#define APP_INFO_BLOCK  0
#define APP_BLOCK       8
#define APP_BLOCK_END   16

void wait_flash(void)
{
  int cnt = 0x100000;
  while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)){
    if(cnt<=0)break;
    cnt--;
  }
  if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP)){
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
  }
}


void flash_rdp()
{
  wait_flash();
  
  WRITE_REG(FLASH->OPTKEYR, FLASH_KEY1);
  WRITE_REG(FLASH->OPTKEYR, FLASH_KEY2);
  
  SET_BIT(FLASH->CR, FLASH_CR_OPTER);
  SET_BIT(FLASH->CR, FLASH_CR_STRT);
  wait_flash();
  
  CLEAR_BIT(FLASH->CR, FLASH_CR_OPTER);
  SET_BIT(FLASH->CR, FLASH_CR_OPTPG);
  //OB->RDP = RDP_KEY;  
  wait_flash();
  
  CLEAR_BIT(FLASH->CR, FLASH_CR_OPTPG);
}


void flash_unlock(void)
{
  if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
  {
    /* Authorize the FLASH Registers access */
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);
    while(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET);
  }
}

void flash_lock(void)
{
  SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}


static void erase_pages(uint32_t from, uint32_t to)
{
  wait_flash();
  for(;from<to;from+=FLASH_PAGE_SIZE){
    SET_BIT(FLASH->CR, FLASH_CR_PER);
    WRITE_REG(FLASH->AR, from);
    SET_BIT(FLASH->CR, FLASH_CR_STRT);
    wait_flash();
    CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
  }
}

static void FLASH_Program_HalfWord(uint32_t Address, uint16_t Data)
{
  /* Clean the error context */
  
#if defined(FLASH_BANK2_END)
  if(Address <= FLASH_BANK1_END)
  {
#endif /* FLASH_BANK2_END */
    /* Proceed to program the new data */
    SET_BIT(FLASH->CR, FLASH_CR_PG);
#if defined(FLASH_BANK2_END)
  }
  else
  {
    /* Proceed to program the new data */
    SET_BIT(FLASH->CR2, FLASH_CR2_PG);
  }
#endif /* FLASH_BANK2_END */

  /* Write data in the address */
  *(__IO uint16_t*)Address = Data;
}

static int program_data(uint32_t addr, const void* data, uint32_t len)
{
  const uint16_t* src = (const uint16_t*)data;
  wait_flash();
  for(uint32_t i=0;i<len/2;i++){
    SET_BIT(FLASH->CR, FLASH_CR_PG);
    ((__IO uint16_t*)addr)[i] = src[i];
    wait_flash();
    CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
  }
  const uint8_t* p1 = (const uint8_t*)data;
  const uint8_t* p2 = (const uint8_t*)addr;
  while(len){
    if(*p1 != *p2){
      return -1;
    }
    len--;
  }
  return 0;
}

void deinit(void)
{
  __HAL_RCC_GPIOA_FORCE_RESET();
  
  __HAL_RCC_GPIOA_RELEASE_RESET();
}

void delay_200ms()
{
  __IO uint32_t v = 150000;
  for(;v;v--);
}

void uart_putc(uint8_t c)
{
	while((USART1->ISR & UART_FLAG_TXE) != UART_FLAG_TXE);
	USART1->TDR = c & (uint8_t)0xFFU;
}

int8_t uart_getc(uint8_t *p)
{
	if(uart1_rx_buf.len != 0){	
		*p = uart1_rx_buf.buf[uart1_rx_buf.get_id];
		uart1_rx_buf.get_id++;
		if(uart1_rx_buf.get_id == UART_RX_INT_BUF_LEN)
			uart1_rx_buf.get_id = 0;
		uart1_rx_buf.len--;
		return 0;
	}
	
	return -1;
}

int16_t uart_gets(uint8_t *p, uint16_t len)
{
	uint16_t index = 0;
	uint16_t ret = 0;
	if(uart1_rx_buf.len != 0 && p && len > 0){	
		ret = len = len > uart1_rx_buf.len ? uart1_rx_buf.len : len;
		if(uart1_rx_buf.get_id + len > UART_RX_INT_BUF_LEN){
			memcpy(p, uart1_rx_buf.buf + uart1_rx_buf.get_id, UART_RX_INT_BUF_LEN - uart1_rx_buf.get_id);
			index = UART_RX_INT_BUF_LEN - uart1_rx_buf.get_id;
			uart1_rx_buf.get_id = 0;
			uart1_rx_buf.len -= index;
			len -= index;
		}
		memcpy(p+index, uart1_rx_buf.buf + uart1_rx_buf.get_id, len);
		uart1_rx_buf.get_id += len;
		uart1_rx_buf.len -= len;
		return ret;
	}
	
	return 0;
}

void SystemClock_Config(void)
{
	__HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(16);
	__HAL_RCC_HSI14ADC_DISABLE();
	__HAL_RCC_HSI14_ENABLE();
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSI14RDY) == RESET);
	//while(!(RCC->CR2 & 0x2));
	__HAL_RCC_HSI14_CALIBRATIONVALUE_ADJUST(16);
	__HAL_RCC_PLL_DISABLE();
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  != RESET);
	//while(RCC->CR & 0x2000000);
	__HAL_RCC_PLL_CONFIG(RCC_PLLSOURCE_HSI,
                             RCC_PREDIV_DIV1,
                             RCC_PLL_MUL6);
  /* Enable the main PLL. */
  __HAL_RCC_PLL_ENABLE();
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  == RESET);
	//while(!(RCC->CR & 0x2000000));
	__HAL_FLASH_SET_LATENCY(0x1);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_SYSCLK_DIV1);
	__HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_PLLCLK);
	while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK);
	//while((RCC->CFGR & 0xc) != 0x00000008);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE, RCC_HCLK_DIV1);
	__HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK1);
}

int8_t ip_callback(void *buf, uint16_t len)
{
	m6315_socket_send(1, buf, len);
	return 0;
}

void at_uart_puts(void *buf, uint32_t len)
{
	uint8_t *p = (uint8_t *)buf;
	while(len > 0){
		uart_putc(*p);
		p++;
		len--;
	}
}

uint32_t at_uart_gets(void *buf, uint32_t max_len)
{
	return uart_gets(buf, max_len);
}

void at_delay_ms(uint16_t count)
{
	int i = 0;
	for(i = 0; i < 0xfffff * count; i++);
}

int main()
{
	uint8_t fd = 0;
	SystemClock_Config();
	
	//__HAL_RCC_GPIOA_CLK_ENABLE();
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER |= (GPIO_MODE_OUTPUT_PP << (5 * 2)) | (GPIO_MODE_AF_PP << (9 * 2)) | (GPIO_MODE_AF_PP << (10 * 2));
  //GPIOA->AFR[9 >> 3U] |= GPIO_AF1_USART1 << ((9 & 0x07U) * 4U);
	//GPIOA->AFR[10 >> 3U] |= GPIO_AF1_USART1 << ((10 & 0x07U) * 4U);
	GPIOA->AFR[1] |= 0x110;
  GPIOA->OSPEEDR |= GPIO_SPEED_FREQ_LOW << (5 * 2) | GPIO_SPEED_FREQ_HIGH << (9 * 2) | GPIO_SPEED_FREQ_HIGH << (10 * 2);
	GPIOA->BSRR = 0x1 << 5;
	
	//__HAL_RCC_USART1_CLK_ENABLE();
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	USART1->BRR = ((48000000) + ((115200)/2U)) / (115200);
	USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE /*| USART_CR1_PEIE*/ | USART_CR1_RXNEIE;
	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
  //USART1->CR3 |= USART_CR3_EIE;
	NVIC_SetPriority(USART1_IRQn,0);
	NVIC_EnableIRQ(USART1_IRQn);
	
	init_m6315();
	fd = m6315_socket_open("29485b68g3.qicp.vip", "18306", ip_callback);
	if(fd > 0){
		m6315_socket_send(fd, (uint8_t *)"12345", 5);
	}
	while(1){
		delay_200ms();
		m6315_ip_process();
	}
}

void SystemInit(void)
{
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001U;

  /* Reset SW[1:0], HPRE[3:0], PPRE[2:0], ADCPRE, MCOSEL[2:0], MCOPRE[2:0] and PLLNODIV bits */
  RCC->CFGR &= (uint32_t)0x08FFB80CU;
  
  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFFU;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFFU;

  /* Reset PLLSRC, PLLXTPRE and PLLMUL[3:0] bits */
  RCC->CFGR &= (uint32_t)0xFFC0FFFFU;

  /* Reset PREDIV[3:0] bits */
  RCC->CFGR2 &= (uint32_t)0xFFFFFFF0U;

  /* Reset USART1SW[1:0], I2C1SW and ADCSW bits */
  RCC->CFGR3 &= (uint32_t)0xFFFFFEECU;

  /* Reset HSI14 bit */
  RCC->CR2 &= (uint32_t)0xFFFFFFFEU;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000U;

}

/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{
  volatile uint32_t isrflags   = USART1->ISR;
  volatile uint32_t cr1its     = USART1->CR1;
  uint32_t errorflags;
	
  /* If no error occurs */
  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
	if(errorflags == RESET && (isrflags & USART_ISR_RXNE) != RESET){
		uart1_rx_buf.buf[uart1_rx_buf.put_id] = USART1->RDR & (uint8_t)0xFFU;
		uart1_rx_buf.put_id++;
		if(uart1_rx_buf.put_id == UART_RX_INT_BUF_LEN)
			uart1_rx_buf.put_id = 0;
		// drop old data
		uart1_rx_buf.len++;
		if(uart1_rx_buf.len > UART_RX_INT_BUF_LEN){
			uart1_rx_buf.len = UART_RX_INT_BUF_LEN;
			uart1_rx_buf.get_id++;
			if(uart1_rx_buf.get_id == UART_RX_INT_BUF_LEN)
				uart1_rx_buf.get_id = 0;
		}
		return;
	}

  if(errorflags != RESET){
		USART1->ICR |= (UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);
  } /* End if some error occurs */

//  /* UART in mode Transmitter ------------------------------------------------*/
//  if(((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
//  {
//    UART_Transmit_IT(huart);
//    return;
//  }

//  /* UART in mode Transmitter (transmission end) -----------------------------*/
//  if(((isrflags & USART_ISR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
//  {
//    UART_EndTransmit_IT(huart);
//    return;
//  }
}

