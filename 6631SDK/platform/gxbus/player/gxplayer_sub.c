#include "gx_media.h"
#include "gxplayer_internal.h"
#include "gx_avout.h"

#define GX_SUBCTRL_CLASS_MAX (16)
static int subctrl_class_num = 0;
static PISubCtrl* subctrl_class[GX_SUBCTRL_CLASS_MAX];
void GxPlayerSubtitleRegister(PISubCtrl* cls)
{
	int i;
	if (subctrl_class_num == 0)
		memset(&subctrl_class, 0, sizeof(subctrl_class));

	if (subctrl_class_num >= GX_SUBCTRL_CLASS_MAX)
		return;

	for (i=0; i<subctrl_class_num; i++) {
		if (subctrl_class[i] == cls)
			return;
	}

	subctrl_class[subctrl_class_num++] = cls;
}


static PISubCtrl* sub_get_class_from_type(int type)
{
	int i;

	for (i = 0; subctrl_class[i]; i++)
		if (type == subctrl_class[i]->type)
			return subctrl_class[i];

	return NULL;
}

PISubBlock* GxPlayer_SubOpen(PISubBlock* subblock, PISubParam* subpara)
{
	PISubCtrl* sc;
	PISubBlock* psb = NULL;

	if (subblock==NULL)
		return NULL;

	sc = sub_get_class_from_type(subblock->subtype);
	if (!sc)
		return NULL;

	subblock->subout = (PlayerSubtitle*)av_mallocz(sizeof(PlayerSubtitle));
	if (subblock->subout == NULL)
		return NULL;

	if (sc->open)
		psb = sc->open(subblock, subpara);

	if (psb == NULL) {
		av_free(subblock->subout);
		subblock->subout = NULL;
	} else
		subblock->subpara = *subpara;

	return psb;
}

#define GXPLAYER_SUBCTRL(sb) do {                    \
	if (sb == NULL) return;                          \
	sc = sub_get_class_from_type(sb->subtype);       \
	if (sc == NULL) return;                          \
} while(0)

void GxPlayer_SubClose(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->close)
		sc->close(subblock);

	if (subblock->subout) {
		av_free(subblock->subout);
		subblock->subout = NULL;
	}
}

PISubBlock* GxPlayer_SubSwitch(PISubBlock* subblock, int type, PISubParam* subpara)
{
	PISubCtrl *sc1 = NULL, *sc2 = NULL;

	if(subblock == NULL)
		return NULL;

	sc1 = sub_get_class_from_type(subblock->subtype);
	sc2 = sub_get_class_from_type(type);
	if(!sc1 || !sc2)
		return NULL;

	if(sc1->close)
		sc1->close(subblock);

	if (sc2->open)
		sc2->open(subblock, subpara);

	if (sc2->switch_stream)
		sc2->switch_stream(subblock,subpara->pid);

	subblock->subtype = type;
	subblock->subpara = *subpara;

	return subblock;
}

void GxPlayer_SubSwitchStream(PISubBlock* subblock,PlayerSubPID pid)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->switch_stream)
		sc->switch_stream(subblock,pid);

	subblock->subpara.pid = pid;
}

void GxPlayer_SubSwitchRender(PISubBlock* subblock,PlayerSubRender render)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->switch_render)
		sc->switch_render(subblock,render);
}

void GxPlayer_SubHide(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->hide)
		sc->hide(subblock);
}

void GxPlayer_SubShow(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->show)
		sc->show(subblock);
}

void GxPlayer_SubPause(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->pause)
		sc->pause(subblock);
}

void GxPlayer_SubStop(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->stop)
		sc->stop(subblock);
}

void GxPlayer_SubResume(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->resume)
		sc->resume(subblock);
}

void GxPlayer_SubSync(PISubBlock* subblock,int32_t timems)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->sync)
		sc->sync(subblock,timems);
}

void GxPlayer_SubSwitchResolution(PISubBlock* subblock,VideoOutputMode mode)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->switch_resolution)
		sc->switch_resolution(subblock,mode);
}

void GxPlayer_SubResetSource(PISubBlock* subblock)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->reset_source)
		sc->reset_source(subblock);
}

void GxPlayer_SubGetInfo(PISubBlock* subblock, PlayerProgInfo* info)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->get_info)
		sc->get_info(subblock,info);

}

void GxPlayer_SubGetTime(PISubBlock* subblock, uint64_t *curtime, uint64_t *duration)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->get_timeinfo)
		sc->get_timeinfo(subblock, curtime, duration);
}

void GxPlayer_SubGotoLocalTime(PISubBlock* subblock, uint64_t timems)
{
	PISubCtrl* sc;

	GXPLAYER_SUBCTRL(subblock);
	if(sc->goto_localtime)
		sc->goto_localtime(subblock, timems);
}
