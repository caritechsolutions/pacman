/*
 * =====================================================================================
 *
 *       Filename:  common_commands.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  10/28/2014 05:04:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Z.Z.R
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include <stddef.h>
#include <stdio.h>

#include "../include/module_command.h"
#include "../include/module_device.h"
#include "../include/common_autotest.h"
#include "app_config.h"
#include "module/app_log.h"
#if PCDEMO_TEST_ENABLE
#include "frontend/fe_property.h"
#endif
#define APP_AUTOTEST_VERSION "1.0.0"


unsigned int g_ver1=0, g_ver2=0, g_ver3=0;
static dev_tbl_t *dev_ops = NULL;
static unsigned char sg_thread_flag = 1;
static unsigned char sg_set_tp_flag = 0;
static unsigned int sg_lock_time = 0;
static handle_t sg_mutex_id = 0;
static GxTime sg_time_stamp;
static int status_thread_id = 0;

static void _frontend_read_status(void *arg)
{
	GxTime time_stamp;

	GxCore_ThreadDetach();
	while(sg_thread_flag){
		GxCore_MutexLock(sg_mutex_id);
		if(sg_set_tp_flag){
			if(get_lock_status() > 0){
				sg_set_tp_flag = 0;
				GxCore_GetTickTime(&time_stamp);
				sg_lock_time = (time_stamp.seconds*1000+time_stamp.microsecs/1000) - (sg_time_stamp.seconds*1000+sg_time_stamp.microsecs/1000);
			}
		}
		GxCore_MutexUnlock(sg_mutex_id);
		GxCore_ThreadDelay(20);
	}
}

static signed int do_get_version(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
	char result[10] = {0};

	if((argc > cmdtp->maxargs) || (argc < 2)){
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	sscanf(argv[1],"%*[^=]=%d.%d.%d", &g_ver1, &g_ver2, &g_ver3);
	if(g_ver1 < 1){
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: hat_version is too old!";
		strcat(result, "(0) ");
		strcat(result, APP_AUTOTEST_VERSION);
		goto err;
	}

	strcat(result, "(1) ");
	strcat(result, APP_AUTOTEST_VERSION);
	pResult = "successful";
	auto_test_reply(cmdtp->name, pResult, result_code, result);
	return 0;
err:
	auto_test_reply(cmdtp->name, pResult, result_code, result);
	return -1;
}

static signed int do_enable_crccheck(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(argc > cmdtp->maxargs){
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
    g_ver1 = 100;// enable crc check
	pResult = "successful";
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}


static signed int do_set_mode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
    char dev_name[10] = {0};
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if((argc > cmdtp->maxargs) || (argc < 2))
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	sscanf(argv[1],"%*[^=]=%s",dev_name);
	if(strlen(dev_name) == 0)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: demod type is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	dev_ops = find_dev(dev_name);
	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: the demod type is wrong or not register!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(status_thread_id == 0){
		GxCore_MutexCreate(&sg_mutex_id);
		GxCore_ThreadCreate("time_autotest", &status_thread_id, _frontend_read_status, NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY-1);
	}

	pResult = "successful";
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	// TODO: reply lock status
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}


static signed int do_set_tp(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	void *tp_info = NULL;
	void *frontend_info = NULL;
	signed int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(dev_ops->parse == NULL)
	{
		// no function for parse
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no parse function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	ret = dev_ops->parse(orgcmd, &frontend_info, &tp_info);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parse command failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	// init frontend parameter: device, demux, and so on.
	ret = init_frontend(frontend_info);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: initialize frontend failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}

	if(dev_ops->lock_tp == NULL)
	{
		// no function for set tp
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no set tp function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	GxCore_MutexLock(sg_mutex_id);
	GxCore_GetTickTime(&sg_time_stamp);
	sg_set_tp_flag = 0;
	sg_lock_time = 0;
	GxCore_MutexUnlock(sg_mutex_id);

	ret = dev_ops->lock_tp(tp_info);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: set tp failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	GxCore_MutexLock(sg_mutex_id);
	sg_set_tp_flag = 1;
	GxCore_MutexUnlock(sg_mutex_id);

	pResult = "successful";
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	// TODO: reply lock status
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}

static signed int do_set_destroy_friontend(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	//char buffer[100] = {0};
	int ret = 0;
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	sg_thread_flag = 0;
	GxCore_ThreadJoin(status_thread_id);//wait for read_status thread exit
	GxCore_MutexDelete(sg_mutex_id);
	status_thread_id = 0;
	sg_set_tp_flag = 0;
	sg_thread_flag = 1;

	ret = destroy_frontend();
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: destroy frontend failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	pResult = "successful";
	// TODO: reply lock status
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}


static signed int do_get_lock_status(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int lock_status = 0;
	char *pResult = NULL;
	char buffer[100] = {0};
	char result[10] = {0};
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	lock_status = get_lock_status();
	if(lock_status < 0){
		pResult = "failed: get lock status failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	if(lock_status > 0){
		sprintf(buffer, "%s: %s %s[%d]", "successful", "locked", "lock_time", sg_lock_time);
		strcat(result, "(1)");
	}else{
		sprintf(buffer, "%s: %s", "successful", "unlocked");
		strcat(result, "(0)");
	}

	pResult = buffer;
	// TODO: reply lock status
	auto_test_reply(cmdtp->name, pResult, result_code, result);
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}


static signed int do_set_play(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	void *prog_info = NULL;
	signed int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(dev_ops->parse == NULL)
	{
		// no function for parse
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no parse function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	ret = dev_ops->parse(orgcmd, &prog_info, NULL);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parse command failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	// init play parameter
	ret = init_play(prog_info);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: initialize play failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}

	if(dev_ops->play == NULL)
	{
		// no function for play
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no play function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	ret = dev_ops->play(prog_info);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: play program failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	pResult = "successful";
	// TODO: replay the ts compare result
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}


static signed int do_set_ts_compare(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	void *ts_info = NULL;
	signed int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(dev_ops->parse == NULL)
	{
		// no function for parse
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no parse function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	ret = dev_ops->parse(orgcmd, &ts_info, NULL);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parse command failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = compare_ts_data(ts_info);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: set ts data compare failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	pResult = "successful";
	// TODO: replay the ts compare result
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}


static signed int do_get_ts_compare_result(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ReceivePackageCount = 0, ErrorPackageCount = 0, ret = 0;
	char *pResult = NULL;
	char buffer[256] = {0};
	int lock_status = 0;
	unsigned int result_code = IS_SUCCESSFUL;
	char result[50] = {0};

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = get_ts_compare_result(&ReceivePackageCount,&ErrorPackageCount);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: get ts compare result failed! maybe the function is not work";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// get lock status
	lock_status = get_lock_status();
	if(lock_status < 0)
	{
		pResult = "failed: get lock status failed! maybe the frontend device has been destroyed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	if(lock_status > 0)
		sprintf(buffer, "%s: %s(lock status) %d(receive packages) %d(error packages)", "successful", "locked", ReceivePackageCount, ErrorPackageCount);
	else
		sprintf(buffer, "%s: %s(lock status) %d(receive packages) %d(error packages)", "successful", "unlocked", ReceivePackageCount, ErrorPackageCount);

	sprintf(result, "(%d,%d,%d)", lock_status, ReceivePackageCount, ErrorPackageCount);
	pResult = buffer;
	// TODO: replay the ts compare result
	auto_test_reply(cmdtp->name, pResult, result_code, result);
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_set_play_stop(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = stop_play();
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: stop play failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	pResult = "successful";
	// TODO: replay the ts compare result
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}

static signed int do_set_ts_compare_stop(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = stop_ts_compare();
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: stop compare ts failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	pResult = "successful";
	// TODO: replay the ts compare result
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}

static signed int do_set_all_stop(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
	if(dev_ops == NULL)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: do not select a vaild demod mode! set mode first!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = stop_all();
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: stop all functions failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	dev_ops = NULL;
	pResult = "successful";
	// TODO: replay the ts compare result
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}

static signed int do_set_factory_reset(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;

	if(argc > cmdtp->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	pResult = "successful";
	// TODO: replay the factory reset result
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	factory_reset();
	return 0;
err:
	// TODO: failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;

}

#if PCDEMO_TEST_ENABLE
#define AUTOTEST_PCDEMO_MODE_DEMOD_ID 0
#define AUTOTEST_READ_DEMOD_REG_NAME "read_demod_reg"
#define AUTOTEST_WRITE_DEMOD_REG_NAME "write_demod_reg"
#define AUTOTEST_READ_TUNER_REG_NAME "read_tuner_reg"
#define AUTOTEST_WRITE_TUNER_REG_NAME "write_tuner_reg"
#define AUTOTEST_READ_OTHER_REG_NAME "read_other_reg"
#define AUTOTEST_WRITE_OTHER_REG_NAME "write_other_reg"

static signed int do_read_register(cmd_tbl_t *cmdreg, int flag, int argc, char *argv[], char *orgcmd)
{
	handle_t fe_handle;
	struct cmd_property msg = {0};
	struct cmd_reg_params reg_param = {0};
	signed int ret = 0;
	char *pResult = NULL, *result = NULL;
	char tempbuf[20] = {0};
	unsigned int result_code = IS_SUCCESSFUL, i = 0;
	struct cmd_reg_params* params = NULL;

	if(argc > cmdreg->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	if(dev_ops->parse == NULL)
	{
		// no function for parse
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no parse function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	ret = dev_ops->parse(orgcmd, (void*)&params, NULL);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parse command failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	reg_param.reg = params->reg;
	reg_param.data = params->data;
	reg_param.base = params->base;
	reg_param.width = params->width;
	reg_param.type = params->type;
	reg_param.count = params->count;

	if(strcmp(cmdreg->name, AUTOTEST_READ_DEMOD_REG_NAME) == 0)
	{
		msg.type = CMD_READ_DEMOD_REGISTER;
		if(reg_param.count != 1)
			goto err;
		result = malloc(20);
	}
	else if(strcmp(cmdreg->name, AUTOTEST_READ_TUNER_REG_NAME) == 0)
	{
		msg.type = CMD_READ_TUNER_REGISTER;
		if(reg_param.count != 1)
			goto err;
		result = malloc(20);
	}
	else
	{
		msg.type = CMD_READ_OTHER_REGISTER;
		result = malloc(reg_param.count*(sizeof(unsigned int)*2+3)+1);
	}
	if(result == NULL)
	{
		app_log_error("AUTO TEST malloc failed count=%d\n", reg_param.count);
		goto err;
	}
	msg.buffer = (void*)&reg_param;
	fe_handle = GxFrontend_IdToHandle(AUTOTEST_PCDEMO_MODE_DEMOD_ID);
	ret = ioctl(fe_handle, FE_SET_PROPERTY, &msg);
	if(ret < 0)
	{
		app_log_error("AUTO TEST read reg error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: read_register failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	if(strcmp(cmdreg->name, AUTOTEST_READ_OTHER_REG_NAME) == 0)
	{
		memset(result, 0, reg_param.count*(sizeof(unsigned int)*2+3)+1);
		for(i = 0; i<reg_param.count; i++)
		{
			memset(tempbuf, 0, 20);
			sprintf(tempbuf, "%08x ", *(reg_param.data+i));
			strcat(result, tempbuf);
		}
	}
	else
	{
		memset(result, 0, 20);
		sprintf(result, "%02x", *reg_param.data);
	}

	pResult = "successful";
	auto_test_reply(cmdreg->name, pResult, result_code, result);
	free(result);
	return 0;
err:
	auto_test_reply(cmdreg->name, pResult, result_code, NULL);
	if(result)
		free(result);
	return -1;
}

static signed int do_write_register(cmd_tbl_t *cmdreg, int flag, int argc, char *argv[], char *orgcmd)
{
	handle_t fe_handle;
	struct cmd_property msg = {0};
	struct cmd_reg_params reg_param = {0};
	signed int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
	struct cmd_reg_params* params = NULL;

	if(argc > cmdreg->maxargs)
	{
		// parameter error
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	if(dev_ops->parse == NULL)
	{
		// no function for parse
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: software error, no parse function!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	ret = dev_ops->parse(orgcmd, (void*)&params, NULL);
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: parse command failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	reg_param.reg = params->reg;
	reg_param.data = params->data;
	reg_param.base = params->base;
	reg_param.width = params->width;
	reg_param.type = params->type;
	reg_param.count = params->count;

	if(strcmp(cmdreg->name, AUTOTEST_WRITE_DEMOD_REG_NAME) == 0)
	{
		msg.type = CMD_WRITE_DEMOD_REGISTER;
	}
	else if(strcmp(cmdreg->name, AUTOTEST_WRITE_TUNER_REG_NAME) == 0)
	{
		msg.type = CMD_WRITE_TUNER_REGISTER;
	}
	else
	{
		msg.type = CMD_WRITE_OTHER_REGISTER;
	}
	msg.buffer = (void*)&reg_param;
	fe_handle = GxFrontend_IdToHandle(AUTOTEST_PCDEMO_MODE_DEMOD_ID);
	ret = ioctl(fe_handle, FE_SET_PROPERTY, &msg);
	if(ret < 0)
	{
		app_log_error("AUTO TEST write reg error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: write_register failed!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	pResult = "successful";
	auto_test_reply(cmdreg->name, pResult, result_code, NULL);
	// TODO: reply lock status
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdreg->name, pResult, result_code, NULL);
	return -1;

}
#endif


#if PCDEMO_TEST_ENABLE
cmd_tbl_t cmd_read_demod_register={
	.name = AUTOTEST_READ_DEMOD_REG_NAME,
	.maxargs = 5,
	.repeatable = 1,
	.cmd = do_read_register,
	.usage = "read demod register, need register addr"
};

cmd_tbl_t cmd_write_demod_register={
	.name = AUTOTEST_WRITE_DEMOD_REG_NAME,
	.maxargs = 6,
	.repeatable = 1,
	.cmd = do_write_register,
	.usage = "write demod register, need register addr and value"
};

cmd_tbl_t cmd_read_tuner_register={
	.name = AUTOTEST_READ_TUNER_REG_NAME,
	.maxargs = 5,
	.repeatable = 1,
	.cmd = do_read_register,
	.usage = "read tuner register, need register addr"
};

cmd_tbl_t cmd_write_tuner_register={
	.name = AUTOTEST_WRITE_TUNER_REG_NAME,
	.maxargs = 6,
	.repeatable = 1,
	.cmd = do_write_register,
	.usage = "write tuner register, need register addr and value"
};

cmd_tbl_t cmd_read_other_register={
	.name = AUTOTEST_READ_OTHER_REG_NAME,
	.maxargs = 5,
	.repeatable = 1,
	.cmd = do_read_register,
	.usage = "read other register, need register addr"
};

cmd_tbl_t cmd_write_other_register={
	.name = AUTOTEST_WRITE_OTHER_REG_NAME,
	.maxargs = 6,
	.repeatable = 1,
	.cmd = do_write_register,
	.usage = "write other register, need register addr and value"
};
#endif

cmd_tbl_t cmd_get_stb_version={
	.name = "get_version",
	.maxargs = 3,
	.repeatable = 1,
	.cmd = do_get_version,
	.usage = "get software version."
};

cmd_tbl_t cmd_enable_crccheck={
	.name = "enable_crccheck",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_enable_crccheck,
	.usage = "enable CRC checking."
};

cmd_tbl_t cmd_set_mode={
	.name = "set_demod_type",
	.maxargs = 3,
	.repeatable = 1,
	.cmd = do_set_mode,
	.usage = "set the demod mode,such as dvbs or dvbc."
};

cmd_tbl_t cmd_set_tp={
	.name = "set_tp",
	.maxargs = 11,
	.repeatable = 1,
	.cmd = do_set_tp,
	.usage = "set tp, need demux parameters"
};

cmd_tbl_t cmd_set_destroy_frontend={
	.name = "clear_tp_lock",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_destroy_friontend,
	.usage = "destroy frontend resource"
};


cmd_tbl_t cmd_get_lock_status={
	.name = "get_tp_status",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_get_lock_status,
	.usage = "get lock status, include demod status and demux status"
};

cmd_tbl_t cmd_set_play ={
	.name = "play",
	.maxargs = 7,
	.repeatable = 1,
	.cmd = do_set_play,
	.usage = "play program, need set tp first"
};

cmd_tbl_t cmd_set_play_stop ={
	.name = "stop",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_play_stop,
	.usage = "stop play, need set tp first"
};


cmd_tbl_t cmd_set_ts_compare ={
	.name = "set_ts_compare",
	.maxargs = 3,
	.repeatable = 1,
	.cmd = do_set_ts_compare,
	.usage = "start ts compare, need set tp first"
};

cmd_tbl_t cmd_get_ts_compare_result ={
	.name = "get_ts_compare_result",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_get_ts_compare_result,
	.usage = "get the result of the TS compare, need set tp first"
};

cmd_tbl_t cmd_set_ts_compare_stop ={
	.name = "clear_ts_compare",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_ts_compare_stop,
	.usage = "stop comparing TS"
};

cmd_tbl_t cmd_set_all_stop ={
	.name = "exit",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_all_stop,
	.usage = "stop all functions"
};

cmd_tbl_t cmd_set_reset ={
	.name = "reset",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_factory_reset,
	.usage = "factory reset when function is abnormal"
};


