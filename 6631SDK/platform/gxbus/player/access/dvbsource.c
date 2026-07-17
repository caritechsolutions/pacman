#include "dvbsource.h"
#include "gx_common.h"
#include "avstring.h"

#define DVBSOURCE_CLASS_MAX (4)
static int dvbsource_class_num = 0;
static DVBSourceClass* dvbsource_classes[DVBSOURCE_CLASS_MAX];
void GxDVBSourceRegister(DVBSourceClass * cls)
{
	int i;
	if (dvbsource_class_num == 0)
		memset(&dvbsource_classes, 0, sizeof(dvbsource_classes));

	if (dvbsource_class_num >= DVBSOURCE_CLASS_MAX)
		return;

	for (i=0; i<DVBSOURCE_CLASS_MAX; i++) {
		if (dvbsource_classes[i] == cls)
			return;
	}

	dvbsource_classes[dvbsource_class_num++] = cls;
}

void dvbsource_getcls(DVBSource* ds, const char* url)
{
	if (ds && url) {
		int i, delayms, cachems;
		DVBSourceType type;

		ds->cls = NULL;
		strncpy(ds->url, url, DVBSOURCE_URL_LONG);

		GxUrl_GetItem(ds->url, GX_URL_KEY_TSBUFF_MS, &delayms);
		GxUrl_GetItem(ds->url, GX_URL_KEY_TSCACHE_MS, &cachems);
		//delayms = 3000;
		//cachems = 1000;
		if (delayms > 0)
			type = DVBSOURCE_TSBUFF;
		else if (cachems > 0)
			type = DVBSOURCE_TSCACHE;
		else
			type = DVBSOURCE_NORMAL;

		for (i=0; i<dvbsource_class_num; i++) {
			if (dvbsource_classes[i]->type == type) {
				ds->type = type;
				ds->cls = dvbsource_classes[i];
				break;
			}
		}
	}
}

DVBSource* dvbsource_open(const char* url)
{
	DVBSource* ds = av_mallocz(sizeof(DVBSource));

	if (ds) {
		snprintf (ds->url, DVBSOURCE_URL_LONG-1, "%s", url);

		dvbsource_getcls(ds, url);
		if (ds->cls == NULL)
			return NULL;

		ds->handle = ds->cls->open(ds->url);
		if (ds->handle == NULL) {
			av_free(ds);
			return NULL;
		}
	}

	return ds;
}

int dvbsource_config(DVBSource* ds, char* url, GxOptions *op)
{
	if (ds->cls)
		return ds->cls->config(ds->handle, ds->url, op);
	return GX_PLAYER_ERROR;
}

int dvbsource_run(DVBSource* ds)
{
	if (ds && ds->cls)
		ds->cls->run(ds->handle);
	return GX_PLAYER_OK;
}

int dvbsource_stop(DVBSource* ds)
{
	if (ds && ds->cls)
		ds->cls->stop(ds->handle);
	return GX_PLAYER_OK;
}

int dvbsource_pause(DVBSource* ds)
{
	if (ds && ds->cls && ds->cls->pause)
		ds->cls->pause(ds->handle);
	return GX_PLAYER_OK;
}

int dvbsource_resume(DVBSource* ds)
{
	if (ds->cls && ds->cls->resume)
		ds->cls->resume(ds->handle);
	return GX_PLAYER_OK;
}

int dvbsource_close(DVBSource* ds)
{
	if (ds) {
		if (ds->cls)
			ds->cls->close(ds->handle);
		av_free(ds);
	}

	return GX_PLAYER_OK;
}

int dvbsource_set_tp(int dev, int demux, const char* url, GxOptions *op)
{
#ifdef CONFIG_DVB_FE
	int tuner,type;
	GxFrontend frontend;
	int fre,symb,qam,polar,bandwidth,sat22k,workmode;
	int data_plp_id, common_plp_exist,common_plp_id;
	int pls_code = 0;
	int pls_ts_id = 0;
	int pls_change = 0;
	int pls_status = 0;
	int if_channel_index =0;
	int lnb_index =0x20;
	int centre_fre =0;
	int if_fre =0;
	int pls_num = 0;
	int port = 0;
	char ipStr[16] = {0};
	int tp_mode = 0;

	if (strncasecmp(url,"dvbc",4) == 0)
		type = FRONTEND_DVB_C;
	else if (strncasecmp(url,"dvbs",4) == 0)
		type = FRONTEND_DVB_S;
	else if (strncasecmp(url,"dvbt2",5) == 0)
		type = FRONTEND_DVB_T2;
	else if (strncasecmp(url,"dvbt",4) == 0)
		type = FRONTEND_DVB_T;
	else if (strncasecmp(url,"dtmb",4) == 0)
		type = FRONTEND_DTMB;
	else if(strncasecmp(url,"dvbnet",6) == 0)
		type = FRONTEND_DVB_NET;
	else
		return -1;

	dvbsource_disconnect_l(dev, demux, url);

	GxUrl_GetItem(url,GX_URL_KEY_TUNER,&tuner);
	GxUrl_GetItemStr(url,GX_URL_KEY_IP,(uint8_t*)ipStr, 16);
	GxUrl_GetItem(url,GX_URL_KEY_PORT,&port);
	GxUrl_GetItem(url,GX_URL_KEY_FRE,&fre);
	GxUrl_GetItem(url,GX_URL_KEY_SYM,&symb);
	GxUrl_GetItem(url,GX_URL_KEY_QAM,&qam);
	GxUrl_GetItem(url,GX_URL_KEY_POLAR,&polar);
	GxUrl_GetItem(url,GX_URL_KEY_BANDW,&bandwidth);
	GxUrl_GetItem(url,GX_URL_KEY_22K,&sat22k);
	GxUrl_GetItem(url,GX_URL_KEY_CWMODE,&workmode);
	GxUrl_GetItem(url,GX_URL_KEY_DATA_PLPID,&data_plp_id);
	GxUrl_GetItem(url,GX_URL_KEY_COMM_PLPID_EXIST,&common_plp_exist);
	GxUrl_GetItem(url,GX_URL_KEY_COMM_PLPID,&common_plp_id);
	GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_LNB_INDEX,&lnb_index);
	GxUrl_GetItem(url,GX_URL_KEY_PLS_TSID,&pls_ts_id);
	GxUrl_GetItem(url,GX_URL_KEY_PLS_CODE,&pls_code);
	GxUrl_GetItem(url,GX_URL_KEY_PLS_CHANGE,&pls_change);
	GxUrl_GetItem(url,GX_URL_KEY_PLS_STATUS,&pls_status);
	GxUrl_GetItem(url,GX_URL_KEY_PLS_NUM,&pls_num);
	GxUrl_GetItem(url,GX_URL_KEY_TP_MODE,&tp_mode);
	if(lnb_index == -1)
	{
		if_channel_index =0;
		lnb_index =0x20;
		centre_fre =0;
		if_fre =0;
	}
	else
	{
		GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_IF_INDEX,&if_channel_index);
		GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_CENTRE_FRE,&centre_fre);
		GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_IF_FRE,&if_fre);
	}

	if (FRONTEND_DVB_NET == type)
	{
		if (op != NULL)
			frontend.url = GxOptions_Get_By_Id(op, " -H", OPTIONS_TS_SOURCE);
		else
			frontend.url = NULL;
		frontend.ip = ipStr;
		frontend.port = port;
		frontend.dev = dev;
		frontend.demux = demux;
		frontend.tuner = (tuner < 0) ? 0 : tuner;
		frontend.type = type;
		GxFrontend_SetTp(&frontend);
	}
	else if (fre > 0) {
		frontend.fre        = fre;
		frontend.symb       = symb;
		frontend.pls_n		= pls_num;
		frontend.qam        = qam;
		frontend.sat22k     = sat22k;
		frontend.polar      = polar;
		frontend.bandwidth  = bandwidth;
		frontend.type_1501  = workmode;
		frontend.data_plp_id = data_plp_id;
		frontend.common_plp_exist = common_plp_exist;
		frontend.common_plp_id = common_plp_id;

		frontend.dev        = dev;
		frontend.demux      = demux;
		frontend.tuner      = (tuner < 0) ? 0 : tuner;
		frontend.type       = type;

		frontend.if_channel_index= if_channel_index;
		frontend.lnb_index= lnb_index;

		frontend.centre_fre= centre_fre;
		frontend.if_fre= if_fre;
		frontend.pls_code = pls_code;
		frontend.pls_ts_id = pls_ts_id;
		frontend.pls_chanage= pls_change;
		frontend.pls_ts_status= pls_status;
		if (frontend.lnb_index < 0x10&&frontend.centre_fre!=0 &&frontend.if_fre!=0) {
			frontend.fre = frontend.if_fre;
		}
		frontend.set_tp_mode = tp_mode;
		GxFrontend_SetTp(&frontend);
	}

#endif
	return 0;
}

int dvbsource_disconnect_l(int dev, int demux, const char* url)
{
#ifdef CONFIG_DVB_FE
	int tuner,type;
	GxFrontend frontend;
	int fre,symb,qam,polar,bandwidth,sat22k,workmode;
	int data_plp_id, common_plp_exist,common_plp_id;

	if (strncasecmp(url,"dvbc",4) == 0)
		type = FRONTEND_DVB_C;
	else if (strncasecmp(url,"dvbs",4) == 0)
		type = FRONTEND_DVB_S;
	else if (strncasecmp(url,"dvbt2",5) == 0)
		type = FRONTEND_DVB_T2;
	else if (strncasecmp(url,"dvbt",4) == 0)
		type = FRONTEND_DVB_T;
	else if (strncasecmp(url,"dtmb",4) == 0)
		type = FRONTEND_DTMB;
	else if(strncasecmp(url,"dvbnet",6) == 0)
		type = FRONTEND_DVB_NET;
	else
		return -1;

	GxUrl_GetItem(url,GX_URL_KEY_TUNER,&tuner);
	GxUrl_GetItem(url,GX_URL_KEY_FRE,&fre);
	GxUrl_GetItem(url,GX_URL_KEY_SYM,&symb);
	GxUrl_GetItem(url,GX_URL_KEY_QAM,&qam);
	GxUrl_GetItem(url,GX_URL_KEY_POLAR,&polar);
	GxUrl_GetItem(url,GX_URL_KEY_BANDW,&bandwidth);
	GxUrl_GetItem(url,GX_URL_KEY_22K,&sat22k);
	GxUrl_GetItem(url,GX_URL_KEY_CWMODE,&workmode);
	GxUrl_GetItem(url,GX_URL_KEY_DATA_PLPID,&data_plp_id);
	GxUrl_GetItem(url,GX_URL_KEY_COMM_PLPID_EXIST,&common_plp_exist);
	GxUrl_GetItem(url,GX_URL_KEY_COMM_PLPID,&common_plp_id);

	if (fre > 0) {
		frontend.fre        = fre;
		frontend.symb       = symb;
		frontend.qam        = qam;
		frontend.sat22k     = sat22k;
		frontend.polar      = polar;
		frontend.bandwidth  = bandwidth;
		frontend.type_1501  = workmode;
		frontend.data_plp_id = data_plp_id;
		frontend.common_plp_exist = common_plp_exist;
		frontend.common_plp_id = common_plp_id;

		frontend.dev        = dev;
		frontend.demux      = demux;
		frontend.tuner      = (tuner < 0) ? 0 : tuner;
		frontend.type       = type;

		GxFrontend_Disconnect(&frontend, 0);
	}
#endif
	return 0;
}

int dvbsource_disconnect(DVBSource* ds, const char* url)
{
	if (ds && ds->cls && ds->cls->disconnect)
		ds->cls->disconnect(ds->handle, url);

	return 0;
}

int dvbsource_track_add(DVBSource* ds, GxStreamTrackAdd* ptrack)
{
	if (ds && ds->cls && ds->cls->track_add)
		ds->cls->track_add(ds->handle, ptrack);
	return 0;
}

int dvbsource_track_del(DVBSource* ds, GxStreamTrackDel* ptrack)
{
	if (ds && ds->cls && ds->cls->track_del)
		ds->cls->track_del(ds->handle, ptrack);
	return 0;
}

int dvbsource_audio_switch(DVBSource* ds, unsigned short pid)
{
	if (ds && ds->cls && ds->cls->audio_switch)
		ds->cls->audio_switch(ds->handle, pid);
	return 0;
}

int dvbsource_control(DVBSource* ds, int cmd, void* args)
{
	if (ds && ds->cls && ds->cls->control)
		ds->cls->control(ds->handle, cmd, args);
	return 0;
}

