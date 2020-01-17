#include "stm32f0xx_hal.h"


#define PinMode(v, pin)   ((v) << ((   pin > 7 ? pin-8 : pin  )*4))

#define LED_ON()   do{ GPIOA->BRR = GPIO_PIN_5; }while(0)
#define LED_OFF()   do{ GPIOA->BSRR = GPIO_PIN_5; }while(0)


#define PAGE_SIZE           1024

/* JEDEC Manufacturer ID */
#define MF_ID           (0xEF)

#define DUMMY                       (0xFF)

#define BOOT_ADDR   0x08000000

typedef struct _boot_rom_t
{
  uint32_t sp;
  uint32_t entry_point;
  uint32_t api_force_update;
  uint32_t api_force_jump_sys;
  uint32_t board_version;
  uint32_t boot_version;   // ROM total length, include CRC
  uint8_t  SN[16];         // board serial number
  uint8_t  reserved;
}boot_rom_t;

#define  boot    ((const boot_rom_t*)BOOT_ADDR)

#define SYS_ADDR    0x08001000

#define PROG_ADDR      0x08000800
#define PROG_ADDR_END (0x08000000 + 10*1024)
#define PROG_SIGN   0x474f5250  // PROG

typedef struct _prog_rom_t
{
  uint32_t crc;
  uint32_t entry_point;
  uint32_t sign;
  uint32_t len;           // ROM total length, include CRC
  uint32_t version;       // program version
  uint32_t support_board; // supported board version
  uint32_t support_boot;  // supported boot version
  uint32_t reserved;      //  
  uint32_t VECTOR[16];    // real vector started here
}prog_rom_t;

#define  app    ((const prog_rom_t*)PROG_ADDR)
  

#define INFO_SIGN   0x4f464e49   // INFO
#define MAX_INFO_BLOCK_COUNT   (1024-8)
typedef struct _flash_info_t
{
  uint32_t crc;                  // CRC value of flash info
  uint32_t start_addr;
  uint32_t sign;
  uint32_t len;
  uint32_t version;              // program version
  uint32_t support_board;        // supported board version
  uint32_t support_boot;         // supported boot version
  uint16_t block_size;
  uint16_t block_count;
  uint32_t block_crc[MAX_INFO_BLOCK_COUNT];
}flash_info_t;

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

uint32_t is_prog_valid()
{
  if( IS_FORCE_UPDATE() ){
    RESET_REASON = 0;
    return 0;
  }
  
  const prog_rom_t* prog = (const prog_rom_t*)PROG_ADDR;
  if(prog->sign == PROG_SIGN && prog->len < 8*1024  && prog->entry_point == prog->VECTOR[1]){
    uint32_t crc;
    crc = stm32_crc( &prog->entry_point, (prog->len-4)/4, 1);
    if(prog->crc ==  crc){
      return 1;
    }
  }
  return 0;
}

#define APP_INFO_BLOCK  0
#define APP_BLOCK       8
#define APP_BLOCK_END   16

uint32_t is_flash_valid(uint32_t prog_valid)
{
	volatile uint32_t *flash_data = (uint32_t *)0x8000000;
  prog_rom_t* prog = (prog_rom_t*)(0x8008000);
  //w25qxx_read(APP_BLOCK*PAGE_SIZE, flash_data, 256);
  if(prog->sign != PROG_SIGN){
    return 0;
  }
  if( prog->support_board != boot->board_version) return 0;
  if( prog->support_boot != boot->boot_version ) return 0;
  if( prog->len < 64 ) return 0;
  if( prog->len > APP_BLOCK_END*PAGE_SIZE ) return 0;
  if(prog_valid){
    if(prog->version <= app->version){
      return 0;
    }
  }
  
  uint32_t crc_prog = prog->crc;
  uint32_t len = prog->len;
  uint32_t c_len = 256;
  uint32_t addr = APP_BLOCK*PAGE_SIZE + 256;
  uint32_t crc = stm32_crc( (uint32_t*)(flash_data+4), (256-4)/4, 1);
  while(c_len<len){
    uint32_t l = 1024;
    if(c_len + 1024 > len){
      l = len - c_len;
    }
    //w25qxx_read(addr, flash_data, l);
    crc = stm32_crc((uint32_t*)flash_data, (l+3)/4, 0);
    addr += l;
    c_len += l;
  }
  if(crc != crc_prog)return 0;
  return 1;
}

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

int copy_flash(void)
{
  int r = 0;
	uint32_t *flash_data = (uint32_t *)0x8000000;
  //w25qxx_read(APP_INFO_BLOCK*PAGE_SIZE, flash_data, PAGE_SIZE);
  flash_info_t* info = (flash_info_t*)(0x8000000);
  if(info->sign != INFO_SIGN){
    return -1;
  }
  if(info->start_addr != PROG_ADDR){
    return -1;
  }
  if(info->len > PROG_ADDR_END - PROG_ADDR){
    return -1;
  }
  if(info->block_count > MAX_INFO_BLOCK_COUNT){
    return -1;
  }
  if(info->crc != stm32_crc( &info->start_addr, info->block_count+7, 1) ){
    return -1;
  }
  
  uint32_t end = info->len + FLASH_PAGE_SIZE - 1;
  end &= ~(FLASH_PAGE_SIZE-1);
  end += PROG_ADDR;
  
  flash_unlock();
  erase_pages(PROG_ADDR, end);
  
  uint32_t total_len = info->len;
  uint32_t addr = info->start_addr;
  uint32_t flash_addr = APP_BLOCK*PAGE_SIZE;
  for(uint32_t i=0;i<total_len; i+= PAGE_SIZE){
    LED_ON();
    //w25qxx_read(flash_addr+i, flash_data, PAGE_SIZE);
    uint32_t len = PAGE_SIZE;
    if(len + i > total_len){
      len = total_len - i;
    }
    LED_OFF();
    if(program_data(addr+i, flash_data, len) != 0){
      r = -1;
      break;
    }
  }
  flash_lock();
  return r;
}

void deinit(void)
{
  __HAL_RCC_GPIOA_FORCE_RESET();
  
  __HAL_RCC_GPIOA_RELEASE_RESET();
}

void jump_to_prog(void)
{
  deinit();
  const prog_rom_t* prog = (const prog_rom_t*)PROG_ADDR;
  __set_MSP(prog->VECTOR[0]);
  ( (void(*)(void)) prog->entry_point ) ();
}

void jump_to_sys(void)
{
  deinit();
  const uint32_t* VECTOR = (const uint32_t*)SYS_ADDR;
  __set_MSP(VECTOR[0]);
  ( (void(*)(void)) VECTOR[1] ) ();
}


void delay_200ms()
{
  __IO uint32_t v = 150000;
  for(;v;v--);
}

void success()
{
    LED_ON();
    delay_200ms();
    delay_200ms();
    LED_OFF();
    delay_200ms();
    delay_200ms();
}

void fail()
{
  for(int i=0;i<3;i++){
    LED_ON();
    delay_200ms();
    LED_OFF();
    delay_200ms();
  }
}

void uart_putc(uint8_t c)
{
	while((USART1->ISR & UART_FLAG_TXE) != UART_FLAG_TXE);
	USART1->TDR = c & (uint8_t)0xFFU;
}

uint8_t uart_gutc(void)
{
	while((USART1->ISR & UART_FLAG_RXNE) != UART_FLAG_RXNE);
	return USART1->RDR & (uint8_t)0xFFU;
}

void SystemClock_Config(void)
{
	__HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(16);
	//RCC->CR &= 0xffffff07;
	//RCC->CR |= 0x80;
	__HAL_RCC_HSI14ADC_DISABLE();
	//RCC->CR2 |= 0x4;
	__HAL_RCC_HSI14_ENABLE();
	//RCC->CR2 |= 0x1;
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSI14RDY) == RESET);
	//while(!(RCC->CR2 & 0x2));
	__HAL_RCC_HSI14_CALIBRATIONVALUE_ADJUST(16);
	//RCC->CR2 &= 0xffffff07;
	//RCC->CR2 |= 0x80;
	__HAL_RCC_PLL_DISABLE();
	//RCC->CR &= 0xfeffffff;
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  != RESET);
	//while(RCC->CR & 0x2000000);
	__HAL_RCC_PLL_CONFIG(RCC_PLLSOURCE_HSI,
                             RCC_PREDIV_DIV1,
                             RCC_PLL_MUL6);
	//RCC->CFGR2 &= 0xfffffff0;
	//RCC->CFGR2 |= RCC_PREDIV_DIV1;
	//RCC->CFGR &= 0xffc39fff;
	//RCC->CFGR |= 0x00108000;
  /* Enable the main PLL. */
  __HAL_RCC_PLL_ENABLE();
	//RCC->CR |= 0x1000000;
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  == RESET);
	//while(!(RCC->CR & 0x2000000));
	__HAL_FLASH_SET_LATENCY(0x1);
	//FLASH->ACR |= 0x1;
	MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_SYSCLK_DIV1);
	//RCC->CFGR &= 0xffffff0f;
	__HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_PLLCLK);
	//RCC->CFGR &= 0xfffffffc;
	//RCC->CFGR |= RCC_CFGR_SW_PLL;
	while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK);
	//while((RCC->CFGR & 0xc) != 0x00000008);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE, RCC_HCLK_DIV1);
	//RCC->CFGR &= 0xfffff8ff;
	__HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK1);
	//RCC->CFGR3 &= 0xfffffffc;
}

int main()
{
  if( IS_JUMP_TO_SYSTEM() ){
    RESET_REASON = 0;
    jump_to_sys();
  }
	
	SystemClock_Config();
	
	//__HAL_RCC_GPIOA_CLK_ENABLE();
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	//GPIOA->MODER |= (GPIO_MODE_OUTPUT_PP << (5 * 2)) | (GPIO_MODE_AF_PP << (9 * 2)) | (GPIO_MODE_AF_PP << (10 * 2));
	GPIOA->MODER |= 0x280400;
  //GPIOA->AFR[9 >> 3U] |= GPIO_AF1_USART1 << ((9 & 0x07U) * 4U);
	//GPIOA->AFR[10 >> 3U] |= GPIO_AF1_USART1 << ((10 & 0x07U) * 4U);
	GPIOA->AFR[1] |= 0x110;
  //GPIOA->OSPEEDR |= GPIO_SPEED_FREQ_LOW << (5 * 2) | GPIO_SPEED_FREQ_HIGH << (9 * 2) | GPIO_SPEED_FREQ_HIGH << (10 * 2);
  GPIOA->OSPEEDR |= 0x003C0000;
	//GPIOA->BSRR = 0x1 << 5;
	GPIOA->BSRR = (uint32_t)0x0020;
	
	//__HAL_RCC_USART1_CLK_ENABLE();
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	USART1->BRR = ((48000000) + ((115200)/2U)) / (115200);
	//USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;
	USART1->CR1 = 0xD;

  uint32_t prog_valid = is_prog_valid();
  if( is_flash_valid( prog_valid ) ){
    // copy flash program and data
    if(copy_flash() == 0){
      prog_valid = is_prog_valid();
    }else{
      prog_valid = 0;
    }
  }
  
  if(prog_valid){
    success();
    jump_to_prog();
  }else{
    fail();
    jump_to_sys();
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

