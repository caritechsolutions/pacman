#include "ctr.h"
#include "gxhwlib_registers.h"
#include "io.h"
#include "interrupt.h"
#include "gx_api.h"

#define CTR_INIT_VALUE	(0xFFFFFFFF - 1000)
#define MAX_REG_NUM     10

struct gx_ctr_callback{
	int (*ctr_callback_fun)(void*);
	int interval_time;
	void *private_data;
};

struct gx_reg_info{
	struct gx_ctr_callback reg_ctr_callback;
	int reg_interval_time;
	int reg_repeat_times;
	int used;
};

static int ctr_interrupt_isr(int irq, void *pdata);
static struct gx_reg_info reg_info[MAX_REG_NUM];

int gx_ctr_init(void)
{
	__raw_writel(0x1, COUNTER_1_CONTROL);
	__raw_writel(0x0, COUNTER_1_CONTROL);
	__raw_writel(0x3, COUNTER_1_CONFIG);
	__raw_writel(26, COUNTER_1_PRE);
	__raw_writel(CTR_INIT_VALUE, COUNTER_1_INI);
	__raw_writel(0x2, COUNTER_1_CONTROL);

	memset(reg_info, 0, sizeof(reg_info));
#ifdef CONFIG_ENABLE_CTR_CALLBACK
#ifdef CONFIG_ARCH_ARM_SIRIUS
	gx_request_interrupt(50, IRQ, ctr_interrupt_isr, NULL);
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
	gx_request_interrupt(60, IRQ, ctr_interrupt_isr, NULL);
#else
	gx_request_interrupt(10, IRQ, ctr_interrupt_isr, NULL);
#endif
#endif
	return 0 ;
}

int gx_ctr_disable(void)
{
	__raw_writel(0x0, COUNTER_1_CONFIG);
	return 0 ;
}

#ifdef CONFIG_ENABLE_CTR_CALLBACK
int gx_ctr_callback_register(int (*fun)(void*), int interval_time_ms, int repeat_times, void *private_data)
{
	int i = 0;

	for (i = 0; i < MAX_REG_NUM; i++){
		if (0 == reg_info[i].used){
			reg_info[i].reg_ctr_callback.ctr_callback_fun = fun;
			reg_info[i].reg_ctr_callback.interval_time = interval_time_ms;
			reg_info[i].reg_ctr_callback.private_data = private_data;
			reg_info[i].reg_interval_time = interval_time_ms;
			reg_info[i].reg_repeat_times = repeat_times;
			reg_info[i].used = 1;
			return 0;
		}
	}
	if (i == MAX_REG_NUM){
		printf("%s:register callback fun falied!\n", __func__);
		return -1;
	}

	return 0;
}

static int ctr_interrupt_isr(int irq, void *pdata)
{
	int reg = 0;
	int i = 0;

	for (i = 0; i < MAX_REG_NUM; i++){
		if (0 == reg_info[i].reg_repeat_times)
			continue;
		reg_info[i].reg_interval_time--;
		if (0 == reg_info[i].reg_interval_time){
			(*(reg_info[i].reg_ctr_callback.ctr_callback_fun))(reg_info[i].reg_ctr_callback.private_data);
			reg_info[i].reg_interval_time = reg_info[i].reg_ctr_callback.interval_time;

			if (reg_info[i].reg_repeat_times > 0){
				reg_info[i].reg_repeat_times--;
				if (0 == reg_info[i].reg_repeat_times)
					memset(&reg_info[i], 0, sizeof(reg_info[i]));
			}
		}
	}

	reg = __raw_readl(COUNTER_1_STATUS);
	reg |= 0x1;
	__raw_writel(reg, COUNTER_1_STATUS);

	return HANDLED;
}
#endif

