#include "app.h"
#include "app_msg.h"
#include "app_module.h"
#include "./include/common_autotest.h"
#include "./include/module_command.h"
#include "./include/module_device.h"
#include "app_config.h"
#if AUTO_TEST_ENABLE
// command
extern cmd_tbl_t cmd_get_stb_version;
extern cmd_tbl_t cmd_enable_crccheck;
extern cmd_tbl_t cmd_set_mode;
extern cmd_tbl_t cmd_set_tp;
extern cmd_tbl_t cmd_set_destroy_frontend;
extern cmd_tbl_t cmd_get_lock_status;
extern cmd_tbl_t cmd_set_play;
extern cmd_tbl_t cmd_set_ts_compare;
extern cmd_tbl_t cmd_get_ts_compare_result;
extern cmd_tbl_t cmd_set_play_stop;
extern cmd_tbl_t cmd_set_ts_compare_stop;
extern cmd_tbl_t cmd_set_all_stop;
extern cmd_tbl_t cmd_set_reset;

#if PCDEMO_TEST_ENABLE
extern cmd_tbl_t cmd_read_demod_register;
extern cmd_tbl_t cmd_write_demod_register;
extern cmd_tbl_t cmd_read_tuner_register;
extern cmd_tbl_t cmd_write_tuner_register;
extern cmd_tbl_t cmd_read_other_register;
extern cmd_tbl_t cmd_write_other_register;
#endif

#if CVBS_TEST_SUPPORT
extern cmd_tbl_t cmd_set_cvbs_mode;
extern cmd_tbl_t cmd_set_indicator;
extern cmd_tbl_t cmd_set_volume;
extern cmd_tbl_t cmd_get_all_data;
extern cmd_tbl_t cmd_set_all_data;
#endif

// demod device
extern dev_tbl_t dev_dvbs_reg;
extern dev_tbl_t dev_dvbt2_reg;
extern dev_tbl_t dev_dvbc_reg;
extern dev_tbl_t dev_dvbdtmb_reg;
#if PCDEMO_TEST_ENABLE
extern dev_tbl_t dev_pcdemo_reg;
#endif


// memory
static unsigned char receive_memory[MAX_CMDBUF_SIZE*2] = {0};
static int sg_memory_read_pos	= 0;
static int sg_memory_write_pos	= 0;

static status_t _autotest_service_init(handle_t self, int priority)
{
	handle_t sch;

    // register command
	register_cmd(&cmd_get_stb_version);
	register_cmd(&cmd_enable_crccheck);
	register_cmd(&cmd_set_mode);
	register_cmd(&cmd_set_tp);
	register_cmd(&cmd_set_destroy_frontend);
	register_cmd(&cmd_get_lock_status);
	register_cmd(&cmd_set_play);
	register_cmd(&cmd_set_ts_compare);
	register_cmd(&cmd_get_ts_compare_result);
	register_cmd(&cmd_set_play_stop);
	register_cmd(&cmd_set_ts_compare_stop);
	register_cmd(&cmd_set_all_stop);
	register_cmd(&cmd_set_reset);

#if PCDEMO_TEST_ENABLE
	register_cmd(&cmd_read_demod_register);
	register_cmd(&cmd_write_demod_register);
	register_cmd(&cmd_read_tuner_register);
	register_cmd(&cmd_write_tuner_register);
	register_cmd(&cmd_read_other_register);
	register_cmd(&cmd_write_other_register);
#endif

	// register support demod
	register_dev(&dev_dvbs_reg);
	register_dev(&dev_dvbt2_reg);
	register_dev(&dev_dvbc_reg);
	register_dev(&dev_dvbdtmb_reg);
#if PCDEMO_TEST_ENABLE
	register_dev(&dev_pcdemo_reg);
#endif

#if CVBS_TEST_SUPPORT
    register_cmd(&cmd_set_cvbs_mode);
	register_cmd(&cmd_set_indicator);
	register_cmd(&cmd_set_volume);
	register_cmd(&cmd_get_all_data);
	register_cmd(&cmd_set_all_data);
#endif
	// init console device such as uart
#if !USB_TO_SERIAL_SUPPORT
	int ret = 0;
	ret = uart_init(115200);
	if(ret < 0)
		return GXCORE_ERROR;
#endif

	// create
	sch = GxBus_SchedulerCreate("AutoTestConsoleScheduler", GXBUS_SCHED_CONSOLE,
		1024 * 100, GXOS_DEFAULT_PRIORITY + priority);

	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

static void _autotest_service_destroy(handle_t self)
{
	// unregister command

	// register support demod

	// deinit console device
#if !USB_TO_SERIAL_SUPPORT
    uart_close();
#endif
	// destroy

	GxBus_ServiceUnlink(self);

	return;
}

#if 0
static GxMsgStatus _autotest_service_recvmsg(handle_t self, GxMessage *msg)
{
	return GXMSG_OK;
}
#endif

static void _autotest_service_console(handle_t self)
{
	unsigned char Buffer[MAX_CMDBUF_SIZE] = {0};
	int ByteRealRead = 0;
	if(app_get_uart_handle() > 0)
	{
		if((ByteRealRead = uart_receive(Buffer,MAX_CMDBUF_SIZE)) > 0)
		{// read data
			// deal with the cmd
			if(auto_test_protocol_data_receive(receive_memory, MAX_CMDBUF_SIZE*2,
						&sg_memory_read_pos, &sg_memory_write_pos,
						Buffer, ByteRealRead) == 0)
			{
				auto_test_protocol_data_dealwith(receive_memory, MAX_CMDBUF_SIZE*2,
						&sg_memory_read_pos, &sg_memory_write_pos,
						MAX_CMDBUF_SIZE*2);
			}
		}
	}
	GxCore_ThreadDelay(100);
}

GxServiceClass service_autotest = {
	.name		        = "hareware autotest service",
	.init		        = _autotest_service_init,
	.destroy 	        = _autotest_service_destroy,
	.msg_process 	    = NULL,
	.console_process    = _autotest_service_console,
};
#endif
