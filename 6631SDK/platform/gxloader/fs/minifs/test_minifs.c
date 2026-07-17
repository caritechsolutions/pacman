#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include "string.h"

#include "minifs/api_minifs.h"

#define DRAM_ADDR         0x90000000
#define MINIFS_START_ADDR 0x30000
#define MINIFS_END_ADDR   0x50000
#define MINIFS_TEST_FILE_OLD  "/minifs-file-old"
#define MINIFS_TEST_FILE_NEW  "/minifs-file-new"
#define MINIFS_TEST_DIR       "/minifs-dir"
#define MINIFS_TEST_DIR_FILE  "/minifs-dir/file"

#define BUFFER_LEN (MINIFS_END_ADDR - MINIFS_START_ADDR)

static char s_buffer[BUFFER_LEN];
static char d_buffer[BUFFER_LEN];

static void minifs_test(void)
{
	int ret = 0;
	int len = (BUFFER_LEN - 64*1024);
	minifs_handle_dev  device = NULL;
	minifs_handle_file test_file = NULL;

	memset(s_buffer, 0, BUFFER_LEN);
	memset(d_buffer, 0, BUFFER_LEN);
	memcpy((void *)s_buffer, (void *)DRAM_ADDR, BUFFER_LEN);

	gxflash_erasedata(MINIFS_START_ADDR, MINIFS_END_ADDR - MINIFS_START_ADDR);
	/************************* minifs mount test ***********************************/
	device = minifs_mount(MINIFS_START_ADDR, MINIFS_END_ADDR, NULL, "/", NULL, 0, NULL);
	if (!device) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_mount test success!\n");

	/************************* minifs open/close test ***********************************/
	test_file = minifs_open(device, MINIFS_TEST_FILE_OLD, MINIFS_CREAT, 0644);
	if (!test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_close(test_file);
	if (ret < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_open/close test success!\n");

	/************************* minifs write test ***********************************/
	test_file = minifs_open(device, MINIFS_TEST_FILE_OLD, 0, 0644);
	if (!test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_write(test_file, s_buffer, len, 0);
	if (ret != len) {
		printf("ret = %d\n", ret);
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_close(test_file);
	if (ret < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_write test success!\n");

	/************************* minifs read test ***********************************/
	test_file = minifs_open(device, MINIFS_TEST_FILE_OLD, 0, 0644);
	if (!test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	memset(d_buffer, 0, BUFFER_LEN);
	ret = minifs_read(test_file, d_buffer, len, 0);
	if (ret != len) {
		printf("ret = %d\n", ret);
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	if (strncmp(s_buffer, d_buffer, len) != 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_close(test_file);
	if (ret < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_read test success!\n");

	/************************* minifs rename test ***********************************/
	ret = minifs_rename(device, MINIFS_TEST_FILE_OLD, MINIFS_TEST_FILE_NEW);
	if (ret < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	test_file = minifs_open(device, MINIFS_TEST_FILE_OLD, 0, 0644);
	if (test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	test_file = minifs_open(device, MINIFS_TEST_FILE_NEW, 0, 0644);
	if (!test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	memset(d_buffer, 0, BUFFER_LEN);
	ret = minifs_read(test_file, d_buffer, len, 0);
	if (ret != len) {
		printf("ret = %d\n", ret);
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	if (strncmp(s_buffer, d_buffer, len) != 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_close(test_file);
	if (ret < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_rename test success!\n");

	/************************* minifs rmfile test ***********************************/
	ret = minifs_rmfile(device, MINIFS_TEST_FILE_NEW);
	if (ret == 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	test_file = minifs_open(device, MINIFS_TEST_FILE_NEW, 0, 0644);
	if (test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_rmfile test success!\n");

	/************************* minifs mkdir test ***********************************/
	test_file = minifs_open(device, MINIFS_TEST_DIR_FILE, MINIFS_CREAT, 0644);
	if (test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	if (minifs_mkdir(device, MINIFS_TEST_DIR, 0775) < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	test_file = minifs_open(device, MINIFS_TEST_DIR_FILE, MINIFS_CREAT, 0644);
	if (!test_file) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_write(test_file, s_buffer, len, 0);
	if (ret != len) {
		printf("ret = %d\n", ret);
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	memset(d_buffer, 0, BUFFER_LEN);
	ret = minifs_read(test_file, d_buffer, len, 0);
	if (ret != len) {
		printf("ret = %d\n", ret);
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	if (strncmp(s_buffer, d_buffer, len) != 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	ret = minifs_close(test_file);
	if (ret < 0) {
		printf("error: %s %d\n", __func__, __LINE__);
		return;
	}
	printf("minifs_mkdir test success!\n");

	/************************* minifs umount test ***********************************/
	minifs_umount(device);
	printf("minifs_umount test success!\n");

}

void Minifs_test(void)
{
	printf("\ntest minifs start ...\n\n");
	minifs_test();
	printf("\ntest minifs end ...\n");
}
