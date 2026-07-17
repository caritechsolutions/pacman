/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2006, All right reserved
******************************************************************************

******************************************************************************
* File Name :	com_ttx.c
* Author    : hulj
* Project   :GX6102_SDKDemon
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.1  	   2008.80.13	          hulj	         creation
*****************************************************************************/

#include "module/ttx/gxttx.h"
#include "gxos/gxcore_os_core.h"
#include "module/pm/gxpm_manage.h"
#include "gx_mem.h"
#include "gui_core.h"

uint16_t osd_trans_index = 0;
uint8_t		ttx_working = 0;
uint32_t				g_old_resolution = 0;
uint32_t				g_resolution = 0;
uint8_t                      g_chTtxOsdEn;
uint8_t 						g_chTtxVbiEn = 0;
uint16_t                     s_nTtxPid;
uint8_t*  pSppBuffer = NULL;//for ttx
uint8_t* pchTtxPesBuffer= NULL;
uint8_t* pes_parse_buf= NULL;
uint32_t g_OsdTtxLang;
uint16_t chTtxColour[4][8]={
			{0x000,0x00f,0x0f0,0x0ff,0xf00,0xf0f,0xff0,0xfff},
			{0x000,0x007,0x070,0x077,0x700,0x707,0x770,0x777},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0}
		};//the color is BGR
handle_t ttx_cond_id;
handle_t ttx_mutex_id;

extern void com_av_preload_logo_to_pic(uint8_t *pPicture, uint32_t nSize);
extern void ttx_clut_setup(void);
extern void gx_init_clut(void);
extern void * subt_memset( void * s, int c, uint32_t count );

 void RGBToYUV(uint8_t R_Value,uint8_t G_Value,uint8_t B_Value,uint8_t *Y_Value,uint8_t *U_Value,uint8_t *V_Value)
{

	float R =( float )R_Value;
	float G =( float )G_Value;
	float B =( float )B_Value;

	*Y_Value = (uint8_t)(( 0.587 * G + 0.114 * B + 0.299 * R ) / 255.0 * 219.0  ) + 16;
	*U_Value = (uint8_t)(( -0.331 * G + 0.500 * B - 0.169 * R ) / 255.0 * 224.0 ) + 128;
	*V_Value = (uint8_t)(( -0.445 * G - 0.081 * B + 0.500 * R ) / 255.0 * 224.0 ) + 128;


}


/*****************************************************************************
 * Function	   : com_ttx_task
 * Description    : entry of teletext task
 * Arguments     : null
 * Returns     :
 * Other       :
 ****************************************************************************/
 #ifdef SUBT_SHOW_ON_OSD
static uint32_t head5;
 #endif
extern uint32_t  ColorsBMP_CLUT[];
//extern Handle_t g_OsdDevHandle;
handle_t ttx_demux_thread_id;
handle_t ttx_parse_thread_id;
int ttx_demux_thread_flag = -1;
int ttx_parse_thread_flag = -1;

void ttx_subt_get_cur_pid(uint16_t *pPID,uint16_t *pPage,uint16_t *pAcillary)
{
	*pPID=s_nTtxPid;
	*pPage=g_chMagzineNumber;
	*pAcillary=g_wPageNumber/10*16+g_wPageNumber%10;
}

void ttx_subt_get_show_flag(uint8_t *pFlag)
{
	*pFlag=g_chTtxOsdEn;
}




void ttx_demux_thread(void *param)
{
	while(1)
	{
		ttx_demux_thread_flag = 0;
		GxCore_MutexLock(ttx_mutex_id);
		GxCore_CondWait(ttx_cond_id, ttx_mutex_id, -1);
		GxCore_MutexUnlock(ttx_mutex_id);
		ttx_demux_thread_flag = 1;
		while(1)
		{

			if(g_chTtxOsdEn || g_chTtxVbiEn)
			{
				ttx_filter_handle();
				ttx_api_thread_delay(10);
			}
			else
			{
				if(pes_parse_buf != NULL)
				{
					av_free(pes_parse_buf);
					pes_parse_buf = NULL;
				}
				break;
			}
		}
	}

}
void ttx_parse_thread(void *param)
{
	extern uint8_t* TtxOsdScreenBuffer;
	extern uint32_t  TTX_CLUT[32];
	extern uint32_t  CC_CLUT[32];
	#ifndef SUBT_SHOW_ON_OSD
	volatile uint8_t i;
	#endif

	while(1)
	{
		ttx_parse_thread_flag = 0;
		GxCore_MutexLock(ttx_mutex_id);
		GxCore_CondWait(ttx_cond_id, ttx_mutex_id, -1);
		GxCore_MutexUnlock(ttx_mutex_id);
		ttx_parse_thread_flag = 1;
		if(g_chTtxOsdEn)
		{
			if(0==CcOpenFlag)
			{
				ttx_hd_end_blit_list();
				ttx_osd_disable();
			#if 0 // used the GUI surface, gx1131 can use it for reduse the heap used
				//ttx_record_stb_palette();// TODO:
				//ttx_set_ttx_palette();
			#else
				GUI_StopUpdate(1);
				if(ttx_hd_init())
				{
					g_chTtxOsdEn = 0;
					ttx_hd_exit();
					ttx_api_thread_delay(200);
					ttx_osd_enable();
					ttx_hd_new_blit_list();
				    GUI_StopUpdate(0);
					if(pSppBuffer != NULL)
					{
						av_free(pSppBuffer);
						pSppBuffer = NULL;
					}
					if(pchTtxPesBuffer != NULL)
					{
						av_free(pchTtxPesBuffer);
						pchTtxPesBuffer = NULL;
					}
					ttx_working =0;
					continue;
				}
			#endif
				color_check((uint32_t*)TTX_CLUT, 32, (uint16_t*)chTtxColour, 32, (uint32_t*)CLUT);
				g_chMagzineNumber=1 ;
				g_wPageNumber=0     ;
				//顺序调整，在LINUX环境下，会出现进入TTX显示，菜单，出现花屏的问题。【先提交绘制，再清屏幕】
				ttx_api_thread_delay(100);
				ttx_hd_end_blit_list();
				memset(TtxOsdScreenBuffer,0x8,TTX_SURFACE_WIDTH*TTX_SURFACE_HIGHT);//only for ecos
				//ttx_api_thread_delay(100);
				//ttx_hd_end_blit_list();
				ttx_osd_enable();
			}
			else
			{
				TtxCCRect rect ={0,0,TTX_SURFACE_WIDTH,TTX_SURFACE_HIGHT};
				ttx_spp_open(&rect);
				for(i=0;i<16;i++)
				{
					CLUT[i/8][i%8]= CC_CLUT[i];
				}
				ttx_spp_clear(CLUT[1][0]);
				ttx_spp_enable();
			}
			ttx_working = 1;
			teletext_dec_main();
			if(0==CcOpenFlag)
			{
				ttx_osd_disable();
				memset(TtxOsdScreenBuffer,osd_trans_index,TTX_SURFACE_WIDTH*TTX_SURFACE_HIGHT);// only for ECOS
				#if 0 // GX3113
				ttx_set_stb_palette();
				#else
				ttx_hd_exit();
				#endif
				ttx_api_thread_delay(200);

				ttx_osd_enable();
				gxlogd("\n********ttx exit*********\n");
				ttx_hd_new_blit_list();
				GUI_StopUpdate(0);
#ifdef TTX_PRE_DECODE
{
				extern int teletext_check_is_pre_process_mode(void);
				if(teletext_check_is_pre_process_mode())
				{
#ifndef TTX_PRE_KEEP_DATA
					if(pSppBuffer != NULL)
					{
						av_free(pSppBuffer);
						pSppBuffer = NULL;
					}
#endif
				}
				else if(pSppBuffer != NULL)
				{
					av_free(pSppBuffer);
					pSppBuffer = NULL;
				}

}
#else
				if(pSppBuffer != NULL)
				{
					av_free(pSppBuffer);
					pSppBuffer = NULL;
				}
#endif
			}
			else
			{
				CcOpenFlag=0;
				ttx_spp_disable();
				ttx_spp_close();


				gxlogd("\n********cc exit*********\n");
			}
			ttx_working =0;

		}
	}
}










void GxTtx_Init(void)
{
	GxCore_MutexCreate(&ttx_mutex_id);
	GxCore_CondInit (&ttx_cond_id);
	ttx_demux_open();
	GxCore_ThreadCreate("ttx_demux_thread", &ttx_demux_thread_id, ttx_demux_thread, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);
	GxCore_ThreadCreate("ttx_parse_thread", &ttx_parse_thread_id, ttx_parse_thread, NULL, 16 * 1024, GXOS_DEFAULT_PRIORITY-1);

}





/*****************************************************************************
 * Function	   : GxTtx_TtxStart
 * Description    : start vbi teletext
 * Arguments     : wTtxPid: pid of teletext
 * Returns     :
 * Other       :
 ****************************************************************************/
// extern Handle_t hTtxPesPacket;
void GxTtx_VbiStart(uint16_t wTtxPid)
{
	if(NULL == pes_parse_buf)
	{
		pes_parse_buf = (uint8_t*)av_malloc(PES_PARSE_BUF_SIZE);
	}
	if(pes_parse_buf)
	{
		ttx_filter_free();
		ttx_filter_setup(wTtxPid);
		if(!g_chTtxVbiEn)
		{
			teletext_vbi_inistial();
			g_chTtxVbiEn=1;
		}
		GxCore_CondBroadcast(ttx_cond_id);
	}
}
void GxTtx_VbiStop(void)
{
	g_chTtxVbiEn=0;
	if(!g_chTtxOsdEn)
	{
		ttx_filter_free();
	}
	teletext_vbi_destroy();
}

void GxTtx_TtxStart(uint16_t wTtxPid)
{
#ifdef TTX_PRE_DECODE
    extern int destroy_ttx_pre_process(void);
    extern unsigned char* get_ttx_pre_data(void);
    extern int teletext_check_is_pre_process_mode(void);
    if((teletext_check_is_pre_process_mode() > 0)
		&&(get_ttx_pre_data() != NULL))
	    destroy_ttx_pre_process();
	//gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
#endif
	ttx_filter_free();
	ttx_filter_setup(wTtxPid);
}
void GxTtx_TtxStop(void)
{
	ttx_filter_free();
	while(ttx_parse_thread_flag || ttx_demux_thread_flag)
		GxCore_ThreadDelay(50);

}


/*****************************************************************************
 * Function	   : GxTtx_QuickSearchEnable
 * Description : enable the quick search
 * Arguments   : void
 * Returns     :
 * Other       :
 ****************************************************************************/
void GxTtx_QuickSearchEnable(void)
{
	g_chSetNum = 1;
}


/*****************************************************************************
 * Function	   : GxTtx_ExitFlag
 * Description    : when ttx task is closed, it will eturn 1
 * Arguments     : void
 * Returns     	   : 1:exit ok
 			     0:not exit
 * Other       :
 ****************************************************************************/

uint8_t GxTtx_ExitFlag()
{
	uint8_t chDataTem;
	//g_chTtxOsdEn和ttx_working 只能保证ttx_parse_thread退出了
	//增加ttx_demux_thread_flag == 0判断 保证ttx_demux_thread也退出
	if((0==g_chTtxOsdEn) && (ttx_working == 0) && (0 == ttx_parse_thread_flag) && (0 == ttx_demux_thread_flag))
	{
		/*if(FLASH_LOGO_ENABLE&&(!CHIP_GX3002))
		{
			com_av_preload_logo_to_pic((uint8_t *)FLASH_LOGO_A,FLASH_LOGO_A_SIZE);//change 1104
		}*/
		g_chTtxOsdExit = 1;
		gxlogd("\nttx set g_chTtxOsdExit\n");
	}
	else
	{
		g_chUserActionTtx = ExitKey;
		ttx_api_thread_delay(10);
		g_chTtxOsdExit=0;
	}
	chDataTem=g_chTtxOsdExit;
	g_chTtxOsdExit=0;
	g_chUserActionTtx =99;
	return chDataTem;
}


/*****************************************************************************
 * Function	   : GxTtx_SetPid
 * Description : set the PID of teletext
 * Arguments   : nTtxPid[IN]:the first address of the area which has been allocated
 * Returns     :
 * Other       :
 ****************************************************************************/
void GxTtx_SetPid(uint16_t nTtxPid,uint8_t chMagNum,uint8_t chPageNum)
{
	s_nTtxPid = nTtxPid;
	g_chMagzineNumber=chMagNum;
	g_wPageNumber=chMagNum/16*10+chPageNum%16;
}
/*****************************************************************************
 * Function	   : GxTtx_SetPid
 * Description : set the key people press
 * Arguments   : chTtxKey[IN]:the key number
 * Returns     :
 * Other       :
 ****************************************************************************/
void GxTtx_SetKey(uint8_t chTtxKey)
{
    g_chUserActionTtx = chTtxKey;

}

/*****************************************************************************
 * Function	   : GxTtx_Enable
 * Description : enable display OSD teletext
 * Arguments   :
 * Returns     :
 * Other       :
 ****************************************************************************/
int8_t GxTtx_Enable(void)
{
    int TimeOut = 5;
    while(((ttx_parse_thread_flag == -1) || (ttx_demux_thread_flag == -1))&&(TimeOut--))
    {
        GxCore_ThreadDelay(10);
    }
	if(ttx_parse_thread_flag || ttx_demux_thread_flag)
		return -5;

	pchTtxPesBuffer = NULL;
	pchTtxPesBuffer = (uint8_t*)av_malloc(PES_FROM_SI_NUM*PES_PACKET_MAX_LEN);
	if(pchTtxPesBuffer == NULL)
	{
		if(CcOpenFlag==1)
		{
			GxTtx_TtxStop();
			CcOpenFlag=0;
		}
		return -3;
	}
	if(NULL == pes_parse_buf)
	{
		pes_parse_buf = (uint8_t*)av_malloc(PES_PARSE_BUF_SIZE);
	}
	if(pes_parse_buf == NULL)
	{
		av_free(pchTtxPesBuffer);
		pchTtxPesBuffer = NULL;
		if(CcOpenFlag==1)
		{
			GxTtx_TtxStop();
			CcOpenFlag=0;
		}
		return -2;
	}
	if(CcOpenFlag==0)
	{
#ifdef TTX_PRE_DECODE
		extern unsigned char *get_ttx_pre_data(void);
		extern int teletext_check_is_pre_process_mode(void);
		if(teletext_check_is_pre_process_mode() > 0)
		  pSppBuffer = get_ttx_pre_data();
		else if(pSppBuffer != NULL)
		{
		  av_free(pSppBuffer);
		  pSppBuffer = NULL;
		}
		if(pSppBuffer == NULL)
#endif
		{
			extern int teletext_get_magazine_cache_num(void);
			int nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
			if(nBlocks <= 0)
			{
				av_free(pchTtxPesBuffer);
				pchTtxPesBuffer = NULL;
				av_free(pes_parse_buf);
				pes_parse_buf=NULL;
				return -1;
			}

			pSppBuffer = (uint8_t*)av_malloc(PAGE_BUFFER_SIZE*nBlocks);
			if(pSppBuffer == NULL)
			{
				av_free(pchTtxPesBuffer);
				pchTtxPesBuffer = NULL;
				av_free(pes_parse_buf);
				pes_parse_buf=NULL;
				return -1;
			}
		}
	}
	g_chTtxOsdEn = 1;
    ttx_working = 0;
	GxCore_CondBroadcast(ttx_cond_id);
	return	0;
}

uint8_t GxTtx_IsnotEnable(void)
{
	return g_chTtxOsdEn ;
//	ttx_api_thread_resume(ThreadHandelTtx);
}
/*****************************************************************************
 * Function	   : com_ttx_disable
 * Description : disable display OSD teletext(special for cc)
 * Arguments   :
 * Returns     :
 * Other       :
 ****************************************************************************/

void GxTtx_SubtDisable(void)
{
	//BOOL bSubtFlag;
	//com_dummy_semaphore_query(HandleDummySemSubtShow,&bSubtFlag,NULL);
	//if(bSubtFlag)
	{
		g_chUserActionTtx = ExitKey;//ExitKey;
		while(!GxTtx_ExitFlag());
		if(pchTtxPesBuffer != NULL)
		{
			av_free(pchTtxPesBuffer);
			pchTtxPesBuffer = NULL;
		}
		g_chUserActionTtx=99;
		gxlogd("\ncom_ttx_subt_disable\n");
		//com_dummy_semaphore_clear(HandleDummySemSubtShow);
	}
}

void GxTtx_SubtEnable(uint16_t wPid,uint16_t wMagazineNum,uint16_t wPageNum)
{
	/*BOOL bSubtFlag=0;
	com_dummy_semaphore_query(HandleDummySemSubtDisable,&bSubtFlag,NULL);
	if(bSubtFlag==TRUE)
	{
		com_dummy_semaphore_query(HandleDummySemSubtShow,&bSubtFlag,NULL);
		if(bSubtFlag==TRUE)
		{
			com_dummy_semaphore_clear(HandleDummySemSubtShow);
		}
		return;
	}*/
	GxTtx_SubtDisable();
	//com_dummy_semaphore_set(HandleDummySemSubtShow,NULL);
	//gxos_scheduler_lock();
	//g_chTtxOsdEn=1;
	g_chMagzineNumber=wMagazineNum;
	g_wPageNumber=wPageNum/16*10+wPageNum%16;
	s_nTtxPid=wPid;
	CcOpenFlag=1;
  	//memset(g_SppBuffer,0,720*576*2+180000);
	g_bPageRecived = 0;
	gxlogd("\ncom_ttx_subt_enable\n");
	//gxos_scheduler_unlock();
	GxTtx_Enable();

}


uint8_t GxTtx_CcIsnotEnable(void)
{
	return CcOpenFlag ;
//	ttx_api_thread_resume(ThreadHandelTtx);
}
uint8_t GxTtx_IsnotWorking(void)
{
	return ttx_working ;
}

void GxTtx_SetResolution(uint32_t resolution)
{
	g_resolution = resolution;
}

void GxTtx_SetLang(uint32_t lang)
{
   g_OsdTtxLang = lang;
}

// ttx pre process control
extern void teletext_set_pre_process_flag(int flag);
extern void teletext_set_magazine_cache_num(int CacheNum);
extern int teletext_check_is_pre_process_mode(void);

void GxTtx_TtxPreProcessEnable(void)
{
#ifdef TTX_PRE_DECODE
	teletext_set_pre_process_flag(1);
#endif
}

void GxTtx_TtxPreProcessDisable(void)
{
#ifdef TTX_PRE_DECODE
	teletext_set_pre_process_flag(0);
#endif
}

void GxTtx_TtxConfigs(TeletextConfig_t configs)
{
	teletext_set_magazine_cache_num(configs.MagazineCacheNum);

	if(configs.TeletextPreProcessEnable == 1)
		GxTtx_TtxPreProcessEnable();
	else if(configs.TeletextPreProcessEnable == 0)
		GxTtx_TtxPreProcessDisable();
}


int32_t GxTtx_TtxPreStart(TeletextPreProcessConfig_t configs)
{
	int ret = 0;
#ifdef TTX_PRE_DECODE

	extern int init_ttx_pre_process(unsigned short ttx_pid);

	if(teletext_check_is_pre_process_mode() <= 0)
		return 0;

	ret = init_ttx_pre_process(configs.TtxPid);
#endif
	return ret;
}

int32_t GxTtx_TtxPreStop(void)
{
	int ret = 0;
#ifdef TTX_PRE_DECODE

	extern int destroy_ttx_pre_process(void);

	if(teletext_check_is_pre_process_mode() <= 0)
		return 0;

	ret = destroy_ttx_pre_process();
#endif
	return ret;
}



