#ifndef __DVBSOURCE_H__
#define __DVBSOURCE_H__

#include "gx_stream.h"
#include "gx_common.h"

#ifdef CONFIG_DVB_FE
#include <module/frontend/gxfrontend_module.h>
#endif

#define DVBSOURCE_URL_LONG  PLAYER_URL_LONG

typedef enum {
	DVBSOURCE_NORMAL = 0,
	DVBSOURCE_TSBUFF    ,
	DVBSOURCE_TSCACHE   ,
}DVBSourceType;

typedef struct {
	DVBSourceType type;

	void* (*open)(const char* url);
	int   (*config)(void* ds, const char* url, GxOptions *op);
	int   (*run)(void* ds);
	int   (*stop)(void* ds);
	int   (*pause)(void* ds);
	int   (*resume)(void* ds);
	int   (*close)(void* ds);
	int   (*disconnect)(void* ds, const char* url);
	int   (*track_add)(void* ds, GxStreamTrackAdd* ptrack);
	int   (*track_del)(void* ds, GxStreamTrackDel* ptrack);
	int   (*audio_switch)(void* ds, unsigned short pid);
	int   (*control)(void* ds, int cmd, void* args);
}DVBSourceClass;

typedef struct {
	void* handle;
	char  url[DVBSOURCE_URL_LONG];
	DVBSourceType type;
	DVBSourceClass* cls;
}DVBSource;

DVBSource* dvbsource_open(const char* url);
int dvbsource_config(DVBSource* ds, char* url, GxOptions *op);
int dvbsource_run(DVBSource* ds);
int dvbsource_stop(DVBSource* ds);
int dvbsource_pause(DVBSource* ds);
int dvbsource_resume(DVBSource* ds);
int dvbsource_close(DVBSource* ds);

int dvbsource_set_tp(int dev, int demux, const char* url, GxOptions *op);
int dvbsource_disconnect_l(int dev, int demux, const char* url);
int dvbsource_disconnect(DVBSource* ds, const char* url);
int dvbsource_track_add(DVBSource* ds, GxStreamTrackAdd* ptrack);
int dvbsource_track_del(DVBSource* ds, GxStreamTrackDel* ptrack);
int dvbsource_audio_switch(DVBSource* ds, unsigned short pid);
int dvbsource_control(DVBSource* ds, int cmd, void* args);
void dvbsource_getcls(DVBSource* ds, const char* url);

#endif
