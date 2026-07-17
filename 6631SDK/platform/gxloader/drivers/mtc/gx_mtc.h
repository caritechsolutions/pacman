#ifndef _GX_MTC_
#define _GX_MTC_
#include "interrupt.h"

enum mtc_key_from {
	GXMTC_KEY_FROM_SOFT,
	GXMTC_KEY_FROM_OTP,
	GXMTC_KEY_FROM_CALC,
	GXMTC_KEY_FROM_NDS,
	GXMTC_KEY_FROM_MEM,
};

enum mtc_work_mode {
	GXMTC_DATA_ENCRYPT_MODE,
	GXMTC_DATA_DECRYPT_MODE,
	GXMTC_CW_DECRYPT_MODE,
	GXMTC_CALC_KEY_GENERATE_MODE,
	GXMTC_NDS_KEY_GENERATE_MODE,
	GXMTC_MEM_KEY_GENERATE_MODE,
};

enum mtc_algorithm {
	GXMTC_DES,
	GXMTC_3DES,
	GXMTC_AES128,
	GXMTC_AES192,
	GXMTC_AES256,
	GXMTC_MULTI2,
	GXMTC_SMS4,
};

struct mtc_capability {
    unsigned short have_mtc;     /* 1 : have       0 : don't have */
    unsigned short algo_sup;
#define GXMTC_CAP_DES    0x0001
#define GXMTC_CAP_3DES   0x0002
#define GXMTC_CAP_AES128 0x0010
#define GXMTC_CAP_AES192 0x0020
#define GXMTC_CAP_AES256 0x0040
#define GXMTC_CAP_MULTI2 0x0100
#define GXMTC_CAP_SMS4   0x1000
};
enum mtc_sub_mode {
	GXMTC_ECB,
	GXMTC_CBC,
	GXMTC_CFB,
	GXMTC_OFB,
	GXMTC_CTR,
};

enum mtc_desc_type {
	GXMTC_DESC_TYPE_CAS2,
	GXMTC_DESC_TYPE_DES,
};

enum mtc_cw_odd_even {
	GXMTC_CW_ODD_FIRST,
	GXMTC_CW_EVEN_FIRST,
};

enum mtc_wait_mode {
	GXMTC_QUERY_MODE,
	GXMTC_INTERRUPT_MODE,
};

struct mtc_key_s {
	enum mtc_key_from key_from;
	unsigned char *key_value;
	int key_len;
};

struct mtc_iv_s {
	unsigned char *iv_value;
	int iv_len;
};

struct mtc_counter_s {
	unsigned char *value;
	int  len;
};

enum mtc_residue_msg {
	MTC_RES_CLEAR,
	MTC_RES_CTS,
	MTC_RES_RBT,
	//other is CLEAR
};

enum mtc_short_msg {
	MTC_SHO_CLEAR,
	MTC_SHO_IV,
	MTC_SHO_IV2,
	// other is CLEAR
};

#define MTC_INPUT_MAX_NUM	5
struct mtc_input_s {
	int num;
	unsigned char *buf[MTC_INPUT_MAX_NUM];
	int size[MTC_INPUT_MAX_NUM];
};

struct mtc_output_s {
	unsigned char *buf;
	int size;
};

struct mtc_data_s {
	struct mtc_input_s      input;
	struct mtc_key_s        mtc_key;
	struct mtc_iv_s         mtc_iv;
	struct mtc_counter_s    mtc_counter;
	enum mtc_work_mode      work_mode;
	enum mtc_algorithm      algorithm;
	enum mtc_sub_mode       sub_mode;
	enum mtc_desc_type      desc_type;
	enum mtc_cw_odd_even    odd_even;
	struct mtc_output_s     output;
	enum mtc_wait_mode      wait_mode;
	enum mtc_short_msg      sht_msg;
	enum mtc_residue_msg    res_msg;
};

/*
 *  key table 最多存储 8 个key
 *  nds control unit 最多存储 8 个key
 *  每个 key table 跟每个 nds control unit 一一对应，即一个 nds key 对应一个 nds 配置
 * +-----------------------------------------------------------------------------------+
 * | control unit :  | control unit :  | control unit :  |           | control unit :  | 
 * | con_unit_addr 0 | con_unit_addr 1 | con_unit_addr 2 |           | con_unit_addr 7 |
 * | decrypt/encrypt | decrypt/encrypt | decrypt/encrypt |           | decrypt/encrypt |
 * | algo            | algo            | algo            |           | algo            |
 * | sub_mode        | sub_mode        | sub_mode        |           | sub_mode        |
 * | residue_msg     | residue_msg     | residue_msg     |    ...    | residue_msg     |
 * | short_msg       | short_msg       | short_msg       |           | short_msg       |
 * +-----------------------------------------------------------------------------------+
 * |  nds key table  |  nds key table  |  nds key table  |           |  nds key table  |
 * |   key_addr : 0  |   key_addr : 1  |   key_addr : 2  |           |   key_addr : 7  |
 * |    nds key      |    nds key      |    nds key      |    ...    |    nds key      |
 * |                 |                 |                 |           |                 |
 * +-----------------------------------------------------------------------------------+
 *
 * con_unit_addr : nds control unit write addr
 * NDS SUPPORT : 
 *  (1) algorithm  : GXMTC_DES | GXMTC_3DES | GXMTC_AES128 | GXMTC_AES192 | GXMTC_AES256
	(2) sub_mode   : GXMTC_ECB | GXMTC_CBC | GXMTC_CTR,
	(3) residue_msg: MTC_RES_CLEAR | MTC_RES_CTS | MTC_RES_RBT,
	(4) short_msg  : MTC_SHO_CLEAR | MTC_SHO_IV | MTC_SHO_IV2,
 */

#define MTC_DO_WORK                       0
#define MTC_GET_CAP                       1
enum interrupt_return mtc_interrupt_isr(unsigned int vector, void *pdata);
int gx_mtc_nds_key_generate(struct mtc_data_s *mtc_data);
int gx_mtc_core_nds_decrypt(struct mtc_data_s *mtc_data);
int gx_mtc_core_nds_encrypt(struct mtc_data_s *mtc_data);
int gx_mtc_core_data_decrypt(struct mtc_data_s *mtc_data);
int gx_mtc_core_data_encrypt(struct mtc_data_s *mtc_data);
int gx_mtc_core_otp_decrypt(struct mtc_data_s *mtc_data);
int gx_mtc_core_otp_encrypt(struct mtc_data_s *mtc_data);
int mtc_encrypt_and_decrypt(struct mtc_data_s *mtc_data);
int gx_mtc_core_cw_decrypt(struct mtc_data_s *mtc_data);

int gx3113c_mtc_core_data_decrypt(struct mtc_data_s *mtc_data);
int gx3113c_mtc_core_data_encrypt(struct mtc_data_s *mtc_data);
int gx3113c_mtc_core_cw_decrypt(struct mtc_data_s *mtc_data);

typedef enum {
	GXMTC_DEFAULT_STATUS            = 0x0,
	GXMTC_DATA_FINISH               = 0x1,
	GXMTC_K3_FINISH                 = 0x2,
	GXMTC_AES_ROUND_FINISH          = 0x4,
	GXMTC_TDES_ROUND_FINISH         = 0x8,
	GXMTC_NDS_OPERATE_ERROR         = 0x10,
	GXMTC_M2MKEY_RW_SPACE_ERROR     = 0x20,
	GXMTC_MEMORY_KEY_RD_FINISH      = 0x40,
	GXMTC_MEMORY_KEY_RD_SPACE_ERROR = 0x80,
} mtc_intr_status;

typedef enum {
	GXMTC_NOERR   = 0,
	GXMTC_TIMEOUT = 1,
	GXMTC_ERROR   = 2,
} mtc_final_result;

typedef struct gx_mtc_extra {
	unsigned int    isrvec;
	int             sync_flag;
	mtc_final_result       result;
} gx_mtc_extra;

extern gx_mtc_extra mtc_extra;

#define _DBG_MTC_
#ifdef  _DBG_MTC_
#define MTC_DBG(...) printf("[MTC] DEBUG : "__VA_ARGS__)
#else
#define MTC_DBG(...)
#endif // _DBG_MTC_
#define DO_ENCRYPT 1
#define DO_DECRYPT 0

#define HANDLE_ERROR(result) do {\
    if (result != GXMTC_NOERR)   \
        return -EIO;             \
    else                         \
        return 0;                \
} while (0)

#define MTC_WAIT_TIMEOUT_MS 3000

#endif
