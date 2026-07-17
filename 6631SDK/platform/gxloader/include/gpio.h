#ifndef __GPIO_H__
#define __GPIO_H__

struct gx_gpio_pdm_arg {
	unsigned int gpio_num;
	unsigned int pdm_enable;
	unsigned int div_clock;
	unsigned int duty_cycle;
};

// GPIO trigger function
typedef enum {
	GPIO_TRIGGER_EDGE_FALLING   = 0x01,
	GPIO_TRIGGER_EDGE_RISING    = 0x02,
	GPIO_TRIGGER_LEVEL_HIGH     = 0x04,
	GPIO_TRIGGER_LEVEL_LOW      = 0x08,
} GPIO_TRIGGER_EDGE;

typedef int (*GPIO_CALLBACK)(int gpio, void *pdata);

/**
 * enum pwm_polarity - polarity of a PWM signal
 * @PWM_POLARITY_NORMAL: a high signal for the duration of the duty-
 * cycle, followed by a low signal for the remainder of the pulse
 * period
 * @PWM_POLARITY_INVERSED: a low signal for the duration of the duty-
 * cycle, followed by a high signal for the remainder of the pulse
 * period
 */
enum pwm_polarity {
	PWM_POLARITY_NORMAL,
	PWM_POLARITY_INVERSED,
};

int gx_gpio_init(void);
int gx_gpio_setio(unsigned long gpio, unsigned long io);
int gx_gpio_getio(unsigned long gpio);
int gx_gpio_getlevel(unsigned long gpio);
int gx_gpio_setlevel(unsigned long gpio, unsigned long value);
int gx_gpio_getphy(unsigned long vir_gpio, unsigned long *phy_gpio);
int gx_gpio_set_pdm(struct gx_gpio_pdm_arg *pdm);
int gx_gpio_get_pdm(struct gx_gpio_pdm_arg *pdm);
int gx_gpio_enable_trigger(unsigned long gpio, GPIO_TRIGGER_EDGE edge, GPIO_CALLBACK callback, void *pdata);
int gx_gpio_disable_trigger(unsigned long gpio);
int gx_gpio_enable_pwm(unsigned long gpio, unsigned int period_ns, unsigned int duty_ns, enum pwm_polarity polarity);
int gx_gpio_disable_pwm(unsigned long gpio);

#endif

