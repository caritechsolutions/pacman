#ifndef __GX_PLAYER_DEVICE__
#define __GX_PLAYER_DEVICE__

#include "gx_system.h"

typedef struct {
	void* priv;

	int   (*init)(void);
	void  (*uninit)(void* priv);
	int   (*set)(void* priv, int id, void* args);
	int   (*get)(void* priv, int id, void* args);
}GxDevice;

typedef enum {
	PDEV_SysInit,
	PDEV_SysVersion,

	PDEV_VideoColors,
	PDEV_VideoAspect,
	PDEV_VideoView,
	PDEV_VideoClip,
	PDEV_VideoOutputDefInterface,
	PDEV_VideoOutputDefConfig,
	PDEV_VideoOutputInterface,
	PDEV_VideoOutputUsableIface,
	PDEV_VideoOutputConfig,
	PDEV_VideoOutputDef,
	PDEV_VideoOutScartMode,
	PDEV_VideoDisplayScreen,
	PDEV_VideoAutoAdapt,
	PDEV_VideoAutoConfig,
	PDEV_VideoHide,
	PDEV_VideoShow,
	PDEV_Background,
	PDEV_StreamWidth,
	PDEV_StreamHeight,
	PDEV_StreamStride,
	PDEV_VideoCapure,
	PDEV_CvbsViewport,
	PDEV_VideoPlayBypass,
	PDEV_VideoOutputPower,
	PDEV_VideoOutputPower2,
	PDEV_HdmiEncodingOut,
	PDEV_VideoDeInterlace,
	PDEV_VideoPPZoom,
	PDEV_VideoMaxFb,
	PDEV_VideoDeinterlaceEnable,
	PDEV_VideoMosaicDropGate,

	PDEV_VideoDrcMode,

	PDEV_AudioOutputDefInterface,
	PDEV_AudioOutputInterface,
	PDEV_AudioOutputConfigPort,
	PDEV_AudioVolume,
	PDEV_AudioTrack,
	PDEV_AudioBoostVolume,
	PDEV_AudioDescriptorVaild,
	PDEV_AudioDescriptorTrack,
	PDEV_AudioDescriptorBoostVolume,
	PDEV_AudioMute,
	PDEV_AudioPowerMute,
	PDEV_AudioSetPort,
	PDEV_AudioTurnPort,
	PDEV_AudioSetDACVolume,
	PDEV_AudioVolumeFactor,
	PDEV_AudioDolbyCertify,
	PDEV_AudioDebugAoutConfig,
	PDEV_AudioDebugAoutContent,
	PDEV_AudioDebugAdecConfig,
	PDEV_AudioDebugAdecContent,
	PDEV_AudioSpeedMode,
	PDEV_EncryptKey,

	PDEV_FreezeFrameStart,
	PDEV_FreezeFrameStop,
	PDEV_FreezeFrameSize,

	PDEV_PacketCacheSize,
	PDEV_RecordCacheSize,
	PDEV_IndexCacheSize,
	PDEV_EnableAc3,
	PDEV_EnableAac,
	PDEV_EnableDts,
	PDEV_EnableADPCM,
	PDEV_BufSizeESV,
	PDEV_BufSizeESA,
	PDEV_BufSizeESS,
	PDEV_BufSizeAC3,
	PDEV_BufSizeEAC3,
	PDEV_BufSizePCM,
	PDEV_BufSizeDTS,
	PDEV_BufSizeAAC,

	PDEV_MAX,
}GxDeviceCtrlID;


#ifdef I386_LINUX
extern GxDevice gx_device_i386;
#else
extern GxDevice gx_device_goxceed;
#endif

extern int  GxDevice_Init(void);

extern void GxDevice_Uninit(void);

extern int  GxDevice_Set(int id, void* args);

extern int  GxDevice_Get(int id, void* args);

#endif

