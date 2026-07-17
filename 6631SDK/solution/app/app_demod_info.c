#ifdef ECOS_OS
#include "app_windows.h"
#include "app.h"
#include "module/app_ioctl.h"

#if DEMOD_INFO_SUPPORT
#define TEXT_DEMOD_TYPE          "text_demod_info_type"
#define TEXT_DEMOD_INFO1         "text_demod_info1"
#define TEXT_DEMOD_INFO2         "text_demod_info2"
#define TEXT_DEMOD_INFO3         "text_demod_info3"
#define TEXT_DEMOD_INFO4         "text_demod_info4"
#define TEXT_DEMOD_INFO5         "text_demod_info5"
#define TEXT_DEMOD_INFO6         "text_demod_info6"
#define TEXT_DEMOD_INFO7         "text_demod_info7"
#define TEXT_DEMOD_INFO8         "text_demod_info8"
#define TEXT_DEMOD_INFO9         "text_demod_info9"
#define TEXT_DEMOD_INFO10        "text_demod_info10"
#define TEXT_DEMOD_INFO11        "text_demod_info11"
#define TEXT_DEMOD_INFO12        "text_demod_info12"
#define TEXT_DEMOD_INFO13        "text_demod_info13"
#define TEXT_DEMOD_INFO14        "text_demod_info14"
#define TEXT_DEMOD_INFO15        "text_demod_info15"
#define TEXT_DEMOD_INFO16        "text_demod_info16"
#define TEXT_DEMOD_INFO17        "text_demod_info17"
#define TEXT_DEMOD_INFO18        "text_demod_info18"
#define TEXT_DEMOD_INFO19        "text_demod_info19"
#define TEXT_DEMOD_INFO20        "text_demod_info20"
#define TEXT_DEMOD_INFO21        "text_demod_info21"
#define TEXT_DEMOD_INFO22        "text_demod_info22"
#define TEXT_DEMOD_INFO23        "text_demod_info23"

#define TEXT_DEMOD_INFO1_VALUE   "text_demod_info1_value"
#define TEXT_DEMOD_INFO2_VALUE   "text_demod_info2_value"
#define TEXT_DEMOD_INFO3_VALUE   "text_demod_info3_value"
#define TEXT_DEMOD_INFO4_VALUE   "text_demod_info4_value"
#define TEXT_DEMOD_INFO5_VALUE   "text_demod_info5_value"
#define TEXT_DEMOD_INFO6_VALUE   "text_demod_info6_value"
#define TEXT_DEMOD_INFO7_VALUE   "text_demod_info7_value"
#define TEXT_DEMOD_INFO8_VALUE   "text_demod_info8_value"
#define TEXT_DEMOD_INFO9_VALUE   "text_demod_info9_value"
#define TEXT_DEMOD_INFO10_VALUE  "text_demod_info10_value"
#define TEXT_DEMOD_INFO11_VALUE  "text_demod_info11_value"
#define TEXT_DEMOD_INFO12_VALUE  "text_demod_info12_value"
#define TEXT_DEMOD_INFO13_VALUE  "text_demod_info13_value"
#define TEXT_DEMOD_INFO14_VALUE  "text_demod_info14_value"
#define TEXT_DEMOD_INFO15_VALUE  "text_demod_info15_value"
#define TEXT_DEMOD_INFO16_VALUE  "text_demod_info16_value"
#define TEXT_DEMOD_INFO17_VALUE  "text_demod_info17_value"
#define TEXT_DEMOD_INFO18_VALUE  "text_demod_info18_value"
#define TEXT_DEMOD_INFO19_VALUE  "text_demod_info19_value"
#define TEXT_DEMOD_INFO20_VALUE  "text_demod_info20_value"
#define TEXT_DEMOD_INFO21_VALUE  "text_demod_info21_value"
#define TEXT_DEMOD_INFO22_VALUE  "text_demod_info22_value"
#define TEXT_DEMOD_INFO23_VALUE  "text_demod_info23_value"

static event_list *s_demod_info_timer = NULL;
static fe_type_t demod_type;

static char *s_T2Mode[2] = {"T2_LITE","T2_BASE"};
static char *s_PreambleFormat[8] = {"T2_SISO","T2_MISO","NON_T2","T2_LITE_SISO","T2_LITE_MISO","Reserved","Reserved","Reserved"};
static char *s_FftSizeLite[8] = {"2K","8K","4K","16K","16K","Reserved","8K","Reserved"};
static char *s_FftSizeBase[8] = {"2K","8K","4K","1K","16K","32K","8K","32K"};
static char *s_GIMode[8] = {"1/32","1/16","1/8","1/4","1/128","19/128","19/256","Reserved"};
static char *s_PPMode[8] = {"PP1","PP2","PP3","PP4","PP5","PP6","PP7","PP8"};
static char *s_PLPMode[4] = {"QPSK","16QAM","64QAM","256QAM"};
static char *s_PLPCodeLite[8] = {"1/2","3/5","2/3","3/4","Reserved","Reserved","1/3","2/5"};
static char *s_PLPCodeBase[8] = {"1/2","3/5","2/3","3/4","4/5","5/6","Reserved","Reserved"};
static char *s_PLPFecTypeLite[4] = {"16K LDPC","Reserved","Reserved","Reserved"};
static char *s_PLPFecTypeBase[4] = {"16K LDPC","64K LDPC","Reserved","Reserved"};
static char *s_CCI[2] = {"Not Exist","Exist"};
static char *s_ATV[2] = {"Not Exist","Exist"};
static char *s_TimeIlMode[2] = {"NTI", "PI"};
static char *s_PLPType[3] = {"Common","Type1","Type2"};

static char *s_DvbtFftSize[4] = {"2K","8K","4K","Reserved"};
static char *s_DvbtConstellation[4] = {"QPSK","16QAM","64QAM","Reserved"};
static char *s_DvbtHPCode[8] = {"1/2","2/3","3/4","5/6","7/8","Reserved","Reserved","Reserved"};
static char *s_DvbtLPCode[8] = {"1/2","2/3","3/4","5/6","7/8","Reserved","Reserved","Reserved"};
static char *s_DvbtGIMode[4] = {"1/32","1/16","1/8","1/4"};

static uint32_t _100log(int32_t iNumber_N)
{
	int32_t iLeftMoveCount_M = 0;
	int32_t iChangeN_Y = 0;
	int32_t iBuMaY_X = 0;
	int32_t iReturn_value = 0;
	int32_t iTemp = 0, iResult = 0, k = 0;

	if(iNumber_N <= 0)
		return 0;
	iChangeN_Y = iNumber_N;
	for (iLeftMoveCount_M=0; iLeftMoveCount_M<16; iLeftMoveCount_M++)
	{
		if (0x8000 == (iChangeN_Y & 0x8000))
		{
			break;
		}
		else
		{
			iChangeN_Y = iNumber_N << iLeftMoveCount_M;
		}
	}
	iBuMaY_X = 0x10000 - iChangeN_Y;
	k = iBuMaY_X * 10000 / 65536;
	iTemp = k + (k*k)/20000 + ((k*k/10000)*(k*33/100))/10000 + ((k*k/100000)*(k*k/100000))/400;
	iResult = 48165 - (iTemp * 10000 / 23025);
	k = iResult - 3010 * (iLeftMoveCount_M - 1);
	iReturn_value = k;
	return iReturn_value;
}

static uint32_t _t2_get_error_rate(uint32_t *E_param)
{
	uint32_t temp;
	uint8_t error_num_l, error_num_h;
	uint32_t i,avage_num ;
	uint32_t error_num, error_count,divied;
	uint32_t e_value = 0;
	uint32_t return_value = 0;

	error_count = 0 ;
	avage_num = 1 ;

	for(i=0;i<avage_num;i++)
	{
		error_num_l = *(volatile unsigned int*)(0xa4e00000+(0xf585<<2));
		error_num_h = (*(volatile unsigned int*)(0xa4e00000+(0xf595<<2)))&0xc0;
		error_num = error_num_l + (error_num_h<<8);
		error_count = error_count + error_num ;
	}
	divied = 4096*128;
	for(i=0;i<7;i++)
	{
		temp = (int)(error_count/divied);
		if (temp)
		{
			return_value = (uint32_t)(error_count/(divied/100));
			break;
		}
		else
		{
			e_value ++;
			error_count *=10;
		}
	}
	*E_param = e_value;
	return return_value;
}

static uint32_t _t_get_error_rate(uint32_t *E_param)
{
	uint8_t flag = 0;
	uint32_t e_value = 0;
	uint32_t return_value = 0;
	uint32_t temp = 0;
	uint8_t Read_Value[4];
	uint32_t Error_count = 0;
	uint8_t i = 0;
	uint32_t divied = ((1<<(2*5+5))*(204)*8);

	*E_param = 0;
#if defined(GEMINI)
	*(volatile unsigned int*)(0xa4e00000+(0xf3c0<<2)) |= (0x31);
	for (i=0; i<4; i++)
	{
		flag = *(volatile unsigned int*)(0xa4e00000+((0xf3c1+i)<<2));
		Read_Value[i] = flag;
	}
	Error_count = (uint32_t)(Read_Value[0]+ (Read_Value[1]<<8) + (Read_Value[2]<<16) + ((Read_Value[3]&0x03)<<24));
	*(volatile unsigned int*)(0xa4e00000+(0xf3c0<<2)) &= (0xfe);
#elif defined(CYGNUS)
	*(volatile unsigned int*)(0xa4e00000+(0xf3e5<<2)) |= (0x31);
	for (i=0; i<4; i++)
	{
		flag = *(volatile unsigned int*)(0xa4e00000+((0xf3e6+i)<<2));
		Read_Value[i] = flag;
	}
	Error_count = (uint32_t)(Read_Value[0]+ (Read_Value[1]<<8) + (Read_Value[2]<<16) + ((Read_Value[3]&0x03)<<24));
	*(volatile unsigned int*)(0xa4e00000+(0xf3e5<<2)) &= (0xfe);
#endif
	for (i=0; i<20; i++)
	{
		temp = Error_count/divied;
		if (temp)
		{
			return_value = Error_count/(divied/100);
			break;
		}
		else
		{
			e_value ++;
			Error_count *=10;
		}
	}
	*E_param = e_value;
	return return_value;
}

static int app_demod_info_show_t2_info(void)
{
	GUI_SetProperty(TEXT_DEMOD_TYPE, "string", "DVBT2");

	GUI_SetProperty(TEXT_DEMOD_INFO1, "string", "T2 Mode:");
	GUI_SetProperty(TEXT_DEMOD_INFO2, "string", "Preamble Format:");
	GUI_SetProperty(TEXT_DEMOD_INFO3, "string", "FFT Size:");
	GUI_SetProperty(TEXT_DEMOD_INFO4, "string", "GI Mode:");
	GUI_SetProperty(TEXT_DEMOD_INFO5, "string", "PP Mode:");
	GUI_SetProperty(TEXT_DEMOD_INFO6, "string", "PLP Type:");
	GUI_SetProperty(TEXT_DEMOD_INFO7, "string", "PLP Mode:");
	GUI_SetProperty(TEXT_DEMOD_INFO8, "string", "PLP Code:");
	GUI_SetProperty(TEXT_DEMOD_INFO9, "string", "PLP Fec Type:");
	GUI_SetProperty(TEXT_DEMOD_INFO10, "string", "TIME IL Mode:");
	GUI_SetProperty(TEXT_DEMOD_INFO11, "string", "OFDM:");
	GUI_SetProperty(TEXT_DEMOD_INFO12, "string", "PLP Num Blocks:");
	GUI_SetProperty(TEXT_DEMOD_INFO13, "string", "PLP Num:");
	GUI_SetProperty(TEXT_DEMOD_INFO14, "string", "CCI:");
	GUI_SetProperty(TEXT_DEMOD_INFO15, "string", "ATV:");
	GUI_SetProperty(TEXT_DEMOD_INFO16, "string", "Lock Status:");
	GUI_SetProperty(TEXT_DEMOD_INFO17, "string", "SNR_BER:");
	GUI_SetProperty(TEXT_DEMOD_INFO18, "string", "OutSideGI Status:");
	GUI_SetProperty(TEXT_DEMOD_INFO19, "string", "Iter Cnt:");
	GUI_SetProperty(TEXT_DEMOD_INFO20, "string", "Dfreq Over Write:");
	GUI_SetProperty(TEXT_DEMOD_INFO21, "string", "Impulse Flag:");
	GUI_SetProperty(TEXT_DEMOD_INFO22, "string", "Echo Delay:");
	GUI_SetProperty(TEXT_DEMOD_INFO23, "string", "Echo Ahd:");

	GUI_SetProperty(TEXT_DEMOD_INFO1, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO2, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO3, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO4, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO5, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO6, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO7, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO8, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO9, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO10, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO11, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO12, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO13, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO14, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO15, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO16, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO17, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO18, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO19, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO20, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO21, "state", "show");
#ifdef CYGNUS
	GUI_SetProperty(TEXT_DEMOD_INFO22, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO23, "state", "show");
#endif

	return 0;
}

static int app_demod_info_show_t2_info_value(void)
{
	uint8_t t2_mode = 0;
	uint8_t preamble_format = 0;
	uint8_t fft_size = 0;
	uint8_t bw_ext = 0;
	uint8_t gi_mode = 0;
	uint8_t pp_mode = 0;
	uint8_t plp_mode = 0;
	uint8_t plp_code = 0;
	uint8_t plp_fec_type = 0;
	uint8_t cci = 0;
	uint8_t atv = 0;
	uint8_t ofdm_l = 0;
	uint8_t ofdm_h = 0;
	uint32_t ofdm = 0;
	uint8_t time_il_length = 0;
	uint8_t time_il_type = 0;
	uint8_t plp_num = 0;
	uint8_t plp_type = 0;
	uint8_t num_blocks_l = 0;
	uint8_t num_blocks_h = 0;
	uint32_t num_blocks = 0;
	uint8_t miso_mode = 0;
	char buffer[30] = {0};

	t2_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf4ec<<2)))&0x01;
	GUI_SetProperty(TEXT_DEMOD_INFO1_VALUE, "string", s_T2Mode[t2_mode]);

	preamble_format = (*(volatile unsigned int*)(0xa4e00000+(0xf5a1<<2))>>4)&0x07;
	miso_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf545<<2)))&0x07;
	memset(buffer, 0, sizeof(buffer));
	if((preamble_format == 1) || (preamble_format == 4))
		sprintf((char*)buffer, "%s %d", s_PreambleFormat[preamble_format], miso_mode);
	else
		sprintf((char*)buffer, "%s", s_PreambleFormat[preamble_format]);
	GUI_SetProperty(TEXT_DEMOD_INFO2_VALUE, "string", buffer);

	gi_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf5a2<<2))>>4)&0x07;
	GUI_SetProperty(TEXT_DEMOD_INFO4_VALUE, "string", s_GIMode[gi_mode]);

	pp_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf5b4<<2))>>4)&0x0f;
	GUI_SetProperty(TEXT_DEMOD_INFO5_VALUE, "string", s_PPMode[pp_mode]);

	plp_type = (*(volatile unsigned int*)(0xa4e00000+(0xf5d4<<2))>>4)&0x07;
	GUI_SetProperty(TEXT_DEMOD_INFO6_VALUE, "string", s_PLPType[plp_type]);

	plp_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf5d8<<2))>>2)&0x07;
	GUI_SetProperty(TEXT_DEMOD_INFO7_VALUE, "string", s_PLPMode[plp_mode]);

	bw_ext = (*(volatile unsigned int*)(0xa4e00000+(0xf5a1<<2))>>7)&0x01;
	fft_size = (*(volatile unsigned int*)(0xa4e00000+(0xf5a1<<2))>>1)&0x07;
	plp_code = (*(volatile unsigned int*)(0xa4e00000+(0xf5d8<<2))>>5)&0x07;
	plp_fec_type = (*(volatile unsigned int*)(0xa4e00000+(0xf5d8<<2)))&0x03;

	if(t2_mode == 0)
	{
		memset(buffer, 0, sizeof(buffer));
		if(bw_ext == 1)
			snprintf(buffer, sizeof(buffer), "%s%s", s_FftSizeLite[fft_size], "ext");
		else
			snprintf(buffer, sizeof(buffer), "%s", s_FftSizeLite[fft_size]);
		GUI_SetProperty(TEXT_DEMOD_INFO3_VALUE, "string", buffer);

		GUI_SetProperty(TEXT_DEMOD_INFO8_VALUE, "string", s_PLPCodeLite[plp_code]);

		GUI_SetProperty(TEXT_DEMOD_INFO9_VALUE, "string", s_PLPFecTypeLite[plp_fec_type]);
	}
	else if(t2_mode == 1)
	{
		memset(buffer, 0, sizeof(buffer));
		if(bw_ext == 1)
			snprintf(buffer, sizeof(buffer), "%s%s", s_FftSizeBase[fft_size], "ext");
		else
			snprintf(buffer, sizeof(buffer), "%s", s_FftSizeBase[fft_size]);
		GUI_SetProperty(TEXT_DEMOD_INFO3_VALUE, "string", buffer);

		GUI_SetProperty(TEXT_DEMOD_INFO8_VALUE, "string", s_PLPCodeBase[plp_code]);

		GUI_SetProperty(TEXT_DEMOD_INFO9_VALUE, "string", s_PLPFecTypeBase[plp_fec_type]);
	}

	time_il_length = *(volatile unsigned int*)(0xa4e00000+(0xf5dc<<2));
	time_il_type =  (*(volatile unsigned int*)(0xa4e00000+(0xf5d6<<2))>>7)&0x01;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%s_%d", s_TimeIlMode[time_il_type], time_il_length);
	GUI_SetProperty(TEXT_DEMOD_INFO10_VALUE, "string", buffer);

	ofdm_l = *(volatile unsigned int*)(0xa4e00000+(0xf5b2<<2));
	ofdm_h = *(volatile unsigned int*)(0xa4e00000+(0xf5b3<<2));
	ofdm = ofdm_l|((ofdm_h&0x0f)<<8);
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", ofdm);
	GUI_SetProperty(TEXT_DEMOD_INFO11_VALUE, "string", buffer);

	num_blocks_l = *(volatile unsigned int*)(0xa4e00000+(0xf5d9<<2));
	num_blocks_h = (*(volatile unsigned int*)(0xa4e00000+(0xf5da<<2)))&0x03;
	num_blocks = num_blocks_l + num_blocks_h*256;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", num_blocks);
	GUI_SetProperty(TEXT_DEMOD_INFO12_VALUE, "string", buffer);

	plp_num = *(volatile unsigned int*)(0xa4e00000+(0xf5c2<<2));
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", plp_num);
	GUI_SetProperty(TEXT_DEMOD_INFO13_VALUE, "string", buffer);

	cci = (*(volatile unsigned int*)(0xa4e00000+(0xf547<<2))>>7)&0x01;
	GUI_SetProperty(TEXT_DEMOD_INFO14_VALUE, "string", s_CCI[cci]);

	atv = (*(volatile unsigned int*)(0xa4e00000+(0xf545<<2))>>4)&0x01;
	GUI_SetProperty(TEXT_DEMOD_INFO15_VALUE, "string", s_ATV[atv]);

	GUI_SetProperty(TEXT_DEMOD_INFO1_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO2_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO3_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO4_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO5_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO6_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO7_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO8_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO9_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO10_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO11_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO12_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO13_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO14_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO15_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO16_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO17_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO18_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO19_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO20_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO21_VALUE, "state", "show");
#ifdef CYGNUS
	GUI_SetProperty(TEXT_DEMOD_INFO22_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO23_VALUE, "state", "show");
#endif

	return 0;
}

static int app_demod_info_timer_show_t2_info_value(void)
{
	uint8_t snr_l = 0;
	uint8_t snr_m = 0;
	uint8_t snr_h = 0;
	uint32_t n_pow = 0;
	uint32_t snr100 = 0;
	uint32_t e_value = 0;
	uint32_t ber_value = 0;
	uint8_t lock_status = 0;
	uint8_t iter_cnt = 0;
	uint8_t impulse_flag = 0;
	uint8_t dfreq_over_write = 0;
	uint8_t outside_gi_status = 0;
	uint32_t delay_us = 0;
	uint8_t echo_ahd_nbhd = 0;
	uint8_t value1 = 0;
	uint8_t value2 = 0;
	uint32_t value = 0;
	uint8_t fft_mode = 0;
	char buffer[30] = {0};

	lock_status = *(volatile unsigned int*)(0xa4e00000+(0xf40b<<2));
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "0x%x", lock_status);
	GUI_SetProperty(TEXT_DEMOD_INFO16_VALUE, "string", buffer);

	snr_l = *(volatile unsigned int*)(0xa4e00000+(0xf525<<2));
	snr_m = *(volatile unsigned int*)(0xa4e00000+(0xf526<<2));
	snr_h = *(volatile unsigned int*)(0xa4e00000+(0xf527<<2));
	n_pow = snr_l|(snr_m<<8)|((snr_h&0x07)<<16);
	if(n_pow > 0)
		snr100 = _100log(n_pow/32)/10;
	else
		snr100 = 0;

	ber_value = _t2_get_error_rate(&e_value);
	memset(buffer, 0, sizeof(buffer));
	snprintf((char*)buffer, sizeof(buffer), "%d.%d  %d.%02dE-%02d", snr100/100,snr100%100, ber_value/100, ber_value%100, e_value);
	GUI_SetProperty(TEXT_DEMOD_INFO17_VALUE, "string", buffer);

#if defined(GEMINI)
	outside_gi_status = (*(volatile unsigned int*)(0xa4e00000+(0xf53a<<2))>>7)&0x01;
#elif defined(CYGNUS)
	outside_gi_status = (*(volatile unsigned int*)(0xa4e00000+(0xf541<<2))>>5)&0x01;
#endif
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", outside_gi_status);
	GUI_SetProperty(TEXT_DEMOD_INFO18_VALUE, "string", buffer);

	iter_cnt = (*(volatile unsigned int*)(0xa4e00000+(0xf583<<2)))&0x3f;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", iter_cnt);
	GUI_SetProperty(TEXT_DEMOD_INFO19_VALUE, "string", buffer);

	dfreq_over_write = (*(volatile unsigned int*)(0xa4e00000+(0xf5b8<<2))>>6)&0x03;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", dfreq_over_write);
	GUI_SetProperty(TEXT_DEMOD_INFO20_VALUE, "string", buffer);

	impulse_flag = (*(volatile unsigned int*)(0xa4e00000+(0xf545<<2))>>3)&0x01;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", impulse_flag);
	GUI_SetProperty(TEXT_DEMOD_INFO21_VALUE, "string", buffer);

#ifdef CYGNUS
	value1 = *(volatile unsigned int*)(0xa4e00000+(0xf536<<2));
	value2 = *(volatile unsigned int*)(0xa4e00000+(0xf540<<2));
	value = ((value1&0xe0)>>5) + (value2<<3);
	fft_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf496<<2))>>1)&0x07;
	if((fft_mode == 7) || (fft_mode == 5)) //32k
		delay_us =32768*7/64/6*value/1024; // 1LSB=us
	else if(fft_mode == 4) //16k
		delay_us = 16384*7/64/3*value/1024;
	else if((fft_mode == 6) || (fft_mode == 1)) //8k
		delay_us = 8192*7/64/3*value/1024;
	else if(fft_mode == 2) //4k
		delay_us = 4096*7/64/3*value/1024;
	else if(fft_mode == 0) //2k
		delay_us = 2048*7/64/3*value/256;
	else if(fft_mode == 3) //1k
		delay_us = 1024*7/64/3*value/256;

	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%dus", delay_us);
	GUI_SetProperty(TEXT_DEMOD_INFO22_VALUE, "string", buffer);

	echo_ahd_nbhd = (*(volatile unsigned int*)(0xa4e00000+(0xf541<<2))>>4)&0x01;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d", echo_ahd_nbhd);
	GUI_SetProperty(TEXT_DEMOD_INFO23_VALUE, "string", buffer);
#endif

	return 0;
}

static int app_demod_info_show_t_info(void)
{
	GUI_SetProperty(TEXT_DEMOD_TYPE, "string", "DVBT");

	GUI_SetProperty(TEXT_DEMOD_INFO1, "string", "FFT Size:");
	GUI_SetProperty(TEXT_DEMOD_INFO2, "string", "Constellation:");
	GUI_SetProperty(TEXT_DEMOD_INFO3, "string", "HP Code:");
	GUI_SetProperty(TEXT_DEMOD_INFO4, "string", "LP Code:");
	GUI_SetProperty(TEXT_DEMOD_INFO5, "string", "GI Mode:");
	GUI_SetProperty(TEXT_DEMOD_INFO6, "string", "SNR:");
	GUI_SetProperty(TEXT_DEMOD_INFO7, "string", "BER:");
	GUI_SetProperty(TEXT_DEMOD_INFO8, "string", "Lock Status:");
	GUI_SetProperty(TEXT_DEMOD_INFO9, "string", "Echo Delay:");

	GUI_SetProperty(TEXT_DEMOD_INFO1, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO2, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO3, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO4, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO5, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO6, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO7, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO8, "state", "show");
#ifdef CYGNUS
	GUI_SetProperty(TEXT_DEMOD_INFO9, "state", "show");
#endif
	return 0;
}

static int app_demod_info_show_t_info_value(void)
{
	uint8_t fft_size = 0;
	uint8_t constellation = 0;
	uint8_t hp_code = 0;
	uint8_t hierarchical = 0;
	uint8_t lp_code = 0;
	uint8_t gi_mode = 0;

	fft_size = (*(volatile unsigned int*)(0xa4e00000+(0xf320<<2))>>6)&0x03;
	GUI_SetProperty(TEXT_DEMOD_INFO1_VALUE, "string", s_DvbtFftSize[fft_size]);

	constellation = (*(volatile unsigned int*)(0xa4e00000+(0xf320<<2))>>4)&0x03;
	GUI_SetProperty(TEXT_DEMOD_INFO2_VALUE, "string", s_DvbtConstellation[constellation]);

	hp_code = (*(volatile unsigned int*)(0xa4e00000+(0xf321<<2))>>4)&0x07;
	GUI_SetProperty(TEXT_DEMOD_INFO3_VALUE, "string", s_DvbtHPCode[hp_code]);

	hierarchical = (*(volatile unsigned int*)(0xa4e00000+(0xf320<<2)))&0x03;
	if(hierarchical == 0)
	{
		GUI_SetProperty(TEXT_DEMOD_INFO4_VALUE, "string", "No Use");
	}
	else
	{
		lp_code = (*(volatile unsigned int*)(0xa4e00000+(0xf321<<2))>>1)&0x07;
		GUI_SetProperty(TEXT_DEMOD_INFO4_VALUE, "string", s_DvbtLPCode[lp_code]);
	}

	gi_mode = (*(volatile unsigned int*)(0xa4e00000+(0xf320<<2))>>2)&0x03;
	GUI_SetProperty(TEXT_DEMOD_INFO5_VALUE, "string", s_DvbtGIMode[gi_mode]);

	GUI_SetProperty(TEXT_DEMOD_INFO1_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO2_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO3_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO4_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO5_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO6_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO7_VALUE, "state", "show");
	GUI_SetProperty(TEXT_DEMOD_INFO8_VALUE, "state", "show");
#ifdef CYGNUS
	GUI_SetProperty(TEXT_DEMOD_INFO9_VALUE, "state", "show");
#endif
	return 0;
}

static int app_demod_info_timer_show_t_info_value(void)
{
	uint8_t snr_l = 0;
	uint8_t snr_m = 0;
	uint8_t snr_h = 0;
	uint32_t n_pow = 0;
	uint32_t snr100 = 0;
	uint32_t e_value = 0;
	uint32_t ber_value = 0;
	uint8_t lock_status = 0;
	uint8_t value1 = 0;
	uint8_t value2 = 0;
	uint8_t value3 = 0;
	uint32_t value = 0;
	uint32_t delay_us = 0;
	char buffer[20] = {0};

#if defined(GEMINI)
	snr_l = *(volatile unsigned int*)(0xa4e00000+(0xf345<<2));
	snr_h = *(volatile unsigned int*)(0xa4e00000+(0xf346<<2));
	n_pow = snr_l|(snr_h<<8);
	if(n_pow > 0)
		snr100 = _100log(32768/n_pow)/10;
	else
		snr100 = 0;
#elif defined(CYGNUS)
	snr_l = *(volatile unsigned int*)(0xa4e00000+(0xf3b5<<2));
	snr_m = *(volatile unsigned int*)(0xa4e00000+(0xf3b6<<2));
	snr_h = *(volatile unsigned int*)(0xa4e00000+(0xf3b7<<2));
	n_pow = snr_l|(snr_m<<8)|((snr_h && 0x07)<<16);
	if(n_pow > 0)
		snr100 = _100log(n_pow/32)/10;
	else
		snr100 = 0;
#endif
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%d.%d", snr100/100,snr100%100);
	GUI_SetProperty(TEXT_DEMOD_INFO6_VALUE, "string", buffer);

	ber_value = _t_get_error_rate(&e_value);
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%d.%02dE-%02d", ber_value/100, ber_value%100, e_value);
	GUI_SetProperty(TEXT_DEMOD_INFO7_VALUE, "string", buffer);

	lock_status = *(volatile unsigned int*)(0xa4e00000+(0xf333<<2));
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "0x%x", lock_status);
	GUI_SetProperty(TEXT_DEMOD_INFO8_VALUE, "string", buffer);

#ifdef CYGNUS
	value1 = *(volatile unsigned int*)(0xa4e00000+(0xf3a2<<2));
	value2 = *(volatile unsigned int*)(0xa4e00000+(0xf3a6<<2));
	value3 = *(volatile unsigned int*)(0xa4e00000+(0xf3a5<<2));
	value = ((value1 & 0x20)<<6) + ((value2&0x07)<<8) + value3;
	delay_us = 8192*7/64/3*value/2048*3/4;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char*)buffer, "%dus", delay_us);
	GUI_SetProperty(TEXT_DEMOD_INFO9_VALUE, "string", buffer);
#endif
	return 0;
}

static int timer_show_demod_info(void* userdata)
{
	if(demod_type == FE_DVB_T2)
	{
		app_demod_info_timer_show_t2_info_value();
	}
	else if(demod_type == FE_DVB_T)
	{
		app_demod_info_timer_show_t_info_value();
	}

	return 0;
}

SIGNAL_HANDLER int app_demod_info_create(const char* widgetname, void *usrdata)
{
	AppFrontend_Info info = {0};

	app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_INFO_GET, &info);
	demod_type = info.type;
	if(demod_type == FE_DVB_T2)
	{
		app_demod_info_show_t2_info();
		app_demod_info_show_t2_info_value();
		app_demod_info_timer_show_t2_info_value();
	}
	else if(demod_type == FE_DVB_T)
	{
		app_demod_info_show_t_info();
		app_demod_info_show_t_info_value();
		app_demod_info_timer_show_t_info_value();
	}
	APP_TIMER_ADD(s_demod_info_timer, timer_show_demod_info, 500, TIMER_REPEAT);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_demod_info_destroy(const char* widgetname, void *usrdata)
{
	APP_TIMER_REMOVE(s_demod_info_timer);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_demod_info_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_EXIT:
			case STBK_MENU:
				GUI_EndDialog("wnd_demod_info");
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}
#endif
#endif

