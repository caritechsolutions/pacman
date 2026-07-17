#ifndef __chiptest_module_h__
#define __chiptest_module_h__

typedef enum service_id {
	E_MACROVISION,
} ServiceID;

typedef enum cmd_id {
	/*pc send*/
	ACP_SET_PARAM,
	ACP_GET_PARAM,
	ACP_DUMP_REG,
	/*pc receive*/
	ACP_GET_PARAM_ACK,
	ACP_DUMP_REG_ACK,
} AcpCmdID;

typedef enum vout_standard {
	NTSC_525I,
	NTSC_525P,
	NSSC_443 ,
	PAL_M    ,
	PAL_60   ,
	PAL_625I ,
	PAL_625P ,
	PAL_N    ,
	PAL_NC   ,
	STD_BUTT ,
} VoutStandard;

typedef enum pic_ratio {
	E_286_714,
	E_300_700,
} PicRatio;

typedef enum setup_choice {
	E_75_IRE,
	E_00_IRE,
} SetupChoice;

typedef struct {
	unsigned char cpc[2];
	unsigned char cps[33];
} CpsFormatParam;

typedef struct {
	unsigned int n[23];
} ApsFormatParam;

typedef struct dump_reg {
	unsigned vout0_reg[64];
	unsigned vout1_reg[64];
	unsigned vpu_reg[64];
	unsigned svpu_reg[64];
} DumpRegParam;

struct acp_cmd_param {
	unsigned char  acp_on;      /*set param*/
	VoutStandard   standard;    /*set param*/
	PicRatio       pic_ratio;   /*set_param*/
	SetupChoice    setup_choice;/*set_param*/
	CpsFormatParam acp_param;   /*set_param & get_param*/
	DumpRegParam   regs;        /*dump_reg*/
};

typedef struct acp_cmd_param MacvParam;
typedef struct acp_cmd_param AcpCmdParam;

typedef struct acp_cmd {
	ServiceID   service_id;
	AcpCmdID    cmd_id;
	AcpCmdParam param;
} AcpCmd;

#endif

