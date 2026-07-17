#ifdef NO_OS
#include "gxloader.h"
#endif
#include <string.h>
#include "config.h"
#include "common.h"
#include "gxota_msg.h"
#include "gui.h"
#include "gxdlp_api.h"
#include "common/gxpm.h"
#include "upgrade.h"
#include "installer.h"
#include "parser.h"
#include "progress.h"

#if defined(CONFIG_NET) && defined(CONFIG_ENABLE_NET_HTTP)
static const char *gui_update_prompt(GXUPDATE_STATUS status, GxDLP_Download_Type download_type)
{
	switch (status) {
		case GXUPDATE_CONFIG:
			return "Status:Setup network";
		case GXUPDATE_CONFIG_OK:
			return "Status:Network setup ok";
		case GXUPDATE_DOWNLOAD_DATA:
			return "Status:Receiving data, please wait...";
		case GXUPDATE_DOWNLOAD_DATA_OK:
			return "Status:Data receive finish";
		case GXUPDATE_PARSE:
			return "Status:Parse data";
		case GXUPDATE_PARSE_OK:
			return "Status:Parse data ok";
		case GXUPDATE_INSTALL:
			return "Status:Waiting...Please don't power off while upgrading!";
		case GXUPDATE_INSTALL_OK:
			return "Status:Upgrade Finished";
		case GXUPDATE_SUCCESS:
			return "Status:Upgrade successed";
		case GXUPDATE_FAIL:
			return "Status:Upgrade failed";
		default:
			return NULL;
	}
}
#endif

static void gui_show_process(GxUpdateMsg *msg)
{
	struct GxOTA_Msg ota_msg;
	if (msg->error_code == GXUPDATE_PARTITION_ERROR) {
		ota_msg.type = GXOTA_GUI_STATUS;
#if defined(CONFIG_ENABLE_GDI) || defined(CONFIG_ENABLE_GUI)
		gui_show(0, &ota_msg, "Status: Partition error, pls contact vendor");
#endif
	} else {
#if defined(CONFIG_NET) && defined(CONFIG_ENABLE_NET_HTTP)
		if (msg->download_type == GXDLP_NET_HTTP_DOWNLOAD
			&& msg->type == GXUPDATE_MSG_STATUS) {
			const char *prompt = gui_update_prompt(msg->status, msg->download_type);
			ota_msg.type = GXOTA_GUI_STATUS;
			if (prompt != NULL)
				gui_show(0, &ota_msg, (char *)prompt);
		} else {
#endif
			update_msg_to_ota_msg(msg, &ota_msg);
#if defined(CONFIG_ENABLE_GDI) || defined(CONFIG_ENABLE_GUI)
			gui_show(1, &ota_msg, NULL);
#endif
#if defined(CONFIG_NET) && defined(CONFIG_ENABLE_NET_HTTP)
		}
#endif
	}
}

void upgrade(void)
{
	struct upgrade_data  udata[MAX_FILE];
	struct download_data ddata[MAX_FILE];
	int32_t num;
	int repeat_times = 0;
	char oemValue[20] = {0};
	GxDLP_Download_Type download_type;
	int32_t i = 0;
	GxDLP_Download_Type download_type_list[GXDLP_DOWNLOAD_SEQ_MAX] = {GXDLP_NONE_DOWNLOAD};

	memset(&ddata[0], 0, sizeof(struct download_data)*MAX_FILE);
	memset(&udata[0], 0, sizeof(struct upgrade_data)*MAX_FILE);

	for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++)
		GxDLP_Get_UpdateTypeByIndex(&download_type_list[i], i);

	repeat_times = GxDLP_Get_OtaRepeatTimes();
	if (repeat_times == 0) {
		if (check_partition_status(0) == FLASH_PARTITION_MAX_NUM) {
			GxDLP_Clear_UpdateType();
			GxDLP_Set_UpdateType(GXDLP_OTA_FAILED);
			GxDLP_Sync();
		}
	} else {
		if (repeat_times > 0)
			repeat_times--;
		else
			repeat_times = 0;

		memset(oemValue, 0, strlen(oemValue));
		sprintf(oemValue, "%d",repeat_times);
		GxDLP_Set_OtaRepeatTimes(oemValue);
		GxDLP_Sync();
	}

	for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++) {
		download_type = download_type_list[i];
		if (download_type != GXDLP_NONE_DOWNLOAD
		    && download_type != GXDLP_OTA_FAILED) {
			update_progress_init(download_type, UPDATE_SHOW_PROGRESS_MSG, gui_show_process);
			if(0 == download_data(ddata, download_type))
				break;
		}
	}

	if (i == GXDLP_DOWNLOAD_SEQ_MAX)
		goto err;

	num = parse_data(&ddata[0],udata, MAX_FILE);
	if (0 == num) {
		gxloge("update bin has no partition data to be updated\n");
		goto err;
	}

	update_progress_percent(50);

	if (repeat_times == 0 && check_partition_status(0) == FLASH_PARTITION_MAX_NUM) {
		for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++)
			GxDLP_Set_UpdateTypeByIndex(download_type_list[i], i);
		GxDLP_Sync();
	}

	if(-1 == update_data(udata, num))
		goto err;

	update_progress_percent(100);

	GxCore_ThreadDelay(2000);
	GxCore_Reboot();
	return ;
err:
	if (is_upgrade_all_bin() == true) {
		GxDLP_Close();
		GxDLP_Init(CONFIG_DLP_PATH, CONFIG_BACKUP_DLP_PATH);
	}
	gxloge("[GXOTA] Update Data Failed!\r\n");
	update_progress_error(GXUPDATE_NO_ERROR);

	gxloge("repeat_times = %d\n",repeat_times);

	if (0 == repeat_times && check_partition_status(0) == FLASH_PARTITION_MAX_NUM) {
		GxDLP_Clear_UpdateType();
		GxDLP_Set_UpdateType(GXDLP_OTA_FAILED);
		GxDLP_Sync();
	}

	GxCore_ThreadDelay(2000);

	GxCore_Reboot();
	return ;
}

