
#include "vo.h"

typedef struct {
	int      dev;
	handle_t vout;
	handle_t vpu;
	handle_t fifo_in;
	int      freezed;
	int running;
}VoHwPriv;

enum {
	VO_STOPPED,
	VO_FREEZED_STOPPED,
	VO_RUNNING,
};

static int vo_hw_run(GxMediaFilter*  filter)
{
	VoHwPriv* priv;
	GxVideoOut* vo = GXVOUT(filter);
	GxVpuProperty_LayerEnable layer_en;

	if(vo)
	{
		priv = (VoHwPriv*)vo->priv;
		if(priv && priv->running != VO_RUNNING)
		{
			int flag;
			GxPlayer_SystemGet(PSYS_VOUT_CLOSED, &flag);

			layer_en.layer = GX_LAYER_VPP;
			if(flag == 0 && !vo->header->isquiet){
				layer_en.enable = 1;
			}else{
				layer_en.enable = 0;
			}
			GxAVSetProperty(priv->dev, priv->vpu, GxVpuPropertyID_LayerEnable, &layer_en, sizeof(GxVpuProperty_LayerEnable));

			priv->running = VO_RUNNING;
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			return GX_PLAYER_OK;

		}
	}

	return GX_PLAYER_ERROR;
}

static int vo_hw_pause(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vo_hw_resume(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vo_hw_config(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vo_hw_stop(GxMediaFilter*  filter)
{
	VoHwPriv* priv;
	GxVideoOut* vo = GXVOUT(filter);
	GxVpuProperty_LayerEnable layer_en = {0};

	if(vo)
	{
		int flag;
		priv = (VoHwPriv*)vo->priv;

		if (priv && priv->running != VO_STOPPED) {
			GxPlayer_SystemGet(PSYS_VOUT_CLOSED, &flag);

			if (flag == 0 && priv->freezed == 0) {
				layer_en.enable = 0;
				layer_en.layer  = GX_LAYER_VPP;
				GxAVSetProperty(priv->dev, priv->vpu, GxVpuPropertyID_LayerEnable, &layer_en, sizeof(GxVpuProperty_LayerEnable));
			}

			if(priv->freezed)
				priv->running = VO_FREEZED_STOPPED;
			else
				priv->running = VO_STOPPED;
		}

		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int vo_hw_open(GxVideoOut*  vo)
{
	VoHwPriv* priv = NULL;

	priv = (void* )av_malloc(sizeof(VoHwPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR;

	vo->priv = (void* )priv;
	memset(vo->priv, 0, sizeof(VoHwPriv));

	priv->dev = GxAvdev_CreateDevice(0);
	if(priv->dev < 0){
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->vout = GxAvdev_OpenModule(priv->dev, GXAV_MOD_VIDEO_OUT, 0);
	if(priv->vout < 0){
		GxAvdev_DestroyDevice(priv->dev);
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->vpu = GxAvdev_OpenModule(priv->dev, GXAV_MOD_VPU, 0);
	if(priv->vpu < 0){
		GxAvdev_DestroyDevice(priv->dev);
		GxAvdev_CloseModule(priv->dev, priv->vout);
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	return GX_PLAYER_OK;
}

static int vo_hw_close(GxVideoOut*  vo)
{
	VoHwPriv* priv = NULL;

	if(vo)
	{
		priv = (VoHwPriv*)vo->priv;
		if(priv)
		{
			vo_hw_stop((GxMediaFilter*)vo);
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAvdev_CloseModule(priv->dev, priv->vout);
			GxAvdev_CloseModule(priv->dev, priv->vpu);
			GxAvdev_DestroyDevice(priv->dev);
			av_free(vo->priv);
		}
	}
	return GX_PLAYER_OK;

}

static int vo_hw_draw_frame(GxVideoOut*  vo, uint8_t * frame, int len, int type)
{
	return GX_PLAYER_OK;
}

static int vo_hw_draw_sub(GxVideoOut*  vo, uint8_t * data, int len, int type)
{
	return GX_PLAYER_OK;
}

static int vo_hw_control(GxVideoOut*  vo, int cmd, void *arg)
{
	VoHwPriv* priv = NULL;

	if(vo)
	{
		priv = (VoHwPriv*)vo->priv;
		if(priv)
		{
			switch (cmd) {
				case GX_VO_CTRL_FREEZE_VIDEO:
					priv->freezed = *(int*)arg;
					break;
				default:
					return GX_PLAYER_ERROR;
			}
		}
	}

	return GX_PLAYER_OK;
}

static int vo_hw_reset(GxMediaFilter*  filter)
{
	if (filter) {
		vo_hw_stop(filter);
		vo_hw_config(filter);
		vo_hw_run(filter);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

GxVideoOutClass gx_vout_hw = {
	._inherit = {
		._inherit = {
			.name    = "Vout HW",
			.parent  = &gx_VideoOutBase,
			.size    = sizeof(GxVideoOut),
			.create  = NULL,
			.release = NULL,
		},
		.run    = vo_hw_run,
		.pause  = vo_hw_pause,
		.resume = vo_hw_resume,
		.config = vo_hw_config,
		.stop   = vo_hw_stop,
		.reset  = vo_hw_reset,
	},
	DEF_AUTHOR("VideoOut","hw","No description","L.F","No comment"),

	.name       = "HW VideoDecoder",
	.open       = vo_hw_open,
	.close      = vo_hw_close,
	.draw_frame = vo_hw_draw_frame,
	.draw_sub   = vo_hw_draw_sub,
	.control    = vo_hw_control,
};


