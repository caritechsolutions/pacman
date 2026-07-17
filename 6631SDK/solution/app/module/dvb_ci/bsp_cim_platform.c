
#include <stdio.h>
//#include "cim_kernel.h"
#include "cim_platform.h"
#include "app_module.h"
#include "guestbus.h"
#include "module/app_frontend.h"
#include <time.h>

#ifdef HAVE_LIB_CIM
void gb_init(void);
int gb_detect_card(void);
void gb_reset_card(void);
void gb_readmem(unsigned int addr,unsigned char *rbuf,unsigned int len);
void gb_writemem(unsigned int addr, unsigned char *wbuf, unsigned int len);
void gb_readio(unsigned int addr,unsigned char *rbuf,unsigned int len);
void gb_writeio(unsigned int addr, unsigned char *wbuf, unsigned int len);

// extern function
static int _ts_control(cim_bool bEnable)
{
    AppFrontend_Config cfg = {0};

	app_ioctl(0, FRONTEND_CONFIG_GET, &cfg);
	if(FALSE == bEnable)
	{// normal output
		if(cfg.ts_src != 0/*TS_SOURCE_3*/)
        	cfg.ts_src = 0/*TS_SOURCE_3*/;//TS3
        else
			cfg.ts_src = 0x1fff;// invalid
	}
	else
	{// ci output
		if(cfg.ts_src != 1/*TS_SOURCE_2*/)
        	cfg.ts_src = 1/*TS_SOURCE_2*/;//TS2
        else
			cfg.ts_src = 0x1fff;// invalid
	}
	// ts config

	app_log_debug("\nCI, %s, %d,bEnable=%d, cfg.ts_src  =%d \n",__FUNCTION__,__LINE__,bEnable,cfg.ts_src );
	if(cfg.ts_src != 0x1fff)
	{
		app_ioctl(0, FRONTEND_CONFIG_SET, &cfg);
		app_ioctl(0,FRONTEND_DEMUX_CFG,NULL);
		// play param change
		g_AppPlayOps.normal_play.ts_src = cfg.ts_src;
	}
	// update pmt

	return 0;
}

cim_void CAM_TimeDelay(cim_u32 nCnt)
{
	cim_u32 cnt = nCnt*200;
	while(--cnt != 0);
}

cim_void CAM_Wait_Gb_Stat(cim_u32 nCnt)
{
#if 0
	cim_u32 cnt = nCnt;
	while(!(GB_STAT&0x1) && cnt--)
	{
		app_log_debug("%s", __FUNCTION__);
	}
#endif
}

// ³õÊ¼»¯Ó²¼þ
cim_void CAM_InitDevice(cim_void)
{
    gb_init();
}

// ¼ì²â¿¨
cim_bool CAM_DetectCard(cim_void)
{
	return gb_detect_card();
}

// ¸´Î»¿¨
cim_void CAM_ResetCard(cim_void)
{
	cim_printf("%s", __FUNCTION__);
    gb_reset_card();
}

// Ê¹ÄÜ TS Á÷Êä³ö
cim_void CAM_EnableTSI(cim_bool bEnable)
{
	_ts_control(bEnable);
}

// ¶Á´æ´¢¿Õ¼ä
cim_void CAM_ReadMem(cim_uint nStart, cim_u8 *pBuff, cim_uint nLen)
{
    gb_readmem(nStart, pBuff, nLen);
}

// Ð´´æ´¢¿Õ¼ä
cim_void CAM_WriteMem(cim_uint nStart, cim_u8 *pBuff, cim_uint nLen)
{
    gb_writemem(nStart, pBuff, nLen);
}

// ¶Á IO ¿Õ¼ä
cim_void CAM_ReadIO(cim_uint nRegAddr, cim_u8 *pBuff, cim_uint nLen)
{
    gb_readio(nRegAddr, pBuff, nLen);
}

// Ð´ IO ¿Õ¼ä
cim_void CAM_WriteIO(cim_uint nRegAddr, cim_u8 *pBuff, cim_uint nLen)
{
     gb_writeio(nRegAddr, pBuff, nLen);
}
cim_void cim_init(void)
{
    cim_icams[0].InitDevice = CAM_InitDevice;
	cim_icams[0].DetectCard = CAM_DetectCard;
	cim_icams[0].ResetCard = CAM_ResetCard;
	cim_icams[0].EnableTSI = CAM_EnableTSI;
	cim_icams[0].ReadMem = CAM_ReadMem;
	cim_icams[0].WriteMem = CAM_WriteMem;
	cim_icams[0].ReadIO = CAM_ReadIO;
	cim_icams[0].WriteIO = CAM_WriteIO;

}


#endif

