#include <board-common.h>
#include <div64.h>
#include <interrupt.h>
#include "gx_gpio.h"

static struct gx_gpio_pri_info_s *p_info;

static int gx_gpio_convert(unsigned long vir_gpio, unsigned long *phy_gpio, unsigned int *reg_base, unsigned long *offs)
{
	if (vir_gpio >= GX_GPIO_TOTAL_NUM)
		return -1;

	if(vir_gpio == p_info->table[vir_gpio].vir_gpio) {

		if (!p_info->table[vir_gpio].valid)
			return -1;

		*phy_gpio = p_info->table[vir_gpio].phy_gpio;

	} else
		return -1;

	if (*phy_gpio < GX_GPIO_PHY_NUM) {
		*reg_base = p_info->reg_base[*phy_gpio / ONE_GROUP_GPIO_NUM];
		*offs = *phy_gpio % ONE_GROUP_GPIO_NUM;
	} else
		return -1;

	return 0;
}

/* setting gpio input/output mode, 0: input, 1: output */
int gx_gpio_setio(unsigned long gpio, unsigned long io)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;
	unsigned int tmp;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	p_info->table[gpio].io_mode = io;
#ifdef CONFIG_GPIO_GX_V1
	tmp = __raw_readl(reg_base + GPIO_EPDDR);
	if (io)
		tmp |= 1 << offs;
	else
		tmp &= ~(1 << offs);
	__raw_writel(tmp, reg_base + GPIO_EPDDR);
#elif defined(CONFIG_GPIO_GX_V2)
	if (io)
		__raw_writel(1 << offs, reg_base + GPIO_SET_OUT);
	else
		__raw_writel(1 << offs, reg_base + GPIO_SET_IN);
#endif

	return 0;
}

/* getting gpio input/output mode, returns 0: input, 1: output */
int gx_gpio_getio(unsigned long gpio)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

#ifdef CONFIG_GPIO_GX_V1
	return (__raw_readl(reg_base + GPIO_EPDDR) & (1 << offs)) ? 1 : 0;
#elif defined(CONFIG_GPIO_GX_V2)
	return (__raw_readl(reg_base + GPIO_DIR) & (1 << offs)) ? 1 : 0;
#endif
}

/* set the gpio high/low level for output pins, 0: low, 1: high */
int  gx_gpio_setlevel(unsigned long gpio, unsigned long value)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;

	if (value != 0 && value != 1)
		return -1;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	p_info->table[gpio].output_value= value;

#ifdef CONFIG_GPIO_GX_V1
	if (value)
		__raw_writel(1 << offs, reg_base + GPIO_EPBSET);
	else
		__raw_writel(1 << offs, reg_base + GPIO_EPBCLR);
#elif defined(CONFIG_GPIO_GX_V2)
	if (value)
		__raw_writel(1 << offs, reg_base + GPIO_SET_HI);
	else
		__raw_writel(1 << offs, reg_base + GPIO_SET_LO);
#endif

	return 0;
}

/* get the gpio level for input/output pins, returns 0: low, 1: high */
int  gx_gpio_getlevel(unsigned long gpio)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned long tmp;
	unsigned int reg_base;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	if (p_info->output_read_support) {
#ifdef CONFIG_GPIO_GX_V1
		return (__raw_readl(reg_base + GPIO_EPDR) & (1 << offs)) ? 1 : 0;
#elif defined(CONFIG_GPIO_GX_V2)
		return (__raw_readl(reg_base + GPIO_PIN_VAL) & (1 << offs)) ? 1 : 0;
#endif
	} else {
#ifdef CONFIG_GPIO_GX_V1
		tmp = __raw_readl(reg_base + GPIO_EPDDR);
		if (tmp & (1 << offs)) {
			return p_info->table[gpio].output_value;
		} else
			return (__raw_readl(reg_base + GPIO_EPDR) & (1 << offs)) ? 1 : 0;
#endif
	}
}

#ifdef CONFIG_GPIO_GX_V1
int gx_gpio_set_pdm(struct gx_gpio_pdm_arg *pdm)
{
	int ret = 0;
	unsigned long offs = 0;
	unsigned long phy_gpio = 0;
	unsigned int reg_base;
	unsigned int tmp;

	ret = gx_gpio_convert(pdm->gpio_num, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	if (pdm->pdm_enable) {
		tmp = __raw_readl(reg_base + GPIO_PDM_SEL);
		tmp |= 1 << offs;
		__raw_writel(tmp, reg_base + GPIO_PDM_SEL);

		pdm->div_clock = (pdm->div_clock > 7) ? 7 : pdm->div_clock;
		__raw_writel(pdm->div_clock, reg_base + GPIO_PDM_CLKSEL);

		pdm->duty_cycle = (pdm->duty_cycle > 100) ? 100 : pdm->duty_cycle;
		tmp = pdm->duty_cycle*0xffff/100;
		__raw_writel(tmp, reg_base + GPIO_PDM_DACIN);

		tmp = __raw_readl(reg_base + GPIO_PDM_DACEN);
		tmp |= (1<<0);
		tmp &= ~(1<<1);
		__raw_writel(tmp, reg_base + GPIO_PDM_DACEN);
	}

	else {
		tmp = __raw_readl(reg_base + GPIO_PDM_DACEN);
		tmp &= ~(3<<0);
		__raw_writel(tmp, reg_base + GPIO_PDM_DACEN);

		tmp = __raw_readl(reg_base + GPIO_PDM_SEL);
		tmp &= ~(1 << offs);
		__raw_writel(tmp, reg_base + GPIO_PDM_SEL);
	}
	return 0;
}

int gx_gpio_get_pdm(struct gx_gpio_pdm_arg *pdm)
{
	int ret = 0;
	unsigned long offs = 0;
	unsigned long phy_gpio = 0;
	unsigned int reg_base;
	unsigned int tmp;

	ret = gx_gpio_convert(pdm->gpio_num, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	tmp = __raw_readl(reg_base + GPIO_PDM_CLKSEL);
	pdm->div_clock = (tmp > 7) ? 7 : tmp;

	tmp = __raw_readl(reg_base + GPIO_PDM_DACIN);
	pdm->duty_cycle = tmp*100/0xffff;
	pdm->duty_cycle += (tmp*100%0xffff) ? 1 : 0;

	tmp = __raw_readl(reg_base + GPIO_PDM_DACEN);
	tmp = ((tmp & 0x3) == 0x1) ? 1 : 0;
	pdm->pdm_enable = __raw_readl(reg_base + GPIO_PDM_SEL) & (1 << offs) ? tmp : 0;

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GX_V2
struct gpio_pwm_channel {
	unsigned int period_ns;
	unsigned int duty_ns;
	unsigned int counter;
	enum pwm_polarity polarity;
};

struct gpio_pwm_info {
	struct gpio_pwm_channel pwm_channel[PWM_CHANNEL_NUM];
	unsigned int gpio_to_channel[ONE_GROUP_GPIO_NUM];
};

static struct gpio_pwm_info pwm_info[GPIO_GROUP_NUM];
static unsigned int pwm_pre_div = 1;

static unsigned int __gpio_get_clk(void)
{
#define NSEC_PER_SEC    1000000000L

#ifdef CONFIG_ARCH_CKMMU_CYGNUS
	unsigned int refdiv;
	unsigned int postdiv2;
	unsigned int postdiv1;
	unsigned int FBdiv;
	unsigned int xtal_clk;
	unsigned int CONFIG_PLL_DTO_BASE = 0xa030a0c0;
	unsigned int config_pll_dto_value = *(volatile unsigned int *)CONFIG_PLL_DTO_BASE;
	unsigned int CONFIG_PLL_CPU_BASE = 0xa030a0c8;
	unsigned int config_pll_cpu_value = *(volatile unsigned int *)CONFIG_PLL_CPU_BASE;
	unsigned int CONFIG_CLOCK_DIV_CONFIG2_BASE = 0xa030a178;
	unsigned int config_clock_div_config2_value = *(volatile unsigned int *)CONFIG_CLOCK_DIV_CONFIG2_BASE;
	unsigned char ahb_div;

	refdiv =   max(((config_pll_dto_value >> 20) & 0x3F), 1);
	postdiv2 = max(((config_pll_dto_value >> 16) & 0x7), 1);
	postdiv1 = max(((config_pll_dto_value >> 12) & 0x7), 1);
	FBdiv =    max(((config_pll_dto_value) & 0xFFF), 1);

	/* The pll dto frequency of cygnus is always 1188 MHz no matter the
	 * crystal oscillator is 24 MHz or 27 MHz */
	xtal_clk = 1188000000 / FBdiv * postdiv1 * postdiv2 * refdiv;

	refdiv =   max(((config_pll_cpu_value >> 20) & 0x3F), 1);
	postdiv2 = max(((config_pll_cpu_value >> 16) & 0x7), 1);
	postdiv1 = max(((config_pll_cpu_value >> 12) & 0x7), 1);
	FBdiv =    max(((config_pll_cpu_value) & 0xFFF), 1);

	if ((config_clock_div_config2_value & 0x8) == 0x8)
		ahb_div = 6;
	else if ((config_clock_div_config2_value & 0x4) == 0x4)
		ahb_div = 5;
	else if ((config_clock_div_config2_value & 0x2) == 0x2)
		ahb_div = 4;
	else if ((config_clock_div_config2_value & 0x1) == 0x1)
		ahb_div = 3;
	else
		ahb_div = 2;

	pwm_pre_div = 2; //for support output 2KHz pwm, please see issue #337554

	return (xtal_clk * FBdiv / postdiv1 / postdiv2 / refdiv) / ahb_div;
#elif defined(CONFIG_ARCH_ARM_SIRIUS)
	pwm_pre_div = 1;

	return 167714000; //167.714MHz
#elif defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI)
	pwm_pre_div = 1;

	return 118800000; //118.8MHz
#elif defined(CONFIG_ARCH_ARM_CANOPUS)
#include "cpu_config.h"
	unsigned int ahb0_div = ((*(volatile unsigned int *)(CONFIG_BASE_MMU+CONFIG_CLOCK_DIV_CONFIG2_SEC)) & 0x3F) + 1;
	pwm_pre_div = 1; //for support output 2KHz pwm, please see issue #337554
	/* The pll dto frequency of canopus is always 1188 MHz */

	return 1188000000 / ahb0_div;
#elif defined(CONFIG_ARCH_ARM_VEGA)
#include "cpu_config.h"
	unsigned int ahb0_div = ((*(volatile unsigned int *)(CONFIG_CLOCK_BASE+CONFIG_CLOCK_DIV_CONFIG2_SEC)) & 0x3F) + 1;
	pwm_pre_div = 1; //for support output 2KHz pwm, please see issue #337554
	/* The pll dto frequency of canopus is always 1188 MHz */

	return 1188000000 / ahb0_div;

#elif defined(CONFIG_ARCH_CKMMU_SCORPIO)
	pwm_pre_div = 1;

	return 118800000; //118.8MHz
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	pwm_pre_div = 1;

	return 118800000; //118.8MHz

#else
#error "you must define gpio clk for chip"
#endif

	return 0;
}

int gx_gpio_enable_pwm(unsigned long gpio, unsigned int period_ns, unsigned int duty_ns, enum pwm_polarity polarity)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;
	struct gpio_pwm_info *p_pwm_info = NULL;
	int i = 0;
	unsigned int gpio_clk = __gpio_get_clk();
	unsigned int period_ns_in_cycle = lldiv(lldiv(period_ns * (uint64_t)gpio_clk, pwm_pre_div), NSEC_PER_SEC);
	unsigned int duty_ns_in_cycle = lldiv(lldiv(duty_ns * (uint64_t)gpio_clk, pwm_pre_div), NSEC_PER_SEC);;
	unsigned int gpio_use_pwm = 0;

	printf("period_ns_in_cycle : %u\n", period_ns_in_cycle);
	printf("duty_ns_in_cycle : %u\n", duty_ns_in_cycle);
	if (period_ns_in_cycle > 0xFFFF || duty_ns_in_cycle > 0xFFFF) {
		printf("the cycle is too big\n");
		return -1;
	} else if (period_ns_in_cycle < duty_ns_in_cycle) {
		printf("duty_ns_in_cycle is bigger than period_ns_in_cycle\n");
		return -1;
	}

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	p_pwm_info = &pwm_info[phy_gpio / 32];

	if (p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] > 0)
		p_pwm_info->pwm_channel[p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] - 1].counter -= 1;

	for (i = 0; i < PWM_CHANNEL_NUM; i++) {
		if (p_pwm_info->pwm_channel[i].period_ns == period_ns &&
		    p_pwm_info->pwm_channel[i].duty_ns == duty_ns &&
		    p_pwm_info->pwm_channel[i].polarity == polarity) {
			unsigned int pwm_sel_val = 0;
			unsigned int pwm_sel_reg = reg_base + GPIO_PWMSEL00_07 + ((phy_gpio % ONE_GROUP_GPIO_NUM) / 8) * PWM_SEL_WIDTH;

			pwm_sel_val = __raw_readl(pwm_sel_reg);
			pwm_sel_val &= ~(0x7 << ((phy_gpio % 8) * PWM_SEL_WIDTH));
			pwm_sel_val |= (i & 0x7) << ((phy_gpio % 8) * PWM_SEL_WIDTH);
			__raw_writel(pwm_sel_val, pwm_sel_reg);

			p_pwm_info->pwm_channel[i].counter += 1;
			p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] = i + 1;
			break;
		}
	}

	if (i >= PWM_CHANNEL_NUM) {
		for (i = 0; i < PWM_CHANNEL_NUM; i++) {
			if (p_pwm_info->pwm_channel[i].counter == 0) {
				unsigned int pwm_sel_val = 0;
				unsigned int pwm_sel_reg = reg_base + GPIO_PWMSEL00_07 + ((phy_gpio % ONE_GROUP_GPIO_NUM) / 8) * PWM_SEL_WIDTH;
				unsigned int pwm_duty_reg = reg_base + GPIO_PWM_DUTY0 + i * 4;
				unsigned int pwm_cyc_reg = reg_base + GPIO_PWM_CYC0 + i * 4;

				pwm_sel_val = __raw_readl(pwm_sel_reg);
				pwm_sel_val &= ~(0x7 << ((phy_gpio % 8) * PWM_SEL_WIDTH));
				pwm_sel_val |= (i & 0x7) << ((phy_gpio % 8) * PWM_SEL_WIDTH);
				__raw_writel(pwm_sel_val, pwm_sel_reg);

				p_pwm_info->pwm_channel[i].period_ns = period_ns;
				p_pwm_info->pwm_channel[i].duty_ns = duty_ns;
				p_pwm_info->pwm_channel[i].polarity = polarity;
				p_pwm_info->pwm_channel[i].counter += 1;
				p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] = i + 1;

				if (polarity == PWM_POLARITY_NORMAL) {
					__raw_writel(duty_ns_in_cycle, pwm_duty_reg);
					__raw_writel(period_ns_in_cycle, pwm_cyc_reg);
				} else {
					__raw_writel((duty_ns_in_cycle << 16) | (period_ns_in_cycle), pwm_duty_reg);
					__raw_writel(period_ns_in_cycle, pwm_cyc_reg);
				}

				break;
			}
		}

		if (i == PWM_CHANNEL_NUM) {
			printf("No enough PWM channel\n");
			return -1;
		}
	}

	__raw_writel(pwm_pre_div - 1, reg_base + GPIO_PWM_PREDIV);
	__raw_writel(0xFF, reg_base + GPIO_PWM_START);
	__raw_writel(0x00, reg_base + GPIO_PWM_PAUSE);
	gpio_use_pwm = __raw_readl(reg_base + GPIO_USE_PWM);
	gpio_use_pwm |= 1 << (phy_gpio % ONE_GROUP_GPIO_NUM);
	__raw_writel(gpio_use_pwm, reg_base + GPIO_USE_PWM);


	return ret;
}

int gx_gpio_disable_pwm(unsigned long gpio)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;
	struct gpio_pwm_info *p_pwm_info = NULL;
	unsigned int gpio_use_pwm;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	p_pwm_info = &pwm_info[phy_gpio / 32];

	if (p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] > 0) {
		p_pwm_info->pwm_channel[p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] - 1].counter -= 1;
		p_pwm_info->gpio_to_channel[phy_gpio % ONE_GROUP_GPIO_NUM] = 0;

		gpio_use_pwm = __raw_readl(reg_base + GPIO_USE_PWM);
		gpio_use_pwm &= ~(1 << (phy_gpio % ONE_GROUP_GPIO_NUM));
		__raw_writel(gpio_use_pwm, reg_base + GPIO_USE_PWM);
	}

	return ret;
}

#ifdef CONFIG_ENABLE_IRQ
struct gpio_irq_info {
	unsigned int port;
	GPIO_CALLBACK callback;
	void *pdata;
};

struct gpio_trigger_group {
	unsigned int counter;
	unsigned int reg_base;
	int          gpio_isrvec;
	struct gpio_irq_info irq_info[ONE_GROUP_GPIO_NUM];
};

static struct gpio_trigger_group s_gpio_trigger_groups[GPIO_GROUP_NUM] = {{0}};

static enum interrupt_return gpio_interrupt_isr(uint32_t vector, void* pdata)
{
	struct gpio_trigger_group *trigger_group = (struct gpio_trigger_group *)pdata;

	if (trigger_group == NULL)
		goto exit;

	int i = 0;
	unsigned int reg_base = trigger_group->reg_base;
	unsigned int int_mask = 0;
	unsigned int int_status = 0;

	gx_mask_interrupt(vector); //mask irq
	for (i = 0; i < ONE_GROUP_GPIO_NUM; i++) {
		int_status = __raw_readl((volatile void *)(reg_base + GPIO_INT_STATUS));
		if (int_status & (1 << i)) {
			int_mask = __raw_readl((volatile void *)(reg_base + GPIO_INT_MASK));
			int_mask |= 1 << i;
			__raw_writel(int_mask, reg_base + GPIO_INT_MASK);
			__raw_writel(int_status & (1 << i), reg_base + GPIO_INT_STATUS);
			trigger_group->irq_info[i].callback(trigger_group->irq_info[i].port, trigger_group->irq_info[i].pdata);
			int_mask &= ~(1 << i);
			__raw_writel(int_mask, reg_base + GPIO_INT_MASK);
		}
	}

exit:
	gx_unmask_interrupt(vector);

	return HANDLED;
}

int gx_gpio_enable_trigger(unsigned long gpio, GPIO_TRIGGER_EDGE edge, GPIO_CALLBACK callback, void *pdata)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;
	unsigned int int_en;
	unsigned int int_mask;
	unsigned int int_hi_en;
	unsigned int int_lo_en;
	unsigned int int_rise_en;
	unsigned int int_fall_en;
	unsigned int chip_id;
	int isrvec[GPIO_GROUP_NUM];

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	if (callback == NULL) {
		printf("callback can not be NULL\n");
		return -1;
	}

	if (gx_gpio_setio(gpio, 0))
		return -1;

	struct gpio_trigger_group *trigger_group;

#if defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
	isrvec[0] = 76 - 32;
	isrvec[1] = 75 - 32;
	isrvec[2] = 74 - 32;
	isrvec[3] = 73 - 32;
#elif defined(CONFIG_ARCH_ARM_SIRIUS)
	isrvec[0] = 71 - 32;
	isrvec[1] = 70 - 32;
	isrvec[2] = 69 - 32;
#else
	isrvec[0] = 19;
	isrvec[1] = 20;
	isrvec[2] = 21;
#endif
	if (reg_base == p_info->reg_base[0]) {
		trigger_group = &s_gpio_trigger_groups[0];
		trigger_group->gpio_isrvec = isrvec[0];
	} else if (reg_base == p_info->reg_base[1]) {
		trigger_group = &s_gpio_trigger_groups[1];
		trigger_group->gpio_isrvec = isrvec[1];
	} else if (reg_base == p_info->reg_base[2]) {
		trigger_group = &s_gpio_trigger_groups[2];
		trigger_group->gpio_isrvec = isrvec[2];
#if GPIO_GROUP_NUM == 4
	} else if (reg_base == p_info->reg_base[3]) {
		trigger_group = &s_gpio_trigger_groups[3];
		trigger_group->gpio_isrvec = isrvec[3];
#endif
	}
	trigger_group->reg_base = reg_base;

	if (trigger_group->irq_info[phy_gpio % 32].callback == NULL) {
		trigger_group->irq_info[phy_gpio % 32].callback = callback;
		trigger_group->irq_info[phy_gpio % 32].pdata = pdata;
		trigger_group->irq_info[phy_gpio % 32].port = gpio;
		trigger_group->counter += 1;

		int_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_EN));
		int_en |= 1 << phy_gpio % 32;
		__raw_writel(int_en, reg_base + GPIO_INT_EN);

		if (edge & GPIO_TRIGGER_LEVEL_HIGH) {
			int_hi_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_HI_EN));
			int_hi_en |= 1 << phy_gpio % 32;
			__raw_writel(int_hi_en, reg_base + GPIO_INT_HI_EN);
		}
		else {
			int_hi_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_HI_EN));
			int_hi_en &= ~(1 << phy_gpio % 32);
			__raw_writel(int_hi_en, reg_base + GPIO_INT_HI_EN);
		}
		if (edge & GPIO_TRIGGER_LEVEL_LOW) {
			int_lo_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_LO_EN));
			int_lo_en |= 1 << phy_gpio % 32;
			__raw_writel(int_lo_en, reg_base + GPIO_INT_LO_EN);
		}
		else {
			int_lo_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_LO_EN));
			int_lo_en &= ~(1 << phy_gpio % 32);
			__raw_writel(int_lo_en, reg_base + GPIO_INT_LO_EN);
		}
		if (edge & GPIO_TRIGGER_EDGE_RISING) {
			int_rise_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_RISE_EN));
			int_rise_en |= 1 << phy_gpio % 32;
			__raw_writel(int_rise_en, reg_base + GPIO_INT_RISE_EN);
		}
		else {
			int_rise_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_RISE_EN));
			int_rise_en &= ~(1 << phy_gpio % 32);
			__raw_writel(int_rise_en, reg_base + GPIO_INT_RISE_EN);
		}
		if (edge & GPIO_TRIGGER_EDGE_FALLING) {
			int_fall_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_FALL_EN));
			int_fall_en |= 1 << phy_gpio % 32;
			__raw_writel(int_fall_en, reg_base + GPIO_INT_FALL_EN);
		}
		else {
			int_fall_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_FALL_EN));
			int_fall_en &= ~(1 << phy_gpio % 32);
			__raw_writel(int_fall_en, reg_base + GPIO_INT_FALL_EN);
		}

		int_mask = __raw_readl((volatile void *)(reg_base + GPIO_INT_MASK));
		int_mask &= ~(1 << phy_gpio % 32);
		__raw_writel(int_mask, reg_base + GPIO_INT_MASK);

		if (trigger_group->counter == 1) {
			gx_request_interrupt(trigger_group->gpio_isrvec, IRQ, gpio_interrupt_isr, trigger_group);
		}
		return 0;
	}
	return 0;
}

int gx_gpio_disable_trigger(unsigned long gpio)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base;
	unsigned int int_en;
	unsigned int int_mask;
	unsigned int int_status;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	struct gpio_trigger_group *trigger_group;
	if (reg_base == p_info->reg_base[0])
		trigger_group = &s_gpio_trigger_groups[0];
	else if (reg_base == p_info->reg_base[1])
		trigger_group = &s_gpio_trigger_groups[1];
	else if (reg_base == p_info->reg_base[2])
		trigger_group = &s_gpio_trigger_groups[2];
#if GPIO_GROUP_NUM == 4
	else if (reg_base == p_info->reg_base[3])
		trigger_group = &s_gpio_trigger_groups[3];
#endif

	if (trigger_group->irq_info[phy_gpio % 32].callback != NULL) {

		trigger_group->irq_info[phy_gpio % 32].callback = NULL;
		trigger_group->irq_info[phy_gpio % 32].pdata = NULL;
		trigger_group->irq_info[phy_gpio % 32].port = 254;   // impossible value
		trigger_group->counter -= 1;   // impossible value

		int_en = __raw_readl((volatile void *)(reg_base + GPIO_INT_EN));
		int_en &= ~(1 << phy_gpio % 32);
		__raw_writel(int_en, reg_base + GPIO_INT_EN);
		int_mask = __raw_readl((volatile void *)(reg_base + GPIO_INT_MASK));
		int_mask = 1 << phy_gpio % 32;
		__raw_writel(int_mask, reg_base + GPIO_INT_MASK);
		int_status = 0;
		int_status |= 1 << phy_gpio % 32;
		__raw_writel(int_status, reg_base + GPIO_INT_STATUS);

	} else {
		printf("This gpio haven't be registe!\n");
		return -1;
	}
	printf("trigger_group->counter = %d\n", trigger_group->counter);
	return 0;
}
#else
int gx_gpio_enable_trigger(unsigned long gpio, GPIO_TRIGGER_EDGE edge, GPIO_CALLBACK callback, void *pdata)
{
	printf("please enable irq to support gpio interrupt\n");
	return -1;
}

int gx_gpio_disable_trigger(unsigned long gpio)
{
	printf("please enable irq to support gpio interrupt\n");
	return -1;
}
#endif
#endif

#ifdef CONFIG_GPIO_GX_V1
int gx_gpio_enable_pwm(unsigned long gpio, unsigned int period_ns, unsigned int duty_ns, enum pwm_polarity polarity)
{
	printf("gpio v1 not support pwm\n");
	return -1;
}
int gx_gpio_disable_pwm(unsigned long gpio)
{
	printf("gpio v1 not support pwm\n");
	return -1;
}

int gx_gpio_enable_trigger(unsigned long gpio, GPIO_TRIGGER_EDGE edge, GPIO_CALLBACK callback, void *pdata)
{
	printf("gpio v1 not support irq\n");
	return -1;
}

int gx_gpio_disable_trigger(unsigned long gpio)
{
	printf("gpio v1 not support irq\n");
	return -1;
}
#endif

int gx_gpio_getphy(unsigned long vir_gpio, unsigned long *phy_gpio)
{
	unsigned long offs = 0;
	unsigned int reg_base;
	return gx_gpio_convert(vir_gpio, phy_gpio, &reg_base, &offs);
}

/* copy and construct gpio map from SRAM into SDRAM */
int gx_gpio_init(void)
{
	unsigned int i = 0;
	unsigned int gx_gpio_table_offset = GPIO_TABLE_START_ADDR;
	struct gpio_entry_bootloader *sram_entry;
	struct gpio_table_header *sram_gpio_table_header;

	if (p_info != NULL)
		return 0;

	p_info = (struct gx_gpio_pri_info_s *)malloc(sizeof(struct gx_gpio_pri_info_s));
	if (!p_info) {
		printf("gpio malloc failed!\n");
		goto failed;
	}

	p_info->reg_base[0] = REG_BASE_GPIO1;
	p_info->reg_base[1] = REG_BASE_GPIO2;
	p_info->reg_base[2] = REG_BASE_GPIO3;
#if GPIO_GROUP_NUM == 4
	p_info->reg_base[3] = REG_BASE_GPIO4;
#endif

#ifdef CONFIG_GPIO_GX_V1
	p_info->output_read_support = 0;
#elif defined(CONFIG_GPIO_GX_V2)
	p_info->output_read_support = 1;
#endif

	GPIO_PRINTF("\t register ioremap addr 0x%x \n", p_info->reg_base[0]);
	GPIO_PRINTF("\t register ioremap addr 0x%x \n", p_info->reg_base[1]);
	GPIO_PRINTF("\t register ioremap addr 0x%x \n", p_info->reg_base[2]);
#if GPIO_GROUP_NUM == 4
	GPIO_PRINTF("\t register ioremap addr 0x%x \n", p_info->reg_base[3]);
#endif

	sram_gpio_table_header = (struct gpio_table_header *)(gx_gpio_table_offset);
	sram_entry = (struct gpio_entry_bootloader *)(gx_gpio_table_offset + sizeof(struct gpio_table_header));

	/* initialize the vir & phy to make it equal, and default invalid */
	for (i = 0; i < GX_GPIO_TOTAL_NUM; i++) {
		p_info->table[i].valid       = 0;
		p_info->table[i].vir_gpio    = i;
		p_info->table[i].phy_gpio    = i;
		p_info->table[i].io_mode     = 0;
		p_info->table[i].output_value= 0;
	}

	/* if detected table, update from sram. if not detected, vir & phy is equal, and all valid */
	if ((GPIO_MAGIC == sram_gpio_table_header->magic) && (sram_gpio_table_header->valid)
			&& (sram_gpio_table_header->entry_num < GX_GPIO_TOTAL_NUM)) {
		for (i = 0; i < sram_gpio_table_header->entry_num; i++) {
			if ((sram_entry[i].phy_gpio >= GX_GPIO_PHY_NUM) || (sram_entry[i].vir_gpio >= GX_GPIO_TOTAL_NUM))
				goto failed;

			p_info->table[sram_entry[i].vir_gpio].valid       = sram_entry[i].config_valid;
			p_info->table[sram_entry[i].vir_gpio].vir_gpio    = sram_entry[i].vir_gpio;
			p_info->table[sram_entry[i].vir_gpio].phy_gpio    = sram_entry[i].phy_gpio;
			p_info->table[sram_entry[i].vir_gpio].io_mode     = sram_entry[i].io_mode;
			p_info->table[sram_entry[i].vir_gpio].output_value= sram_entry[i].output_value;
		}
		GPIO_PRINTF("\n");
		for (i = 0; i < sram_gpio_table_header->entry_num; i++)
			GPIO_PRINTF("[SRAM entry No. %d] vir_gpio:%d, phy_gpio:%d, io_mode:%d, output_value:%d\n",
					i, sram_entry[i].vir_gpio, sram_entry[i].phy_gpio, sram_entry[i].io_mode,\
					sram_entry[i].output_value);
		GPIO_PRINTF("\n");
	} else {
		for (i = 0; i < GX_GPIO_TOTAL_NUM; i++)
			p_info->table[i].valid = 0;
	}

	for (i = 0; i < GX_GPIO_TOTAL_NUM; i++)
		GPIO_PRINTF("[vir_gpio entry NO. %d] valid:%d, phy_gpio:%d, io_mode:%d, output_value:%d\n",
				p_info->table[i].vir_gpio, p_info->table[i].valid, p_info->table[i].phy_gpio,\
				p_info->table[i].io_mode, p_info->table[i].output_value);

	return 0;

failed:
	if (p_info) {
		free(p_info);
		p_info = NULL;
	}
	return -1;
}
