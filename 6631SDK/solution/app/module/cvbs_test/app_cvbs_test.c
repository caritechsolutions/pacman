
#include "app.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include <stdio.h>
#include <fcntl.h>
#include "app_default_params.h"
#include "app_wnd_system_setting_opt.h"

#include "full_screen.h"    /* only for signal state  */
#if CVBS_TEST_SUPPORT
#include <sys/time.h>
#include <sys/select.h>
#include "module/channel_edit.h"
#include "av/hal/gxav_hal_vout_vpf.h"
#include "av/hal/gxav_hal_vout.h"
#include "av/hal/gxav_hal_vout_sec.h"
#include "app_wnd_file_list.h"

#define CVBS_MAX_ITEM_NUM 24
#define TOTAL_SIZE 512
#define MAX_IP_LENGTH	15
#define MAX_EXPORT_NAME_LEN	100
#define CVBS_FIXER_ERROR	0xFF
#define CVBSFILE_NAME_INVALID    "\x5c\x2f\x2c\x22\x3b\x27\x60"
#define WND_CVBS_TEST   "wnd_cvbs_test"
#define IMG_BAR_HALF_UNFOCUS "s_bar_half_unfocus"
#define IMG_BAR_HALF_FOCUS_R "s_bar_half_focus_r"
#define IMG_BAR_HALF_FOCUS_L_IP "s_bar_half_focus_l"
#define IMG_BAR_HALF_FOCUS_OK_IP "s_bar_half_focus_ok"
#define STR_ID_VECTOR  "VectorUGain"
#define STR_ID_CVBS_TEST     "CVBS Test"
#define STR_ID_BAR_LEVEL     "Bar Level"
#define STR_ID_SYNC_LEVEL    "Sync Level"
#define STR_ID_CHROMA_GAIN   "Chroma Gain"
#define STR_ID_CB_GAIN       "VectorUGain"
#define STR_ID_CR_GAIN       "VectorVGain"
#define STR_ID_CHROMA_SYNC    "Chroma Sync"
#define STR_ID_FRE_RESPONSE  "Frequency Response"
#define STR_ID_RESET         "Reset"
#define STR_ID_JITTER        "Jitter"
#define STR_ID_LUM_NOLINEARITY     "Luminance Nolinearity"

// cvbs set
#define	CVBS_BAR_LEVEL_F		"test>bar_level"
#define	CVBS_SYNC_LEVEL_F		"test>sync_level"
#define	CVBS_CHROMA_GAIN_F		"test>chr_gain"
#define	CVBS_CB_GAIN_F		    "test>cb_gain"
#define	CVBS_CR_GAIN_F		    "test>cr_gain"
#define	CVBS_CHOMA_SYNC_F	    "test>cho_sync"
#define	CVBS_FRE_RESPONSE_F		"test>fre_res"
#define CVBS_DEFAULT_VALUE      36
#define ITEM_OFFSET_CONTENT     "[-36,-35,-34,-33,-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,-16,-15,-14,-13,-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,+1,+2,+3,+4,+5,+6,+7,+8,+9,+10,+11,+12,+13,+14,+15,+16,+17,+18,+19,+20,+21,+22,+23,+24,+25,+26,+27,+28,+29,+30,+31,+32,+33,+34,+35,+36]"

#define CVBS_STR_MARK_BAR_LEVEL   "M0 Bar Level"
#define CVBS_STR_GDAC0_IPS        "Sync Level Y1"
#define CVBS_STR_GDAC1_IPS        "Sync Level Y2"
#define CVBS_STR_GDAC2_IPS        "Sync Level Y3"
#define CVBS_STR_GDAC3_IPS        "Sync Level Y4"
#define CVBS_STR_MARK_M1_SCALE1   "Chroma Gain M1"
#define CVBS_STR_MARK_M2_SCALE2   "Chroma Gain M2"
#define CVBS_STR_MARK_BURST_AMP_A "Chroma Sync A"
#define CVBS_STR_MARK_BURST_AMP_B "Chroma sync B"
#define CVBS_STR_MARK_TV_FIR_VAL0 "Fre Response V0"
#define CVBS_STR_MARK_TV_FIR_VAL1 "Fre Response V1"
#define CVBS_STR_MARK_TV_FIR_VAL2 "Fre Response V2"
#define CVBS_STR_MARK_TV_FIR_VAL3 "Fre Response V3"
#define CVBS_STR_MARK_TV_FIR_VAL4 "Fre Response V4"
#define CVBS_STR_MARK_BLANK_AMP   "Blank amp1"
#define CVBS_STR_MARK_BLACK_AMP   "Black amp2"
#define CVBS_STR_MARK_Y_SHIFT     "Y shift3"

#define DEFAULT_BAR_LEVEL 160
#define DEFAULT_SYNC_LEVEL_Y1 29
#define DEFAULT_SYNC_LEVEL_Y2 29
#define DEFAULT_SYNC_LEVEL_Y3 29
#define DEFAULT_SYNC_LEVEL_Y4 28
#define DEFAULT_CHROMA_GAIN_M1 150
#define DEFAULT_CHROMA_GAIN_M2 212
#define DEFAULT_CHROMA_SYNC_A  106
#define DEFAULT_CHROMA_SYNC_B  150
#define DEFAULT_FRE_RESPONSE_V0 7692
#define DEFAULT_FRE_RESPONSE_V1 549491732
#define DEFAULT_FRE_RESPONSE_V2 638093639
#define DEFAULT_FRE_RESPONSE_V3 577014138
#define DEFAULT_FRE_RESPONSE_V4 590901355

typedef enum
{
	ITEM_SYNC_LEVEL = 0,
	ITEM_BAR_LEVEL,
	ITEM_CHROMA_GAIN,
	ITEM_CHOMA_SYNC,
	ITEM_FRE_RESPONSE,
    ITEM_CB_GAIN,
	ITEM_CR_GAIN,
	ITEM_RESET,
	ITEM_CVBS_TEST_TOTAL
}cvbsTestOptSel;

typedef enum
{
	CVBS_INDEX_DEFAULE = 0,
	CVBS_INDEX_RAW,
	CVBS_INDEX_SET_PARA
}CvbsTestSetFixer;

typedef struct SyscvbsTestPara_s
{
    char sync_level;
	char bar_level;
	char chroma_gain;
	char cb_gain;
	char cr_gain;
	char choma_sync;
	char fre_response;
}SyscvbsTestPara_t;

typedef struct _menu_t
{
	char * imgwidget;
	char * txtwidget;
	char * focuswidget;
}MKMENUWIDGET_T;

static char *file_path = NULL;
static char *file_path_bak = NULL;
static SyscvbsTestPara_t s_cvbs_test_info_para = {CVBS_DEFAULT_VALUE, CVBS_DEFAULT_VALUE, CVBS_DEFAULT_VALUE, CVBS_DEFAULT_VALUE, CVBS_DEFAULT_VALUE, CVBS_DEFAULT_VALUE, CVBS_DEFAULT_VALUE};
static cvbsTestOptSel s_CvbsTestSel = ITEM_SYNC_LEVEL;
static SystemSettingOpt s_cvbs_test_opt;
static SystemSettingItem s_cvbs_test_item[ITEM_CVBS_TEST_TOTAL];

static char s_export_name[MAX_EXPORT_NAME_LEN];
static char *s_test_item_content_text[] ={STR_ID_SYNC_LEVEL,STR_ID_BAR_LEVEL, STR_ID_CHROMA_GAIN,STR_ID_CHROMA_SYNC,
    STR_ID_FRE_RESPONSE,STR_ID_VECTOR,"VectorVGain",STR_ID_RESET};
static int s_key = 0;

static char *s_test_item_content_edit_cmb[] =
{
	"cmb_cvbs_test_opt1",
	"cmb_cvbs_test_opt2",
	"cmb_cvbs_test_opt3",
	"cmb_cvbs_test_opt4",
	"cmb_cvbs_test_opt5",
	"cmb_cvbs_test_opt6",
	"cmb_cvbs_test_opt7",
	"cmb_cvbs_test_opt8",
	"btn_cvbs_test_opt9",
	"btn_cvbs_test_opt10"
};

static char *s_test_item_title[] =
{
	"text_cvbs_test_item1",
	"text_cvbs_test_item2",
	"text_cvbs_test_item3",
	"text_cvbs_test_item4",
	"text_cvbs_test_item5",
	"text_cvbs_test_item6",
	"text_cvbs_test_item7",
	"text_cvbs_test_item8",
	"text_cvbs_test_item9",
	"text_cvbs_test_item10"
};

static char *s_test_item_choice_back[] =
{
	"img_cvbs_test_item_choice1",
	"img_cvbs_test_item_choice2",
	"img_cvbs_test_item_choice3",
	"img_cvbs_test_item_choice4",
	"img_cvbs_test_item_choice5",
	"img_cvbs_test_item_choice6",
	"img_cvbs_test_item_choice7",
	"img_cvbs_test_item_choice8",
	"img_cvbs_test_item_choice9",
	"img_cvbs_test_item_choice10"
};

typedef struct {
    size_t total;
    size_t used;
    char *buf;
}CvbsTestPrintBuf;

CvbsTestPrintBuf export_ctrl;
static GxVideoPerformanceFixer gs_fixer;
extern int GxVoutHAL_ChannelSetScaleConfig(GxVoutChannelID channel, GxVoutScaleConfig params);
static void app_cvbs_test_result_para_get(SyscvbsTestPara_t *ret_para);
static int app_cvbs_test_para_save(SyscvbsTestPara_t *ret_para);
static void app_test_system_para_get(int default_para_tag);
static void app_cvbs_test_vpf_print(GxVideoPerformanceFixer *fixer);

static int app_cvbs_import_fixer_data(CvbsTestSetFixer mode)
{
    int ret = 0;
    GxVideoPerformanceFixer fixer;

    memset(&fixer, 0, sizeof(GxVideoPerformanceFixer));
    // 首先: 读出当前参数
    GxVoutHAL_VideoPerformanceFixerGet(&fixer);
    if(mode == CVBS_INDEX_DEFAULE)
    {
        //修改指标测试得到的各个参数.
        fixer.m0_scale0 = DEFAULT_BAR_LEVEL;
        //sync level
        fixer.gdac0_ips = DEFAULT_SYNC_LEVEL_Y1;
        fixer.gdac1_ips = DEFAULT_SYNC_LEVEL_Y2;
        fixer.gdac2_ips = DEFAULT_SYNC_LEVEL_Y3;
        fixer.gdac3_ips = DEFAULT_SYNC_LEVEL_Y4;

        fixer.m1_scale1 = DEFAULT_CHROMA_GAIN_M1;
        fixer.m2_scale2 = DEFAULT_CHROMA_GAIN_M2;

        fixer.burst_amp_a = DEFAULT_CHROMA_SYNC_A;
        fixer.burst_amp_b = DEFAULT_CHROMA_SYNC_B;

        fixer.tv_fir_val0 = DEFAULT_FRE_RESPONSE_V0;
        fixer.tv_fir_val1 = DEFAULT_FRE_RESPONSE_V1;
        fixer.tv_fir_val2 = DEFAULT_FRE_RESPONSE_V2;
        fixer.tv_fir_val3 = DEFAULT_FRE_RESPONSE_V3;
        fixer.tv_fir_val4 = DEFAULT_FRE_RESPONSE_V4;
    }
    else if(mode == CVBS_INDEX_RAW)
    {
        app_cvbs_test_para_save(&s_cvbs_test_info_para);
        return 0;
    }
    // 最后: 更新参数
    GxVoutHAL_VideoPerformanceFixerSet(&fixer);
    app_cvbs_test_vpf_print(&fixer);
    return ret;
}

static int app_cvbs_test_init_para_status(void)
{
    GxVoutScaleConfig scale = {GXVOUT_SCALE_STANDARD, GXVOUT_SCALE_STANDARD, };
    VideoColor color = {50, 50, 50, 50, 50};
    GxPlayer_SetVideoColors(color);
    //GxBus_ConfigSetInt(TV_STANDARD_KEY, TV_STANDARD);
    //close pp
    GxPlayer_SetVideoDeinterlace(0);
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DRCMODE,GX_VDRC_BYPASS);//PSYS_VDRC_MODE
    GxPlayer_SetVideoDrcMode(GX_VDRC_BYPASS);
    GxPlayer_SetVideoAspect(ASPECT_RAW_SIZE);//设置 AspectRatio 为 原始大小.
    GxBus_ConfigSetInt(PMPSET_SAVE_ASPECT_RATIO, ASPECT_RAW_RATIO);
    //printf("***[%s]*****[DRC]0x8a80a004=0x%x\n",__FUNCTION__,*(volatile unsigned int*)0x8a80a004);
    GxVoutHAL_ChannelSetScaleConfig(GXAV_VOUT_RCA, scale);//设置 指标 Scale 参数为标准参数

    return 0;
}

static int app_cvbs_index_value_setup(GxVideoPerformanceType type,int value)
{
    int ret_value = 0;

    if(value > CVBS_DEFAULT_VALUE)
    {
        if(s_key == STBK_LEFT)
        {
            ret_value = GxVoutHAL_VideoPerformanceFixerDecrease(type);
        }
        else
        {
            ret_value = GxVoutHAL_VideoPerformanceFixerIncrease(type);
        }
    }
    /*else if(value == CVBS_DEFAULT_VALUE)
    {
        GxVoutHAL_VideoPerformanceFixerReset(type);
        ret_value = 0;
    }*/
    else
    {
        if(s_key == STBK_RIGHT)
        {
            ret_value = GxVoutHAL_VideoPerformanceFixerIncrease(type);
        }
        else
        {
            ret_value = GxVoutHAL_VideoPerformanceFixerDecrease(type);
        }

    }

    if(ret_value < 0)
    {
        GUI_SetProperty("btn_cvbs_test_opt10", "string", "Data overflow");
        GUI_SetProperty("btn_cvbs_test_opt10", "forecolor", "[key_red,key_red,key_red]");
    }
    else
    {
        GUI_SetProperty("btn_cvbs_test_opt10", "string", " ");
    }
    return ret_value;
}

static GxVideoPerformanceType _get_vpf_type(int mode_type)
{
    GxVideoPerformanceType vfp_type;

    switch(mode_type)
    {
        case ITEM_BAR_LEVEL:
            vfp_type = VPF_BarLevel;
            break;
		case ITEM_SYNC_LEVEL:
            vfp_type = VPF_SyncLevel;
            break;
		case ITEM_CHROMA_GAIN:
            vfp_type = VPF_ChromaGain;
            break;
        case ITEM_CB_GAIN:
            vfp_type = VPF_ChromaGainM1Scale1;
            break;
        case ITEM_CR_GAIN:
            vfp_type = VPF_ChromaGainM2Scale2;
            break;
		case ITEM_CHOMA_SYNC:
            vfp_type = VPF_ChromaSyncLevel;
            break;
		case ITEM_FRE_RESPONSE:
            vfp_type = VPF_FrequencyResponse;
            break;
        default:
            vfp_type = mode_type;
            break;
    }
    return vfp_type;
}

static int app_cvbs_test_set_fix_value(void)
{
    int ret = 0;
	PopDlg  pop;

    app_cvbs_test_result_para_get(&s_cvbs_test_info_para);
#if 0
    app_cvbs_index_value_setup(VPF_BarLevel,s_cvbs_test_info_para.bar_level);
    app_cvbs_index_value_setup(VPF_SyncLevel,s_cvbs_test_info_para.sync_level);
    app_cvbs_index_value_setup(VPF_ChomaGain,s_cvbs_test_info_para.chroma_gain);
    app_cvbs_index_value_setup(VPF_ChromaSyncLevel,s_cvbs_test_info_para.choma_sync);
    app_cvbs_index_value_setup(VPF_FrequencyResponse,s_cvbs_test_info_para.fre_response);
#endif
	memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.str = STR_ID_SAVE_INFO;
	//if(popdlg_create(&pop) == POP_VAL_OK)
	{
		app_cvbs_test_para_save(&s_cvbs_test_info_para);
		ret = 1;
	}

    return ret;
}

static int app_cvbs_test_fixer_reset(void)
{
    int index = 0;
    int init_value = CVBS_DEFAULT_VALUE;

    for(index=0;index<ITEM_RESET; index++)
    {
        GUI_SetProperty(s_test_item_content_edit_cmb[index], "select", &init_value);
        GxVoutHAL_VideoPerformanceFixerReset(_get_vpf_type(index));
    }
    GUI_SetProperty(WND_CVBS_TEST,"draw_now",NULL);
    return 0;
}
static char* app_strndup(const char* src, int len)
{
	char *p = NULL;

	if(strlen(src) < len)
	{
		app_log_error("[GxCore_Strndup] length err!!!\n");
		return NULL;
	}

	p = (char*)GxCore_Calloc(len+1 , sizeof(char));

	if(p == NULL)
	{
		app_log_error("[GxCore_Strndup] calloc err!!!\n");
		return NULL;
	}

	strncpy(p, src, len);

	return p;
}

static void app_cvbs_test_vpf_print(GxVideoPerformanceFixer *fixer)
{
	if(NULL == fixer)
		return;

	app_log_info("******************************************************************************\n");
	app_log_info("********\t\t Video out performance parameter \t\t ********");
	app_log_info("******************************************************************************\n");
	app_log_info("-------------------------\t\t Sync Level DAC Gain \t ------------------------\n");
	app_log_info("***gdac0_ips \t\t\t = \t %d(0x%x)***\n", fixer->gdac0_ips, fixer->gdac0_ips);
	app_log_info("***gdac1_ips \t\t\t = \t %d(0x%x)***\n", fixer->gdac1_ips, fixer->gdac1_ips);
	app_log_info("***gdac2_ips \t\t\t = \t %d(0x%x)***\n", fixer->gdac2_ips, fixer->gdac2_ips);
	app_log_info("***gdac3_ips \t\t\t = \t %d(0x%x)***\n", fixer->gdac3_ips, fixer->gdac3_ips);
	app_log_info("---------------------------\t\t Bar Level \t  -----------------------\n");
	app_log_info("***m0_scale0 \t\t\t = \t %d(0x%x)***\n", fixer->m0_scale0, fixer->m0_scale0);
	app_log_info("---------------------------\t\t Chroma Gain \t ------------------------\n");
	app_log_info("***m1_scale1 \t\t\t = \t %d(0x%x)***\n", fixer->m1_scale1, fixer->m1_scale1);
	app_log_info("***m2_scale2 \t\t\t = \t %d(0x%x)***\n", fixer->m2_scale2, fixer->m2_scale2);
	app_log_info("--------------------------\t Chroma sync level \t ------------------------\n");
	app_log_info("***burst_amp_a \t\t\t = \t %d(0x%x)***\n", fixer->burst_amp_a, fixer->burst_amp_a);
	app_log_info("***burst_amp_b \t\t\t = \t %d(0x%x)***\n", fixer->burst_amp_b, fixer->burst_amp_b);
	app_log_info("--------------------------\t Frequency Response \t ------------------------\n");
	app_log_info("***tv_fir_val0 \t\t\t = \t %d(0x%x)***\n", fixer->tv_fir_val0, fixer->tv_fir_val0);
	app_log_info("***tv_fir_val1 \t\t\t = \t %d(0x%x)***\n", fixer->tv_fir_val1, fixer->tv_fir_val1);
	app_log_info("***tv_fir_val2 \t\t\t = \t %d(0x%x)***\n", fixer->tv_fir_val2, fixer->tv_fir_val2);
	app_log_info("***tv_fir_val3 \t\t\t = \t %d(0x%x)***\n", fixer->tv_fir_val3, fixer->tv_fir_val3);
	app_log_info("***tv_fir_val4 \t\t\t = \t %d(0x%x)***\n", fixer->tv_fir_val4, fixer->tv_fir_val4);
    app_log_info("-----------------------------\t -----Sync level \t ------------------------\n");
	app_log_info("***blank_amp \t\t\t = \t %d(0x%x)***\n", fixer->blank_amp, fixer->blank_amp);
	app_log_info("***black_amp \t\t\t = \t %d(0x%x)***\n", fixer->black_amp, fixer->black_amp);
	app_log_info("***y_shift \t\t\t\t = \t %d(0x%x)***\n", fixer->y_shift, fixer->y_shift);
	app_log_info("******************************************************************************\n");
}


static int app_cvbs_test_get_data_from_txt_file(const char *path)
{
    int item_num = 0;
    char *buffer = NULL;
    FILE *fp_handle = NULL;
    char *end=NULL, *temp=NULL;
    char *str_mark=NULL, *value=NULL;

    app_log_info("get_cvbs_test_data_from_txt_file\n");
    fp_handle = fopen(path,"rb");
    if(fp_handle)
    {
        buffer = (char *)GxCore_Malloc(TOTAL_SIZE);
        while((fgets(buffer,TOTAL_SIZE,fp_handle))&&(item_num<CVBS_MAX_ITEM_NUM))
        {
            if(strlen(buffer)>3)
            {
                if(buffer[strlen(buffer)-1]=='\n')
                {
                    buffer[strlen(buffer)-1] = '\0';
                }

                str_mark = NULL;
                value = NULL;

                //get key
                temp=buffer;
                end=strchr(temp, ':');
                if(end&&end>(temp+1))
                {
                    str_mark = app_strndup(temp, end-temp);

                    //get value
                    temp=end+1;
                    end=strchr(temp, ':');
                    if(end&&end>(temp+1))
                    {
                        value = app_strndup(temp, end-temp);

                    }
                    else
                    {
                        if(strlen(temp)>0)
                        {
                            value = GxCore_Strdup(temp);
                        }
                    }
                }

                if(str_mark&&value)
                {
                    if(0 == strcasecmp(str_mark, CVBS_STR_MARK_BAR_LEVEL))
                    {
                        gs_fixer.m0_scale0 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_GDAC0_IPS))
                    {
                        gs_fixer.gdac0_ips = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_GDAC1_IPS))
                    {
                        gs_fixer.gdac1_ips = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_GDAC2_IPS))
                    {
                        gs_fixer.gdac2_ips = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_GDAC3_IPS))
                    {
                        gs_fixer.gdac3_ips = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_M1_SCALE1))
                    {
                        gs_fixer.m1_scale1 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_MARK_M2_SCALE2))
                    {
                        gs_fixer.m2_scale2 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_BURST_AMP_A))
                    {
                        gs_fixer.burst_amp_a = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_BURST_AMP_B))
                    {
                        gs_fixer.burst_amp_b = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_TV_FIR_VAL0))
                    {
                        gs_fixer.tv_fir_val0 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_TV_FIR_VAL1))
                    {
                        gs_fixer.tv_fir_val1 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_TV_FIR_VAL2))
                    {
                        gs_fixer.tv_fir_val2 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark, CVBS_STR_MARK_TV_FIR_VAL3))
                    {
                        gs_fixer.tv_fir_val3 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_MARK_TV_FIR_VAL4))
                    {
                        gs_fixer.tv_fir_val4 = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_MARK_BLANK_AMP))
                    {
                        gs_fixer.blank_amp = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_MARK_BLACK_AMP))
                    {
                        gs_fixer.black_amp = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,CVBS_STR_MARK_Y_SHIFT))
                    {
                        gs_fixer.y_shift = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_SYNC_LEVEL))
                    {
                        s_cvbs_test_info_para.sync_level = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_BAR_LEVEL))
                    {
                        s_cvbs_test_info_para.bar_level = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_CHROMA_GAIN))
                    {
                        s_cvbs_test_info_para.chroma_gain = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_CB_GAIN))
                    {
                        s_cvbs_test_info_para.cb_gain = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_CR_GAIN))
                    {
                        s_cvbs_test_info_para.cr_gain = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_CHROMA_SYNC))
                    {
                        s_cvbs_test_info_para.choma_sync = atoi(value);
                    }
                    else if(0 == strcasecmp(str_mark,STR_ID_FRE_RESPONSE))
                    {
                        s_cvbs_test_info_para.fre_response = atoi(value);
                    }

                    item_num++;
                }
                else
                {
                    APP_FREE(str_mark);
                    APP_FREE(value);
                }
            }
            memset(buffer, 0, TOTAL_SIZE);
        }
        APP_FREE(buffer);
        fclose(fp_handle);
    }
    return 0;
}

static int app_cvbs_test_update_import_para(void)
{
    int init_val = 0;

    //app_test_system_para_get(CVBS_DEFAULT_VALUE);
    init_val = s_cvbs_test_info_para.sync_level;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_SYNC_LEVEL], "select", &init_val);
    init_val = s_cvbs_test_info_para.bar_level;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_BAR_LEVEL], "select", &init_val);
    init_val = s_cvbs_test_info_para.chroma_gain;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_CHROMA_GAIN], "select", &init_val);
    init_val = s_cvbs_test_info_para.cb_gain;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_CB_GAIN], "select", &init_val);
    init_val = s_cvbs_test_info_para.cr_gain;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_CR_GAIN], "select", &init_val);
    init_val = s_cvbs_test_info_para.choma_sync;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_CHOMA_SYNC], "select", &init_val);
    init_val = s_cvbs_test_info_para.fre_response;
    GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_FRE_RESPONSE], "select", &init_val);
    GUI_SetProperty(WND_CVBS_TEST,"draw_now",NULL);
    return 0;
}

static int _app_cvbs_test_import_text_data(void)
{
    int ret = 0;
    GxVideoPerformanceFixer fixer;

    // 首先: 读出当前参数
    memset(&fixer, 0, sizeof(GxVideoPerformanceFixer));
    GxVoutHAL_VideoPerformanceFixerGet(&fixer);

    memcpy(&fixer, &gs_fixer, sizeof(GxVideoPerformanceFixer));

    // 然后: 修改指标测试得到的各个参数.
    fixer.m0_scale0 = gs_fixer.m0_scale0;
    //sync level
    fixer.gdac0_ips = gs_fixer.gdac0_ips;
    fixer.gdac1_ips = gs_fixer.gdac1_ips;
    fixer.gdac2_ips = gs_fixer.gdac2_ips;
    fixer.gdac3_ips = gs_fixer.gdac3_ips;

    fixer.m1_scale1 = gs_fixer.m1_scale1;
    fixer.m2_scale2 = gs_fixer.m2_scale2;

    fixer.burst_amp_a = gs_fixer.burst_amp_a;
    fixer.burst_amp_b = gs_fixer.burst_amp_b;

    fixer.tv_fir_val0 = gs_fixer.tv_fir_val0;
    fixer.tv_fir_val1 = gs_fixer.tv_fir_val1;
    fixer.tv_fir_val2 = gs_fixer.tv_fir_val2;
    fixer.tv_fir_val3 = gs_fixer.tv_fir_val3;
    fixer.tv_fir_val4 = gs_fixer.tv_fir_val4;


    // 最后: 更新参数
    GxVoutHAL_VideoPerformanceFixerSet(&fixer);
    app_cvbs_test_vpf_print(&fixer);
    app_cvbs_test_update_import_para();
    return ret;
}

static void _app_cvbs_text_import_tip(int err)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));

    if(0 == err)
    {
        pop.str = "Import OK!";
    }
    else
    {
        pop.str = "Import Fail!";
    }
    pop.type = POP_TYPE_WAIT;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 2;
    popdlg_create(&pop);
}

static int _app_cvbs_text_filedlg_exit_cb(WndStatus ret)
{
    int str_len = 0;
    char *str_ext = NULL;

    if(ret == WND_OK)
    {
        app_get_file_real_path(&file_path);

        if (file_path != NULL)
        {
            ////backup the path...////
            APP_FREE(file_path_bak);
            str_len = strlen(file_path);
            file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if (file_path_bak != NULL)
            {
                memcpy(file_path_bak, file_path, str_len);
                file_path_bak[str_len] = '\0';
            }

            if((!file_path)||(strlen(file_path)==0))
            {
                app_log_error("input error\n");
                _app_cvbs_text_import_tip(2);
                return 2;
            }
            str_ext = strrchr(file_path, '.');
            if((str_ext)&&(0 == strcasecmp(str_ext, ".txt")))
            {
                app_cvbs_test_get_data_from_txt_file(file_path);
                _app_cvbs_test_import_text_data();
            }
            else
            {
                app_log_info("[CVBS TEST]unsupport file format\n");
                _app_cvbs_text_import_tip(4);
                return 4 ;
            }
        }
    }
    else
    {
       _app_cvbs_text_import_tip(1);
       return 1 ;
    }

    _app_cvbs_text_import_tip(0);
    return 0;
}

static int app_cvbs_text_import_data(void)
{
#define CVBS_FILE_SUFFFIX    "xml;XML;txt;TXT;m3u8;M3U8"
    FileListParam file_para;

    if (check_usb_status() == false)
    {
        app_log_error(STR_ID_INSERT_USB);
        _app_cvbs_text_import_tip(3);
        return 3;
    }
    if (GXCORE_FILE_EXIST != GxCore_FileExists(file_path_bak))
        APP_FREE(file_path_bak);

    memset(&file_para, 0, sizeof(file_para));
    file_para.cur_path = file_path_bak;
    file_para.dest_path = &file_path;
    file_para.suffix = CVBS_FILE_SUFFFIX;
    file_para.dest_mode = DEST_MODE_FILE;
    file_para.exit_cb = _app_cvbs_text_filedlg_exit_cb;
    app_get_file_path_dlg(&file_para);
    return 0;
}

static void app_cvbs_test_file_name_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
    {
        memset(s_export_name, 0, sizeof(s_export_name));
        GUI_SetProperty(WND_CVBS_TEST,"update",NULL);
        return;
    }

    if (data->out_name == NULL)
        return;

    memset(s_export_name, 0, MAX_EXPORT_NAME_LEN);
    strncpy(s_export_name, data->out_name, strlen(data->out_name));
}
static void app_cvbs_test_export_file_name(char *name)
{
	static PopKeyboard keyboard;

    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = name;
    keyboard.max_num = MAX_EXPORT_NAME_LEN/2;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = NULL;
    keyboard.release_cb = app_cvbs_test_file_name_keyboard_proc;
    keyboard.usr_data   = CVBSFILE_NAME_INVALID;
    keyboard.pos.x = 500;
    multi_language_keyboard_create(&keyboard);
    GUI_StopSchedule();
	while(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
	{
	   GUI_LoopEvent();
	   GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();
    GUI_SetInterface("flush",NULL);

}

static int app_cvbs_test_file_name_get(char *file_name, size_t len)
{
    char time_buf[48];
    HotplugPartitionList *phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (!phpl || phpl->partition_num < 1)
        return -1;
    memset(file_name, 0, len);
    strcpy(file_name, phpl->partition[0].partition_entry);
    strcat(file_name, "/cvbs");
    if (GxCore_FileExists(file_name) != GXCORE_FILE_EXIST)
        GxCore_Mkdir(file_name);
    strcat(file_name, "/");

    GxTime time = {0};
    GxCore_GetLocalTime(&time);
    time_t t = time.seconds;
    struct tm *p = gmtime(&t);
    size_t tmp_len = strlen(file_name);
    memset(time_buf, 0, sizeof(time_buf));
    strftime(time_buf, len-tmp_len, "%Y-%m-%d_%H-%M-%S", p);
    app_cvbs_test_export_file_name(time_buf);
    if(*s_export_name != 0)
    {
        sprintf(file_name+tmp_len, "%s", s_export_name);
    }
    else
    {
        strftime(file_name+tmp_len, len-tmp_len, "%Y-%m-%d_%H-%M-%S", p);
    }
    strcat(file_name, ".txt");
    return 0;
}

static int _puts_fixer_data(void)
{
    CvbsTestPrintBuf *ctrl = &export_ctrl;
    int rc = -1;

    memset(ctrl->buf, 0, TOTAL_SIZE);
    rc = sprintf(ctrl->buf,"%s:%d\n",CVBS_STR_MARK_BAR_LEVEL,gs_fixer.m0_scale0);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n",CVBS_STR_GDAC0_IPS,gs_fixer.gdac0_ips);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n",CVBS_STR_GDAC1_IPS,gs_fixer.gdac1_ips);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", CVBS_STR_GDAC2_IPS,gs_fixer.gdac2_ips);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", CVBS_STR_GDAC3_IPS,gs_fixer.gdac3_ips);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used,"%s:%d\n", CVBS_STR_MARK_M1_SCALE1,gs_fixer.m1_scale1);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used,"%s:%d\n",CVBS_STR_MARK_M2_SCALE2,gs_fixer.m2_scale2);
    ctrl->used += rc;

    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n",CVBS_STR_MARK_BURST_AMP_A,gs_fixer.burst_amp_a);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_BURST_AMP_B,gs_fixer.burst_amp_b);
    ctrl->used += rc;

    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_TV_FIR_VAL0,gs_fixer.tv_fir_val0);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_TV_FIR_VAL1,gs_fixer.tv_fir_val1);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_TV_FIR_VAL2,gs_fixer.tv_fir_val2);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_TV_FIR_VAL3,gs_fixer.tv_fir_val3);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_TV_FIR_VAL4,gs_fixer.tv_fir_val4);
    ctrl->used += rc;

    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n",CVBS_STR_MARK_BLANK_AMP,gs_fixer.blank_amp);
    ctrl->used += rc;
    //strcat(ctrl->buf+ctrl->used, "\nSync level1:");
    //ctrl->used += 13;
    //rc = snprintf(ctrl->buf+ctrl->used, USED_LEN, "%012d\n", gs_fixer.blank_amp);
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%d\n", CVBS_STR_MARK_BLACK_AMP,gs_fixer.black_amp);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", CVBS_STR_MARK_Y_SHIFT,gs_fixer.y_shift);
    ctrl->used += rc;

    app_cvbs_test_result_para_get(&s_cvbs_test_info_para);
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", STR_ID_SYNC_LEVEL,s_cvbs_test_info_para.sync_level);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", STR_ID_BAR_LEVEL,s_cvbs_test_info_para.bar_level);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", STR_ID_CHROMA_GAIN,s_cvbs_test_info_para.chroma_gain);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", STR_ID_CHROMA_SYNC,s_cvbs_test_info_para.choma_sync);
    ctrl->used += rc;
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", STR_ID_FRE_RESPONSE,s_cvbs_test_info_para.fre_response);
    ctrl->used += rc;
    //rc = sprintf(ctrl->buf+ctrl->used, "VectorUGain:%0d\n", s_cvbs_test_info_para.cb_gain);
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n",STR_ID_CB_GAIN,s_cvbs_test_info_para.cb_gain);
    ctrl->used += rc;
    //rc = sprintf(ctrl->buf+ctrl->used, "VectorVGain:%0d\n", s_cvbs_test_info_para.cr_gain);
    rc = sprintf(ctrl->buf+ctrl->used, "%s:%0d\n", STR_ID_CR_GAIN,s_cvbs_test_info_para.cr_gain);
    ctrl->used += rc;

    return rc;
}

static int app_cvbs_test_print_to_usb(void)
{
    static char file_name[128];
    FILE *fp = NULL;
    CvbsTestPrintBuf *ctrl = &export_ctrl;

    if ((ctrl->buf = GxCore_Malloc(TOTAL_SIZE)) == NULL)
    {
        GxCore_Free(ctrl->buf);
        return 2;
    }

    if (strlen(file_name) != 0 || access(file_name, F_OK) != 0)
    {
        if (app_cvbs_test_file_name_get(file_name, sizeof(file_name)) < 0)
            return -1;
    }
    if ((fp = fopen(file_name, "a+")) == NULL)
        return -1;
    _puts_fixer_data();
    if (ctrl->used == 0)
    {
        fclose(fp);
        return 0;
    }
    fwrite(ctrl->buf, 1, ctrl->used, fp);
    ctrl->used = 0;
    fclose(fp);

    GxCore_Free(ctrl->buf);
    return 0;
}


static int app_cvbs_test_get_vpf_para(void)
{
	GxVoutHAL_VideoPerformanceFixerGet(&gs_fixer);
    app_cvbs_test_vpf_print(&gs_fixer);
    return 0;
}

static int app_cvbs_test_get_fixer_para(int mode_type)
{
    int ret_value = 0;
    GxVideoPerformanceFixer fixer;

    memset(&fixer, 0, sizeof(GxVideoPerformanceFixer));
	GxVoutHAL_VideoPerformanceFixerGet(&fixer);
    app_cvbs_test_vpf_print(&fixer);
    switch(mode_type)
    {
        case ITEM_BAR_LEVEL:
            ret_value = fixer.m0_scale0;
            break;
		case ITEM_SYNC_LEVEL:
            ret_value = fixer.gdac0_ips;
            break;
		//case ITEM_CHROMA_GAIN:
        case ITEM_CB_GAIN:
            if(((fixer.m1_scale1&0xFF) > 0xFF)||((fixer.m1_scale1&0xFF) < 0))
            {
                ret_value = CVBS_FIXER_ERROR;
            }
            else
            {
                ret_value = fixer.m1_scale1&0xFF;
            }
            break;
        case ITEM_CR_GAIN:
            if(((fixer.m2_scale2&0xFF) > 0xFF)||((fixer.m2_scale2&0xFF) < 0))
            {
                ret_value = CVBS_FIXER_ERROR;
            }
            else
            {
                ret_value = fixer.m2_scale2&0xFF;
            }
            break;
		case ITEM_CHOMA_SYNC:
		case ITEM_FRE_RESPONSE:
        default:
            ret_value = 0;
            break;
    }

    return ret_value;
}

static void app_cvbs_test_item_data_update(cvbsTestOptSel sel)
{

#define TEXT_FORE_COLOR "[text_color,text_color,text_disable_color]"

	if(s_cvbs_test_opt.item[sel].itemType == ITEM_CHOICE)
	{
		GUI_GetProperty(s_test_item_content_edit_cmb[sel], "select", &s_cvbs_test_opt.item[sel].itemProperty.itemPropertyCmb.sel);
	}

	if(s_cvbs_test_opt.item[sel].itemType == ITEM_EDIT)
	{
		GUI_GetProperty(s_test_item_content_edit_cmb[sel], "string", &s_cvbs_test_opt.item[sel].itemProperty.itemPropertyEdit.string);
	}

	if(s_cvbs_test_opt.item[sel].itemType == ITEM_PUSH)
	{
		GUI_GetProperty(s_test_item_content_edit_cmb[sel], "string", &s_cvbs_test_opt.item[sel].itemProperty.itemPropertyEdit.string);
	}

	GUI_SetProperty(s_test_item_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
	GUI_SetProperty(s_test_item_title[sel], "forecolor", TEXT_FORE_COLOR);
	return;
}

static void app_test_system_para_get(int default_para_tag)
{
	int init_value = default_para_tag;

    GxBus_ConfigGetInt(CVBS_BAR_LEVEL_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.bar_level = init_value;

	GxBus_ConfigGetInt(CVBS_SYNC_LEVEL_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.sync_level = init_value;

	GxBus_ConfigGetInt(CVBS_CHROMA_GAIN_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.chroma_gain= init_value;

    GxBus_ConfigGetInt(CVBS_CB_GAIN_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.cb_gain= init_value;

    GxBus_ConfigGetInt(CVBS_CR_GAIN_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.cr_gain= init_value;

	GxBus_ConfigGetInt(CVBS_CHOMA_SYNC_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.choma_sync = init_value;

    GxBus_ConfigGetInt(CVBS_FRE_RESPONSE_F, &init_value, default_para_tag);
    s_cvbs_test_info_para.fre_response = init_value;


	return;
}

static int app_cvbs_test_para_save(SyscvbsTestPara_t *ret_para)
{
	int init_value = 0;

	init_value = ret_para->bar_level;
    GxBus_ConfigSetInt(CVBS_BAR_LEVEL_F,init_value);

	init_value = ret_para->sync_level;
    GxBus_ConfigSetInt(CVBS_SYNC_LEVEL_F,init_value);

	init_value = ret_para->chroma_gain;
    GxBus_ConfigSetInt(CVBS_CHROMA_GAIN_F,init_value);

	init_value = ret_para->cb_gain;
    GxBus_ConfigSetInt(CVBS_CB_GAIN_F,init_value);

    init_value = ret_para->cr_gain;
    GxBus_ConfigSetInt(CVBS_CR_GAIN_F,init_value);

	init_value = ret_para->choma_sync;
    GxBus_ConfigSetInt(CVBS_CHOMA_SYNC_F,init_value);

    init_value = ret_para->fre_response;
    GxBus_ConfigSetInt(CVBS_FRE_RESPONSE_F,init_value);

    app_cvbs_import_fixer_data(CVBS_INDEX_SET_PARA);

	return 0;
}

static void app_cvbs_test_result_para_get(SyscvbsTestPara_t *ret_para)
{

	ret_para->bar_level = s_cvbs_test_item[ITEM_BAR_LEVEL].itemProperty.itemPropertyCmb.sel;
	ret_para->sync_level = s_cvbs_test_item[ITEM_SYNC_LEVEL].itemProperty.itemPropertyCmb.sel;
	ret_para->chroma_gain = s_cvbs_test_item[ITEM_CHROMA_GAIN].itemProperty.itemPropertyCmb.sel;
	ret_para->cb_gain = s_cvbs_test_item[ITEM_CB_GAIN].itemProperty.itemPropertyCmb.sel;
	ret_para->cr_gain = s_cvbs_test_item[ITEM_CR_GAIN].itemProperty.itemPropertyCmb.sel;
	ret_para->choma_sync = s_cvbs_test_item[ITEM_CHOMA_SYNC].itemProperty.itemPropertyCmb.sel;
	ret_para->fre_response = s_cvbs_test_item[ITEM_FRE_RESPONSE].itemProperty.itemPropertyCmb.sel;

	return;
}

static bool app_cvbs_test_para_change(SyscvbsTestPara_t *ret_para)
{
	bool ret = FALSE;
   	if(s_cvbs_test_info_para.bar_level!= ret_para->bar_level)
	{
		ret = TRUE;
	}

	if(s_cvbs_test_info_para.sync_level!= ret_para->sync_level)
	{
		ret = TRUE;
	}

	if(s_cvbs_test_info_para.chroma_gain!= ret_para->chroma_gain)
	{
		ret = TRUE;
	}

	if(s_cvbs_test_info_para.cb_gain!= ret_para->cb_gain)
	{
		ret = TRUE;
	}

    if(s_cvbs_test_info_para.cr_gain!= ret_para->cr_gain)
	{
		ret = TRUE;
	}

	if(s_cvbs_test_info_para.choma_sync!= ret_para->choma_sync)
	{
		ret = TRUE;
	}

    if(s_cvbs_test_info_para.fre_response != ret_para->fre_response)
	{
		ret = TRUE;
	}

	return ret;
}

static void app_cvbs_test_edit_item_init(void)
{
	int n_index = 0;

	for(n_index = ITEM_SYNC_LEVEL; n_index < ITEM_CVBS_TEST_TOTAL; n_index++)
	{
		switch(n_index)
		{
			case ITEM_BAR_LEVEL:
			case ITEM_SYNC_LEVEL:
			case ITEM_CHROMA_GAIN:
			case ITEM_CB_GAIN:
			case ITEM_CR_GAIN:
			case ITEM_CHOMA_SYNC:
			case ITEM_FRE_RESPONSE:
				s_cvbs_test_item[n_index].itemTitle = s_test_item_content_text[n_index];
				break;
			default:
				s_cvbs_test_item[n_index].itemTitle = s_test_item_content_text[n_index];
				s_cvbs_test_item[n_index].itemType = ITEM_EDIT;

				s_cvbs_test_item[n_index].itemProperty.itemPropertyEdit.format = "edit_string";
				s_cvbs_test_item[n_index].itemProperty.itemPropertyEdit.maxlen = "35";
				s_cvbs_test_item[n_index].itemCallback.editCallback.EditPress= NULL;
				s_cvbs_test_item[n_index].itemProperty.itemPropertyEdit.intaglio = NULL;
				s_cvbs_test_item[n_index].itemProperty.itemPropertyEdit.default_intaglio = NULL;
				s_cvbs_test_item[n_index].itemProperty.itemPropertyEdit.string = NULL;
				s_cvbs_test_item[n_index].itemCallback.editCallback.EditReachEnd= NULL;
				s_cvbs_test_item[n_index].itemStatus = ITEM_NORMAL;
				break;
		}

	}

	return;
}

static int app_cvbs_test_bar_level_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_BAR_LEVEL], "select", &box_sel);
	s_cvbs_test_info_para.bar_level = box_sel;

	return 0;
}

static void app_cvbs_test_bar_level_item_init(void)
{
	s_cvbs_test_item[ITEM_BAR_LEVEL].itemTitle = STR_ID_BAR_LEVEL;
	s_cvbs_test_item[ITEM_BAR_LEVEL].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_BAR_LEVEL].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_BAR_LEVEL].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.bar_level;
	s_cvbs_test_item[ITEM_BAR_LEVEL].itemCallback.cmbCallback.CmbChange= app_cvbs_test_bar_level_change_callback;
	s_cvbs_test_item[ITEM_BAR_LEVEL].itemStatus = ITEM_NORMAL;
}

static int app_cvbs_test_sync_level_change_callback(int sel)
{
	int box_sel;

	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_SYNC_LEVEL], "select", &box_sel);
	s_cvbs_test_info_para.sync_level= box_sel;

	return 0;
}

static void app_cvbs_test_sync_level_item_init(void)
{
	s_cvbs_test_item[ITEM_SYNC_LEVEL].itemTitle = STR_ID_SYNC_LEVEL;
	s_cvbs_test_item[ITEM_SYNC_LEVEL].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_SYNC_LEVEL].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_SYNC_LEVEL].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.sync_level;
	s_cvbs_test_item[ITEM_SYNC_LEVEL].itemCallback.cmbCallback.CmbChange= app_cvbs_test_sync_level_change_callback;
	s_cvbs_test_item[ITEM_SYNC_LEVEL].itemStatus = ITEM_NORMAL;
}

static int app_cvbs_test_chroma_gain_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CHROMA_GAIN], "select", &box_sel);
	s_cvbs_test_info_para.chroma_gain= box_sel;
	return 0;
}

static void app_cvbs_test_chroma_gain_item_init(void)
{
	s_cvbs_test_item[ITEM_CHROMA_GAIN].itemTitle = STR_ID_CHROMA_GAIN;
	s_cvbs_test_item[ITEM_CHROMA_GAIN].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_CHROMA_GAIN].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_CHROMA_GAIN].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.chroma_gain;
	s_cvbs_test_item[ITEM_CHROMA_GAIN].itemCallback.cmbCallback.CmbChange= app_cvbs_test_chroma_gain_change_callback;
	s_cvbs_test_item[ITEM_CHROMA_GAIN].itemStatus = ITEM_NORMAL;
}

static int app_cvbs_test_cb_gain_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CB_GAIN], "select", &box_sel);
	s_cvbs_test_info_para.cr_gain= box_sel;
	return 0;
}

static void app_cvbs_test_cb_gain_item_init(void)
{
	s_cvbs_test_item[ITEM_CB_GAIN].itemTitle = STR_ID_VECTOR;
	s_cvbs_test_item[ITEM_CB_GAIN].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_CB_GAIN].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_CB_GAIN].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.cb_gain;
	s_cvbs_test_item[ITEM_CB_GAIN].itemCallback.cmbCallback.CmbChange= app_cvbs_test_cb_gain_change_callback;
	s_cvbs_test_item[ITEM_CB_GAIN].itemStatus = ITEM_NORMAL;
}

static int app_cvbs_test_cr_gain_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CR_GAIN], "select", &box_sel);
	s_cvbs_test_info_para.cr_gain= box_sel;
	return 0;
}

static void app_cvbs_test_cr_gain_item_init(void)
{
	s_cvbs_test_item[ITEM_CR_GAIN].itemTitle = STR_ID_CR_GAIN;
	s_cvbs_test_item[ITEM_CR_GAIN].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_CR_GAIN].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_CR_GAIN].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.cr_gain;
	s_cvbs_test_item[ITEM_CR_GAIN].itemCallback.cmbCallback.CmbChange= app_cvbs_test_cr_gain_change_callback;
	s_cvbs_test_item[ITEM_CR_GAIN].itemStatus = ITEM_NORMAL;
}


static int app_cvbs_test_choma_sync_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CHOMA_SYNC], "select", &box_sel);
	s_cvbs_test_info_para.choma_sync = box_sel;
	return 0;
}

static void app_cvbs_test_choma_sync_item_init(void)
{
	s_cvbs_test_item[ITEM_CHOMA_SYNC].itemTitle = STR_ID_CHROMA_SYNC;
	s_cvbs_test_item[ITEM_CHOMA_SYNC].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_CHOMA_SYNC].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_CHOMA_SYNC].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.choma_sync;
	s_cvbs_test_item[ITEM_CHOMA_SYNC].itemCallback.cmbCallback.CmbChange = app_cvbs_test_choma_sync_change_callback;
	s_cvbs_test_item[ITEM_CHOMA_SYNC].itemStatus = ITEM_NORMAL;
}

static int app_cvbs_test_frequency_response_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_FRE_RESPONSE], "select", &box_sel);
	s_cvbs_test_info_para.choma_sync = box_sel;
	return 0;
}

static void app_cvbs_test_frequency_response_item_init(void)
{
	s_cvbs_test_item[ITEM_FRE_RESPONSE].itemTitle = STR_ID_CHROMA_SYNC;
	s_cvbs_test_item[ITEM_FRE_RESPONSE].itemType = ITEM_CHOICE;
	s_cvbs_test_item[ITEM_FRE_RESPONSE].itemProperty.itemPropertyCmb.content= ITEM_OFFSET_CONTENT;
	s_cvbs_test_item[ITEM_FRE_RESPONSE].itemProperty.itemPropertyCmb.sel = s_cvbs_test_info_para.fre_response;
	s_cvbs_test_item[ITEM_FRE_RESPONSE].itemCallback.cmbCallback.CmbChange = app_cvbs_test_frequency_response_change_callback;
	s_cvbs_test_item[ITEM_FRE_RESPONSE].itemStatus = ITEM_NORMAL;
}

static void app_cvbs_test_item_property(int sel, ItemProPerty *property)
{

	if(sel >= ITEM_CVBS_TEST_TOTAL)
		return;

	if(s_cvbs_test_opt.item[sel].itemType == ITEM_CHOICE)
	{
		if(property->itemPropertyCmb.content != NULL)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "content", property->itemPropertyCmb.content);
		}
		GUI_SetProperty(s_test_item_content_edit_cmb[sel], "select", &(property->itemPropertyCmb.sel));
	}
	else if(s_cvbs_test_opt.item[sel].itemType == ITEM_EDIT)
	{
		if(property->itemPropertyEdit.string != NULL)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "string", property->itemPropertyEdit.string);
		}
	}
	GUI_SetProperty(s_test_item_title[sel], "string", s_cvbs_test_opt.item[sel].itemTitle);
}

static void app_cvbs_set_disable_item(cvbsTestOptSel sel)
{
#define DISABLE_FORE_COLOR "[text_disable_color,text_disable_color,text_disable_color]"
	char *state = NULL;
	char *img = NULL;
	char *fore_color = NULL;

	if(s_cvbs_test_item[sel].itemStatus == ITEM_DISABLE)
	{
		state = "disable";
		fore_color = DISABLE_FORE_COLOR;
		img = IMG_BAR_HALF_UNFOCUS;

		GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", state);
		GUI_SetProperty(s_test_item_choice_back[sel], "img", img);
		GUI_SetProperty(s_test_item_title[sel], "forecolor", fore_color);
	}
}

static void app_cvbs_set_item_state_update(int sel, ItemDisplayStatus state)
{
	if(sel >= ITEM_CVBS_TEST_TOTAL)
		return;

	if(state == ITEM_HIDE)
	{
		//app_system_set_hide_item(sel);
	}
	else
	{
		app_cvbs_set_disable_item(sel);
	}
}

static int _app_cvbs_test_exit_popdlg_exit_cb(PopDlgRet ret)
{
	SyscvbsTestPara_t para_ret;

	if (POP_VAL_OK == ret)
	{
		memset(&para_ret,0,sizeof(para_ret));
		app_cvbs_test_result_para_get(&para_ret);
		app_cvbs_test_para_save(&para_ret);
	}
    else
    {
        app_cvbs_test_fixer_reset();
    }
	GUI_EndDialog(WND_CVBS_TEST);
	return 0 ;
}

static int app_cvbs_test_exit_callback(ExitType exit_type)
{
	int ret = 0;
	SyscvbsTestPara_t para_ret;

	memset(&para_ret,0,sizeof(para_ret));
	app_cvbs_test_result_para_get(&para_ret);

	if(app_cvbs_test_para_change(&para_ret) == TRUE)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_SAVE_INFO;
		pop.exit_cb = _app_cvbs_test_exit_popdlg_exit_cb;
		popdlg_create(&pop);
		ret = 1;
	}

	return ret;
}


static void app_cvbs_test_focus_item(int sel)
{
#define FT_FOCUS_FORE_COLOR "[text_focus_color,text_color,text_disable_color]"
	if(s_cvbs_test_opt.item[sel].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(s_test_item_choice_back[sel], "img", IMG_BAR_HALF_FOCUS_L_IP);
		GUI_SetProperty(s_test_item_title[sel], "string", s_cvbs_test_opt.item[sel].itemTitle);
		GUI_SetProperty(s_test_item_title[sel], "forecolor", FT_FOCUS_FORE_COLOR);

		if(s_cvbs_test_opt.item[sel].itemType == ITEM_CHOICE)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "focus");
		}
		else if(s_cvbs_test_opt.item[sel].itemType == ITEM_EDIT)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "focus");
		}
		else if(s_cvbs_test_opt.item[sel].itemType == ITEM_PUSH)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "focus");
		}
		s_CvbsTestSel = sel;

	}
}


static void app_cvbs_test_unfocus_item(int sel)
{
#define FT_UNFOCUS_FORE_COLOR "[text_color,text_color,text_disable_color]"
	if(s_cvbs_test_opt.item[sel].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(s_test_item_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
		GUI_SetProperty(s_test_item_title[sel], "string", s_cvbs_test_opt.item[sel].itemTitle);
		GUI_SetProperty(s_test_item_title[sel], "forecolor", FT_UNFOCUS_FORE_COLOR);
	}
}

static int app_cvbs_test_init(SystemSettingOpt *settingOpt)
{
	int sel;
	int n_item_total  = 0;

	if(settingOpt == NULL)
		return -1;

	GUI_CreateDialog(WND_CVBS_TEST);
	GUI_SetProperty("text_cvbs_test_title", "string", settingOpt->menuTitle);

	if(settingOpt->itemNum > ITEM_CVBS_TEST_TOTAL)
	{
		n_item_total = ITEM_CVBS_TEST_TOTAL;
	}
	else
	{
		n_item_total = settingOpt->itemNum;
	}

	for(sel = 0; sel < n_item_total; sel++)
	{
		app_cvbs_test_item_property(sel, &settingOpt->item[sel].itemProperty);
		app_cvbs_set_item_state_update(sel, settingOpt->item[sel].itemStatus);

	}
	s_CvbsTestSel = ITEM_SYNC_LEVEL;
	app_cvbs_test_focus_item(s_CvbsTestSel);

	return 0;
}


static cvbsTestOptSel app_cvbs_test_set_next_sel(cvbsTestOptSel cur_sel)
{
	cvbsTestOptSel sel = cur_sel;

	while(1)
	{
		(sel + 1 == s_cvbs_test_opt.itemNum) ? sel = ITEM_SYNC_LEVEL : sel++;
		if((s_cvbs_test_opt.item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}

	}
}

static cvbsTestOptSel app_cvbs_test_set_last_sel(cvbsTestOptSel cur_sel)
{
	cvbsTestOptSel sel = cur_sel;
	while(1)
	{
		(sel  == ITEM_SYNC_LEVEL) ? sel = s_cvbs_test_opt.itemNum - 1 : sel--;
		if((s_cvbs_test_opt.item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}
	}
}


static void app_cvbs_test_update_all_edit_para(void)
{
	int i = 0;

	for(i = 0; i < s_cvbs_test_opt.itemNum; i++)
	{
		app_cvbs_test_item_data_update(i);
	}

	return;
}

void app_cvbs_test_menu_exec(void)
{
	memset(&s_cvbs_test_opt, 0 ,sizeof(s_cvbs_test_opt));
	app_test_system_para_get(CVBS_DEFAULT_VALUE);

	s_cvbs_test_opt.menuTitle = STR_ID_CVBS_TEST;
	s_cvbs_test_opt.itemNum = ITEM_CVBS_TEST_TOTAL;
	s_cvbs_test_opt.item = s_cvbs_test_item;

	app_cvbs_test_bar_level_item_init();
	app_cvbs_test_sync_level_item_init();
	app_cvbs_test_chroma_gain_item_init();
	app_cvbs_test_cb_gain_item_init();
	app_cvbs_test_cr_gain_item_init();
	app_cvbs_test_choma_sync_item_init();
    app_cvbs_test_frequency_response_item_init();

	app_cvbs_test_edit_item_init();

	s_cvbs_test_opt.exit = app_cvbs_test_exit_callback;
	app_cvbs_test_init(&s_cvbs_test_opt);

	return;
}


void app_cvbs_index_test_para_init(void)
{
    GxVoutMacrovision params;

    GxVoutHAL_ChannelGetMacrovision(GX_VIDEO_OUTPUT_RCA,&params);
    params.data[SYSTEM_625].mode = GX_MACROVISION_MODE_TYPE0;
    params.data[SYSTEM_525].mode = GX_MACROVISION_MODE_TYPE0;
    GxVoutHAL_ChannelSetMacrovision(GX_VIDEO_OUTPUT_RCA,&params);

    app_cvbs_test_init_para_status();
    app_cvbs_import_fixer_data(CVBS_INDEX_RAW);
    GxVoutHAL_VideoPerformanceFixerInit();
}

// FOR CVBS Autotest
void app_cvbs_autotest_init(int is_576)
{
    GxMsgProperty_PlayerVideoModeConfig  mode_config;
    int32_t init_value = 0;
    /// HDMI output
    GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
    if(((init_value != GX_VIDEO_OUTPUT_576I) && (1 == is_576))
            || ((init_value != GX_VIDEO_OUTPUT_480I) && (1 != is_576)))
    {
        if(1 == is_576)
            init_value = GX_VIDEO_OUTPUT_576I;
        else
            init_value = GX_VIDEO_OUTPUT_480I;

        GxBus_ConfigSetInt(TV_STANDARD_KEY,init_value);

        mode_config.interface = GX_VIDEO_OUTPUT_HDMI;
        mode_config.mode = init_value;
        GxPlayer_SetVideoOutputConfig(mode_config);
        /// RCA output
        mode_config.interface = GX_VIDEO_OUTPUT_RCA;
        mode_config.mode = app_av_get_cvbs_standard(init_value);
        if(GX_VIDEO_OUTPUT_MODE_MAX != mode_config.mode)
        {
            GxPlayer_SetVideoOutputConfig(mode_config);
        }
        app_cvbs_index_test_para_init();
    }
}


SIGNAL_HANDLER int app_cvbs_test_create(GuiWidget *widget, void *usrdata)
{
    GxVoutMacrovision params;
    app_cvbs_test_init_para_status();
    GxVoutHAL_ChannelGetMacrovision(GX_VIDEO_OUTPUT_RCA,&params);
    GUI_SetProperty(s_test_item_title[ITEM_RESET+1], "string", "Macrovision Status");
    if((params.data[SYSTEM_625].mode == GX_MACROVISION_MODE_TYPE0 )
            || (params.data[SYSTEM_525].mode == GX_MACROVISION_MODE_TYPE0))
    {
        GUI_SetProperty("btn_cvbs_test_opt9", "string", "Off");
    }
    else
    {
        GUI_SetProperty("btn_cvbs_test_opt9", "string", "On");
    }

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cvbs_test_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cvbs_test_keypress(GuiWidget *widget, void *usrdata)
{
    static int s_dis_osd_status = 1;
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
    usb_check_state usb_state = USB_OK;
    PopDlg  pop;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;
		case GUI_MOUSEBUTTONDOWN:
			break;
		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_UP:
					app_cvbs_test_unfocus_item(s_CvbsTestSel);
					s_CvbsTestSel = app_cvbs_test_set_last_sel(s_CvbsTestSel);
					app_cvbs_test_focus_item(s_CvbsTestSel);
					break;
				case STBK_DOWN:
					app_cvbs_test_unfocus_item(s_CvbsTestSel);
					s_CvbsTestSel = app_cvbs_test_set_next_sel(s_CvbsTestSel);
					app_cvbs_test_focus_item(s_CvbsTestSel);
					break;
				case STBK_OK:
                    if(s_CvbsTestSel == ITEM_RESET)
                    {
                        app_cvbs_test_fixer_reset();
                        memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_WAIT;
						pop.str = STR_ID_RESET;
						pop.mode = POP_MODE_UNBLOCK;
                        pop.timeout_sec = 2;
						popdlg_create(&pop);
                    }
                    else if(s_CvbsTestSel == ITEM_CVBS_TEST_TOTAL)
                    {
                        app_cvbs_test_set_fix_value();
                    }
                    break;
                case STBK_BLUE:
                    if(s_dis_osd_status)
                    {
                        s_dis_osd_status = 0;
                        GUI_StopUpdate(true);
                        GUI_SetInterface("osd_disable", NULL);
                    }
                    else
                    {
                        s_dis_osd_status = 1;
                        GUI_SetInterface("osd_enable", NULL);
                        GUI_StopUpdate(false);
                    }
                    break;
				case STBK_RED:
					{
                        app_cvbs_text_import_data();
					}
					break;
				case STBK_EXIT:
				case STBK_MENU:
					app_cvbs_test_update_all_edit_para();
					if(s_cvbs_test_opt.exit != NULL)
					{
						if(s_cvbs_test_opt.exit(0) == 1)
						{
							break;
						}
					}

					GUI_EndDialog(WND_CVBS_TEST);
					break;

				case STBK_GREEN:
                    app_cvbs_test_get_vpf_para();
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.timeout_sec= 2;

                    usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
                    if(usb_state == USB_NO_SPACE)
                    {
                        pop.str = STR_ID_NO_SPACE;
                        popdlg_create(&pop);
                        break;
                    }
                    else if(usb_state == USB_READ_ONLY)
                    {
                        pop.str = STR_ID_NOWRITE_PERMISSIONS;
                        popdlg_create(&pop);
                        break;
                    }
                    else if(USB_ERROR == usb_state)
                    {
                        pop.str = STR_ID_INSERT_USB;
                        if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
                            popdlg_create(&pop);
                        break;
                    }
                    else if(USB_OK == usb_state)
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        if(app_cvbs_test_print_to_usb()==0)
                        {
                            app_cvbs_test_set_fix_value();
                            pop.str = STR_ID_SUCCESS;
                        }
                        else
                        {
                            pop.str = "Export fail";
                        }
                        pop.type = POP_TYPE_WAIT;
						pop.mode = POP_MODE_UNBLOCK;
                        pop.timeout_sec = 2;
						popdlg_create(&pop);
                    }
					break;
                case STBK_INFO:
                    if(s_CvbsTestSel == ITEM_RESET)
                    {
                        GxVoutHAL_VideoPerformanceFixerInit();
                        app_cvbs_test_fixer_reset();
                        memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_WAIT;
						pop.str = "Save as Default";
						pop.mode = POP_MODE_UNBLOCK;
                        pop.timeout_sec = 2;
						popdlg_create(&pop);
                    }

                    break;
				default:
					if(STBK_0<= event->key.sym && STBK_9 >= event->key.sym)
					{
						app_cvbs_test_unfocus_item(s_CvbsTestSel);
						s_CvbsTestSel = event->key.sym - STBK_0 - 1;
						if(s_CvbsTestSel >= ITEM_CVBS_TEST_TOTAL-1 )
							s_CvbsTestSel = ITEM_CVBS_TEST_TOTAL -1;
						app_cvbs_test_focus_item(s_CvbsTestSel);
					}
					break;
			}

		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_cvbs_test_edit3_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_cvbs_test_opt.itemNum > ITEM_FRE_RESPONSE) && (s_cvbs_test_opt.item[ITEM_FRE_RESPONSE].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_cvbs_test_opt.item[ITEM_FRE_RESPONSE].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_cvbs_test_edit3_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_cvbs_test_opt.itemNum > ITEM_FRE_RESPONSE) && (s_cvbs_test_opt.item[ITEM_FRE_RESPONSE].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_FRE_RESPONSE], "string", &s);
		ret = s_cvbs_test_opt.item[ITEM_FRE_RESPONSE].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_cvbs_test_set_cmb1_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
    int select = 0;
    int max_value = 0;
    int min_value = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
        if((s_cvbs_test_opt.itemNum >= ITEM_SYNC_LEVEL) && ( s_cvbs_test_opt.item[s_CvbsTestSel].itemCallback.cmbCallback.CmbChange!= NULL))
		{
			//ret = s_cvbs_test_opt.item[s_CvbsTestSel].itemCallback.cmbCallback.CmbPress(event->key.sym);
            s_key = find_virtualkey_ex(event->key.scancode,event->key.sym);
            //if(s_CvbsTestSel == ITEM_FRE_RESPONSE)
            {
                if((s_key == STBK_RIGHT)||(s_key == STBK_LEFT))
                {
                    GUI_GetProperty(s_test_item_content_edit_cmb[s_CvbsTestSel], "select", &select);
                    if(s_key == STBK_RIGHT)
                    {
                        select = select+1;
                    }
                    else
                    {
                        select = select -1;
                    }
                    if(s_CvbsTestSel == ITEM_FRE_RESPONSE)
                    {
                        ret =app_cvbs_index_value_setup(_get_vpf_type(s_CvbsTestSel),select);
                    }
                    else if((s_CvbsTestSel == ITEM_SYNC_LEVEL)||(s_CvbsTestSel == ITEM_CHOMA_SYNC))
                    {
                        max_value = CVBS_DEFAULT_VALUE+21;
                        min_value = CVBS_DEFAULT_VALUE-21;
                    }
                    else
                    {
                        max_value = CVBS_DEFAULT_VALUE*2;
                        min_value = 0;
                    }
                    if((ret < 0)||(select == min_value)||(select == max_value ))
                    {
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                }
            }
        }
	}
	return ret;

}

SIGNAL_HANDLER int app_cvbs_test_cmb1_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    int sel = CVBS_DEFAULT_VALUE;

	if((s_cvbs_test_opt.itemNum >= ITEM_SYNC_LEVEL) && ( s_cvbs_test_opt.item[s_CvbsTestSel].itemCallback.cmbCallback.CmbChange!= NULL))
	{
	    if((s_key == STBK_RIGHT)||(s_key == STBK_LEFT))
        {
            GUI_GetProperty(s_test_item_content_edit_cmb[s_CvbsTestSel], "select", &sel);
            GUI_GetProperty(s_test_item_content_edit_cmb[s_CvbsTestSel], "select", &s_cvbs_test_opt.item[s_CvbsTestSel].itemProperty.itemPropertyCmb.sel);
            if(s_CvbsTestSel != ITEM_FRE_RESPONSE)
            {
                app_cvbs_index_value_setup(_get_vpf_type(s_CvbsTestSel),sel);
                if((sel == 0)||(sel == CVBS_DEFAULT_VALUE*2)
                        ||(app_cvbs_test_get_fixer_para(s_CvbsTestSel)==CVBS_FIXER_ERROR))
                {
                    sel = CVBS_DEFAULT_VALUE;
                    GUI_SetProperty(s_test_item_content_edit_cmb[s_CvbsTestSel], "select", &sel);
                    GxVoutHAL_VideoPerformanceFixerReset(_get_vpf_type(s_CvbsTestSel));
                }

            }
        }
        else
        {
            ret =  s_cvbs_test_opt.item[s_CvbsTestSel].itemCallback.cmbCallback.CmbChange(sel);
        }

	}
	return ret;
}

SIGNAL_HANDLER int app_cvbs_test_lost_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(s_test_item_content_edit_cmb[s_CvbsTestSel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cvbs_test_got_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(s_test_item_content_edit_cmb[s_CvbsTestSel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
	app_log_debug("_____ debug func:%s:line:%d\n",__FUNCTION__,__LINE__);

	return EVENT_TRANSFER_STOP;
}

#endif

