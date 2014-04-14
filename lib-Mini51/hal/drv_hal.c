#include "hal.h"
#include "drv_serial.h"
#include "drv_pwm.h"
#include "drv_gpio.h"

#include "lib_i2c.h"
#include "lib_spi.h"


#define FLASH_PAGE_SIZE                 ((uint16_t)0x200)
#define FLASH_WRITE_ADDR                (16*1024 - FLASH_PAGE_SIZE)  // use the last 0.5 KB for storage
#define EEP_SIZE                        (FLASH_PAGE_SIZE)

static bool flash_available = false;
static uint32_t last_write_index = FLASH_PAGE_SIZE;

static int set_data_flash_base(uint32_t u32DFBA);

void lib_hal_init(void)
{
    SYS_UnlockReg();
    
    // Init System Clock

    // Enable internal 22.1184MHz
//    CLK->PWRCON |= CLK_PWRCON_IRC22M_EN_Msk;

    // Waiting for clock ready
//    CLK_WaitClockReady(CLK_CLKSTATUS_IRC22M_STB_Msk);

    /* Switch HCLK clock source to XTL */
   // CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_XTAL,CLK_CLKDIV_HCLK(1));
    // Stay with reset value = internal oscillator

    /* STCLK to XTL STCLK to XTL */
   // CLK_SetSysTickClockSrc(CLK_CLKSEL0_STCLK_S_XTAL);
    // Stay with reset value = internal oscillator

    // Update SystemCoreClock and CyclesPerUs
    SystemCoreClockUpdate();         
    
    FMC_Open();
    if (set_data_flash_base(FLASH_WRITE_ADDR) == 0)
    {
        flash_available = true;
    }
    FMC_Close();
    

    // Init UART clock
    uartInit();

    // Init PWM clock and hardware
    drv_pwm_config_t pwm;
    pwmInit(&pwm);

    lib_i2c_init();
    lib_spi_init();

    SYS_LockReg();
#if CONTROL_BOARD_TYPE == CONTROL_BOARD_WLT_V202
#else
    gpio_config_t gpio;
    drv_pwm_config_t pwm;

    // Turn on clocks for stuff we use
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_ADC1 | RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_ClearFlag();

    // Make all GPIO in by default to save power and reduce noise
    gpio.pin = Pin_All;
    gpio.mode = Mode_AIN;
    gpioInit(GPIOA, &gpio);
    gpioInit(GPIOB, &gpio);
    gpioInit(GPIOC, &gpio);

    pwm.airplane = false;
    pwm.useUART = false;
    pwm.usePPM = true;
    pwm.enableInput = true;
    pwm.useServos = false;
    pwm.extraServos = false;
    pwm.motorPwmRate = 498;
    pwm.servoPwmRate = 50;

    pwmInit(&pwm);
#endif
}

///////////////////////////////////////////////////////////////////////
// EEPROM

//static uint8_t eep[EEP_SIZE];

// Mask for alignment by word boundary
#define ALIGN_SIZE sizeof(int)
#define ALIGN_MASK (ALIGN_SIZE - 1)


size_t eeprom_write_block(const void *src, uint16_t index, size_t size)
{
    if (!flash_available) return 0;
    SYS_UnlockReg();
    FMC_Open();

    if (index < last_write_index) {
        FMC_Erase(FLASH_WRITE_ADDR);
    }
    uint8_t *read_src = (uint8_t *) src;
    uint32_t addr = (FLASH_WRITE_ADDR + index) & ~ALIGN_MASK;
    int i = index & ALIGN_MASK;
    uint32_t data;
    if (i) {
        data = FMC_Read(addr);
    }
    uint32_t endAddr = (FLASH_WRITE_ADDR + index + size + (ALIGN_SIZE-1)) & ~ALIGN_MASK;
    for (; addr < endAddr; addr += ALIGN_SIZE)
    {
        int l = FLASH_WRITE_ADDR + index + size - addr;
        if (l > ALIGN_SIZE) l = ALIGN_SIZE;
        if (l != ALIGN_SIZE) {
            data = FMC_Read(addr);
        }
        for (; i < l;) {
            ((uint8_t *) &data)[i++] = *read_src++;
        }
        FMC_Write(addr, data);
        i = 0;
    }

    FMC_Close();
    SYS_LockReg();
    last_write_index = index + size;
    
    return size;
}

void eeprom_commit(void)
{
}

size_t eeprom_read_block(void *dst, uint16_t index, size_t size)
{
    if (!flash_available) return 0;
    SYS_UnlockReg();
    FMC_Open();

    uint8_t *write_dst = (uint8_t *) dst;
    uint32_t addr = (FLASH_WRITE_ADDR + index) & ~ALIGN_MASK;
    int i = index & ALIGN_MASK;
    uint32_t endAddr = (FLASH_WRITE_ADDR + index + size + (ALIGN_SIZE-1)) & ~ALIGN_MASK;
    for (; addr < endAddr; addr += ALIGN_SIZE)
    {
        uint32_t data = FMC_Read(addr);
        int l = FLASH_WRITE_ADDR + index + size - addr;
        if (l > ALIGN_SIZE) l = ALIGN_SIZE;
        for (; i < l;) {
            *write_dst++ = ((uint8_t *) &data)[i++];
        }
        i = 0;
    }

    FMC_Close();
    SYS_LockReg();
    
    return size;
}


// Copied from Nuvoton's sample code
static int set_data_flash_base(uint32_t u32DFBA)
{
    uint32_t   au32Config[2];
    
    if (FMC_ReadConfig(au32Config, 2) < 0)
    {
        return -1;
    }
        
    if ((!(au32Config[0] & 0x1)) && (au32Config[1] == u32DFBA))
        return 0;
        
    FMC_EnableConfigUpdate();

    if (u32DFBA !=0)    
        au32Config[0] &= ~0x1;
    else
        au32Config[0] |= 0x1;
    au32Config[1] = u32DFBA;

    if (FMC_WriteConfig(au32Config, 2) < 0)
        return -1;
        
    // Perform chip reset to make new User Config take effect
    SYS->IPRSTC1 = SYS_IPRSTC1_CHIP_RST_Msk;
    // This is to pacify compiler - it never reaches this point
    return 0;
}
