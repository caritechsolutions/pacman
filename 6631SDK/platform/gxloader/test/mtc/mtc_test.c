#include "stdio.h"
#include "mtc.h"
#include "time.h"
#include "sys/types.h"
#include "string.h"

#define MTC_DEST_LEN		50000
extern unsigned char decrypt_test_bin[];
extern unsigned int decrypt_test_bin_len;
static unsigned char *src_buf = (unsigned char *)decrypt_test_bin;
static unsigned char key[] = {0x88, 0x77, 0x66, 0x55, 0x00, 0x00, 0x00, 0x00,\
	0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00,\
		0x44, 0x33, 0x22, 0x11, 0x00, 0x00, 0x00, 0x00,\
		0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00};
static unsigned char iv[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,\
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char counter[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

unsigned char dest_buf[MTC_DEST_LEN] __attribute__ ((aligned(64))) = {0};
unsigned char dest_buf_extra[MTC_DEST_LEN] __attribute__ ((aligned(64))) = {0};

static int mtc_simple_test(int argc, const char *argv[])
{
	int ret = -1;
	int i = 0;
	struct mtc_data_s mtc_data;

	if (argc < 6)
		return ret;

	mtc_data.input.buf[0] = (unsigned char *)((uint32_t)src_buf);
	mtc_data.input.size[0] = decrypt_test_bin_len;
	mtc_data.input.num = 1;

	mtc_data.mtc_counter.value = counter;
	mtc_data.mtc_counter.len   = sizeof (counter);
	mtc_data.mtc_iv.iv_value = iv;
	mtc_data.mtc_iv.iv_len   = sizeof(iv);
	mtc_data.mtc_key.key_from = GXMTC_KEY_FROM_SOFT;
	mtc_data.mtc_key.key_value = key;
	mtc_data.mtc_key.key_len = 8;
	mtc_data.wait_mode = GXMTC_QUERY_MODE;

	if (strcmp(argv[2], "soft") == 0)
		mtc_data.mtc_key.key_from = GXMTC_KEY_FROM_SOFT;
	else if (strcmp(argv[2], "otp") == 0)
		mtc_data.mtc_key.key_from = GXMTC_KEY_FROM_OTP;
	else if (strcmp(argv[2], "calc") == 0)
		mtc_data.mtc_key.key_from = GXMTC_KEY_FROM_CALC;
	else if (strcmp(argv[2], "nds") == 0)
		mtc_data.mtc_key.key_from = GXMTC_KEY_FROM_NDS;
	else if (strcmp(argv[2], "mem") == 0)
		mtc_data.mtc_key.key_from = GXMTC_KEY_FROM_MEM;
	else
		return ret;

	if (strcmp(argv[3], "de") == 0)
		mtc_data.work_mode = GXMTC_DATA_DECRYPT_MODE;
	else if (strcmp(argv[3], "en") == 0)
		mtc_data.work_mode = GXMTC_DATA_ENCRYPT_MODE;
	else
		return ret;

	if (strcmp(argv[4], "cbc") == 0) {
		mtc_data.sub_mode = GXMTC_CBC;
	} else if (strcmp(argv[4], "ecb") == 0) {
		mtc_data.sub_mode = GXMTC_ECB;
	} else if (strcmp(argv[4], "cfb") == 0) {
		mtc_data.sub_mode = GXMTC_CFB;
	} else if (strcmp(argv[4], "ofb") == 0) {
		mtc_data.sub_mode = GXMTC_OFB;
	} else if (strcmp(argv[4], "ctr") == 0) {
		mtc_data.sub_mode = GXMTC_CTR;
	} else {
		printf("wrong parameters\n");
		return ret;
	}

	if (strcmp(argv[5], "des") == 0)
		mtc_data.algorithm = GXMTC_DES;
	else if (strcmp(argv[5], "3des") == 0) {
		mtc_data.algorithm = GXMTC_3DES;
		mtc_data.mtc_key.key_len = 24;
	} else if (strncmp(argv[5], "aes", 3) == 0) {
		if (strcmp(argv[5] + 3, "128") == 0) {
			mtc_data.algorithm = GXMTC_AES128;
			mtc_data.mtc_key.key_len = 16;
		} else if (strcmp(argv[5] + 3, "192") == 0) {
			mtc_data.algorithm = GXMTC_AES192;
			mtc_data.mtc_key.key_len = 24;
		} else if (strcmp(argv[5] + 3, "256") == 0) {
			mtc_data.algorithm = GXMTC_AES256;
			mtc_data.mtc_key.key_len = 32;
		} else {
			printf("aes wrong\n");
			return ret;
		}
	} else {
		printf("algo wrong\n");
		return ret;
	}

	memset(dest_buf, 0, sizeof(dest_buf));
	mtc_data.output.buf = (unsigned char *)((uint32_t)dest_buf);
	mtc_data.output.size = decrypt_test_bin_len;
	ret = gx_mtc_ioctl(MTC_DO_WORK, &mtc_data);

	if (ret < 0) {
		printf("error : %s %d\n", __func__, __LINE__);
		return ret;
	}

	printf("dest_buf : ");
	for (i = 0; i < 16; i++) {
		printf("0x%02x ", dest_buf[i]);
	}
	printf("\n");

	return ret;
}


#define MTC_TEST_RESULT_PRINT_LEN 100
#define K(s) (s * 1024)
#define M(s) (s * 1024 * 1024)

//static int target_size[] = {128, 512, K(1), K(2), K(4)};
static int target_size[] = {K(1), K(2), K(4), K(8), K(16), K(32)};
static int target_num = sizeof(target_size) / sizeof(int);

#if 0
static void mtc_print_result(enum mtc_work_mode work, enum mtc_algorithm algo, enum mtc_sub_mode sub, int op_size,\
		int time_used_us)
{
	char final_print[MTC_TEST_RESULT_PRINT_LEN] = {0};
	int before_time_len;
	char unit;
	int size;

	switch (work) {
	case GXMTC_DATA_ENCRYPT_MODE:
		strcat(final_print, "ENCRYPT");
		break;
	case GXMTC_DATA_DECRYPT_MODE:
		strcat(final_print, "DECRYPT");
		break;
	case GXMTC_CW_DECRYPT_MODE:
		strcat(final_print, "CW_DECRYPT");
		break;
	case GXMTC_CALC_KEY_GENERATE_MODE:
		strcat(final_print, "CALC_KEY");
		break;
	case GXMTC_NDS_KEY_GENERATE_MODE:
		strcat(final_print, "NDS_KEY");
		break;
	case GXMTC_MEM_KEY_GENERATE_MODE:
		strcat(final_print, "MEM_KEY");
		break;
	default :
		strcat(final_print, "xxxx");
		break;
	}

	switch (algo) {
	case GXMTC_DES:
		strcat(final_print, "-DES   ");
		break;
	case GXMTC_3DES:
		strcat(final_print, "-3DES  ");
		break;
	case GXMTC_AES128:
		strcat(final_print, "-AES128");
		break;
	case GXMTC_AES192:
		strcat(final_print, "-AES192");
		break;
	case GXMTC_AES256:
		strcat(final_print, "-AES256");
		break;
	default :
		strcat(final_print, "-xxxxxx");
		return ;
	}

	switch (sub) {
	case GXMTC_ECB:
		strcat(final_print, "-ECB");
		break;
	case GXMTC_CBC:
		strcat(final_print, "-CBC");
		break;
	case GXMTC_CFB:
		strcat(final_print, "-CFB");
		break;
	case GXMTC_OFB:
		strcat(final_print, "-OFB");
		break;
	case GXMTC_CTR:
		strcat(final_print, "-CTR");
		break;
	default :
		strcat(final_print, "-xxx");
		return ;
	}

	unit = (op_size / 1024) > 0 ? (op_size / (1024 * 1024) > 0 ? 'M' : 'K') : 'B';
	size = (op_size / 1024) > 0 ? (op_size / (1024 * 1024) > 0 ? op_size / (1024 * 1024) : op_size / 1024) : op_size;
	before_time_len = strlen(final_print);
	sprintf(final_print + before_time_len, "-size <%4d%c> used <%5d> us", size, unit, time_used_us);
	printf("%s\n", final_print);
}
#endif

static int common_data_decrypt_or_encrypt(enum mtc_work_mode work, enum mtc_algorithm al, \
		enum mtc_sub_mode sub, enum mtc_key_from key_from, enum mtc_wait_mode wait_mode, unsigned char *input_buf, unsigned char *out_buf, int op_size)
{
	int ret = -1;
	static struct mtc_data_s m_d;

	memset(&m_d, 0, sizeof(struct mtc_data_s));
	m_d.input.num         = 1;
	m_d.input.buf[0]      = input_buf;
	m_d.input.size[0]     = op_size;
	m_d.mtc_key.key_from  = key_from;
	m_d.mtc_key.key_value = key;
	m_d.mtc_key.key_len   = 8;
	m_d.mtc_iv.iv_value   = iv;
	m_d.mtc_iv.iv_len     = 8;
	m_d.mtc_counter.value = counter;
	m_d.mtc_counter.len   = 8;
	m_d.work_mode         = GXMTC_DATA_ENCRYPT_MODE;
	m_d.algorithm         = GXMTC_DES;
	m_d.sub_mode          = GXMTC_ECB;
	m_d.output.buf        = out_buf;
	m_d.output.size       = op_size;
	m_d.wait_mode         = wait_mode;

	m_d.work_mode = work;
	m_d.algorithm = al;
	switch (al) {
	case GXMTC_DES:
		break;
	case GXMTC_3DES:
		m_d.mtc_key.key_len = 24;
		break;
	case GXMTC_AES128:
		m_d.mtc_key.key_len = 16;
		m_d.mtc_iv.iv_len   = 16;
		break;
	case GXMTC_AES192:
		m_d.mtc_key.key_len = 24;
		m_d.mtc_iv.iv_len   = 16;
		break;
	case GXMTC_AES256:
		m_d.mtc_key.key_len = 32;
		m_d.mtc_iv.iv_len   = 16;
		break;
	default :
		printf("unknow algorithm\n");
		return ret;
	}

	m_d.sub_mode = sub;
	switch (sub) {
	case GXMTC_ECB:
		break;
	case GXMTC_CBC:
		break;
	case GXMTC_CFB:
		break;
	case GXMTC_OFB:
		break;
	case GXMTC_CTR:
		break;
	default :
		printf("unknow sub_mode\n");
		return ret;
	}

	unsigned long  t_s, t_e;
	t_s = gx_time_get_ms();
	ret = gx_mtc_ioctl(MTC_DO_WORK, &m_d);
	t_e = gx_time_get_ms();

	if (ret < 0)
		printf("error : %s %d\n", __func__, __LINE__);

	return ret;
	//return t_e - t_s < 0 ? (t_e - t_s + 1000 * 1000) : t_e - t_s;
}

static void mtc_complete_test_for_common_data(void)
{
	int algo_i, sub_i, key_i, wait_i, i, j;
	int ret = 0;

	for (sub_i = 0; sub_i <= GXMTC_CTR; sub_i++) {
		for (algo_i = 0; algo_i <= GXMTC_AES256; algo_i++) {
			for (key_i = 0; key_i <= GXMTC_KEY_FROM_OTP; key_i++) {
				for (wait_i = 0; wait_i <= GXMTC_INTERRUPT_MODE; wait_i++) {
					for (i = 0; i < target_num; i++) {
						memset(dest_buf, 0, sizeof(dest_buf));
						memset(dest_buf_extra, 0, sizeof(dest_buf_extra));
						ret = common_data_decrypt_or_encrypt(GXMTC_DATA_ENCRYPT_MODE, algo_i, sub_i,
								key_i, wait_i, (unsigned char *)((uint32_t)src_buf), (unsigned char *)((uint32_t)dest_buf), target_size[i]);
						if (-1 == ret) {
							printf("en failed %s %d: mtc_sub_mode: %d, mtc_algorithm: %d, mtc_key_from: %d, result get: %d, target_size: %d\n", __func__, __LINE__, sub_i, algo_i, key_i, wait_i, target_size[i]);
							return;
						}
						ret = common_data_decrypt_or_encrypt(GXMTC_DATA_DECRYPT_MODE, algo_i, sub_i,
								key_i, wait_i, (unsigned char *)((uint32_t)dest_buf), (unsigned char *)((uint32_t)dest_buf_extra), target_size[i]);
						if (-1 == ret) {
							printf("de failed %s %d: mtc_sub_mode: %d, mtc_algorithm: %d, mtc_key_from: %d, result get: %d, target_size: %d\n", __func__, __LINE__, sub_i, algo_i, key_i, wait_i, target_size[i]);
							return;
						}
						for (j =0 ;j < target_size[i]; j++) {
							if (src_buf[j] != dest_buf_extra[j]) {
								printf("en and de failed %s %d : mtc_sub_mode: %d, mtc_algorithm: %d, mtc_key_from: %d, result get: %d, target_size: %d\n", __func__, __LINE__, sub_i, algo_i, key_i, wait_i, target_size[i]);
								return;
							}
						}
						printf("en and de success: mtc_sub_mode: %d, mtc_algorithm: %d, mtc_key_from: %d, result get: %d, target_size: %d\n", sub_i, algo_i, key_i, wait_i, target_size[i]);
					}
				}
			}
		}
	}
}

static int do_mtc_test(int argc, const char *argv[])
{
	int ret = 0;

	gx_mtc_open();

	if (strcmp(argv[1], "com_test") == 0) {
		mtc_complete_test_for_common_data();
		ret = 0;
	} else
		ret = mtc_simple_test(argc, argv);

	gx_mtc_close();

	return ret;
}

void help(void)
{
	printf("Use examples:\n");
	printf("             mtc sim_test <soft/otp/calc/nds/mem> <de/en> <ecb/cbc/cfb/ofb/ctr>  <des/3des/aes128/aes192/aes256>\n");
	printf("             mtc com_test\n");
	printf("\n");
}

void mtc_test(int argc, const char *argv[])
{
	if (argc < 2) {
		help();
		return;
	}
	do_mtc_test(argc, argv);
}
