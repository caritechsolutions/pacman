/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gxcomm.c
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   The common functions in lowpower
*****************************************************************************/
/* Includes --------------------------------------------------------------- */
#include "gxcomm.h"

/* Private types/constants ------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */
#if 0
#define irr_debug GXLOWPOWER_TinyPutStr
#else
#define irr_debug(pstr)
#endif


/* Private Variables ------------------------------------------------------ */

/* Export Variables ------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */


/*****************************************************************************
 * Function    : GXLOWPOWER_TinyPutStr
 * Description : Put the string by UART
 * Arguments   : [in]  *pStr the address of the string which want to be sended
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GXLOWPOWER_TinyPutStr(char *pStr)
{
#if DEBUG_EN > 0
    while('\0' != (*pStr))
    {
        if('\n' == (*pStr))
        {
            while(!(rUART_SCISR & (1 << 7)));
            rUART_SCIDR = '\r';
        }

        while(!(rUART_SCISR & (1 << 7)));
        rUART_SCIDR = (U32) *pStr ++;
    }
#endif
}
/*****************************************************************************
 * Function    : GXLOWPOWER_TinyPutNum
 * Description : Put the number by UART
 * Arguments   : [in]  Num   the address of the number which want to be sended
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GXLOWPOWER_TinyPutNum(S32 Num)
{
#if DEBUG_EN > 0
    U8  NumLen;
    U8  NumStr[10];
    U32 NumTemp;
    S8  Index;
    if(0 == Num)
    {
        NumStr[0] = '0';
        NumStr[1] = 'x';
        NumStr[2] = '0';
        NumLen = 3;
    }
    else
    {
        if(0 > Num)
        {
            NumTemp = 0xFFFFFFFF - (-Num - 1);
        }
        else
        {
            NumTemp = Num;
        }
        NumLen = 0;
        while(0 != NumTemp)
        {
            //NumStr[NumLen] = NumTemp % 16;//
            NumStr[NumLen] = (U8)(NumTemp & 0x0F);
            if(10 <= NumStr[NumLen])
            {
                NumStr[NumLen] = 'A' + NumStr[NumLen] - 10;
            }
            else
            {
                NumStr[NumLen] = '0' + NumStr[NumLen];
            }
            //NumTemp = NumTemp / 16;//
            NumTemp = (NumTemp >> 4);
            NumLen++;
        }
        NumStr[NumLen] = 'x';
        NumStr[NumLen+1] = '0';
        NumLen += 2;
    }
    for(Index = NumLen-1;Index >= 0;Index--)
    {
        while(!(rUART_SCISR & (1 << 7)));
        rUART_SCIDR = (U32)(NumStr[Index]);
    }
#endif
}

/*****************************************************************************************
 * interrupt / fast interrupt control
 *****************************************************************************************/
#define IRQ_CHANNEL_NUM  (INTC_IRQ_NSOURCE_END - INTC_IRQ_NSOURCE03_00 + 4)
#define NR_IRQS          IRQ_CHANNEL_NUM
static unsigned int irq_num[NR_IRQS] = {0};
static unsigned int irq_channel[NR_IRQS] = {0};

static void request_irq(int irq)
{
	int channel;
	unsigned int value;
	unsigned int base_addr = REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE03_00;
	int i, j;

	// get free channel
	for (channel = 0; channel < IRQ_CHANNEL_NUM; channel++)
		if (irq_channel[channel] == 0xff)
			break;

	if (channel >= IRQ_CHANNEL_NUM) {
		GXLOWPOWER_TinyPutStr("\n irq channel num error!\n");
		return;
	}

	// join channel and irq
	irq_channel[channel] = irq;
	irq_num[irq] = channel;

	// write intc register
	i = channel / 4;
	j = channel % 4;
	value = __raw_readl(base_addr + i * 4);
	value &= ~(0xff << (j * 8));
	value |= (irq << (j * 8));
	__raw_writel(value, base_addr + i * 4);

}

void gx_irq_init(void)
{
	int i;

	for (i = 0; i < IRQ_CHANNEL_NUM; i++)
		irq_channel[i] = 0xff;
	for (i = 0; i < IRQ_CHANNEL_NUM / 4; i++)
		__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE03_00 + i * 4);

	request_irq(10); /* COUNTER1 */

	/* enable all interrupts */
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_irq_ENABLEINT);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_irq_ENABLEINT_HI);
	/* mask all interrupts */
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_irq_MASK);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_irq_MASK_HI);
}

void gx_disable_all_interrupt(void)
{
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_irq_MASK);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_irq_MASK_HI);

	__raw_writel(0x0, REG_INTC_BASE_NO_MMU + INTC_irq_ENABLEINT);
	__raw_writel(0x0, REG_INTC_BASE_NO_MMU + INTC_irq_ENABLEINT_HI);

	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE03_00);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE07_04);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE11_08);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE15_12);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE19_16);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE23_20);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE27_24);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE31_28);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE35_32);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE39_36);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE43_40);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE47_44);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE51_48);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE55_52);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE59_56);
	__raw_writel(0xFFFFFFFF, REG_INTC_BASE_NO_MMU + INTC_IRQ_NSOURCE63_60);
}

void gx_mask_irq(unsigned int irq)
{
	unsigned int reg = 0;
	unsigned int channel = irq_num[irq];

	if (channel >= 32) {
		reg = __raw_readl(REG_INTC_BASE_NO_MMU+INTC_irq_MASK_HI) | 1<<(channel-32);
		__raw_writel(reg, REG_INTC_BASE_NO_MMU+INTC_irq_MASK_HI);
	}
	else {
		reg = __raw_readl(REG_INTC_BASE_NO_MMU+INTC_irq_MASK) | 1<<channel;
		__raw_writel(reg, REG_INTC_BASE_NO_MMU+INTC_irq_MASK);
	}
}

void gx_unmask_irq(unsigned int irq)
{
	unsigned int reg = 0;
	unsigned int channel = irq_num[irq];

	if (channel >= 32) {
		reg = __raw_readl(REG_INTC_BASE_NO_MMU+INTC_irq_MASK_HI) &~ (1<<(channel-32));
		__raw_writel(reg, REG_INTC_BASE_NO_MMU+INTC_irq_MASK_HI);
	}
	else {
		reg = __raw_readl(REG_INTC_BASE_NO_MMU+INTC_irq_MASK) &~ (1<<channel);
		__raw_writel(reg, REG_INTC_BASE_NO_MMU+INTC_irq_MASK);
	}
}

//
// interrupt handler
//
static struct {
	irq_handler_t handler;
	void *pdata;
} s_irq_info[NR_IRQS];

void gx_request_irq(int irq, irq_handler_t handler, void *pdata)
{
	s_irq_info[irq_num[irq]].handler = handler;
	s_irq_info[irq_num[irq]].pdata = pdata;
	gx_unmask_irq(irq);
}

void gx_free_irq(int irq)
{
	gx_mask_irq(irq);
	s_irq_info[irq_num[irq]].handler = 0;
	s_irq_info[irq_num[irq]].pdata = 0;
}

void gx_irq_handler(void)
{
	int i = 0;
	unsigned int mask = 1;
	unsigned int status = 0;
	unsigned int irq = 0;

	status = *(volatile unsigned int*)(REG_INTC_BASE_NO_MMU + INTC_irq_STATUS);

	for (i=0; status; ++i, mask<<=1) {
		if (status & mask) {
			status &= ~mask;
			irq = irq_channel[i];
			gx_mask_irq(irq);	/* If no handler, left it masked */
			if (s_irq_info[i].handler)
			{
				s_irq_info[i].handler(irq, s_irq_info[i].pdata);
				gx_unmask_irq(irq);
			}
		}
	}

	/* check status II for 64 INTs */
	if (NR_IRQS<=32)
		return;

	status = *(volatile unsigned int*)(REG_INTC_BASE_NO_MMU + INTC_irq_STATUS_HI);

	mask = 1;
	for (i=32; status; ++i, mask<<=1) {
		if (status & mask) {
			status &= ~mask;
			irq = irq_channel[i];
			gx_mask_irq(irq);	/* If no handler, left it masked */
			if (s_irq_info[i].handler)
			{
				s_irq_info[i].handler(irq, s_irq_info[i].pdata);
				gx_unmask_irq(irq);
			}
		}
	}
}

/*****************************************************************************************
 * counter
 *****************************************************************************************/
#define CTR_INIT_VALUE	(0xFFFFFFFF - 1000)
#define MAX_REG_NUM     2

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
	int i = 0;

	__raw_writel(0x1, rCOUNTER1_STATUS);
	__raw_writel(0x1, rCOUNTER1_CTRL);
	__raw_writel(0x0, rCOUNTER1_CTRL);
	__raw_writel(0x3, rCOUNTER1_CFIG);
	__raw_writel(26, rCOUNTER1_DIV);
	__raw_writel(CTR_INIT_VALUE, rCOUNTER1_INI);
	__raw_writel(0x2, rCOUNTER1_CTRL);

	for (i = 0; i < MAX_REG_NUM; i++) {
		reg_info[i].used = 0;
		reg_info[i].reg_repeat_times = 0;
		reg_info[i].reg_interval_time = 0;
		reg_info[i].reg_ctr_callback.interval_time = 0;
		reg_info[i].reg_ctr_callback.private_data = NULL;
		reg_info[i].reg_ctr_callback.ctr_callback_fun = NULL;
	}
	gx_request_irq(10, ctr_interrupt_isr, NULL);

	return 0 ;
}

int gx_ctr_disable(void)
{
	__raw_writel(0x0, rCOUNTER1_STATUS);
	return 0 ;
}

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
		GXLOWPOWER_TinyPutStr("\nregister callback fun falied!\n");
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
				if (0 == reg_info[i].reg_repeat_times) {
					reg_info[i].used = 0;
					reg_info[i].reg_repeat_times = 0;
					reg_info[i].reg_interval_time = 0;
					reg_info[i].reg_ctr_callback.interval_time = 0;
					reg_info[i].reg_ctr_callback.private_data = NULL;
					reg_info[i].reg_ctr_callback.ctr_callback_fun = NULL;

				}
			}
		}
	}

	reg = __raw_readl(rCOUNTER1_STATUS);
	reg |= 0x1;
	__raw_writel(reg, rCOUNTER1_STATUS);

	return 0;
}

/*****************************************************************************************
 * irr
 *****************************************************************************************/
static int irr_parse_nec(struct gx_irr_info* info)
{
	unsigned int i, sig_time, mark_time;
	unsigned int num = info->pulse_num;

	mark_time = (info->pulse_val[0] & 0xFFFF);

	if (((num > IRR_MAX_SIMCODE_PULSE_NUM) &&
				(num < IRR_MAX_FULLCODE_PULSE_NUM)) ||
			(num > IRR_MAX_FULLCODE_PULSE_NUM) || (num == 0))
	{
		irr_debug("<warn>Receive interfere code.\n");
		return -1;
	}

	if ((mark_time < (7 * IRR_PROTCOL_TIME))
			|| (mark_time > (20 * IRR_PROTCOL_TIME)))
	{
		irr_debug("<warn>Receive interfere code,mark time error.\n");
		return -1;
	}

	/* deal with simcode */
	if (num <= IRR_MAX_SIMCODE_PULSE_NUM)
	{
		//info->counter_simple++;
		irr_debug("counter_simple\n");
		return -1;
	}
	/* decode */
	info->key_code = 0;
	for (i = 1; i < IRR_MAX_FULLCODE_PULSE_NUM; i++)
	{
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);
		info->key_code <<= 1;
		if ((sig_time) < (IRR_AVERAGE_PULSE_WIDTH))
			info->key_code |= 1;
	}
	info->protocol = IRR_PROTOCOL_NEC;

	return 0;
}

static int irr_parse_philips(struct gx_irr_info* info)
{
	unsigned int i;
	unsigned int sig_time;
	unsigned int mark_time;
	unsigned int low_time;
	unsigned int index = 0;
	unsigned int num = info->pulse_num;
	unsigned int key_bit   = 0;
	unsigned int key_value = 0;
	if ( (num < IRR_MIN_PULSE_NUM_PHILIPS)
			|| (num > IRR_MAX_PULSE_NUM_PHILIPS))
	{
		irr_debug("<warn>Receive interfere code.\n");
		return -1;
	}

	/* decode */
	info->key_code = 0;
	index++; 		//first, we assume bit 0 = 0 for inverse
	for (i = 0; i < num; i++) {

		sig_time  = (info->pulse_val[i] >> 16);
		mark_time = (info->pulse_val[i] & 0xFFFF);
		low_time  = sig_time - mark_time;

		//irr_debug("line(%d) mark_time[%d]:0x%x sig_time[%d]:0x%x\n",__LINE__, i, mark_time, i, sig_time);

		if ((2 * mark_time) > (IRR_RC5_PROTCOL_TIME * 3)){

			key_bit |= (1 << index++);
			key_bit |= (1 << index++);
		}
		else{
			key_bit |= (1 << index++);
		}

		if ((2 * low_time) > (IRR_RC5_PROTCOL_TIME * 3)) {

			index++;
			index++;
		}
		else{
			index++;
		}
	}
	switch(index){

		case 25: //110
			key_bit |= (1 << index++);
			key_bit |= (1 << index++);
			index++;

			break;
		case 26: //10
			key_bit |= (1 << index++);
			index++;

			break;
		case 27: //1
			key_bit |= (1 << index++);

			break;
		default: //index == 27,0-27 28bit

			break;
	}

	/* in order to discard error keycode.*/
	for(i = 0; i < 28; i += 2){
		key_value <<= 1;
		if(0x0 == (unsigned char)((key_bit >> i) & 0x01)
		 && 0x1 == (unsigned char)((key_bit >> (i+1)) & 0x01))
		{
			key_value |= 1;
		}else if(0x01 == (unsigned char)((key_bit >> i) & 0x01)
        		&& 0x00 == (unsigned char)((key_bit >> (i+1)) & 0x01))
		{
			continue;
		}else
		{
			//irr_debug("<warn>Receive interfere code.\n");
			return -1;
		}
	}

	info->key_code   = key_value;

	if ((info->key_code & 0x2000) == 0) {
		//irr_debug("<warn>Receive interfere code.\n");
		return -1;
	}

	info->key_code &= 0x7ff; //get last 11 bits
	info->protocol = IRR_PROTOCOL_PHILIPS;

	return 0;
}

static int irr_parse_dvb_40bit(struct gx_irr_info* info)
{
	unsigned int mark_time;
	unsigned int sig_time;
	unsigned int index;
	unsigned int key_code = 0;

	//不处理引导码，默认是合法数据
    	//32 bit key code
	key_code = 0;
	for (index = 16; index >8; index--)
	{
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40))
		{
			key_code |= 1;
		}
	}

	for (index = 24; index >16; index--)
	{
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40)){
			key_code |= 1;
		}
	}

	for (index = 33; index >25; index--)
	{
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40)){
			key_code |= 1;
		}
	}

	for (index = 41; index >33; index--)
	{
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40)){
			key_code |= 1;
		}
	}

	info->key_code = key_code;
	info->protocol = IRR_PROTOCOL_40BIT;

	return 0;
}

static int irr_parse_bescon(struct gx_irr_info* info)
{
	unsigned int mark_time;
	unsigned int sig_time;
	unsigned int index;
	unsigned int key_code = 0;

	U64 bit_str;
	unsigned int sig_sub_mark;
	unsigned int pulse_index;

	//D16-D14 :3位引导码
	//D13    :奇偶校验位，用来判断是否是简码，如取反则为全码，反之为简码
	//D12-D8 :5位系统码，恒为0
	//D7 -D0 :8位键码
	key_code = 0;
	bit_str = 0;
	pulse_index = 0;
	index = 0;

	//不处理第1个引导码
	for(pulse_index = 1; pulse_index < info->pulse_num; pulse_index++)
	{
		mark_time = (info->pulse_val[pulse_index] & 0xFFFF);
		sig_time = (info->pulse_val[pulse_index] >> 16);

		sig_sub_mark = sig_time - mark_time;

		if (2 * mark_time > 3 * IRR_PROTCOL_TIME_BESCON)
		{
			bit_str |= (1 << index);
			index++;
			bit_str |= (1 << index);
			index++;
		}
		else{
			bit_str |= (1 << index);
			index++;
		}

		if(2 * sig_sub_mark > 3 * IRR_PROTCOL_TIME_BESCON){

			index += 2;
		}
		else{
			index++;
		}
	}

	//因GX芯片截取红外脉冲的特征，可能会没记录到最后的一些位的数据
	//需要进行修正
	if((2 * IRR_MAX_PULSE_NUM_BESCON) - index == 1)
	{
		bit_str |= (1 << (index));
	}
	else if((2 * IRR_MAX_PULSE_NUM_BESCON) - index == 2)
	{
		bit_str |= (1 << (index));
	}
	else if((2 * IRR_MAX_PULSE_NUM_BESCON) - index == 3)
	{
		bit_str |= (1 << (index));
		bit_str |= (1 << (index + 1));
	}

	//计算key_code
	for(index = 0; index<(2*IRR_MAX_PULSE_NUM_BESCON); index += 2)
	{
		key_code <<= 1;
		if(0x00 == (unsigned char)((bit_str >> index) & 0x01)
				&&0x01 == (unsigned char)((bit_str >> (index+1)) & 0x01))
		{
			key_code |= 1;
		}
		else if(0x01 == (unsigned char)((bit_str >> index) & 0x01)
				&&0x00 == (unsigned char)((bit_str >> (index+1)) & 0x01))
		{
			continue;
		}
		else
		{
			key_code = 0xFFFFFFFF;
			return -1;
		}
	}

	info->key_code = key_code;
	info->protocol = IRR_PROTOCOL_BESCON;

	return 0;
}

static int irr_parse_panasonic(struct gx_irr_info* info)
{
	unsigned int i, sig_time, mark_time;
	unsigned char data0 = 0, data1 = 0;

	if (info->pulse_num < 49)
		return -1;

	// 16 bit key code
	data0 = 0;
	for (i = 33; i < 41; i++)
	{
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);

		if(sig_time > (IRR_PROTCOL_TIME_PANASONIC * 3))
		{
			data0 |= (1<<(i-33));
		}
	}

	data1 = 0;
	for (i = 41; i < 49; i++)
	{
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);

		if(sig_time > (IRR_PROTCOL_TIME_PANASONIC * 3))
			data1 |= (1<<(i-41));
	}
	info->key_code = (data0 <<24)|(data1<<8);
	info->protocol = IRR_PROTOCOL_PANASONIC;

	return 0;
}
static int irr_get_fifo_data(struct gx_irr_info* info)
{
	unsigned int i;
	info->pulse_num = (rIRR_INT >> 3) & 0x3F;;
	if (info->pulse_num == 0)
		return -1;

	//irr_debug("Receive data num:%d\n", info->pulse_num);

	if(info->pulse_num > IRR_MAX_PULSE_NUM )
		info->pulse_num = IRR_MAX_PULSE_NUM;

	for(i = 0; i < info->pulse_num; i++)
		info->pulse_val[i] = rIRR_FIFO;

	return 0;
}
int gxlowpower_irrGetKey(struct gx_irr_info* info)
{
	unsigned int pulse_num;
	unsigned int mark_time = 0;
	if((rIRR_INT & 0x02))
	{
		rIRR_INT = (1 << 1);
		irr_get_fifo_data(info);
		pulse_num = info->pulse_num;
		//根据脉冲的数目自适应不同的标准
		if ((IRR_MIN_PULSE_NUM_PHILIPS <= pulse_num
					&& IRR_MAX_PULSE_NUM_PHILIPS >= pulse_num)
				|| (IRR_MIN_PULSE_NUM_BESCON <= pulse_num
					&& IRR_MAX_PULSE_NUM_BESCON >= pulse_num))
		{
			mark_time = (info->pulse_val[0] & 0xFFFF);
			//bescon的起始位高电平恒为420us
			if (2 * mark_time < IRR_PROTCOL_TIME_BESCON * 3
					&& (2 * mark_time > IRR_PROTCOL_TIME_BESCON * 1)) // bescon标准
				return irr_parse_bescon(info);
			else // philips标准
				return irr_parse_philips(info);
		}
		else if (IRR_MAX_SIMCODE_PULSE_NUM >= pulse_num
				|| IRR_MAX_FULLCODE_PULSE_NUM <= pulse_num) {
			if(pulse_num >=49) //松下标准
				return irr_parse_panasonic(info);
			else if(pulse_num >=42)
				return irr_parse_dvb_40bit(info);
			else if(pulse_num >= 33){ //NEC标准
				if(pulse_num > 33){
					unsigned int i = 0;
					for(i = 33; i < pulse_num; i++){ //simple_pulse range (11000,12000)
						if(((info->pulse_val[i] >> 16) > 12000) || ((info->pulse_val[i]) >> 16) < 11000)
							return -1;
					}
					info->pulse_num = 33;
				}
				return irr_parse_nec(info);
			}
			else if(pulse_num <= IRR_MAX_SIMCODE_PULSE_NUM)
				;
			else //protocol not suported
				return -1;
		}
		else
			return -1;
	}
	return -1;
}
