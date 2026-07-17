
#include "gx_device.h"

static GxDevice* pdevice = NULL;

int  GxDevice_Init(void)
{
	if(pdevice == NULL)
	{
#ifdef I386_LINUX
		pdevice = &gx_device_i386;
#else
		pdevice = &gx_device_goxceed;
#endif
	}

	if(pdevice)
	{
		if(pdevice->init)
		{
			return pdevice->init();
		}
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_OK;
}

void GxDevice_Uninit(void)
{
	if(pdevice && pdevice->uninit)
	{
		pdevice->uninit(pdevice->priv);
	}
}

int  GxDevice_Set(int id, void* args)
{
	if(pdevice && pdevice->set)
	{
		return pdevice->set(pdevice->priv, id, args);
	}

	return GX_PLAYER_ERROR;
}

int  GxDevice_Get(int id, void* args)
{
	if(pdevice && pdevice->get)
	{
		return pdevice->get(pdevice->priv, id, args);
	}

	return GX_PLAYER_ERROR;
}


