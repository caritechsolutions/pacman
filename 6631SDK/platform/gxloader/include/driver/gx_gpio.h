#ifndef __GX_GPIO_H__
#define __GX_OPIO_H__

enum {
	GX_GPIO_CONFIG_INVALID = 0,
	GX_GPIO_CONFIG_VALID
};

enum {
	GX_GPIO_INPUT = 0,
	GX_GPIO_OUTPUT
};

enum {
	GX_GPIO_LOW = 0,
	GX_GPIO_HIGH
};

// gx gpio v1 register
#define		GPIO_EPDDR			0x00
#define		GPIO_EPDR			0x04
#define		GPIO_EPBSET			0x08
#define		GPIO_EPBCLR			0x0c
#define		GPIO_EPFER			0x10
#define		GPIO_EPFR			0x14
#define		GPIO_EPPAR			0x18
#define		GPIO_EPINV			0x1c
#define		GPIO_EPODR			0x20
#define		GPIO_PDM_SEL			0x24
#define		GPIO_PDM_DACEN			0x80
#define		GPIO_PDM_CLKSEL			0x84
#define		GPIO_PDM_DACIN			0x88

// gx gpio v2 register
#define		GPIO_DIR			0x00
#define		GPIO_SET_OUT			0x04
#define		GPIO_SET_IN			0x08
#define		GPIO_DOUT			0x10
#define		GPIO_SET_HI			0x14
#define		GPIO_SET_LO			0x18
#define		GPIO_PIN_VAL			0x1c
#define		GPIO_PWMSEL00_07		0x20
#define		GPIO_PWMSEL08_15		0x24
#define		GPIO_PWMSEL16_23		0x28
#define		GPIO_PWMSEL24_31		0x2c
#define		GPIO_USE_PWM			0x30
#define		GPIO_DS_EN			0x34
#define		GPIO_DS_CNT			0x38
#define		GPIO_MASK			0x3c
#define		GPIO_INT_EN			0x40
#define		GPIO_INT_STATUS			0x44
#define		GPIO_INT_MASK			0x48
#define		GPIO_INT_HI_EN			0x50
#define		GPIO_INT_LO_EN			0x54
#define		GPIO_INT_RISE_EN		0x58
#define		GPIO_INT_FALL_EN		0x5c
#define		GPIO_PWM_DUTY0			0x80
#define		GPIO_PWM_DUTY1			0x84
#define		GPIO_PWM_DUTY2			0x88
#define		GPIO_PWM_DUTY3			0x8c
#define		GPIO_PWM_DUTY4			0x90
#define		GPIO_PWM_DUTY5			0x94
#define		GPIO_PWM_DUTY6			0x98
#define		GPIO_PWM_DUTY7			0x9c
#define		GPIO_PWM_CYC0			0xc0
#define		GPIO_PWM_CYC1			0xc4
#define		GPIO_PWM_CYC2			0xc8
#define		GPIO_PWM_CYC3			0xcc
#define		GPIO_PWM_CYC4			0xd0
#define		GPIO_PWM_CYC5			0xd4
#define		GPIO_PWM_CYC6			0xd8
#define		GPIO_PWM_CYC7			0xdc
#define		GPIO_PWM_PAUSE			0xe0
#define		GPIO_PWM_START			0xe4
#define		GPIO_PWM_PREDIV			0xf0

/* GPIO entry in bootloader */
struct gpio_entry_bootloader {
	unsigned char vir_gpio;		/* gpio virtual number */
	unsigned char phy_gpio;		/* gpio physical number */
	unsigned char config_valid:1;
	unsigned char io_mode:1;	/* 0: input, 1: output */
	unsigned char output_value:1;	/* 0: low, 1: high (only for output mode) */
	unsigned char reserved:5;
	unsigned char reserverd2;
};

#define GPIO_MAGIC       0x6770696f      // 'g' 'p' 'i' 'o'

/* GPIO table header in SRAM */
struct gpio_table_header {
	unsigned int   magic;
	unsigned char  valid;
	unsigned char  entry_num;
};

#endif

