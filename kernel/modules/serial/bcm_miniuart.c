#include <config.h>
#include <stddef.h>
#include <stdint.h>
#include <util/stdio.h>

/**
 * miniUART Driver for Broadcom SoCs
 * 
 * Raspberry Pi Devices have multiple UART devices.
 * 
 * Model               | first PL011 (UART0)   | mini UART
 * --------------------|-----------------------|------------
 * Raspberry Pi Zero   | primary               | secondary
 * Raspberry Pi Zero W | secondary (Bluetooth) | primary
 * Raspberry Pi 1      | primary               | secondary
 * Raspberry Pi 2      | primary               | secondary
 * Raspberry Pi 3      | secondary (Bluetooth) | primary
 * Raspberry Pi 4      | secondary (Bluetooth) | primary
 * 
 * This driver will be the primary UART for Pi Zero W, Pi 3 and Pi 4
 **/
#if defined(PLAT_BCM2835) || defined(PLAT_BCM2836) || \
	defined(PLAT_BCM2837B0) || defined(PLAT_BCM2711)

// Memory-Mapped I/O output
static inline void mmio_write32(size_t reg, uint32_t data){
    *(volatile uint32_t*)reg = data;
}
 
// Memory-Mapped I/O input
static inline uint32_t mmio_read32(size_t reg){
	return *(volatile uint32_t*)reg;
}

#if defined(PLAT_BCM2835)
  // RPi0w
  #define BCM_PERIPH_BASE     (0x20000000)
#elif defined(PLAT_BCM2836)
  // RPi2
  #define BCM_PERIPH_BASE     (0x3F000000)
#elif defined(PLAT_BCM2837B0)
  // Pi 3B+ and 3A+
  #define BCM_PERIPH_BASE     (0x3F000000)
#elif defined(PLAT_BCM2711)
  // RPi4
  #define BCM_PERIPH_BASE     (0xFE000000) 
#endif

// GPIO Registers
#define BCM_GPIO_BASE       (BCM_PERIPH_BASE + 0x200000)
#define GPFSEL1             (BCM_GPIO_BASE + 0x04)
#define GPSET0              (BCM_GPIO_BASE + 0x1C)
#define GPCLR0              (BCM_GPIO_BASE + 0x28)
#define GPPUD               (BCM_GPIO_BASE + 0x94)
#define GPPUDCLK0           (BCM_GPIO_BASE + 0x98)

// Auxiliary Peripherals (includes miniUART) Registers
#define BCM_AUX_BASE        (BCM_PERIPH_BASE + 0x215000)

#define AUX_ENABLES         (BCM_AUX_BASE + 0x04)
#define AUX_MU_IO_REG       (BCM_AUX_BASE + 0x40)
#define AUX_MU_IER_REG      (BCM_AUX_BASE + 0x44)
#define AUX_MU_IIR_REG      (BCM_AUX_BASE + 0x48)
#define AUX_MU_LCR_REG      (BCM_AUX_BASE + 0x4C)
#define AUX_MU_MCR_REG      (BCM_AUX_BASE + 0x50)
#define AUX_MU_LSR_REG      (BCM_AUX_BASE + 0x54)
#define AUX_MU_MSR_REG      (BCM_AUX_BASE + 0x58)
#define AUX_MU_SCRATCH      (BCM_AUX_BASE + 0x5C)
#define AUX_MU_CNTL_REG     (BCM_AUX_BASE + 0x60)
#define AUX_MU_STAT_REG     (BCM_AUX_BASE + 0x64)
#define AUX_MU_BAUD_REG     (BCM_AUX_BASE + 0x68)

#define MBOX_BASE           (BCM_PERIPH_BASE + 0xB880)
#define MBOX_READ           (MBOX_BASE + 0x00)
#define MBOX_STATUS         (MBOX_BASE + 0x18)
#define MBOX_WRITE          (MBOX_BASE + 0x20)

// A Mailbox message with set clock rate of PL011 to 3MHz tag
static volatile unsigned int  __attribute__((aligned(16))) mbox[9] = {
    9*4, 0, 0x38002, 12, 8, 2, 3000000, 0 ,0
};

static void putc(int c) {
    while(1){
        if(mmio_read32(AUX_MU_LSR_REG)&0x20) break;
    }
    mmio_write32(AUX_MU_IO_REG, c);
}
 
static void puts(char* str) {
    for (size_t i = 0; str[i] != '\0'; i ++)
        putc((int)str[i]);
}

static stdio_ops_t ops = {
    .putc = putc,
    .puts = puts
};

static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(count): [count]"0"(count) : "cc");
}

void bcm_miniuart_init(){
    unsigned int ra;
    mmio_write32(AUX_ENABLES,1);
    mmio_write32(AUX_MU_IER_REG,0);
    mmio_write32(AUX_MU_CNTL_REG,0);
    mmio_write32(AUX_MU_LCR_REG,3);
    mmio_write32(AUX_MU_MCR_REG,0);
    mmio_write32(AUX_MU_IIR_REG,0xC6);
    mmio_write32(AUX_MU_BAUD_REG,270);

    ra=mmio_read32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    mmio_write32(GPFSEL1,ra);

    mmio_write32(GPPUD,0);
    delay(150);
    mmio_write32(GPPUDCLK0, (1<<14)|(1<<15));
    delay(150);
    mmio_write32(GPPUDCLK0,0);

#ifdef PLAT_BCM2711
		// System clock freq = VPU Clock = 500MHz
    // baud = (system_clock_freq)/(8(baud_reg+1))
    mmio_write32(AUX_MU_BAUD_REG,541); //115200
#endif

    mmio_write32(AUX_MU_CNTL_REG,2);
    
    RegisterStdioInterface(STDIO_DEFAULT, &ops);
    RegisterStdioInterface(STDIO_KERN_INFO, &ops);
    RegisterStdioInterface(STDIO_KERN_WARN, &ops);
    RegisterStdioInterface(STDIO_KERN_ERR, &ops);
}

#endif
