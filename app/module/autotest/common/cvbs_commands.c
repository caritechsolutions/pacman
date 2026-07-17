/*
 * =====================================================================================
 *
 *       Filename:  cvbs_commands.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  03/14/2023 21:34:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  B.Z.
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
#if CVBS_TEST_SUPPORT
#include "module/app_log.h"

extern int cvbs_work_mode_switch(char *mode);
extern int cvbs_indicator_set(char* str, int steps, int *error_step);
extern int cvbs_set_volume(char *cmd);
extern int cvbs_get_all_data(char *data, unsigned int size);
extern int cvbs_set_all_data(char *data);

static signed int do_set_cvbs_mode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	int ret = 0;
	unsigned int result_code = IS_SUCCESSFUL;
    char buf[64] = {0};

    memset(buf, 0, sizeof(buf));

	if((argc > cmdtp->maxargs)
            || (argc < (cmdtp->maxargs - 1)))
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: Parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

    // cvbs test mode control
    ret = cvbs_work_mode_switch(argv[1]);
	if(0 > ret)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
        if(-2 == ret)
            pResult = "failed: Have been start test!";
        else if(-3 == ret)
            pResult = "failed: No start, please start first!";
        else
            pResult = "failed: Parameter is wrong!";
        result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	pResult = "successful";
    sprintf(buf, "%s %s", argv[0], argv[1]);
	auto_test_reply(buf, pResult, result_code, NULL);
	// TODO:
	return 0;
err:
	// TODO: reply failed
    sprintf(buf, "%s %s", argv[0], argv[1]);
	auto_test_reply(buf, pResult, result_code, NULL);
	return -1;
}


static signed int do_set_indicator(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
    int step = 0;
	char *pResult = NULL;
	int ret = 0;
    int error_step = 0;
	unsigned int result_code = IS_SUCCESSFUL;
    char overstep_str[64] = {0};
    char buf[64] = {0};

    memset(buf, 0, sizeof(buf));

	if((argc > cmdtp->maxargs)
            || (argc < 3))
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: CVBS parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	// get step
	sscanf(argv[2], "%d", &step);

    // cvbs indicator process
    ret = cvbs_indicator_set(argv[1], step, &error_step);
	if(0 > ret)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
        if(ret == -2)
        {
            memset(overstep_str, 0, sizeof(overstep_str));
            sprintf(overstep_str, "failed: Overstep! error_step=%d!", error_step);
            pResult = overstep_str;
        }
        else if(-3 == ret)
        {
            pResult = "failed: No start, please start first!";
        }
        else
        {
            pResult = "failed: CVBS parameter is wrong!";
        }
        result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	pResult = "successful";
    sprintf(buf, "%s %s %s", argv[0], argv[1], argv[2]);
	auto_test_reply(buf, pResult, result_code, NULL);
	// TODO: reply lock status
	return 0;
err:
	// TODO: reply failed
    sprintf(buf, "%s %s %s", argv[0], argv[1], argv[2]);
	auto_test_reply(buf, pResult, result_code, NULL);
	return -1;
}


static signed int do_set_volume(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	int ret = 0;
	unsigned int result_code = IS_SUCCESSFUL;
    char buf[64] = {0};

    memset(buf, 0, sizeof(buf));

	if((argc > cmdtp->maxargs)
            || (argc < (cmdtp->maxargs - 1)))
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: Parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

    // cvbs set volume
    ret = cvbs_set_volume(argv[1]);
	if(0 > ret)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
        if(-3 == ret)
        {
            pResult = "failed: No start, please start first!";
        }
        else
        {
            pResult = "failed: Parameter is wrong!";
        }
        result_code = PARAMETER_IS_INVALID;
		goto err;
	}

	pResult = "successful";
    sprintf(buf, "%s %s", argv[0], argv[1]);
	auto_test_reply(buf, pResult, result_code, NULL);
	// TODO:
	return 0;
err:
	// TODO: reply failed
    sprintf(buf, "%s %s", argv[0], argv[1]);
	auto_test_reply(buf, pResult, result_code, NULL);
	return -1;
}

static signed int do_get_all_data(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
    char data[200] = {0};
	int ret = 0;
	unsigned int result_code = IS_SUCCESSFUL;

	if(argc > cmdtp->maxargs)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: Parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
    // cvbs set volume
    ret = cvbs_get_all_data(data, 200);
	if(0 > ret)
    {
        // set mode failed
        app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
        if(-3 == ret)
        {
            pResult = "failed: No start, please start first!";
            result_code = PARAMETER_IS_INVALID;
        }
        else
        {
            pResult = "failed: Get date failed!";
            result_code = IS_CODE_ERROR;
        }
        goto err;
    }

	pResult = "successful";
	auto_test_reply(cmdtp->name, pResult, result_code, data);
	// TODO:
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_set_all_data(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	char *pResult = NULL;
	int ret = 0;
	unsigned int result_code = IS_SUCCESSFUL;

	if(argc > cmdtp->maxargs)
	{
		// set mode failed
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		pResult = "failed: Parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}

    // cvbs set volume
    ret = cvbs_set_all_data(argv[1]);
	if(0 > ret)
    {
        // set mode failed
        app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
        if(-3 == ret)
        {
            pResult = "failed: No start, please start first!";
        }
        else
        {
            pResult = "failed: Parameter is wrong!";
        }
        result_code = PARAMETER_IS_INVALID;
        goto err;
    }

	pResult = "successful";
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	// TODO:
	return 0;
err:
	// TODO: reply failed
	auto_test_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}


cmd_tbl_t cmd_set_cvbs_mode={
	.name = "set_cvbs",
	.maxargs = 3,
	.repeatable = 1,
	.cmd = do_set_cvbs_mode,
	.usage = "Start or stop cvbs test."
};

cmd_tbl_t cmd_set_indicator={
	.name = "set_indicator",
	.maxargs = 4,
	.repeatable = 1,
	.cmd = do_set_indicator,
	.usage = "Set indicator of the specified type."
};

cmd_tbl_t cmd_set_volume={
	.name = "set_volume",
	.maxargs = 3,
	.repeatable = 1,
	.cmd = do_set_volume,
	.usage = "Set volume."
};

cmd_tbl_t cmd_get_all_data={
	.name = "get_all_data",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_get_all_data,
	.usage = "Get all indicator data."
};

cmd_tbl_t cmd_set_all_data={
	.name = "set_all_data",
	.maxargs = 3,
	.repeatable = 1,
	.cmd = do_set_all_data,
	.usage = "Set all indicator data."
};


#endif


