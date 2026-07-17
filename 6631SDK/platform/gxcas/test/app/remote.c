#include "gxcore.h"
#include "porting.h"
#include "sys/time.h"
#include "sys/select.h"

#define KEY_UP (0xbb77)
#define KEY_DOWN (0xbbd7)
#define KEY_0 (0xbbff)
#define KEY_1 (0xbb7f)
#define KEY_2 (0xbbbf)
#define KEY_3 (0xbb3f)
#define KEY_4 (0xbbdf)
#define KEY_5 (0xbb5f)
#define KEY_6 (0xbb9f)
#define KEY_7 (0xbb1f)
#define KEY_8 (0xbbef)
#define KEY_9 (0xbb6f)

int32_t remote_fd;

void remote_deal(void *arg)
{
	fd_set fds;
	struct timeval timeout = {0};

	while(1) {
		FD_ZERO(&fds);
		FD_SET(remote_fd, &fds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 150000;
		if (select(remote_fd + 1, &fds, NULL, NULL, &timeout) > 0) {
			if (FD_ISSET(remote_fd, &fds)) {
				unsigned int code = 0;
				int32_t ret = 0;

				ret = GxCore_Read(remote_fd, &code, sizeof(code), 1);
				if (ret <= 0)
					continue;
				code = (((code >> 24) & 0xff) << 8) | ((code >> 8) & 0xff);
				switch(code) {
					case KEY_UP:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEYUP);
						break;
					case KEY_DOWN:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEYDOWN);
						break;
					case KEY_0:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY0);
						break;
                                        case KEY_1:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY1);
						break;
                                        case KEY_2:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY2);
						break;
                                        case KEY_3:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY3);
						break;
                                        case KEY_4:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY4);
						break;
                                        case KEY_5:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY5);
						break;
                                        case KEY_6:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY6);
						break;
                                        case KEY_7:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY7);
						break;
                                        case KEY_8:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY8);
						break;
                                        case KEY_9:
						GxCas_CaTest_KeyPress(GXCAS_TEST_KEY9);
						break;
					default:
						break;
				}
			}
		}
	}
}

int remote_init()
{
	handle_t remote;
	int32_t ret = 0;

	remote_fd = GxCore_Open("/dev/gxirr0", "r");
	if (remote_fd < 0)
		return -1;
	ret = GxCore_ThreadCreate("remote process", &remote, remote_deal, NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
	if (ret != GXCORE_SUCCESS)
		return -1;
}
