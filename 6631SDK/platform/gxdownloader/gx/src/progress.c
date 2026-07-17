#include "gxota_msg.h"
#include "progress.h"
#include "gxdlp_api.h"
#include "config.h"

struct update_progress {
	GxUpdateMsg msg;
	handle_t msg_queue;
	handle_t lock;
	UPDATE_PROGRESS_MODE update_progress_mode;
	show_progress_fn show_progress;
};

static struct update_progress progress;

void update_progress_init(GxDLP_Download_Type download_type, UPDATE_PROGRESS_MODE mode, show_progress_fn handler)
{
	struct update_progress *prbar = &progress;

	memset(&prbar->msg, 0x00, sizeof(prbar->msg));

	prbar->msg.download_type = download_type;
	prbar->update_progress_mode = mode;
	if (prbar->lock == 0)
		GxCore_MutexCreate(&prbar->lock);
	GxCore_MutexLock(prbar->lock);
	if (mode == UPDATE_SEND_PROGRESS_MSG) {
		if (prbar->msg_queue != 0)
			GxCore_QueueDelete(prbar->msg_queue);
		GxCore_QueueCreate(&prbar->msg_queue, 128, sizeof(GxUpdateMsg));
	} else if (mode == UPDATE_SHOW_PROGRESS_MSG)
		prbar->show_progress = handler;
	GxCore_MutexUnlock(prbar->lock);
}

int update_msg_to_ota_msg(GxUpdateMsg *up_msg, struct GxOTA_Msg *ota_msg)
{
	int ret = 0;
	ota_msg->type = up_msg->type;
	if (ota_msg->type == GXOTA_GUI_PROCESS)
		ota_msg->percent = up_msg->percent;
	else {
		switch (up_msg->status) {
			case GXUPDATE_START:
				ota_msg->msg_id = GXOTA_STATUS_INIT;
				break;
			case GXUPDATE_CONFIG:
				ota_msg->msg_id = GXOTA_STATUS_FRONTEND_SETUP;
				break;
			case GXUPDATE_CONFIG_OK:
				ota_msg->msg_id = GXOTA_STATUS_FRONTEND_LOCK;
				break;
			case GXUPDATE_DOWNLOAD_DATA:
				ota_msg->msg_id = GXOTA_STATUS_DMX_START_FILTER;
				break;
			case GXUPDATE_DOWNLOAD_DATA_OK:
				ota_msg->msg_id = GXOTA_STATUS_DMX_DATA_FINISH;
				break;
			case GXUPDATE_PARSE_OK:
				ota_msg->msg_id = GXOTA_STATUS_VERSION_OK;
				break;
			case GXUPDATE_INSTALL:
				ota_msg->msg_id = GXOTA_STATUS_WRITE_FLASH;
				break;
			case GXUPDATE_INSTALL_OK:
				ota_msg->msg_id = GXOTA_STATUS_WRITE_FLASH_OK;
				break;
			case GXUPDATE_SUCCESS:
				ota_msg->msg_id = GXOTA_STATUS_UPDATE_SUCCESS;
				break;
			case GXUPDATE_FAIL:
				switch (up_msg->error_code) {
					case GXUPDATE_FRONTEND_ERROR:
						ota_msg->msg_id = GXOTA_STATUS_FRONTEND_ERROR;
						break;
					case GXUPDATE_DMX_CONFIG_ERROR:
						ota_msg->msg_id = GXOTA_STATUS_DMX_ERROR;
						break;
					case GXUPDATE_DMX_FILTER_TIMEOUT:
						ota_msg->msg_id = GXOTA_STATUS_DMX_TIMEOUT;
						break;
					case GXUPDATE_IMAGE_VERSION_ERROR:
						ota_msg->msg_id = GXOTA_STATUS_VERSION_ERROR;
						break;
					case GXUPDATE_IMAGE_NO_DATA_ERROR:
					case GXUPDATE_IMAGE_CRC_ERROR:
						ota_msg->msg_id = GXOTA_STATUS_NO_DATA_UPDATE;
						break;
					case GXUPDATE_WRITE_FLASH_FAILED:
						ota_msg->msg_id = GXOTA_STATUS_WRITE_FLASH_FAILED;
						break;
					default:
						ota_msg->msg_id = GXOTA_STATUS_UPDATE_FAILED;
						break;
				}
				break;
			default:
				ret = -1;
				break;
		}
	}

	return ret;
}

static void send_progress_msg(void)
{
	struct update_progress *prbar = &progress;

	if (prbar->update_progress_mode == UPDATE_SEND_PROGRESS_MSG) {
		if (prbar->msg_queue != 0)
			GxCore_QueuePut(prbar->msg_queue, &prbar->msg, sizeof(GxUpdateMsg), 10);
	} else if (prbar->update_progress_mode == UPDATE_SHOW_PROGRESS_MSG) {
		if (prbar->show_progress)
			prbar->show_progress(&prbar->msg);
	}
}

void update_progress_clean_msg(void)
{
	struct update_progress *prbar = &progress;

	GxCore_MutexLock(prbar->lock);
	if (prbar->msg_queue != 0)
		GxCore_QueueDelete(prbar->msg_queue);
	prbar->msg_queue = 0;
	GxCore_MutexUnlock(prbar->lock);
}

void update_progress_percent(int perc)
{
	struct update_progress *prbar = &progress;

	if (perc != prbar->msg.percent) {
		GxCore_MutexLock(prbar->lock);
		prbar->msg.type = GXUPDATE_MSG_PROCESS;
		prbar->msg.percent = perc;
		send_progress_msg();
		GxCore_MutexUnlock(prbar->lock);
	}
}

void update_progress_status(GXUPDATE_STATUS status)
{
	struct update_progress *prbar = &progress;

	GxCore_MutexLock(prbar->lock);
	prbar->msg.type = GXUPDATE_MSG_STATUS;
	prbar->msg.status = status;
	if (status != GXUPDATE_IDLE)
		send_progress_msg();
	GxCore_MutexUnlock(prbar->lock);
}

void update_progress_error(GXUPDATE_ERROR_CODE error)
{
	struct update_progress *prbar = &progress;

	GxCore_MutexLock(prbar->lock);
	prbar->msg.type = GXUPDATE_MSG_STATUS;
	prbar->msg.error_code = error;
	prbar->msg.status = GXUPDATE_FAIL;
	send_progress_msg();
	GxCore_MutexUnlock(prbar->lock);
}

void update_get_cur_progress(GxUpdateMsg *msg)
{
	struct update_progress *prbar = &progress;
	GxCore_MutexLock(prbar->lock);
	memcpy(msg, &prbar->msg, sizeof(*msg));
	GxCore_MutexUnlock(prbar->lock);
}

int GxUpdate_ReceiveMsg(GxUpdateMsg *msg)
{
	struct update_progress *prbar = &progress;
	unsigned int size = 0;
	int ret = -1;

	GxCore_MutexLock(prbar->lock);
	if (prbar->msg_queue != 0 && prbar->msg.status != GXUPDATE_IDLE)
		ret = GxCore_QueueGet(prbar->msg_queue, msg, sizeof(GxUpdateMsg), &size, 10);
	GxCore_MutexUnlock(prbar->lock);

	if (ret)
		gxlogd("receive update msg failed\n");

	return ret;
}
