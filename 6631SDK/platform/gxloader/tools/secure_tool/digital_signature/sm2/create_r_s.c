#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/ecdsa.h>

#define R_S_LENGTH (32)

static void help(void)
{
	printf("\n./create_r_s <sig_file> <r_file> <s_file>\n");
	printf("    sig_file : the input sign file\n");
	printf("    x_file   : the output r file\n");
	printf("    y_file   : the output s file\n\n");
}

int main(int argc, char **argv)
{
	int ret = -1;
	FILE *fp = NULL;
	char *input_file = NULL;
	char *output_r_file = NULL;
	char *output_s_file = NULL;
	const BIGNUM *sig_r = NULL;
	const BIGNUM *sig_s = NULL;
	char *str_r = NULL;
	char *str_s = NULL;
	unsigned int str_r_len = 0;
	unsigned int str_s_len = 0;
	ECDSA_SIG *sm2sig = NULL;
	const unsigned char *p = NULL;
	unsigned int sig_len = 0;
	unsigned char *sig = NULL;

	if (argc < 4) {
		help();
		goto exit;
	}
	input_file = argv[1];
	output_r_file = argv[2];
	output_s_file = argv[3];

	fp=fopen(input_file, "rb");
	if (fp == NULL) {
		printf("error: %s %d\n", __FILE__, __LINE__);
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	sig_len = ftell(fp);
	sig = (char *)malloc(sig_len);
	if(sig == NULL) {
		printf("error: %s %d\n", __FILE__, __LINE__);
		fclose(fp);
		fp = NULL;
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	fread(sig, sig_len, 1, fp);
	fclose(fp);
	fp = NULL;

	p = sig;
	if (!(sm2sig = d2i_ECDSA_SIG(NULL, &p, sig_len))) {
		printf("error: %s %d\n", __FILE__, __LINE__);
		goto exit;
	}
	ECDSA_SIG_get0(sm2sig, &sig_r, &sig_s);
	str_r = BN_bn2hex(sig_r);
	str_s = BN_bn2hex(sig_s);

	str_r_len = strlen(str_r);
	str_s_len = strlen(str_s);
	if (((str_r_len / 2) != R_S_LENGTH) && ((str_s_len / 2) != R_S_LENGTH)) {
		printf("error: %s %d\n", __FILE__, __LINE__);
		goto exit;
	}

	fp = fopen(output_r_file, "wb");
	if (fp == NULL) {
		printf("error: %s %d\n", __FILE__, __LINE__);
		goto exit;
	}
	fwrite(str_r, str_r_len, 1, fp);
	fclose(fp);
	fp = NULL;

	fp = fopen(output_s_file, "w+");
	if (fp == NULL) {
		printf("error: %s %d\n", __FILE__, __LINE__);
		goto exit;
	}
	fwrite(str_s, str_s_len, 1, fp);
	fclose(fp);
	fp = NULL;

	ret = 1;
exit:
	if (sig) {
		free(sig);
		sig = NULL;
	}
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
	if (sm2sig) {
		ECDSA_SIG_free(sm2sig);
		sm2sig = NULL;
	}
	if (str_r) {
		OPENSSL_free(str_r);
		str_r = NULL;
	}
	if (str_s) {
		OPENSSL_free(str_s);
		str_s = NULL;
	}

	return ret;
}
