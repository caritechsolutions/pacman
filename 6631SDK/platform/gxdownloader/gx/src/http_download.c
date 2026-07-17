#ifdef ECOS_OS
#include "http_download.h"
#include "miniwget.h"
#include "gxdlp_api.h"
#include "common/gxmalloc.h"
#include <curl/curl.h>
#include "gxota_msg.h"
#include "config.h"
#include "progress.h"
#include "upgrade.h"
#include "parser.h"
#include "gxupdate_api.h"
#include <jansson.h>
#include "ota_mem.h"
#include "common/libini.h"

typedef struct {
	handle_t thread;
	handle_t lock;
	bool stop_flag;
	char *url;
	int pull_version_cycle; /* unit: s */
	uint8_t ready_write_flash;
	GxUpdateImageInfo image_info;
	GxUpdateDetectVersionProgress detect_progress;
} http_channel_data_t;

static http_channel_data_t http_channel =
{
	.pull_version_cycle = 1,
};

int http_download_data(struct download_data *ddata)
{
	if (ddata == NULL)
		return -2;

	const char *url = GxDLP_Get_URL();
	int status_code = 0;

	update_progress_status(GXUPDATE_DOWNLOAD_DATA);
	//ddata->data = miniwget("http://192.168.110.67:10000/update.bin.extend", (int *)&ddata->len, 0, &status_code);
	ddata->data = miniwget(url, (int *)&ddata->len, 0, &status_code);

	gxlogd("ddata->len ; %d\n", ddata->len);
	if (ddata->data == NULL || status_code != 200) {
		if (ddata->data != NULL)
			download_memhole_free(ddata->data);
		gxloge("network error : %s\n", strerror(errno));
		gxloge("http get error code : %d\n", status_code);
		update_progress_error(GXUPDATE_HTTP_GET_ERROR);
		GxCore_ThreadDelay(2000);
		return -1;
	}
	update_progress_status(GXUPDATE_DOWNLOAD_DATA_OK);
	return 0;
}

bool http_download_is_stop(void)
{
	return http_channel.stop_flag;
}

void http_upgrade_thread(void *arg)
{
	http_channel_data_t *channel_data = &http_channel;
	struct download_data ddata;
	struct upgrade_data  udata[MAX_FILE];
	int num = 0;
	int status_code = 0;

	GxCore_ThreadDetach();

	memset(&ddata, 0x00, sizeof(struct download_data));

	http_upgrade_set_install_flag(0);
	update_progress_status(GXUPDATE_DOWNLOAD_DATA);
	ddata.data = miniwget(channel_data->url, (int *)&ddata.len, 0, &status_code);
	if (ddata.data == NULL || status_code != 200) {
		gxloge("network error : %s\n", strerror(errno));
		gxloge("http get error code : %d\n", status_code);
		update_progress_error(GXUPDATE_HTTP_GET_ERROR);
		goto err;
	}
	update_progress_status(GXUPDATE_DOWNLOAD_DATA_OK);

	update_progress_status(GXUPDATE_PARSE);
	num = parse_data(&ddata, udata, MAX_FILE);
	if (0 == num) {
		update_progress_error(GXUPDATE_IMAGE_NO_DATA_ERROR);
		GxCore_ThreadDelay(10);
		goto err;
	}
	update_progress_status(GXUPDATE_PARSE_OK);

	while (http_channel.ready_write_flash == 0) {
		if (http_channel.stop_flag == true)
			goto err;
		GxCore_ThreadDelay(10);
	}
	http_upgrade_set_install_flag(0);

	update_progress_percent(50);
	GxCore_ThreadDelay(100);

	if (update_data(udata, num) != 0)
		goto err;

	update_progress_percent(100);
	GxCore_ThreadDelay(100);
	update_progress_status(GXUPDATE_SUCCESS);
	GxCore_ThreadDelay(100);
	GxDLP_Close();

err:
	if (ddata.data != NULL)
		download_memhole_free(ddata.data);
	while (http_channel.stop_flag != true) {
		GxCore_ThreadDelay(10);
	}
	update_progress_clean_msg();
	GxCore_MutexLock(http_channel.lock);
	http_channel.thread = 0;
	GxCore_MutexUnlock(http_channel.lock);
}

void http_upgrade_set_install_flag(char flag)
{
	GxCore_MutexLock(http_channel.lock);
	http_channel.ready_write_flash = flag;
	GxCore_MutexUnlock(http_channel.lock);
}

int GxUpdate_StartHTTPUpgrade(const char *url)
{
	if (http_channel.lock == 0)
		GxCore_MutexCreate(&http_channel.lock);
	if (http_channel.thread == 0) {
		update_progress_init(GXDLP_NET_HTTP_DOWNLOAD, UPDATE_SEND_PROGRESS_MSG, NULL);
		GxCore_MutexLock(http_channel.lock);
		if (http_channel.url != NULL)
			GxCore_Free(http_channel.url);
		http_channel.url = GxCore_Strdup(url);
		if (http_channel.url == NULL) {
			gxloge("allocate url memory failed\n");
			return -1;
		}
		http_channel.stop_flag = false;
		memset(&http_channel.image_info, 0x00, sizeof(GxUpdateImageInfo));
		GxCore_ThreadCreate("http upgrade thread", &http_channel.thread, http_upgrade_thread, NULL,30 * 1024, GXOS_DEFAULT_PRIORITY);
		GxCore_MutexUnlock(http_channel.lock);
	}

	return 0;
}

int GxUpdate_StopHTTPUpgrade(void)
{
	if (http_channel.thread != 0) {
		GxCore_MutexLock(http_channel.lock);
		http_channel.stop_flag = true;
		GxCore_MutexUnlock(http_channel.lock);
	}
	return 0;
}

bool GxUpdate_HTTPIsTrigger(void)
{
	return image_version_is_ok(&http_channel.image_info, 0);
}

static int parse_upgrade_desc_file(const char *file_data, int len)
{
	handle_t ini_handle = 0;
	ini_handle = ini_loadmemory(file_data, len);
	char *value = NULL;
	if (ini_handle != 0) {
		value = ini_get(ini_handle, "upgradeinfo", "ota_type");
		if (value != NULL)
			http_channel.image_info.upgrade_mode = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "mid");
		if (value != NULL)
			http_channel.image_info.mid = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "hwver");
		if (value != NULL)
			http_channel.image_info.hwver = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "swver");
		if (value != NULL)
			http_channel.image_info.swver = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "sn_range_start");
		if (value != NULL)
			http_channel.image_info.sn_start = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "sn_range_end");
		if (value != NULL)
			http_channel.image_info.sn_end = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "oui");
		if (value != NULL)
			http_channel.image_info.oui = strtoul(value, NULL, 0);
		value = ini_get(ini_handle, "upgradeinfo", "operator_id");
		if (value != NULL)
			http_channel.image_info.operator_id = strtoul(value, NULL, 0);
		http_channel.image_info.download_type = GXDLP_NET_HTTP_DOWNLOAD;
		ini_close(ini_handle);
		http_channel.detect_progress.version_was_got = true;
		http_channel.detect_progress.status = GXUPDATE_GOT_VERSION;
	}

	return 0;
}

int http_pull_version(const char *url)
{
	int status_code = 0;
	struct download_data ddata;
	int ret = 0;
	char *version_data = NULL;
#if 0
	json_t *root = NULL;
	json_error_t error;
	json_t *json_mid;
	json_t *json_hw;
	json_t *json_sw;
	json_t *json_ota_type;
	json_t *json_sn_start;
	json_t *json_sn_end;

	const char *version_json = "{ \
				    \"mid\": \"0x0000\", \
				    \"hwver\": \"0x00000000\", \
				    \"swver\": \"0x00000001\", \
				    \"ota_type\": \"2\", \
				    \"sn_range_start\": \"0x00000001\", \
				    \"sn_range_end\": \"0x00001000\" \
}";
#endif

	ddata.data = miniwget(url, (int *)&ddata.len, 0, &status_code);
	if (ddata.data == NULL || status_code != 200) {
		gxloge("network error : %s", strerror(errno));
		gxloge("http error code : %d\n", status_code);
		if (ddata.data != NULL)
			download_memhole_free(ddata.data);

		return -1;
	}

	version_data = GxCore_Malloc(ddata.len);
	if (version_data == NULL)
		goto err;
	memcpy(version_data, ddata.data, ddata.len);
	if (ddata.data != NULL) {
		download_memhole_free(ddata.data);
		ddata.data = NULL;
	}

#if 0
	root = json_loads((const char *)version_data, 0, &error);
	if (!root) {
		gxloge("json load error\n");
		ret = -1;
		goto err;
	} else {
		json_mid = json_object_get(root, "mid");
		json_hw = json_object_get(root, "hwver");
		json_sw = json_object_get(root, "swver");
		json_ota_type = json_object_get(root, "ota_type");
		json_sn_start = json_object_get(root, "sn_range_start");
		json_sn_end = json_object_get(root, "sn_range_end");

		if (json_mid == NULL
			|| json_hw == NULL
			|| json_sw == NULL
			|| json_ota_type == NULL
			|| json_sn_start == NULL
			|| json_sn_end ==NULL) {
			json_decref(json_mid);
			json_decref(json_hw);
			json_decref(json_sw);
			json_decref(json_ota_type);
			json_decref(json_sn_start);
			json_decref(json_sn_end);
			gxloge("some data is error\n");
			ret = -1;
			goto err;
		}
		image_info.download_type = GXDLP_NET_HTTP_DOWNLOAD;
		image_info.upgrade_mode = strtoul(json_string_value(json_ota_type), NULL, 0);
		image_info.mid = strtoul(json_string_value(json_mid), NULL, 0);
		image_info.hwver = strtoul(json_string_value(json_hw), NULL, 0);
		image_info.swver = strtoul(json_string_value(json_sw), NULL, 0);
		image_info.sn_start = strtoul(json_string_value(json_sn_start), NULL, 0);
		image_info.sn_end = strtoul(json_string_value(json_sn_end), NULL, 0);

		gxloge("mid %08x\n", image_info.mid);
		gxloge("hwver %08x\n", image_info.hwver);
		gxloge("swver %08x\n", image_info.swver);
		gxloge("sn_start %08x\n", image_info.sn_start);
		gxloge("sn_end %08x\n", image_info.sn_end);
		json_decref(json_mid);
		json_decref(json_hw);
		json_decref(json_sw);
		json_decref(json_ota_type);
		json_decref(json_sn_start);
		json_decref(json_sn_end);

	}
#endif

	parse_upgrade_desc_file(version_data, ddata.len);
err:
	//json_decref(root);
	if (ddata.data != NULL)
		download_memhole_free(ddata.data);
	if (version_data != NULL)
		GxCore_Free(version_data);

	return ret;
}

int GxUpdate_GetHTTPDetectVersionProgress(GxUpdateDetectVersionProgress *progress)
{
	if (progress == NULL)
		return -2;
	memcpy(progress, &http_channel.detect_progress, sizeof(GxUpdateDetectVersionProgress));
	progress->version_is_ok = GxUpdate_HTTPIsTrigger();

	return 0;
}

void http_pull_version_thread(void *arg)
{
	http_channel_data_t *channel_data = &http_channel;
	uint32_t seconds = 0;
	GxTime time;

	GxCore_ThreadDetach();
	while (1) {
		if (channel_data->stop_flag == true)
			break;

		GxCore_GetLocalTime(&time);
		if (time.seconds - seconds > channel_data->pull_version_cycle) {
			http_channel.detect_progress.status = GXUPDATE_GETTING_VERSION;
			http_pull_version(channel_data->url);
			seconds = time.seconds;
			if (image_version_is_ok(&http_channel.image_info, 0)) {
				extern GxUpdateImageInfo image_info;
				memcpy(&image_info, &http_channel.image_info, sizeof(GxUpdateImageInfo));
				channel_data->stop_flag = true;
				break;
			}
		}
		GxCore_ThreadDelay(100);
	}
	GxCore_MutexLock(http_channel.lock);
	http_channel.thread = 0;
	GxCore_MutexUnlock(http_channel.lock);
}

int GxUpdate_StartHTTPDetectVersion(const char *url, int detect_cycle)
{
	if (http_channel.lock == 0)
		GxCore_MutexCreate(&http_channel.lock);
	if (http_channel.thread == 0) {
		update_progress_status(GXUPDATE_IDLE);
		GxCore_MutexLock(http_channel.lock);
		if (http_channel.url != NULL)
			GxCore_Free(http_channel.url);
		http_channel.url = GxCore_Strdup(url);
		if (http_channel.url == NULL) {
			gxloge("allocate url memory failed\n");
			GxCore_MutexUnlock(http_channel.lock);
			return -1;
		}
		http_channel.pull_version_cycle = detect_cycle;
		http_channel.stop_flag = false;
		memset(&http_channel.image_info, 0x00, sizeof(GxUpdateImageInfo));
		memset(&http_channel.detect_progress, 0x00, sizeof(GxUpdateDetectVersionProgress));
		GxCore_ThreadCreate("http pull version thread", &http_channel.thread, http_pull_version_thread, NULL,30 * 1024, GXOS_DEFAULT_PRIORITY);
		GxCore_MutexUnlock(http_channel.lock);
	}

	return 0;
}

int GxUpdate_StopHTTPDetectVersion(void)
{
	if (http_channel.thread != 0) {
		GxCore_MutexLock(http_channel.lock);
		http_channel.stop_flag = true;
		GxCore_MutexUnlock(http_channel.lock);
	}
	return 0;
}
#endif
