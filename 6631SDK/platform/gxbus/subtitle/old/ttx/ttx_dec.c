/*****************************************************************************
 *                          CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2006, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :   com_ttx_dec.c
 * Author    :   hulj
 * Project   :   GX6102
 * Type      :   win
 ******************************************************************************
 * Purpose   :   模块实现文件
 ******************************************************************************
 * Release History:
 VERSION   Date              AUTHOR         Description
 0.0      2008/07/24              hulj             creation
 *****************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include "module/ttx/gxttx.h"
#include "gui_core/gui_timer.h"
#include "module/config/gxconfig.h"
#include "module/player/gxplayer_module.h"
#include "gx_mem.h"
#include "av/hal/gxav_hal_fb.h"
//#include "dbase_usrdb.h"

#define SUBTITLE_IN_TTX_CHANGE_COUNTER	(1)


extern uint8_t*  pSppBuffer;
extern uint32_t	  g_old_resolution;
extern int32_t	 g_resolution ;

uint16_t g_wPageNo;                                 //存放用户按键所选择的页号
uint8_t g_chDefaultG0G2;                            //设置默认的字符集
uint8_t g_chDefaultG02;                             //设置默认的字符补集
uint8_t g_chUserActionTtx;
uint8_t g_chSearchNotSetPosition = 0;               //表示是否搜索固定页;1:否，0:是
uint8_t g_chSearchMagNum = 1;                       //要搜索的杂志号
uint16_t g_wSearchPageNum = 0;                      //要搜索的页号
uint16_t g_wSearchSubCode;
uint8_t g_chPageRefreshMode;
uint8_t g_chSetNum = 0;
uint8_t g_bPageRecived = 0;
static event_list* ptime = NULL;
static uint32_t nPackNum;

static uint8_t s_chWhetherShowBackGround;
static uint8_t s_chLine0IsDoubleH = 0;

// for pre-process
static int sg_TtxPreProcessEnable = 0;
static int sg_TtxMagazineCacheNum = DEFAULT_MAGAZINE_NUM_CACHE;



#if SUBTITLE_IN_TTX_CHANGE_COUNTER
static uint8_t gs_ucTTXRefreshCounter = 40;
#endif

uint8_t g_chTtxStartAddX,g_chTtxStartAddY;
enum
{
	TTX_SHOW_BACK_GROUND = 0,
	TTX_BACK_GROUND_HALF_TRANSPARENCE,
	TTX_NOT_SHOW_BACK_GROUND = 7,

};

//放在SDRAM中
struct page_buffer* g_pBuffer;                 //指向下面地址的指针
struct page_buffer g_TtxBuffer;                //存放这在搜索的页面内容的缓存
struct x_29_pack g_X29PackView[8];                //存放解析后 的29包内容
extern uint8_t g_chTtxOsdEn;
extern uint32_t g_nTtxDemuxStartAddr;
extern uint32_t g_nTtxDemuxStopAddr;
extern uint8_t g_chTtxVbiEn;
extern uint8_t chBufBlocFlag[];
extern uint8_t chCcBufBlocFlag[];

uint8_t CcOpenFlag;
static uint8_t gs_chPacketCount;                    //保证只在刚一进入TTX模块时解一次包头
static uint8_t s_chCheckPacketLen =0;               //进入一次TTX检查一次TTX包长

struct page_buffer *g_pTTXCurPage[8];
struct view_page    g_TTXPageShow ;     //指向正在显示的页面的节点
struct page_buffer g_TtxBufferTem ;            //存放一个完整的页

uint8_t  g_chMagzineNumber ;
uint16_t g_wPageNumber     ;
uint8_t g_chTtxOsdExit ;
static uint8_t s_chTtxReceive;
static uint8_t s_chTtxPageRefreshEnable;
static uint16_t s_wDefaultNavigationPage[4];//for dvb_t
//static uint8_t timer_enable = 0;
#define TTX_TIME_SHOW	0

#if SUBTITLE_IN_TTX_CHANGE_COUNTER
void com_ttx_set_refresh_count(uint8_t uc_count);
#endif

void draw_screen_by_line(uint8_t chDrawContent,uint8_t chLineNum);
struct page_buffer* check_page_have(uint8_t ,uint16_t,int16_t );
void ttx_set_default_guide_page(struct page_buffer *pBuffer);
void ttx_page_refresh(void);
static void clear_page_by_line(uint8_t y);


extern  uint8_t* pchTtxPesBuffer;

enum
{
	TTX_REFRESH_AUTO,
	TTX_REFRESH_SUBPAGE,
};

/*uint32_t CLUT[4][8]=
{
    {3,7,5,2,6,8,4,1},//BLACK,RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,WHITE
    {0,11,12,13,14,15,9,10},//TRANSPARENT,HALF_RED,HALF_GREEN,HALF_YELLOW,HALF_BLUE,HALF_MAGENTA,HALF_CYAN,GREY
    {0,0,0,0,0,0,0,0,},
    {0,0,0,0,0,0,0,0,}
};*/


uint32_t CLUT[4][8]=
{
    {0,1,2,3,4,5,6,7},//BLACK,RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,WHITE
    {8,9,10,11,12,13,14,15},//TRANSPARENT,HALF_RED,HALF_GREEN,HALF_YELLOW,HALF_BLUE,HALF_MAGENTA,HALF_CYAN,GREY
    {0,0,0,0,0,0,0,0,},
    {0,0,0,0,0,0,0,0,}
};



// for pre-process and the max magazine cache number
int teletext_check_is_pre_process_mode(void)
{
	if(sg_TtxPreProcessEnable > 0)
		return 1;// use ttx pre process
	else
		return 0;
}

void teletext_set_pre_process_flag(int flag)
{
	if(flag > 0)
		sg_TtxPreProcessEnable = 1;
	else
		sg_TtxPreProcessEnable = 0;
}

int teletext_get_magazine_cache_num(void)
{
	return sg_TtxMagazineCacheNum;
}

void teletext_set_magazine_cache_num(int CacheNum)
{
	if((CacheNum > 0) && (CacheNum <= 8))
	 sg_TtxMagazineCacheNum = CacheNum;
}



uint8_t language_g0_2(uint8_t data)
{
    switch(data)
    {
        case 0x00: return English;
        case 0x01: return German;
        case 0x02: return Swedish;
        case 0x03: return Italian;
        case 0x04: return French;
        case 0x05: return Portuguese;
        case 0x06: return Czech;
        case 0x08: return Polish;
        case 0x09: return German;
        case 0x0a: return Swedish;
        case 0x0b: return Italian;
        case 0x0c: return French;
        case 0x0e: return Czech;
        case 0x10: return English;
        case 0x11: return German;
        case 0x12: return Swedish;
        case 0x13: return Italian;
        case 0x14: return French;
        case 0x15: return Portuguese;
        case 0x16: return Turkish;
        case 0x1d: return Serbian_Latin;
        case 0x1f: return Rumanian;
        case 0x20: return Serbian_Cyrillic;
        case 0x21: return German;
        case 0x22: return Estonian;
        case 0x23: return Lettish;
        case 0x24: return Russian;
        case 0x25: return Ukrainian;
        case 0x26: return Czech;
        case 0x36: return Turkish;
        case 0x37: return Greek;
        case 0x40: return English;
        case 0x44: return French;
/*        case 0x47: return Arabic;
        case 0x55: return Hebrew;
        case 0x57: return Arabic;*/
        default:   return Latin;
    }
}

uint8_t language_g0_g2(uint8_t data)
{
	switch(data)
	{
		case 0x00: return English;
		case 0x01: return German;
		case 0x02: return Swedish;
		case 0x03: return Italian;
		case 0x04: return French;
		case 0x05: return Portuguese;
		case 0x06: return Czech;
		case 0x07: return Rumanian;
		case 0x08: return Polish;
		case 0x09: return German;
		case 0x0a: return Swedish;
		case 0x0b: return Italian;
		case 0x0c: return French;
		case 0x0e: return Czech;
		case 0x10: return English;
		case 0x11: return German;
		case 0x12: return Swedish;
		case 0x13: return Italian;
		case 0x14: return French;
		case 0x15: return Portuguese;
		case 0x16: return Turkish;
		case 0x1d: return Serbian_Latin;
		case 0x1f: return Rumanian;
		case 0x20: return Serbian_Cyrillic;
		case 0x21: return German;
		case 0x22: return Estonian;
		case 0x23: return Lettish;
		case 0x24: return Russian;
		case 0x25: return Ukrainian;
		case 0x26: return Czech;
		case 0x36: return Turkish;
		case 0x37: return Greek;
		case 0x40: return English;
		case 0x44: return French;
		case 0x47: return Arabic;
		case 0x55: return Hebrew;
		case 0x57: return Arabic;
		default:   return Latin;
	}
	return Latin;
}





/*****************************************************************************
 * Function	   : teletext_init_x29
 * Description : initionalize the buffer used for packet 29
 * Arguments   : void
 * Returns     :
 * Other       :
 ****************************************************************************/
void teletext_init_x29(void)
{
	uint8_t i;
    for(i=0;i<8;i++)
    {
        g_X29PackView[i].m_chDefaultG02= 0;//g_chDefaultG02;
        g_X29PackView[i].m_chDefaultG0G2=0; //g_chDefaultG0G2;
        g_X29PackView[i].m_chDefaultFrontClut=0;
        g_X29PackView[i].m_chDefaultFrontCol=7;
        g_X29PackView[i].m_chX29Langctrl = 0;
    }
}

/*****************************************************************************
 * Function	   : teletext_clean_pre_page
 * Description : clean page buffer
 * Arguments   : void
 * Returns     : COM_TTX_BUF_CLEAN_NO_ERROR : ok
                     COM_TTX_BUF_CLEAN_ERROR :error
 * Other       :
 ****************************************************************************/
void teletext_clean_pre_page(void)
{
    g_pBuffer->m_chNowMagazine =9;
	uint16_t x,y;
    for(x=0;x<30;x++)
    {
        for(y=0;y<41;y++)
        {
            g_pBuffer->m_chPageContext[x][y]=32;
        }
    }
}
/*****************************************************************************
 * Function	   : ttx_page_buffer_clear
 * Description : clean all page buffers
 * Arguments   : void
 * Returns     : COM_TTX_BUF_CLEAN_NO_ERROR : ok
                     COM_TTX_BUF_CLEAN_ERROR :error
 * Other       :
 ****************************************************************************/
void teletext_clear_show_page(void)
{
	g_TTXPageShow.m_chDefaultBackClut = 0;
	g_TTXPageShow.m_chDefaultBackCol = 0;
	g_TTXPageShow.m_chDefaultFrontClut=0;
	g_TTXPageShow.m_chDefaultFrontCol=7;
	g_TTXPageShow.m_chDefaultG02 =0;
	g_TTXPageShow.m_chDefaultG0G2 =0;
	g_TTXPageShow.m_chX28Langctrl=0;
	g_TTXPageShow.m_chShow24=0;
	g_TTXPageShow.m_chShowState=0;
	uint8_t chVaryX,chVaryY;
	for(chVaryX = 0;chVaryX <26;chVaryX++)
	{
		for(chVaryY = 0; chVaryY <40; chVaryY ++)
		{
			if((chVaryX==25)&&(chVaryY==TTX_AUTO_REFRESH_COLOR))
				g_TTXPageShow.m_chModChar[chVaryX][chVaryY]=RED;
			else
			{
				if((chVaryX==25)&&(chVaryY== TTX_AUTO_CHAR_FONT))
					g_TTXPageShow.m_chModChar[chVaryX][chVaryY]=0x1b;
				else
					g_TTXPageShow.m_chModChar[chVaryX][chVaryY]=0x20;
			}
		}
	}
	g_TTXPageShow.m_chModChar[25][TTX_AUTO_CHAR_FONT + 1]='A';
	g_TTXPageShow.m_chModChar[25][TTX_AUTO_CHAR_FONT + 2]='U';
	g_TTXPageShow.m_chModChar[25][TTX_AUTO_CHAR_FONT + 3]='T';
	g_TTXPageShow.m_chModChar[25][TTX_AUTO_CHAR_FONT + 4]='O';
	g_TTXPageShow.m_chModChar[25][TTX_AUTO_CHAR_FONT + 5]=0x1b;
	g_TTXPageShow.m_chModChar[25][TTX_AUTO_CHAR_FONT + 6]=WHITE;

	memset(g_TTXPageShow.m_X26Pack,0xff,208*(sizeof(struct x_26_pack)));
	for(chVaryX = 0; chVaryX < 4; chVaryX++)
	{
        //gxlogd("Clear Link Enable!\n");
		g_TTXPageShow.m_PageLink[chVaryX].m_chLinkEn = 0;
		g_TTXPageShow.m_PageLink[chVaryX].m_chMagazine= 0;
		g_TTXPageShow.m_PageLink[chVaryX].m_chPage= 0;
		g_TTXPageShow.m_PageLink[chVaryX].m_wSubcode= 0;
	}

	teletext_init_x29();
}

int page_buffer_force_release(unsigned char SearchMagazie) //SearchMagazie: from 0 to 7
{
#define TTX_MAX_MAGAZINE_NUM 8
	struct page_buffer *pBufferTem;
	uint16_t i, wReleaseNum = 0;
	uint32_t nBlocks = 0;
	char before_num = 0;
	char chReleaseMN = 0;
    uint8_t chReleasePN = 0xFF;
	int cache_magazines = teletext_get_magazine_cache_num();

	if((SearchMagazie > 7) || (cache_magazines < 1))
	{
		return -1;
	}

	before_num = (((cache_magazines - 1)%2) > 0)?(((cache_magazines - 1)/2) + 1):((cache_magazines - 1)/2);

    //get the magazine number to release
    if(SearchMagazie >= before_num)
        chReleaseMN = SearchMagazie - before_num;
    else
        chReleaseMN = SearchMagazie + TTX_MAX_MAGAZINE_NUM - before_num;

    gxlogd("%s:[%d] Magazine %d need to release !! \n", __func__, __LINE__, chReleaseMN);

    //release one page from the magazine
	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	for(i=0;i<nBlocks;i++)
	{
		pBufferTem=(struct page_buffer*)(pSppBuffer+(i*PAGE_BUFFER_SIZE));
		if((chBufBlocFlag[i]==1)&&((pBufferTem->m_chNowMagazine)==chReleaseMN))
		{
            if((pBufferTem->m_chNowPage < chReleasePN))
            {
                //gxlogd("[TTX] Find page need to release : %d buffer index : %d\n", pBufferTem->m_chNowPage, i);
                wReleaseNum = i;
                chReleasePN = pBufferTem->m_chNowPage;
            }
		}
	}

    pBufferTem=(struct page_buffer*)(pSppBuffer+(wReleaseNum*PAGE_BUFFER_SIZE));
    if(pBufferTem != NULL)
    {
        gxlogd("[TTX] Find page need to release : %d buffer index : %d\n", pBufferTem->m_chNowPage, wReleaseNum);
        chBufBlocFlag[wReleaseNum]=0;
        pBufferTem->m_NextPage=NULL;
        pBufferTem->m_PriorPage=NULL;
    }

    return 0;
}

void page_buffer_unused_release(uint8_t chMagazineNum)
{
	struct page_buffer *pBufferTem;
	uint16_t i;
	uint32_t nBlocks = 0;

	if(CcOpenFlag)
	{
		return;
	}
	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	for(i=0;i<nBlocks;i++)
	{
		pBufferTem=(struct page_buffer*)(pSppBuffer+(i*PAGE_BUFFER_SIZE));
		if((chBufBlocFlag[i]==1)&&((pBufferTem->m_chNowMagazine)==chMagazineNum))
		{
			chBufBlocFlag[i]=0;
			pBufferTem->m_NextPage=NULL;
			pBufferTem->m_PriorPage=NULL;
		}
		else if(pBufferTem->m_chNowMagazine>7)
		{
			chBufBlocFlag[i]=0;
			pBufferTem->m_NextPage=NULL;
			pBufferTem->m_PriorPage=NULL;
		}
	}
	g_pTTXCurPage[chMagazineNum]=NULL;

}

/*****************************************************************************
 * Function	   : initial_teletext
 * Description : initionalize all data and buffer
 * Arguments   : void
 * Returns     :
 * Other       :
 ****************************************************************************/
void initial_teletext()
{
	uint8_t i;
	g_chDefaultG0G2 = 0;//English;
	g_chDefaultG02= 0;//English;
	gs_chPacketCount = 0;
	s_chCheckPacketLen = 0;
	g_chTtxOsdExit = 0;
	gxlogd("\nttx clear g_chTtxOsdExit\n");
	s_chTtxReceive=0;
    //SDRAM存储区定义
    s_chWhetherShowBackGround=TTX_SHOW_BACK_GROUND;
	nPackNum=0;
	for(i=0;i<4;i++)
	{
		s_wDefaultNavigationPage[i]=0xffff;
	}
#ifdef TTX_NEW_CHARACTER_SET
	if(1==CcOpenFlag)
	{
		g_old_resolution = g_resolution;
		if(g_resolution)
		{
			g_chTtxStartAddY = 30;
		}
		else
		{
			g_chTtxStartAddY = 78;
		}
	}
	else
	{
		g_chTtxStartAddY = TTX_SURFACE_Y_START;
	}
	g_chTtxStartAddX = TTX_SURFACE_X_START;
#else
	g_chTtxStartAddX=25;
#endif

	g_chSearchMagNum = g_chMagzineNumber;
	g_wSearchPageNum = g_wPageNumber;
	g_wSearchSubCode=0x3f7f;
	g_chPageRefreshMode=TTX_REFRESH_AUTO;
	g_pBuffer=(struct page_buffer*)(&g_TtxBuffer);
	g_pBuffer->m_chTransMeth = 0;
	//并行方式
	if(CcOpenFlag==0)
	{
		g_chSearchNotSetPosition = 0;
	}
	else
	{
		g_chSearchNotSetPosition=1;
	}
#ifdef TTX_PRE_DECODE
	if(teletext_check_is_pre_process_mode())
	{
		extern int get_ttx_pre_flag(void);
		if(get_ttx_pre_flag() <= 0)
		{
			g_chSetNum = 0;
			g_bPageRecived = 0;
			s_chTtxPageRefreshEnable=0;
			for(i=0;i<8;i++)
			{
				g_pTTXCurPage[i]=NULL;
				page_buffer_unused_release(i);
			}
			teletext_clean_pre_page();
			teletext_clear_show_page();
			if(0==CcOpenFlag)
			{
				draw_wait_page();
			}
		}
		else if(CcOpenFlag == 0)
		{
			if(check_page_have(g_chSearchMagNum, g_wSearchPageNum, g_wSearchSubCode))
			{
				s_chTtxPageRefreshEnable = 2;
				gs_ucTTXRefreshCounter = 1;
				ttx_page_refresh();
			}
			else
			{
				g_chSetNum = 0;
				g_bPageRecived = 0;
				s_chTtxPageRefreshEnable=0;
				draw_wait_page();
			}
		}
	}
	else
#endif
	{
		g_chSetNum = 0;
		g_bPageRecived = 0;
		s_chTtxPageRefreshEnable=0;
		for(i=0;i<8;i++)
		{
			g_pTTXCurPage[i]=NULL;
			page_buffer_unused_release(i);
		}
		teletext_clean_pre_page();
		teletext_clear_show_page();
		if(0==CcOpenFlag)
		{
			draw_wait_page();
		}
	}

}

/*****************************************************************************
 * Function	   : teletext_analysis
 * Description : analysis each packet,compose a page with different packets
 * Arguments   : pPacket:  address of the packet
                       chPacketAddress:the number of the packet
                       chMagazineAddress: the number of the magazine
 * Returns     :
 * Other       :
 ****************************************************************************/
void teletext_analysis(struct page_buffer *pPacket, uint8_t chMagazineAddress)
{
	uint8_t chPacketAddress;
	struct page_buffer *pPageBufferTem;
	teletext_clear_show_page();
    uint16_t wNowSubCode = pPacket->m_wNowSubCode;
    uint16_t wSubCodeTmp = 0;

	pPageBufferTem = pPacket;
	while(pPageBufferTem!=NULL)
	{
        wSubCodeTmp = pPageBufferTem->m_wNowSubCode;
        //gxlogd("==>> subcode : %d\n", wSubCodeTmp);
        if((wSubCodeTmp > 0) && ((wSubCodeTmp&0xf0) == (wNowSubCode&0xf0)))
        {
            if(wSubCodeTmp < 10)
            {
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp-1)*3]
                    =WHITE;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp-1)*3+1]
                    =((wSubCodeTmp&0xf0)>>4)+0x30;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp-1)*3+2]
                    =(wSubCodeTmp&0xf)+0x30;
            }
            else
            {
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp%16)*3]
                    =WHITE;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp%16)*3+1]
                    =((wSubCodeTmp&0xf0)>>4)+0x30;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp%16)*3+2]
                    =(wSubCodeTmp&0xf)+0x30;
            }
        }

        if(wSubCodeTmp >= ((wNowSubCode/16)+1)*16)
        {
			g_TTXPageShow.m_chModChar[25][37] = GREEN;
			g_TTXPageShow.m_chModChar[25][38] = '-';
			g_TTXPageShow.m_chModChar[25][39] = '>';
            break;
        }

		pPageBufferTem=pPageBufferTem->m_NextPage;
        //gxlogd("==>> Buffer : %p\n", pPageBufferTem);
	}

	pPageBufferTem = pPacket;
	while(pPageBufferTem!=NULL)
	{
        wSubCodeTmp = pPageBufferTem->m_wNowSubCode;
        //gxlogd("==>> pro subcode : %d\n", wSubCodeTmp);
        if((wSubCodeTmp > 0) && ((wSubCodeTmp&0xf0) == (wNowSubCode&0xf0)))
        {
            if(wSubCodeTmp < 10)
            {
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp-1)*3]
                    =WHITE;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp-1)*3+1]
                    =((wSubCodeTmp&0xf0)>>4)+0x30;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp-1)*3+2]
                    =(wSubCodeTmp&0xf)+0x30;
            }
            else
            {
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp%16)*3]
                    =WHITE;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp%16)*3+1]
                    =((wSubCodeTmp&0xf0)>>4)+0x30;
                g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wSubCodeTmp%16)*3+2]
                    =(wSubCodeTmp&0xf)+0x30;
            }
        }

        if((wNowSubCode >= 16) && (wSubCodeTmp > 0) && (wSubCodeTmp <= ((((wNowSubCode/16)-1)*16) + 9)))
        {
			g_TTXPageShow.m_chModChar[25][37] = GREEN;
			g_TTXPageShow.m_chModChar[25][38] = '<';
			g_TTXPageShow.m_chModChar[25][39] = '-';
            break;
        }

		pPageBufferTem=pPageBufferTem->m_PriorPage;
        //gxlogd("==>> pro Buffer : %p\n", pPageBufferTem);
	}

	if((g_chPageRefreshMode==TTX_REFRESH_SUBPAGE)&&(g_wSearchSubCode==wNowSubCode))
	{
		g_TTXPageShow.m_chModChar[25][TTX_AUTO_REFRESH_COLOR]=WHITE;
		if(g_wSearchSubCode>9)
			g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(g_wSearchSubCode%16)*3]=RED;
		else if(g_wSearchSubCode>0)
			g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(g_wSearchSubCode-1)*3]=RED;
		else
			g_TTXPageShow.m_chModChar[25][TTX_AUTO_REFRESH_COLOR]=RED;

	}

    if(g_chPageRefreshMode==TTX_REFRESH_AUTO)
    {
        g_TTXPageShow.m_chModChar[25][TTX_AUTO_REFRESH_COLOR]=RED;
        if(wNowSubCode>9)
            g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wNowSubCode%16)*3]=YELLOW;
        else if(wNowSubCode>0)
            g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+(wNowSubCode-1)*3]=YELLOW;
        else
            g_TTXPageShow.m_chModChar[25][TTX_AUTO_REFRESH_COLOR]=RED;
    }

	for(chPacketAddress=0;chPacketAddress<30;chPacketAddress++)
    {
        if(pPacket->m_chPageContext[chPacketAddress][40]!=1)
            continue;
        switch (chPacketAddress)
        {
            case 0:
                memcpy(&g_TTXPageShow.m_chModChar[0][0],
                        &pPacket->m_chPageContext[0][0],40);
                memcpy(&g_TTXPageShow.m_chModChar[0][32],&g_pBuffer->m_chPageContext[0][32],8);
                g_TTXPageShow.m_chShowState = pPacket->m_chC4_C10;
                g_TTXPageShow.m_chX28Langctrl= pPacket->m_chDefaultG0G2;
                //gxlogd("X28LangCtrl : %d C4_C10 : 0x%x \n", g_TTXPageShow.m_chX28Langctrl, g_TTXPageShow.m_chShowState);
                break;
            case 26:
                teletext_dec_x26(pPacket);
                break;
            case 27:
                teletext_dec_x27(pPacket);
                break;
            case 28:
                teletext_dec_x28(pPacket);
                break;
            case 29:
                teletext_dec_x29(chMagazineAddress,pPacket);
                break;
            default:
                if((chPacketAddress>0)&&(chPacketAddress<25))
                    teletext_dec_x1_24(pPacket,chPacketAddress);
                break;
        }
    }
}
/*****************************************************************************
 * Function	   : check_page_save
 * Description : check whether the page is exist
 * Arguments   : chMagazineNum      [IN]:the magazine number of the page
 				 chPageNum          [IN]:the page number of the page
 				 wSubPageNum        [IN]:the sub_page number of the page
 				 ppPageBufferAdd    [OUT]:the address of the page buffer
 * Returns     :
 * Other       :
 ****************************************************************************/
void check_page_save(uint8_t chMagazineNum, uint16_t chPageNum,uint16_t wSubPageNum,struct page_buffer**ppPageBufferAdd)
{
	uint16_t i;
	struct page_buffer *PageBufferTem=NULL;
	uint32_t nBlocks;

	if(CcOpenFlag)
	{
		return;
	}
	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	//gxlogd("\n[1]M = %d, p = %d\n", chMagazineNum, chPageNum);
	for(i=0; i<nBlocks; i++)
	{
		PageBufferTem = (struct page_buffer *)(pSppBuffer+(PAGE_BUFFER_SIZE*i));
		if(chBufBlocFlag[i]==1)
		{
			if((PageBufferTem->m_chNowMagazine==chMagazineNum)
				&&(PageBufferTem->m_chNowPage==chPageNum)
				&&(PageBufferTem->m_wNowSubCode==wSubPageNum))
			{
				*ppPageBufferAdd = PageBufferTem;
				return;
			}
		}
	}
	*ppPageBufferAdd = NULL;


}
/*****************************************************************************
 * Function	   : page_buffer_link_open
 * Description : disconnect the link
 * Arguments   : pBuffer           [IN]:the first address of the PES packet
 				 nBufferLen        [IN]:the length of the PES packet
 * Returns     :
 * Other       :
 ****************************************************************************/
void page_buffer_link_open(void)
{
	uint16_t i;
	struct page_buffer *pBufferTem;
	uint32_t nBlocks = 0;

	if(CcOpenFlag)
	{
		return;
	}

	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	for(i=0;i<nBlocks;i++)
	{
		pBufferTem = (struct page_buffer*)(pSppBuffer+(PAGE_BUFFER_SIZE*i));
		pBufferTem->m_NextPage=NULL;
		pBufferTem->m_PriorPage=NULL;
	}
}

static  int ttx_page_refresh_flag_set(void *userdata)
{
	s_chTtxPageRefreshEnable++;
	return 0;
}
void com_timer_create(void)
{
	int time = 500;
	ptime = create_timer(ttx_page_refresh_flag_set, time, NULL,  TIMER_REPEAT);
}

void com_timer_start(void)
{
	reset_timer(ptime);
}
void com_timer_stop(void)
{
	timer_stop(ptime);
}

void com_timer_delete(void)
{
	remove_timer(ptime);
}

int check_magazie_need_cache(unsigned char SearchMagazie, unsigned char DecodeMagazine)
{// SearchMagazie: from 0 to 7; DecodeMagazine: from 0 to 7. [0 means num 8]
// TTX PROTOCOL
#define TTX_MAX_MAGAZINE_NUM 8
	char before_num = 0;
	char after_num = 0;
	int Result = 0;
	int cache_magazines = teletext_get_magazine_cache_num();

	if((SearchMagazie > 7) || (DecodeMagazine > 7) || (cache_magazines < 1))
	{
		//gxlogd("\nTTX, error, %s, %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(cache_magazines == TTX_MAX_MAGAZINE_NUM)
	{
		 Result = 1;
		 //gxlogd("\nTTX, %s, %d\n", __FUNCTION__, __LINE__);
		 goto exit;
	}

	before_num = (((cache_magazines - 1)%2) > 0)?(((cache_magazines - 1)/2) + 1):((cache_magazines - 1)/2);
	after_num = (cache_magazines - 1)/2;


// before deal with
	if(SearchMagazie >= before_num)
	{
		if((DecodeMagazine >= (SearchMagazie - before_num))
			 && (DecodeMagazine <= SearchMagazie))
		{
			 Result = 1;
			 //gxlogd("\nTTX, %s, %d\n", __FUNCTION__, __LINE__);
			 goto exit;
		}
	}
	else
	{
		if((DecodeMagazine <= SearchMagazie)
			 || (DecodeMagazine >= (TTX_MAX_MAGAZINE_NUM - (before_num - SearchMagazie))))
		{
			 Result = 1;
			 //gxlogd("\nTTX, %s, %d\n", __FUNCTION__, __LINE__);
			 goto exit;
		}
	}
// after deal with
	if((after_num + SearchMagazie) < TTX_MAX_MAGAZINE_NUM)
	{
		if((DecodeMagazine >= SearchMagazie)
			&& (DecodeMagazine <= (after_num + SearchMagazie)))
		{
			 Result = 1;
			 //gxlogd("\nTTX, %s, %d\n", __FUNCTION__, __LINE__);
			 goto exit;
		}
	}
	else
	{
		if(((DecodeMagazine < TTX_MAX_MAGAZINE_NUM)
			    && (DecodeMagazine >= SearchMagazie))
			|| (DecodeMagazine <= (after_num - (TTX_MAX_MAGAZINE_NUM - SearchMagazie))))
		{
			 Result = 1;
			 //gxlogd("\nTTX, %s, %d\n", __FUNCTION__, __LINE__);
			 goto exit;
		}
	}
exit:
	//gxlogd("\nTTX, %d, b=%d, a=%d, s=%d, c=%d\n",Result, before_num, after_num, SearchMagazie, DecodeMagazine);
	return Result;
}


#ifdef TTX_PRE_DECODE
static int sg_TTXThreadControl = 0;
static int sg_TTXThreadHandle = 0;
extern uint8_t*  pSppBuffer;//for ttx
static int sg_TTXPreDecodeEnable = 0;

extern status_t ttx_demux_open(void);
extern status_t ttx_filter_free(void);
extern status_t ttx_filter_setup(uint16_t ttx_pid);
extern status_t ttx_filter_query (uint8_t* pflag);
extern size_t ttx_filter_read(uint8_t  *data_buf, size_t  data_len);

// filter ttx data
int init_ttx_demux(unsigned short ttx_pid)
{
	int ret = 0;

	ret = ttx_demux_open();
	if(ret < 0)
	{
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		return ret;
	}
	ttx_filter_free();

	ret = ttx_filter_setup(ttx_pid);
	if(ret < 0)
	{
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		return ret;
	}
	return ret;
}

int init_ttx_pre_memory(void)
{
	int ret = 0;
	uint32_t nBlocks = 0;
#ifdef TTX_PRE_KEEP_DATA
	if(pSppBuffer == NULL)
	{
		nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
		pSppBuffer = (uint8_t*)av_malloc(PAGE_BUFFER_SIZE*nBlocks);
		if(pSppBuffer == NULL)
		{
			gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		memset(pSppBuffer, 0, PAGE_BUFFER_SIZE*nBlocks);
		//gxlogd("\nTTX, %s, %d\n",__FUNCTION__,__LINE__);
	}

#else
	if(pSppBuffer != NULL)
		av_free(pSppBuffer);
	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	pSppBuffer = (uint8_t*)av_malloc(PAGE_BUFFER_SIZE*nBlocks);
	if(pSppBuffer == NULL)
	{
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	memset(pSppBuffer, 0, PAGE_BUFFER_SIZE*nBlocks);
#endif
	sg_TTXPreDecodeEnable = 0;
	return ret;
}

int destroy_ttx_pre_memory(void)
{
	int ret = 0;

	if((pSppBuffer != NULL) && (sg_TTXPreDecodeEnable == 0))
	{
		int i = 0;
		for(i=0;i<8;i++)
		{
			g_pTTXCurPage[i]=NULL;
			page_buffer_unused_release(i);
		}
		av_free(pSppBuffer);
		pSppBuffer = NULL;
		//gxlogd("\nTTX, free memory, %s, %d\n",__FUNCTION__,__LINE__);
	}
	return ret;
}

int destroy_ttx_pre_dmux(void)
{
	int ret = 0;
	ttx_filter_free();
	//ttx_demux_close();
	return ret;
}

int destroy_ttx_pre_process(void)
{
	int ret = 0;
	if(sg_TTXThreadControl == 0)
		return 0;
	destroy_ttx_pre_dmux();
	sg_TTXThreadControl = 0;
	//gxlogd("\nTTX, destroy, %s, %d\n",__FUNCTION__,__LINE__);
	//GxCore_ThreadDelay(100);
	if(sg_TTXThreadHandle > 0)
		GxCore_ThreadJoin(sg_TTXThreadHandle);
	sg_TTXThreadHandle = 0;
	//gxlogd("\nTTX, destroy, %s, %d\n",__FUNCTION__,__LINE__);
	destroy_ttx_pre_memory();
	return ret;
}

unsigned char* get_ttx_pre_data(void)
{
#ifdef TTX_PRE_KEEP_DATA
	if(sg_TTXThreadControl > 0)
	{
		sg_TTXPreDecodeEnable = 1;
	}
	return pSppBuffer;
#else
	if(pSppBuffer)
		sg_TTXPreDecodeEnable = 1;

	return pSppBuffer;
#endif
}

int get_ttx_pre_flag(void)
{
	return sg_TTXPreDecodeEnable;
}

int is_in_ttx_pre_process(void)
{
	return sg_TTXThreadControl;
}


int ttx_data_process(unsigned char *pData,unsigned short *plength)
{
extern void teletext_dec(uint8_t* pBuffer,uint32_t nBufferLen);
	int ret = 0;
	unsigned int DataAddr = 0;
	unsigned short Len = 0;

    if((*(uint8_t*)(pData)==0)&&(*(uint8_t*)(pData+1)==0)
				&&(*(uint8_t*)(pData+2)==1)
				&&(*(uint8_t*)(pData+3)==0xbd))
   	{// right pes package
		DataAddr=(unsigned int)(pData);
		Len = pes_packet_head__PES_packet_length_h(DataAddr);
		Len = Len << 8;
		Len = Len +pes_packet_head__PES_packet_length_l(DataAddr);
		if((Len >= PES_PACKET_MAX_LEN)
			|| (pes_packet_head__PES_header_data_length(DataAddr) != 0x24))
		{
			gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		// decode the data: pData, 6+Len
		teletext_dec((uint8_t*)(pData + 46), Len);
	}
	else
	{// wrong pes package
		ttx_filter_resert();
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	return ret;
}


void ttx_pre_filter_process(void *arg)
{
	unsigned char flag = 0;
	unsigned int read_size;
	unsigned char *pData = NULL;
	unsigned short length= 0;
	int ret = 0;
	unsigned char *pPesData = NULL;
	pPesData = (unsigned char *)av_malloc(PES_PARSE_BUF_SIZE);
	if(pPesData == NULL)
	{
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		sg_TTXThreadControl = 0;
		destroy_ttx_pre_dmux();
		destroy_ttx_pre_process();
	}
	while(sg_TTXThreadControl > 0)
	{
		ret = ttx_filter_query(&flag);
		if((ret == 0) && (flag != 0))
		{// have ttx data to do

			read_size = ttx_filter_read(pPesData, PES_PARSE_BUF_SIZE);
			pData = pPesData;
			while(read_size>0)
			{
				length = (unsigned short)(((pData[4]<<8)|pData[5]) + 6);
				if((length > read_size)||length<=8)
				{
					return;
				}
				ret = ttx_data_process(pData,&length);
				if(ret < 0)
					break;
				pData +=length;
				read_size -= length;
			}
		}
	}
	if(pPesData == NULL)
	{
		av_free(pPesData);
	}

}

int init_ttx_pre_process(unsigned short ttx_pid)
{
	int ret = 0;
	int i = 0;
	// init teletext global value

#ifdef TTX_PRE_KEEP_DATA
	if(sg_TTXThreadControl == 1)
		return -1;
#else
	if((pSppBuffer != NULL) || (sg_TTXThreadControl == 1))
		return -1;
#endif
	CcOpenFlag = 0;
	g_pBuffer=(struct page_buffer*)(&g_TtxBuffer);
	g_pBuffer->m_chTransMeth = 0;              //并行方式
	g_chSearchNotSetPosition = 0;
	g_chSetNum = 0;
	g_chSearchMagNum = 1;
	g_wSearchPageNum = 0;
	g_wSearchSubCode = 0x3f7f;
	g_chPageRefreshMode = 0;
	g_bPageRecived = 0;

	// memory
	ret = init_ttx_pre_memory();
	if(ret < 0)
	{
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
#ifdef TTX_PRE_KEEP_DATA
	for(i = 0; i < 8; i++)
	{
		if(check_magazie_need_cache(1,i) == 0)
		{
			g_pTTXCurPage[i]=NULL;
			page_buffer_unused_release(i);
			//gxlogd("\nTTX, release magazine = %d\n", i);
		}
	}
#else
	for(i=0;i<8;i++)
	{
		g_pTTXCurPage[i]=NULL;
		page_buffer_unused_release(i);
	}
#endif
	teletext_clean_pre_page();
	teletext_clear_show_page();
	// thread
	sg_TTXThreadControl = 1;
	GxCore_ThreadCreate("ttx_pre_process", &sg_TTXThreadHandle, ttx_pre_filter_process, NULL, 128 * 1024, GXOS_DEFAULT_PRIORITY);

	//demux
	ret = init_ttx_demux(ttx_pid);
	if(ret < 0)
	{
		gxlogd("\nTTX, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	//gxlogd("\nTTX, %s, %d\n",__FUNCTION__,__LINE__);
	return ret;
}
#endif

/*****************************************************************************
 * Function	   : teletext_dec
 * Description : decode the PES packet
 * Arguments   : pBuffer           [IN]:the first address of the PES packet
 				 nBufferLen        [IN]:the length of the PES packet
 * Returns     :
 * Other       :
 ****************************************************************************/
static struct page_buffer CCdataBuffer;

void teletext_dec(uint8_t* pBuffer,uint32_t nBufferLen)
{
	uint16_t wRepeatSearch;
	volatile uint16_t i,t;
	uint8_t *pPacket, *pPesPacket;
	static struct page_buffer *g_pTTXTailPage = NULL;
	uint8_t  chTmp1, chTmp2 ;
	struct page_buffer *pBufferTem;
	static uint8_t s_chMagazineAddress,chPacketAddress,chPreValidPack;
	static uint8_t s_chTtxPacketLength;
	static uint8_t s_nTtxHeadRefresh;
	//uint8_t bTtxDecEnable;
	s_chTtxPacketLength = 46;

	wRepeatSearch = nBufferLen/s_chTtxPacketLength;
//	chPageExist = 0;
	for (i = 0;i <wRepeatSearch; i++)                             //保证此PES包中还至少包含一个完整的packet
	{
		pPesPacket = teletext_buffer_packet_ptr(pBuffer,i,s_chTtxPacketLength);

		if((0== CcOpenFlag))
		{
            if( (pes_datafield_head__data_unit_id(pPesPacket)== 0x2)||(pes_datafield_head__data_unit_id(pPesPacket)== 0x3))//过滤空teletext包
            {
                if(*(pPesPacket+1)!=0x2c)
                {
                    gxlogd("(*(pPesPacket+1)!=0x2c) \n");
                    continue;
                }
                pPacket = pes_datafield_head__packet_ptr(pPesPacket);
                //得到杂志号
                chTmp1=teletext_pack_head__magazine_address(pPacket);
                chTmp2 = ((chTmp1&(uint8_t)0x40)>>6) + ((chTmp1&(uint8_t)0x10)>>3) + ((chTmp1&(uint8_t)0x4));
                s_chMagazineAddress = chTmp2 ;
                chTmp2 = teletext_pack_head__packet_address(pPacket);
                //得到包号
                chPacketAddress = ((chTmp2&(uint8_t)0x1)<<4) + ((chTmp2&(uint8_t)0x4)<<1) + ((chTmp2&(uint8_t)0x10)>>2)
                    + ((chTmp2&(uint8_t)0x40)>>5) +((chTmp1&(uint8_t)0x1));
                chPacketAddress = chPacketAddress & 0x1f;
                if(chPacketAddress==0)
                {
                    teletext_dec_x0(pPacket, s_chMagazineAddress);
                    if(	g_chSearchNotSetPosition == 0)
                    {
                        g_TTXPageShow.m_chModChar[0][3] = 7;
                        if (g_chSearchMagNum==0)
                        {
                            g_TTXPageShow.m_chModChar[0][4]=g_chSearchMagNum+0x30+8;
                        }
                        else
                        {
                            g_TTXPageShow.m_chModChar[0][4]=g_chSearchMagNum+0x30;
                        }
                        g_TTXPageShow.m_chModChar[0][5] = g_wSearchPageNum/10+0x30;
                        g_TTXPageShow.m_chModChar[0][6] = g_wSearchPageNum%10+0x30;

                        memcpy(&g_TTXPageShow.m_chModChar[0][7],&g_pBuffer->m_chPageContext[0][7],33);
                    }
                    else
                    {
                        memcpy(&g_TTXPageShow.m_chModChar[0][32],&g_pBuffer->m_chPageContext[0][32],8);
                    }
                    // for pre-process
#ifdef TTX_PRE_DECODE
                    if((teletext_check_is_pre_process_mode() == 0)
                            || (is_in_ttx_pre_process() == 0))
#endif
                    {
                        //防止头包在1秒内重复刷新次数过多
                        if(s_chTtxPageRefreshEnable!=s_nTtxHeadRefresh)
                        {
                            if(s_chLine0IsDoubleH == 1)
                                clear_page_by_line(1);
                            if(0 == draw_screen(1))
                                s_nTtxHeadRefresh=s_chTtxPageRefreshEnable;
                        }
                    }
                }
                // for pre-process
                if(check_magazie_need_cache(g_chSearchMagNum, s_chMagazineAddress) <= 0)
                    continue;

                if(chPacketAddress==0)
                {
                    g_pTTXCurPage[s_chMagazineAddress] = NULL;
                    g_pTTXTailPage = NULL;
                    //if((g_pBuffer->m_chNowMagazine == g_chSearchMagNum) && (g_pBuffer->m_chNowPage == g_wSearchPageNum))
                    //    gxlogd("\033[34m==>> Before check subcode : 0x%x\n\033[0m", g_pBuffer->m_wNowSubCode);
                    check_page_save(g_pBuffer->m_chNowMagazine, g_pBuffer->m_chNowPage,
                            g_pBuffer->m_wNowSubCode,&g_pTTXTailPage);
                    if(g_pTTXTailPage==NULL)
                    {
                        if((1 == ttx_buffer_is_full())
                                && (g_chSearchMagNum == g_pBuffer->m_chNowMagazine)
                                && (g_wSearchPageNum == g_pBuffer->m_chNowPage))
                        {
                            if(0 != page_buffer_force_release(g_chSearchMagNum))
                                continue;
                        }
                        ttx_buffer_malloc((uint8_t**)&g_pTTXTailPage);
                    }
                    if(g_pTTXTailPage==NULL)
                    {
                        //gxlogd("\033[33m %s:[%d] page buf is full. [%01d-%03d-%04x]\n\033[0m",
                        //        __func__,
                        //        __LINE__,
                        //        g_pBuffer->m_chNowMagazine,
                        //        g_pBuffer->m_chNowPage,
                        //        g_pBuffer->m_wNowSubCode);
                        continue;
                    }
                    g_pTTXCurPage[s_chMagazineAddress]=g_pTTXTailPage;
                    if((g_pTTXTailPage->m_chC4_C10&0x1)==1)
                        memset(&g_pTTXTailPage->m_chPageContext[1][0],0,41*29);
                    if((g_bPageRecived == 1)
                            &&(g_pBuffer->m_chNowPage!=g_wSearchPageNum)
                            &&((g_pBuffer->m_chTransMeth)
                                ||(g_pBuffer->m_chNowMagazine==g_chSearchMagNum)))
                    {
                        g_bPageRecived = 2;
                        pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
                        if(pBufferTem!=NULL)
                        {
                            teletext_analysis(pBufferTem, pBufferTem->m_chNowMagazine);
                            g_TTXPageShow.m_chDefaultG0G2=g_X29PackView[pBufferTem->m_chNowMagazine].m_chDefaultG0G2;
                            ttx_set_default_guide_page(pBufferTem);
                            // for pre-process
#ifdef TTX_PRE_DECODE
                            if((teletext_check_is_pre_process_mode() == 0)
                                    || (is_in_ttx_pre_process() == 0))
#endif
                            {
                                draw_screen(0);
                            }
                            page_buffer_link_open();
                        }
                    }
                    if((g_pBuffer->m_chNowMagazine==g_chSearchMagNum)
                            &&(g_pBuffer->m_chNowPage==g_wSearchPageNum))
                    {
                        if(g_chSearchNotSetPosition==0)
                        {
                            g_chSearchNotSetPosition = 1;
                        }
                        if(g_bPageRecived == 0)//保证只将搜索页显示一次
                        {
                            g_bPageRecived = 1;
                        }
                    }
                    memcpy(g_pTTXTailPage,g_pBuffer,47);
                    g_pTTXTailPage->m_chPageContext[0][40] = 1;
                    g_pTTXTailPage->m_NextPage=NULL;
                    g_pTTXTailPage->m_PriorPage=NULL;
                }
                else
                {
                    g_pTTXTailPage = g_pTTXCurPage[s_chMagazineAddress];
                    if((g_pTTXTailPage!=NULL)
                            &&((g_pTTXTailPage->m_chNowMagazine)==s_chMagazineAddress)
                            &&(chPacketAddress<30)
                            /*&&(g_pTTXTailPage->m_chPageContext[chPacketAddress][40]==0)*/)
                    {
                        //if(chPacketAddress >= 26)
                        if((chPacketAddress == 26) || (chPacketAddress == 27))
                        {
                            unsigned char DesignationCode = hamming84(*(pPacket+3));
                            unsigned short Mark = 0;
                            unsigned char *p = NULL;
                            char i = 0;

                            if((DesignationCode < 16) && (DesignationCode >= 0))
                            {
                                switch(chPacketAddress)
                                {
                                    case 26:
                                        memcpy(&g_pTTXTailPage->m_chPageX26Context[DesignationCode][0],(pPacket+3),40);
                                        break;
                                    case 27:
                                        memcpy(&g_pTTXTailPage->m_chPageX27Context[DesignationCode][0],	(pPacket+3),40);
                                        break;
                                    case 28:
                                        //pTemp = g_pTTXTailPage->m_chPageX28Context;
                                        //break;
                                    case 29:
                                        //pTemp = g_pTTXTailPage->m_chPageX29Context;
                                        //break;
                                    default:
                                        //pTemp = g_pTTXTailPage->m_chPageContext;
                                        break;
                                }

                                // 16bit for record the DesignationCode
                                p = g_pTTXTailPage->m_chPageContext[chPacketAddress];
                                Mark =p[0]|(p[1]<<8);
                                Mark = Mark | (1 << DesignationCode);
                                p[0] = Mark&0xff;
                                p[1] = (Mark>>8)&0xff;
                                // one byte for record the m_chPageX26Context count
                                p[2] = 0;
                                for(i = 0; i <16; i++)
                                    p[2] += (((Mark&(1<<i))==1)?1:0);

                                g_pTTXTailPage->m_chPageContext[chPacketAddress][40]=1;
                            }
                        }
                        else
                        {
                            memcpy(&g_pTTXTailPage->m_chPageContext[chPacketAddress][0],
                                    (pPacket+3),40);
                            g_pTTXTailPage->m_chPageContext[chPacketAddress][40]=1;
                        }
                    }
                }
            }
		}
		else
		{
			if((pes_datafield_head__data_unit_id(pPesPacket)== 0x2)||(pes_datafield_head__data_unit_id(pPesPacket)== 0x3))
			{
				if(*(pPesPacket+1)!=0x2c)
				{
					//gxlogd("(*(pPesPacket+1)!=0x2c) \n");
					continue;
				}
				pPacket = pes_datafield_head__packet_ptr(pPesPacket);
				//得到杂志号
				chTmp1=teletext_pack_head__magazine_address(pPacket);
				chTmp2 = ((chTmp1&(uint8_t)0x40)>>6) + ((chTmp1&(uint8_t)0x10)>>3) + ((chTmp1&(uint8_t)0x4));
				s_chMagazineAddress = chTmp2 ;
				chTmp2 = teletext_pack_head__packet_address(pPacket);
				//得到包号
				chPacketAddress = ((chTmp2&(uint8_t)0x1)<<4) + ((chTmp2&(uint8_t)0x4)<<1) + ((chTmp2&(uint8_t)0x10)>>2)
				    + ((chTmp2&(uint8_t)0x40)>>5) +((chTmp1&(uint8_t)0x1));
				if(g_chSearchMagNum!=s_chMagazineAddress)
					continue;
				if(chPacketAddress==0)
				{
					if(s_chTtxReceive)
					{
						teletext_analysis(&CCdataBuffer, g_chSearchMagNum);
						for(i=1;i<24;i++)
						{
							if(CCdataBuffer.m_chPageContext[i][40])
							{
								g_TTXPageShow.m_chDefaultG0G2=g_X29PackView[CCdataBuffer.m_chNowMagazine].m_chDefaultG0G2;
								draw_screen_by_line( 0,i);//draw text
								for(t=0;t<40;t++)
								{
									if((g_TTXPageShow.m_chModChar[i][t]==0x0d)
										||(g_TTXPageShow.m_chModChar[i][t]==0x0f))//make next row not be cleaned when this row is double height
									{
										i++;
										break;
									}
								}
						  	 }
					 		else
							{
								draw_screen_by_line( 1,i);//clean this row
							}
						}
						s_chTtxReceive=0;
						s_chTtxPageRefreshEnable=0;
					}
					teletext_dec_x0(pPacket, s_chMagazineAddress);

					for(chTmp1=0;chTmp1<30;chTmp1++)
					{
						CCdataBuffer.m_chPageContext[chTmp1][40] = 0;
					}

					if(g_pBuffer->m_chNowPage==g_wSearchPageNum)
					{
						memcpy(&CCdataBuffer,g_pBuffer,47);// ?
						CCdataBuffer.m_chPageContext[0][40] = 1;
						CCdataBuffer.m_NextPage=NULL;
						CCdataBuffer.m_PriorPage=NULL;
                        g_bPageRecived=1;
                        s_chTtxReceive=1;
					}
					else
					{
                        g_bPageRecived=0;
					    s_chTtxReceive=0;
					}
					chPreValidPack=chPacketAddress;
				}
				//else if((g_bPageRecived==1)&&(chPacketAddress<24)&&(chPreValidPack<chPacketAddress))
				else if((g_bPageRecived==1)&&(chPacketAddress<28))
				{
                    if((chPacketAddress == 26) || (chPacketAddress == 27))
                    {
                        unsigned char DesignationCode = hamming84(*(pPacket+3));
                        unsigned short Mark = 0;
                        unsigned char *p = NULL;
                        char i = 0;

                        if((DesignationCode < 16) && (DesignationCode >= 0))
                        {
                            switch(chPacketAddress)
                            {
                                case 26:
                                    memcpy(&CCdataBuffer.m_chPageX26Context[DesignationCode][0],(pPacket+3),40);
                                    break;
                                case 27:
                                    memcpy(&CCdataBuffer.m_chPageX27Context[DesignationCode][0],	(pPacket+3),40);
                                    break;
                                case 28:
                                    //pTemp = CCdataBuffer.m_chPageX28Context;
                                    //break;
                                case 29:
                                    //pTemp = CCdataBuffer.m_chPageX29Context;
                                    //break;
                                default:
                                    //pTemp = CCdataBuffer.m_chPageContext;
                                    break;
                            }

                            // 16bit for record the DesignationCode
                            p = CCdataBuffer.m_chPageContext[chPacketAddress];
                            Mark =p[0]|(p[1]<<8);
                            Mark = Mark | (1 << DesignationCode);
                            p[0] = Mark&0xff;
                            p[1] = (Mark>>8)&0xff;
                            // one byte for record the m_chPageX26Context count
                            p[2] = 0;
                            for(i = 0; i <16; i++)
                                p[2] += (((Mark&(1<<i))==1)?1:0);

                            CCdataBuffer.m_chPageContext[chPacketAddress][40]=1;
                        }
                    }
                    else
                    {
                        memcpy(&CCdataBuffer.m_chPageContext[chPacketAddress][0],
                                (pPacket+3),40);
                        CCdataBuffer.m_chPageContext[chPacketAddress][40]=1;
                    }
					s_chTtxReceive = 1;
					//chPreValidPack=chPacketAddress;
				}
			}
		}
	}
}


/*****************************************************************************
 * Function	   : ttx_page_refresh
 * Description : refresh the page which is on screen
 * Arguments   :
 * Returns     :
 * Other       :
 ****************************************************************************/
void ttx_page_refresh(void)
{
	static int16_t g_wShowPageSubCode = 0x3f7f;
	uint8_t i,t;
	struct page_buffer *pBufferTem;
#if SUBTITLE_IN_TTX_CHANGE_COUNTER
	if((0==CcOpenFlag)&&(s_chTtxPageRefreshEnable>gs_ucTTXRefreshCounter))
#else
	if((0==CcOpenFlag)&&(s_chTtxPageRefreshEnable>40))
#endif
	{
        g_wShowPageSubCode=g_wSearchSubCode;
		pBufferTem = check_page_have(g_chSearchMagNum, g_wSearchPageNum,g_wShowPageSubCode);
		if((pBufferTem != NULL)&&(g_chSearchMagNum==pBufferTem->m_chNowMagazine)
			&&(g_wSearchPageNum==pBufferTem->m_chNowPage))
		{
			if((pBufferTem->m_NextPage==NULL)&&(pBufferTem->m_PriorPage==NULL))//if have no sub page
				g_wShowPageSubCode = 0x3f7f;
			else
			{
				g_wShowPageSubCode = pBufferTem->m_wNowSubCode;
			}
			teletext_analysis(pBufferTem, g_chSearchMagNum);
            g_wSearchSubCode = g_wShowPageSubCode;
			page_buffer_link_open();
			g_TTXPageShow.m_chDefaultG0G2=g_X29PackView[pBufferTem->m_chNowMagazine].m_chDefaultG0G2;
			//gxlogd("\n--------------the search page has been draw-0x%x--sub page is %d--------------\n",(int)pBufferTem,g_wShowPageSubCode);
			draw_screen(0);
		}
		s_chTtxPageRefreshEnable=0;
	}
	else if((1==CcOpenFlag)&&(s_chTtxReceive))
	{
		//gxlogd("\n--------------the cc page has been draw---------------\n");
		teletext_analysis(&CCdataBuffer, g_chSearchMagNum);
		for(i=1;i<24;i++)
		{
			if(CCdataBuffer.m_chPageContext[i][40])
			{
				g_TTXPageShow.m_chDefaultG0G2=g_X29PackView[CCdataBuffer.m_chNowMagazine].m_chDefaultG0G2;
				draw_screen_by_line( 0,i);//draw text
				for(t=0;t<40;t++)
				{
					if((g_TTXPageShow.m_chModChar[i][t]==0x0d)
						||(g_TTXPageShow.m_chModChar[i][t]==0x0f))//make next row not be cleaned when this row is double height
					{
						i++;
						break;
					}
				}
			}
			else
			{
                if(g_TTXPageShow.m_chShowState != 0x34)
                    draw_screen_by_line( 1,i);//clean this row
			}

		}
		s_chTtxReceive=0;
		s_chTtxPageRefreshEnable=0;
	}

}


/*****************************************************************************
 * Function	   : quick_search_set_page_num
 * Description : set the magzine and page number people want to search
 * Arguments   : void
 * Returns     :
 * Other       :
 ****************************************************************************/
void quick_search_set_page_num(void)
{
	uint8_t chUserActionTem;
	uint8_t chPostion = 0;
	uint8_t *pPageNumTem;
	uint8_t chPageNumTem[3];
	uint16_t wPageNumTem,i;
	struct page_buffer *pBufferTem;
	pPageNumTem = &g_TTXPageShow.m_chModChar[0][3];
    memcpy(chPageNumTem, &g_TTXPageShow.m_chModChar[0][4], sizeof(chPageNumTem));
	while(g_chSetNum)
	{
		chUserActionTem = g_chUserActionTtx;
		g_chUserActionTtx = 99;
		switch(chUserActionTem)
		{
			case NumKey0:
			case NumKey1:
			case NumKey2:
			case NumKey3:
			case NumKey4:
			case NumKey5:
			case NumKey6:
			case NumKey7:
			case NumKey8:
			case NumKey9:

                if(chUserActionTem == NumKey0 && chPostion == 0)
                {
                    g_chSetNum = 0;
                    break;
                }

				chPostion ++;

				if(chPostion==3)
				{
					g_chUserActionTtx = OkKey;
				}

				if(chPostion==1)
				{
                    *(pPageNumTem + chPostion + 1) = 0x2D;
                    *(pPageNumTem + chPostion + 2) = 0x2D;
				}
				*(pPageNumTem + chPostion) = chUserActionTem+0x30;

				draw_screen(1);
				break;
			case LeftKey:
			case RightKey:
                g_chUserActionTtx = OkKey;
				break;
			case OkKey:
			case UpKey:
			case DownKey:
                pPageNumTem = &g_TTXPageShow.m_chModChar[0][4];
				if(((chPostion > 0) && (chPostion < 3))
                        || ((*pPageNumTem)>'8')
                        || ((*pPageNumTem)<'1'))
                {
                    memcpy(&g_TTXPageShow.m_chModChar[0][4], chPageNumTem, sizeof(chPageNumTem));
                    draw_screen(1);
                }
                else
                {
                    wPageNumTem = (*pPageNumTem-0x30)*100+(*(pPageNumTem+1)-0x30)*10+(*(pPageNumTem+2)-0x30);
                    if(wPageNumTem != (g_chSearchMagNum*100 + g_wSearchPageNum))
                    {
                        if((wPageNumTem>99)&&(wPageNumTem<900))
                        {
                            g_chSearchMagNum = *pPageNumTem - 0x30;
                            if(g_chSearchMagNum==8)
                            {
                                g_chSearchMagNum = 0;
                            }
                            g_wSearchPageNum = (*(pPageNumTem+1)-0x30)*10+(*(pPageNumTem+2)-0x30);
                            pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
                            if(pBufferTem!=NULL)
                            {
                                g_chSearchNotSetPosition = 1;
                                teletext_analysis(pBufferTem, pBufferTem->m_chNowMagazine);
                                draw_screen(0);
                            }
                            else
                            {
                                g_chSearchNotSetPosition = 0;
                                g_bPageRecived = 0;
                                draw_wait_page();
                            }
                        }
                        else
                        {
                            g_TTXPageShow.m_chModChar[0][4]=g_chSearchMagNum+0x30;
                            g_TTXPageShow.m_chModChar[0][5]=g_wSearchPageNum/10+0x30;
                            g_TTXPageShow.m_chModChar[0][6]=g_wSearchPageNum%10+0x30;
                            draw_screen(1);
                        }
                        // for magazine cache control
                        for(i=0; i<8; i++)
                        {
                            if(check_magazie_need_cache(g_chSearchMagNum, i) == 0)
                            {// release unused data
                                //gxlogd("\nTTX, release magazine num = %d\n", i);
                                page_buffer_unused_release(i);
                            }
                        }
                    }
                }

                chPostion = 0;
				g_chSetNum = 0;
				g_chPageRefreshMode=TTX_REFRESH_AUTO;
				break;
			case ExitKey:
				g_chTtxOsdEn = 0;
				com_timer_delete();
				return;
			default:
				break;
		}
		ttx_api_thread_delay(20);
	}
}
/*****************************************************************************
 * Function	   : teletext_dec_main
 * Description : the main function of decoding teletext
 * Arguments   : void
 * Returns     :
 * Other       :
 ****************************************************************************/

enum
{
	TTX_PES_DATA_RECEIVE_INIT,
	TTX_PES_PACKET_HEAD_RECEVIED ,
	TTX_PACKET_SAVED,
	TTX_DATA_ERROR_RECOVER,
	TTX_DATA_RECEIVE_ERROR,
	TTX_PARSE_ERROR
};

static uint8_t s_chTtxDecodeState;
#if 1
static uint32_t nPackLen[PES_FROM_SI_NUM];


void ttx_get_data(uint8_t* pData,uint16_t* pLength)
{
	uint32_t nTmpAddr;


	if((nPackNum == PES_FROM_SI_NUM)&&(g_chTtxOsdEn==1))
	{
		nPackNum=0;
		gxlogd("\n\n***********XXXXXXXXXXXXQQQQQQQQQQ$$$$$$$$$$$$$$$$$\n\n");
		//fflush(stdout);
//		return;
	}
	if((g_chTtxOsdEn||g_chTtxVbiEn))
	{
		if((*(uint8_t*)(pData)==0)&&(*(uint8_t*)(pData+1)==0)
				&&(*(uint8_t*)(pData+2)==1)
				&&(*(uint8_t*)(pData+3)==0xbd))
		{
			s_chTtxDecodeState = TTX_PES_PACKET_HEAD_RECEVIED;
		}
		else
		{
			s_chTtxDecodeState = TTX_DATA_ERROR_RECOVER;
		}

		if(s_chTtxDecodeState == TTX_DATA_ERROR_RECOVER)
	        {
			gxlogd("\nttx data Err\n");
			ttx_filter_resert();
			nPackNum=0;
			s_chTtxDecodeState = TTX_PES_DATA_RECEIVE_INIT;
			return;
	        }
		if(g_chTtxOsdEn)
		{
			nTmpAddr=(uint32_t)(pData);
			nPackLen[nPackNum] = pes_packet_head__PES_packet_length_h(nTmpAddr);    //读取包长的高8位
			nPackLen[nPackNum] = nPackLen[nPackNum] << 8;
			nPackLen[nPackNum] = nPackLen[nPackNum] +pes_packet_head__PES_packet_length_l(nTmpAddr);        //读取包长的低8位
	//		gxlogd();
			nTmpAddr = pes_packet_head__PES_header_data_length(nTmpAddr);
		}
	}
    //copy a pes packet
	if((s_chTtxDecodeState == TTX_PES_PACKET_HEAD_RECEVIED)&&(g_chTtxOsdEn||g_chTtxVbiEn))
	{
		if(nPackLen[nPackNum] >= PES_PACKET_MAX_LEN)  //protect if the pes packet is too large
		{
			s_chTtxDecodeState = TTX_PES_DATA_RECEIVE_INIT;
			gxlogd("\n-------------the ttx packet is too large-0x%x------------------\n",(int)nPackLen);
			return;
		}
		if(g_chTtxOsdEn)
		{
			memcpy((pchTtxPesBuffer+(nPackNum*PES_PACKET_MAX_LEN)),pData,nPackLen[nPackNum]+6);
			s_chTtxDecodeState = TTX_PACKET_SAVED;
			nPackNum ++;
			//gxlogd("+++++++++++++++%d\n",nPackNum);
			//fflush(stdout);
		}
		if((g_chTtxVbiEn)&&(pes_packet_head__stream_id(pData)==0xbd)
			&&(pes_packet_head__PES_header_data_length(pData)==0x24))
		{
			if(g_chTtxVbiEn)
			{
				VBI_READ_FLUG vbi_read_flug;
				vbi_read_flug.vbi_read_ptr=pData+46;
				vbi_read_flug.vbi_len=*pLength;
				teletext_vbi_copy(vbi_read_flug.vbi_read_ptr,vbi_read_flug.vbi_len);
			}
		}

	}
}
#endif
void teletext_dec_main()
{
	uint8_t chUserAction=0,i;
	s_chTtxDecodeState = TTX_PES_DATA_RECEIVE_INIT;
	g_chUserActionTtx = 99;
	if(g_chTtxOsdEn)
	{
		initial_teletext();

		com_timer_create();

	}
	while(g_chTtxOsdEn)
	{
		if(g_chTtxOsdEn)
		{
            if(g_chSetNum == 0)
            {
				chUserAction=g_chUserActionTtx;                            //对用户按键作出反应
				g_chUserActionTtx = 99;
				switch(chUserAction)
				{
				    	case 99:
				        	break;
				    	case ExitKey:
						com_timer_delete();
						g_chTtxOsdEn = 0;
				        	return;//退出ttx
				    	default:
						if(s_chTtxReceive)
						{
                            com_timer_stop();
                            user_interface(chUserAction);
                            com_timer_start();
						}

				        	break;
				}
			}
			else if(s_chTtxReceive==1)
			{
				com_timer_stop();
				quick_search_set_page_num();
				com_timer_start();
			}
			else
			{
				chUserAction=g_chUserActionTtx;                            //对用户按键作出反应
				g_chUserActionTtx = 99;
				switch(chUserAction)
				{
				    	case 99:
				        	break;
				    	case ExitKey:
						com_timer_delete();
						g_chSetNum = 0;
						g_chTtxOsdEn = 0;
				        	return;//退出ttx
				    	default:
				        	break;
				}
			}
 		//ttx_hd_commit_blit();
		}
	    //copy 结束
		if((s_chTtxDecodeState == TTX_PACKET_SAVED)&&g_chTtxOsdEn)
		{
			if ( (pes_packet_head__stream_id(pchTtxPesBuffer)==0xbd)
				&&(pes_packet_head__PES_header_data_length(pchTtxPesBuffer)==0x24))
			{
			        //------------OSD_TTX_DEC---STRAT--------------
				if(CcOpenFlag==0)
				{
					s_chTtxReceive = 1;
				}
				if(s_chTtxPageRefreshEnable>0)
				{
					ttx_page_refresh();
				}
				for(i=0;i<nPackNum;i++)
				{
					teletext_dec((uint8_t*)(pchTtxPesBuffer+(i*PES_PACKET_MAX_LEN) + 46), nPackLen[i]) ;
				}

				//------------OSD_TTX_DEC---END--------------
			}
			else
			{
				ttx_filter_resert();
				nPackLen[0]=0;
				nPackLen[1]=0;
				//gxlogd("\n\nttx_filter_resert111\n");
				ttx_api_thread_delay(10);

			}
			s_chTtxDecodeState = TTX_PES_DATA_RECEIVE_INIT;
			nPackNum = 0;
		}
		else
		{
#if 0
			if((s_chTtxPageRefreshEnable>15)&&(CcOpenFlag==1))//超时清屏
			{
				teletext_clear_show_page();
				g_TTXPageShow.m_chShowState=0x4;
				ttx_spp_clear(CLUT[1][0]);
				gxlogd("\n cc show out time !\n");
				s_chTtxPageRefreshEnable=0;
			}
#endif
			ttx_api_thread_delay(10);
		}
	//ttx_hd_commit_blit();
	}
   	com_timer_delete();
    return;
}

/*****************************************************************************
 * Function	   : sort_sub_page
 * Description : the pages which have the same page number make up a link
 * Arguments   : chMagazineNum:[IN]  magazine number of the page being searched
                 chPageNum:[IN]      page number of the page being searched
                 ppPageBuffer:[OUT]  address of the first page of the link
 * Returns     : null
 * Other       :
 ****************************************************************************/
void sort_sub_page(uint8_t chMagazineNum,uint16_t wPageNum,struct page_buffer ** ppPageBuffer)
{
	uint16_t i,j;
	struct page_buffer *PageBufferTem=NULL;
	struct page_buffer *PageBufferTemComp=NULL;
	struct page_buffer *PageLinkHead=NULL;
	uint32_t nBlocks = 0;

	if(CcOpenFlag)
	{
		return;
	}
	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	for(i=0;i<nBlocks-1;i++)//将700个pagebuffer中页号相同的buffer串成链表
	{
		PageBufferTem = (struct page_buffer *)(pSppBuffer+(PAGE_BUFFER_SIZE*i));
		if((PageBufferTem->m_chNowMagazine == chMagazineNum)
			&&(PageBufferTem->m_chNowPage==wPageNum)&&(chBufBlocFlag[i] ==1))
		{
			PageBufferTem->m_NextPage = NULL;
			PageBufferTem->m_PriorPage = NULL;
			PageLinkHead = PageBufferTem;
            //if((chMagazineNum == g_chSearchMagNum) && (wPageNum == g_wSearchPageNum))
            //    gxlogd("\033[33m Find Buffer : %d subpage : %x\n\033[0m", i, PageBufferTem->m_wNowSubCode);
            //gxlogd("\nthe head page add= 0x%x\n",PageLinkHead);
			for(j = i+1;j < nBlocks;j++)
			{
				PageBufferTemComp = (struct page_buffer *)(pSppBuffer+(PAGE_BUFFER_SIZE*j));
				if((chMagazineNum == PageBufferTemComp->m_chNowMagazine)
					&&(wPageNum == PageBufferTemComp->m_chNowPage)&&(chBufBlocFlag[j] ==1))
				{
					PageBufferTemComp->m_NextPage = NULL;
					PageBufferTemComp->m_PriorPage = NULL;
                    //if((chMagazineNum == g_chSearchMagNum) && (wPageNum == g_wSearchPageNum))
                    //    gxlogd("\033[33m Find Buffer : %d subpage : %x\n\033[0m", j, PageBufferTemComp->m_wNowSubCode);
					if(PageLinkHead->m_wNowSubCode>PageBufferTemComp->m_wNowSubCode)
					{
						PageLinkHead->m_PriorPage = PageBufferTemComp;
						PageBufferTemComp->m_NextPage = PageLinkHead;
						PageLinkHead = PageBufferTemComp;
					}
					else
					{
						PageBufferTem = PageLinkHead;
						while(PageBufferTem->m_NextPage!=NULL)
						{
							if(PageBufferTem->m_NextPage->m_wNowSubCode>PageBufferTemComp->m_wNowSubCode)
							{
								break;
							}
							else
							{
								PageBufferTem = PageBufferTem->m_NextPage;
							}
						}
						if(PageBufferTem->m_NextPage==NULL)
						{
							PageBufferTem->m_NextPage= PageBufferTemComp;
							PageBufferTemComp->m_PriorPage= PageBufferTem;
						}
						else
						{
							PageBufferTem->m_NextPage->m_PriorPage = PageBufferTemComp;
							PageBufferTemComp->m_PriorPage = PageBufferTem;
							PageBufferTemComp->m_NextPage = PageBufferTem->m_NextPage;
							PageBufferTem->m_NextPage = PageBufferTemComp;
						}
					}
				}
			}
			break;//break when find the page want to find
		}
	}

	*ppPageBuffer = PageLinkHead;

}
/*****************************************************************************
 * Function	   : check_page_have
 * Description : check whether the page exist
 * Arguments   : chMagazineNum:  magazine number of the page being searched
                 chPageNum:      page number of the page being searched
 * Returns     : the address of page
 * Other       :
 ****************************************************************************/
struct page_buffer* check_page_have(uint8_t chMagazineNum,uint16_t chPageNum,int16_t wSubPageNum)
{
	static struct page_buffer *pBufferResult,*pBufferTem;
	pBufferResult = NULL;

	sort_sub_page(chMagazineNum, chPageNum, &pBufferResult);
	if(pBufferResult==NULL)
	{
		return pBufferResult;
	}
	pBufferTem=pBufferResult;
	if((pBufferResult->m_NextPage!=NULL)&&(wSubPageNum>-1))
	{
		while(pBufferResult->m_NextPage!=NULL)
		{
			if(wSubPageNum < pBufferResult->m_wNowSubCode)//若真含子页此时要显示的子页肯定不是链表头
			{
				break;
			}
			else
			{
                if(pBufferResult->m_NextPage == NULL)
                    break;
				pBufferResult = pBufferResult->m_NextPage;
			}
		}
		if(wSubPageNum > pBufferResult->m_wNowSubCode)
			pBufferResult = pBufferTem;
		if((wSubPageNum == pBufferResult->m_wNowSubCode)
			&&(g_chPageRefreshMode==TTX_REFRESH_AUTO))
			pBufferResult = pBufferTem;
		if(g_chPageRefreshMode==TTX_REFRESH_SUBPAGE)
		{
			while(pBufferResult->m_PriorPage!=NULL)
			{
				if(pBufferResult->m_wNowSubCode!=wSubPageNum)
                {
                    if(pBufferResult->m_PriorPage == NULL)
                        break;
					pBufferResult=pBufferResult->m_PriorPage;
                }
				else
                {
					break;
                }
            }
        }
    }
	//else
	//	page_buffer_link_open();

    if(g_chPageRefreshMode==TTX_REFRESH_SUBPAGE)
    {
        if(pBufferResult->m_wNowSubCode!=wSubPageNum)
            pBufferResult=NULL;
    }

    return pBufferResult;
}


void ttx_set_default_guide_page(struct page_buffer *pBuffer)
{
	uint8_t t,chMagNum,chPageNum;
	struct page_buffer *pBufferTem;
	chMagNum=pBuffer->m_chNowMagazine;
	chPageNum=pBuffer->m_chNowPage;
	s_wDefaultNavigationPage[0]=chMagNum*100+chPageNum;
	for(t=1;t<4;t++)
	{
		while(1)
		{
			if(chPageNum==99)
			{
				chPageNum=0;
				if(chMagNum==7)
				{
					chMagNum=0;
				}
				else
				{
					chMagNum++;
				}
			}
			else
			{
				chPageNum=chPageNum+1;
			}
			if((chPageNum==pBuffer->m_chNowPage)
				&&(chMagNum==pBuffer->m_chNowMagazine))
			{
				break;
			}
			pBufferTem=NULL;
			sort_sub_page(chMagNum,chPageNum,&pBufferTem);
			if(pBufferTem!=NULL)
			{
                //gxlogd("sort %d : %d\n", chMagNum, chPageNum);
				s_wDefaultNavigationPage[t]=chMagNum*100+chPageNum;
                //gxlogd("s_wDefaultNavigationPage[%d] : %d\n", t, s_wDefaultNavigationPage[t]);
				break;
			}

		}
		if((chPageNum==pBuffer->m_chNowPage)
			&&(chMagNum==pBuffer->m_chNowMagazine))
		{
			for(;t<4;t++)
			{
				s_wDefaultNavigationPage[t]=chMagNum*100+chPageNum;
			}
			break;
		}
	}

}

/*****************************************************************************
 * Function	   : user_interface
 * Description : deal with the key people set
 * Arguments   : chTtxUserAction[IN] :the key number
 * Returns     :
 * Other       :
 ****************************************************************************/
void user_interface(uint8_t chTtxUserAction)
{
	struct page_buffer *pBufferTem;
	struct page_link *pPageLink;
	uint16_t i,chVary;
	uint8_t chPageExist;
    uint8_t chPageInvalid = 0;
	chPageExist = 0;
	pPageLink = g_TTXPageShow.m_PageLink;
	pBufferTem = NULL;
	if (g_chSearchMagNum>7)
	{
		g_chSearchMagNum = 1;
	}
	switch(chTtxUserAction)
	{//上下翻页，左右翻子页
		case UpKey:
			g_wSearchSubCode=0x3f7f;
			g_chPageRefreshMode=TTX_REFRESH_AUTO;
			if(g_wSearchPageNum == 0)
			{
				if(g_chSearchMagNum>0)
				{
					g_chSearchMagNum = g_chSearchMagNum-1;
					g_wSearchPageNum = 99;
					pBufferTem = check_page_have(g_chSearchMagNum,99,-1);
				}
				else
				{
					pBufferTem = check_page_have(7,99,-1);
					g_chSearchMagNum = 7;
					g_wSearchPageNum = 99;
				}
			}
			else
			{
				g_wSearchPageNum = g_wSearchPageNum-1;
				pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
			}
			if(pBufferTem!=NULL)
			{
				g_chSearchNotSetPosition = 1;
				chPageExist = 1;
			}
            else
            {
				for(i = g_wSearchPageNum;i<100;i--)
				{
					pBufferTem = check_page_have(g_chSearchMagNum,i,-1);
					if(pBufferTem!=NULL)
					{
						g_wSearchPageNum = i;
						g_chSearchNotSetPosition = 1;
						chPageExist = 1;
						break;
					}
				}
				if(pBufferTem==NULL)
				{
					if(g_chSearchMagNum==0)
						chVary=7;
					else
						chVary=g_chSearchMagNum-1;
					for(i=100;i<100;i--)
					{
						pBufferTem = check_page_have(chVary,i,-1);
						if(pBufferTem!=NULL)
						{
							g_chSearchMagNum=chVary;
							g_wSearchPageNum = i;
							g_chSearchNotSetPosition = 1;
							chPageExist = 1;
							break;
						}
					}
					if(pBufferTem==NULL)
					{
						g_chSearchNotSetPosition = 0;
						g_bPageRecived = 0;
					}
				}
			}
	       	 break;
		case DownKey:
			g_wSearchSubCode=0x3f7f;
			g_chPageRefreshMode=TTX_REFRESH_AUTO;
			if(g_wSearchPageNum >= 99)
			{
				if(g_chSearchMagNum<7)
				{
					g_chSearchMagNum = g_chSearchMagNum+1;
					g_wSearchPageNum = 0;
					pBufferTem = check_page_have(g_chSearchMagNum,0,-1);
				}
				else
				{
					pBufferTem = check_page_have(0,0,-1);
					g_chSearchMagNum = 0;
					g_wSearchPageNum = 0;
				}
			}
			else
			{
				g_wSearchPageNum = g_wSearchPageNum+1;
				pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);

			}
			if(pBufferTem!=NULL)
			{
				g_chSearchNotSetPosition = 1;
				chPageExist = 1;
			}
			else
			{
				for(i = g_wSearchPageNum;i<100;i++)
				{
					pBufferTem = check_page_have(g_chSearchMagNum,i,-1);
					if(pBufferTem!=NULL)
					{
						g_wSearchPageNum = i;
						g_chSearchNotSetPosition = 1;
						chPageExist = 1;

						break;
					}
				}
				if(pBufferTem==NULL)
				{
					if(g_chSearchMagNum==7)
						chVary=0;
					else
						chVary=g_chSearchMagNum+1;
					for(i=0;i<100;i++)
					{
						pBufferTem = check_page_have(chVary,i,-1);
						if(pBufferTem!=NULL)
						{
							g_chSearchMagNum=chVary;
							g_wSearchPageNum = i;
							g_chSearchNotSetPosition = 1;
							chPageExist = 1;

							break;
						}
					}
					if(pBufferTem==NULL)
					{
						g_chSearchNotSetPosition = 0;
						g_bPageRecived = 0;
					}
				}
			}
			break;
		case LeftKey:
			if(g_chPageRefreshMode==TTX_REFRESH_AUTO)
			{
				sort_sub_page(g_chSearchMagNum, g_wSearchPageNum, &pBufferTem);
				if(pBufferTem!=NULL)
				{
                    while(pBufferTem->m_NextPage!=NULL)
                    {
                        pBufferTem = pBufferTem->m_NextPage;
                    }
					g_wSearchSubCode = pBufferTem->m_wNowSubCode;
					if(g_wSearchSubCode == 0 || g_wSearchSubCode == 0x3f7f)
					{
						return;
					}
					chPageExist = 1;
                    g_chPageRefreshMode=TTX_REFRESH_SUBPAGE;
				}
			}
			else
			{
                sort_sub_page(g_chSearchMagNum, g_wSearchPageNum, &pBufferTem);
				if(pBufferTem != NULL)
				{
                    while(pBufferTem->m_NextPage!=NULL)
                    {
                        if((pBufferTem->m_NextPage->m_wNowSubCode == g_wSearchSubCode)
                                || (pBufferTem->m_wNowSubCode == g_wSearchSubCode))
                        {
                            break;
                        }
                        pBufferTem = pBufferTem->m_NextPage;
                    }

                    if((pBufferTem->m_PriorPage == NULL) && (pBufferTem->m_wNowSubCode == g_wSearchSubCode))
                    {
                        g_chPageRefreshMode=TTX_REFRESH_AUTO;
                        g_TTXPageShow.m_chModChar[25][TTX_AUTO_REFRESH_COLOR]=RED;
                        for(i=0;i<10;i++)
                        {
                            g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+3*i]=WHITE;
                        }
                        g_wSearchSubCode = 0x3f7f;
                    }
                    else
                    {
                        g_wSearchSubCode = pBufferTem->m_wNowSubCode;
                    }
                    chPageExist = 1;
                }
			}
			break;

		case RightKey:
			if(g_chPageRefreshMode==TTX_REFRESH_AUTO)
			{
				sort_sub_page(g_chSearchMagNum, g_wSearchPageNum, &pBufferTem);
				if(pBufferTem!=NULL)
				{
					g_wSearchSubCode = pBufferTem->m_wNowSubCode;
					if(g_wSearchSubCode == 0 || g_wSearchSubCode == 0x3f7f)
					{
						return;
					}
					chPageExist = 1;
                    g_chPageRefreshMode=TTX_REFRESH_SUBPAGE;
				}
			}
			else
			{
                sort_sub_page(g_chSearchMagNum, g_wSearchPageNum, &pBufferTem);
				if(pBufferTem != NULL)
				{
                    while(pBufferTem->m_NextPage!=NULL)
                    {
                        if((pBufferTem->m_wNowSubCode == g_wSearchSubCode))
                        {
                            pBufferTem = pBufferTem->m_NextPage;
                            break;
                        }
                        pBufferTem = pBufferTem->m_NextPage;
                    }

                    if((pBufferTem->m_NextPage == NULL) && (pBufferTem->m_wNowSubCode == g_wSearchSubCode))
                    {
                        g_chPageRefreshMode=TTX_REFRESH_AUTO;
                        g_TTXPageShow.m_chModChar[25][TTX_AUTO_REFRESH_COLOR]=RED;
                        for(i=0;i<10;i++)
                        {
                            g_TTXPageShow.m_chModChar[25][TTX_SUBPAGE_NUM_COLOR+3*i]=WHITE;
                        }
                        g_wSearchSubCode = 0x3f7f;
                    }
                    else
                    {
                        g_wSearchSubCode = pBufferTem->m_wNowSubCode;
                    }
                    chPageExist = 1;
                }
			}
			break;

		case RedKey:
            g_wSearchSubCode=0x3f7f;
			if(pPageLink[0].m_chLinkEn)
			{
				g_chSearchMagNum = pPageLink[0].m_chMagazine;
				g_wSearchPageNum = pPageLink[0].m_chPage;
				pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
				if(pBufferTem!=NULL)
				{
					g_chSearchNotSetPosition = 1;
					chPageExist = 1;
				}
				else
				{
					g_chSearchNotSetPosition = 0;
					g_bPageRecived = 0;
				}
			}
            else
            {
                chPageInvalid = 1;
            }
            g_chPageRefreshMode=TTX_REFRESH_AUTO;
			break;
		case GreenKey:
            g_wSearchSubCode=0x3f7f;
			if(pPageLink[1].m_chLinkEn)
            {
                g_chSearchMagNum = pPageLink[1].m_chMagazine;
                g_wSearchPageNum = pPageLink[1].m_chPage;
                pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
                if(pBufferTem!=NULL)
                {
                    g_chSearchNotSetPosition = 1;
                    chPageExist = 1;
                }
                else
                {
                    g_chSearchNotSetPosition = 0;
                    g_bPageRecived = 0;
                }
            }
            else
            {
                chPageInvalid = 1;
            }
            g_chPageRefreshMode=TTX_REFRESH_AUTO;
			break;
		case YellowKey:
            g_wSearchSubCode=0x3f7f;
			if(pPageLink[2].m_chLinkEn)
			{
				g_chSearchMagNum = pPageLink[2].m_chMagazine;
				g_wSearchPageNum = pPageLink[2].m_chPage;
				pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
				if(pBufferTem!=NULL)
				{
					g_chSearchNotSetPosition = 1;
					chPageExist = 1;
				}
				else
				{
					g_chSearchNotSetPosition = 0;
					g_bPageRecived = 0;
				}
			}
            else
            {
                chPageInvalid = 1;
            }
            g_chPageRefreshMode=TTX_REFRESH_AUTO;
			break;
		case BlueKey:
            g_wSearchSubCode=0x3f7f;
			if(pPageLink[3].m_chLinkEn)
			{
				g_chSearchMagNum = pPageLink[3].m_chMagazine;
				g_wSearchPageNum = pPageLink[3].m_chPage;
				pBufferTem = check_page_have(g_chSearchMagNum,g_wSearchPageNum,-1);
				if(pBufferTem!=NULL)
				{
					g_chSearchNotSetPosition = 1;
					chPageExist = 1;
				}
				else
				{
					g_chSearchNotSetPosition = 0;
					g_bPageRecived = 0;
				}
			}
            else
            {
                chPageInvalid = 1;
            }
            g_chPageRefreshMode=TTX_REFRESH_AUTO;
	        break;
		case OkKey:
			s_chWhetherShowBackGround ++;
			if(s_chWhetherShowBackGround == 8)
			{
				s_chWhetherShowBackGround = 0;
			}

            if(g_chPageRefreshMode == TTX_REFRESH_AUTO)
                pBufferTem = check_page_have(g_chSearchMagNum, g_wSearchPageNum, -1);
            else
                pBufferTem = check_page_have(g_chSearchMagNum, g_wSearchPageNum, g_wSearchSubCode);

			if(pBufferTem!=NULL)
			{
				chPageExist = 1;
			}
			break;
		default:
			break;
	}

    if(chPageInvalid == 1)
        return;

    if(chPageExist == 1)
    {
        chPageExist = 0;
        ttx_set_default_guide_page(pBufferTem);
        teletext_analysis(pBufferTem, pBufferTem->m_chNowMagazine);
        draw_screen(0);
        draw_screen(1);
    }
    else
    {
        draw_wait_page();
        if(g_chSearchMagNum!=0)
        {
            g_TTXPageShow.m_chModChar[0][4]=g_chSearchMagNum+0x30;
        }
        else
        {
            g_TTXPageShow.m_chModChar[0][4]=g_chSearchMagNum+0x38;
        }
        g_TTXPageShow.m_chModChar[0][5]=g_wSearchPageNum/10+0x30;
        g_TTXPageShow.m_chModChar[0][6]=g_wSearchPageNum%10+0x30;
        draw_screen(1);
    }
    //SEARCH_PRINTF("------------after user_action-----------\n");
    //SEARCH_PRINTF("--------magazine = %d------\n",g_chSearchMagNum);
    //SEARCH_PRINTF("--------page = %d------\n",g_wSearchPageNum);

    // for magazine cache control
    for(i=0; i<8; i++)
    {
        if(check_magazie_need_cache(g_chSearchMagNum, i) == 0)
        {// release unused data
            //gxlogd("\nTTX, release magazine num = %d\n", i);
            page_buffer_unused_release(i);
        }
    }
}

static void clear_page_by_line(uint8_t y)
{
    uint8_t x = 0;
	struct osd_view OsdView = {0};

    OsdView.m_chChar = 0x20;
    OsdView.m_chBackColor += 32*s_chWhetherShowBackGround;
    OsdView.m_chFrontColor = 7;
    OsdView.m_chG0G2LanguageSelect = 2;
    OsdView.m_chG02LanguageSelect = 2;
    OsdView.m_chContiguousM = 0xff;
    OsdView.m_chHoldMosaics = 0xff;
    OsdView.m_chHoldChar = 0xff;
    for(x = 0; x < 40; x++)
    {
        osd_draw_teletext(x,y,&OsdView);
    }
    int32_t showmean = 0;
    GxBus_ConfigGetInt(GXBUS_TTX_SHOWMEAN,&showmean,GXBUS_TTX_SHOWMEAN_WRITE);
    if(showmean == GXBUS_TTX_SHOWMEAN_WRITE)
    {
        GxFBHAL_Sync(0,0);
    }
}

void draw_wait_page(void)
{
	uint8_t i;
	uint8_t chOpenFace[24]={"Please Wait!"};

	teletext_clear_show_page();
#if 1
	g_TTXPageShow.m_chModChar[11][14] = 0x1b;

	for(i=1;i<(24+1);i++)//添加进入TTX的初始化界面
	{
		g_TTXPageShow.m_chModChar[11][i+14]=chOpenFace[i-1];
	}
	g_TTXPageShow.m_chModChar[11][24+14] = 0x1b;
#else

	for(i=0;i<24;i++)//添加进入TTX的初始化界面
	{
		g_TTXPageShow.m_chModChar[11][i+14]=chOpenFace[i];
	}
#endif
	g_TTXPageShow.m_chDefaultFrontClut=0;
	g_TTXPageShow.m_chDefaultFrontCol=7;
	g_TTXPageShow.m_chDefaultBackClut=0;
	g_TTXPageShow.m_chDefaultBackCol=0;
	draw_screen(0);
}

/*****************************************************************************
 * Function	   : draw_screen
 * Description : display the teletext page
 * Arguments   : chMode[IN] :display one line or full screen
 * Returns     : 1 : draw err - time show err
 * Other       :
 ****************************************************************************/

int draw_screen(uint8_t chMode)
{
	static struct osd_view OsdView;
	uint8_t x, y,t;                                       //
	struct view_page *pViewPage;
	volatile uint8_t x26;
	volatile uint8_t y26;
	uint8_t chVari;
	struct x_26_pack *pDataBuffer = NULL;
	static uint8_t s_chCharUnDecode;

	uint8_t   chActivePosByX26X[208];//16*13
	uint8_t   chActivePosByX26Y[208];
	uint8_t   chX26Offset[208];
	static uint8_t   chPosReSetCount;
	uint8_t chDoubleHeight;
	uint8_t chDisableDW; //disable double width for whole line
	uint16_t wDataTem;

	pViewPage = &g_TTXPageShow;

	x = 0;
	if((chMode==1)&&(g_chSearchNotSetPosition==1))
	{
		for(x=35;x<40;x++)
		{
			if((pViewPage->m_chModChar[0][x]>=0x20)&&(pViewPage->m_chModChar[0][x]<0x7f))
				break;
            else
            {
                pViewPage->m_chModChar[0][x] = 0x20;
            }
		}
	}

    //防止时间闪的现象
	if((x==40)&&(chMode==1)&&(g_chSetNum==0))
		return 1;

	memset(chActivePosByX26X,0xff,208);
	memset(chActivePosByX26Y,0xff,208);
	memset(chX26Offset,0xff,208);

	chPosReSetCount=0;
	for(y26=0;y26<16;y26++)
	{
		pDataBuffer= &pViewPage->m_X26Pack[y26][0];
		for(x26=0;x26<13;x26++)
		{
			if((pDataBuffer[x26].m_chMode>0x1f)||(pDataBuffer[x26].m_chAddress>0x3f))
				continue;
			if (pDataBuffer[x26].m_chAddress>39)
			{// row
				//gxlogd("\nTTX, row, mode = 0x%x..\n", pDataBuffer[x26].m_chMode);
				switch(pDataBuffer[x26].m_chMode)
				{
					case 0x01:              //set full row color level 2.5 3.5
						break;
					case 0x04:             //set active position
						if (pDataBuffer[x26].m_chAddress == 40)
							chActivePosByX26Y[chPosReSetCount] = 24;
						else
							chActivePosByX26Y[chPosReSetCount]= pDataBuffer[x26].m_chAddress - 40;
						//chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chData;
						chPosReSetCount++;
						break;
					case 0x07:              //display row 0
						break;
					default:
					{
						break;
					}
				}
			}
			else
			{// column
				chX26Offset[chPosReSetCount]=y26*13+x26;
				if(pDataBuffer[x26].m_chMode&0x10)
				{
					chActivePosByX26X[chPosReSetCount]=pDataBuffer[x26].m_chAddress;
					if((chActivePosByX26Y[chPosReSetCount]==0xff)&&(chPosReSetCount>0))
					{
						chActivePosByX26Y[chPosReSetCount]=chActivePosByX26Y[chPosReSetCount-1];
					}
					chPosReSetCount++;
				}
				else
				{
					switch(pDataBuffer[x26].m_chMode)
					{
						case 0x01:             // Block Mosaic Character from the G1 Set
							if(chPosReSetCount < 1)
							{
								gxlogd("\nTTX, not standard..\n");
							}
							else
							{
								chActivePosByX26Y[chPosReSetCount] = chActivePosByX26Y[chPosReSetCount - 1];
								chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chAddress;
								//= pDataBuffer[x26].m_chData;
								chPosReSetCount++;
							}
							break;

						case 0x02:             // Line Drawing or Smoothed Mosaic Character from the G3 Set at Level 1.5
							if(chPosReSetCount < 1)
							{
								gxlogd("\nTTX, not standard..\n");
							}
							else
							{
								chActivePosByX26Y[chPosReSetCount] = chActivePosByX26Y[chPosReSetCount - 1];
								chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chAddress;
								//= pDataBuffer[x26].m_chData;
								chPosReSetCount++;
							}
							break;

						case 0x0F:             //change the char
							if(chPosReSetCount < 1)
							{
								gxlogd("\nTTX, not standard..\n");
							}
							else
							{
								chActivePosByX26Y[chPosReSetCount] = chActivePosByX26Y[chPosReSetCount - 1];
								chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chAddress;
								//= pDataBuffer[x26].m_chData;
								chPosReSetCount++;
							}
							break;

						case 0x10:             // @ symbol replaces * symbol at position 2/A. See P88
							if(chPosReSetCount < 1)
							{
								gxlogd("\nTTX, not standard..\n");
							}
							else
							{
								chActivePosByX26Y[chPosReSetCount] = chActivePosByX26Y[chPosReSetCount - 1];
								chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chAddress;
								//= pDataBuffer[x26].m_chData;
								chPosReSetCount++;
							}
							break;

						default:
							break;
					}
				}
			}
		}
	}

    //开始解码
	for(y=0;y<26;y++)
	{
		if(((pViewPage->m_chShowState&0x6))&&(g_chSearchNotSetPosition==1)&&(g_chSetNum==0))
		{
#if SUBTITLE_IN_TTX_CHANGE_COUNTER
			com_ttx_set_refresh_count(1);
#endif
			if((y>0) && (y<24))
			{
				draw_screen_by_line(0,y);
				for(t=0;t<40;t++)
				{
					if((g_TTXPageShow.m_chModChar[y][t]==0x0d)
						||(g_TTXPageShow.m_chModChar[y][t]==0x0f))//make next row not be cleaned when this row is double height
					{
						y++;
						break;
					}
				}
			}
			else if((TTX_TIME_SHOW)&&(y==0)&&(CcOpenFlag==0))
			{
				draw_screen_by_line(0,y);
			}
			else
			{
				draw_screen_by_line(1,y);
			}

            if (chMode == 1)
            {
                return 0;
            }

			continue;
		}
#if SUBTITLE_IN_TTX_CHANGE_COUNTER
		else
		{
			com_ttx_set_refresh_count(40);
		}
#endif
		if(pViewPage->m_chDefaultBackClut>1)
		{
			pViewPage->m_chDefaultBackClut=0;
		}
		if(pViewPage->m_chDefaultFrontClut>1)
		{
			pViewPage->m_chDefaultFrontClut=0;
		}
	    //每行复位参数表
		OsdView.m_chCharSize = 0;
		chDoubleHeight=0;
        s_chLine0IsDoubleH = 0;
		chDisableDW=0;

		//判断此行是否双倍字体
        if(y<23)
        {
            for(x=0;x<40;x++)
            {
                ttx_parse_x0_x24_char(g_TTXPageShow.m_chModChar[y][x],
                        pViewPage,&OsdView);
                if((OsdView.m_chCharSize&1)!=0)
                {
                    chDoubleHeight=1;
                    if(y == 0)
                        s_chLine0IsDoubleH = 1;
                    break;
                }
            }
        }

		OsdView.m_chG0G2LanguageSelect = language_g0_g2(pViewPage->m_chDefaultG0G2|pViewPage->m_chX28Langctrl);//
		OsdView.m_chG02LanguageSelect = language_g0_2(pViewPage->m_chDefaultG02|pViewPage->m_chX28Langctrl);//
		OsdView.m_chSetSelect = 0;
		OsdView.m_chFlash =0;
		OsdView.m_chConceal = 0;
        OsdView.m_chMosaics = 0;
        OsdView.m_chContiguousM = 0xFF;
		OsdView.m_chHoldMosaics = 0xFF;
		OsdView.m_chHoldChar = 0xFF;
		OsdView.m_chX26ChangeDataEnable=0;
        OsdView.m_chX26ColumnMode16 = 0;
		OsdView.m_chBackColor = CLUT[pViewPage->m_chDefaultBackClut][0];
		OsdView.m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][pViewPage->m_chDefaultFrontCol];
		s_chCharUnDecode=0;
		for(x=0;x<40;x++)
		{
            OsdView.m_chX26ColumnMode16 = 0; //clear X/26 Column Mode16

			if(OsdView.m_chSetSelect  == 1)
				OsdView.m_chSetSelect = 0;

			//if((0 == y)&&(0 == chMode)&&((x<4)||(x>31))&&(g_chSearchNotSetPosition==1))
			if((0 == y)&&(0 == chMode)&&(g_chSearchNotSetPosition==1))
			{
				continue;
			}
			OsdView.m_chChar = g_TTXPageShow.m_chModChar[y][x];

			if(s_chCharUnDecode)
			{
				ttx_parse_x0_x24_char(g_TTXPageShow.m_chModChar[y][x-1],
					pViewPage,&OsdView);
				s_chCharUnDecode=0;
			}
			if((OsdView.m_chHoldMosaics)&&((OsdView.m_chChar<0xc)//make sure the set after attribute
				||((OsdView.m_chChar>0xc)&&(OsdView.m_chChar<0x18))))
			{
				s_chCharUnDecode=1;
			}
			else
			{
				ttx_parse_x0_x24_char(OsdView.m_chChar,
					pViewPage,&OsdView);
				s_chCharUnDecode=0;
			}

            //P118 G1 Block Mosaics Set
            if((OsdView.m_chMosaics == 1)
                    && ((OsdView.m_chChar < 0x40) || (OsdView.m_chChar > 0x5F)))
            {
                if(OsdView.m_chContiguousM == 1)
                {
                    OsdView.m_chSetSelect = 3;
                }
                else
                {
                    OsdView.m_chSetSelect = 2;
                }
            }
            else
            {
                OsdView.m_chSetSelect = 0;
            }

			for(chVari=0;chVari<chPosReSetCount;chVari++)
			{
				if((y==chActivePosByX26Y[chVari])&&(x==chActivePosByX26X[chVari]))
				{
					break;
				}
			}

			if(chVari<chPosReSetCount)
			{
			//对26包进行解码
				pDataBuffer= &pViewPage->m_X26Pack[(chX26Offset[chVari]/13)][0];
				if (pDataBuffer[(chX26Offset[chVari]%13)].m_chAddress<40)
				{
					switch(pDataBuffer[(chX26Offset[chVari]%13)].m_chMode)
					{
						case 0x00://设置前景色
							if ((pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x60)==0)
							{
								t=pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x18>>3;
								if(t>1)
								{
									t=0;
								}
								OsdView.m_chFrontColor =
									CLUT[t][pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x7];
							}
							break;
						case 0x01://选择镶嵌字符
							OsdView.m_chSetSelect = 3;//for G1 Block Mosaics Set
							OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                            break;
						case 0x02://选择平滑镶嵌字符
							OsdView.m_chSetSelect = 4;//for G3 Smooth Mosaics and Line Drawing Set
							OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
							break;
						case 0x03://选择背景色
							if ((pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x60)==0)
							{
								t=pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x18>>3;
								if(t>1)
								{
									t=0;
								}
								OsdView.m_chBackColor=
									CLUT[t][pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x7];
							}
							break;
						case 0x07://闪烁渐变
							break;
						case 0x08://语言切换
							break;
						case 0x09:
							break;
						case 0x0b:
							break;
						case 0x0c:
							break;
						case 0x0f:
                            if(!(((OsdView.m_chChar>0x40)&&(OsdView.m_chChar<0x5B)) || ((OsdView.m_chChar>0x60)&&(OsdView.m_chChar<0x7B))))//#47617 对字母不进行转换
                            {
							    OsdView.m_chSetSelect = 1;//for G2
							    OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                            }
							break;
						case 0x10:
                            if((OsdView.m_chChar == 0x2A) && (pDataBuffer[(chX26Offset[chVari]%13)].m_chData == 0x40))
                            {
                                OsdView.m_chX26ColumnMode16 = 1;//for '@' replace '*'
                                OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                            }
							break;
						default:
                            if (pDataBuffer[(chX26Offset[chVari]%13)].m_chMode&0x10)//替换显示，需要替换的字索引在数组中，数组索引为引用字在G0第4排中的偏移
                            {
                                OsdView.m_chX26ChangeDataEnable=1;
                                OsdView.m_chX26ChangeChar=(pDataBuffer[(chX26Offset[chVari]%13)].m_chMode&0xf)|0x40;
                                OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                            }
                            break;

					}
				}
			}

			if(!((pViewPage->m_chShowState&0x08)&&(y==0)))
			{
				if((OsdView.m_chSetSelect&0x2)&&(OsdView.m_chHoldMosaics != 0xFF))
				{
					chVari = x;
					while(chVari>0)
					{
                        OsdView.m_chHoldChar = pViewPage->m_chModChar[y][chVari-1];
                        if(pViewPage->m_chModChar[y][chVari] == 0x1e)
                            break;

						if((OsdView.m_chHoldChar & 0x20)==0)
						{
							OsdView.m_chHoldChar = pViewPage->m_chModChar[y][chVari];
							if((chVari==1)&&(OsdView.m_chHoldChar < 0x20))
								OsdView.m_chHoldChar =pViewPage->m_chModChar[y][0];
						}
						else
						{
							break;
						}
						chVari--;
					}

					if(OsdView.m_chHoldChar<0x20)
					{
						OsdView.m_chHoldChar=0x20;
					}
				}

				if((OsdView.m_chSetSelect&0x2)&&(OsdView.m_chHoldMosaics != 0xFF)
					&&(OsdView.m_chChar < 0x20))
				{
                    OsdView.m_chChar = OsdView.m_chHoldChar;
				}

                if(OsdView.m_chHoldMosaics == 0)
                {
                    OsdView.m_chHoldMosaics = 0xFF;
                }

                if (OsdView.m_chChar <0x20)
                {
                    OsdView.m_chChar = 0x20;
                }
			}
			else
			{
				if((!((x<7)&&(x>3)&&(1==g_chSetNum)))&&(g_chSearchNotSetPosition==1))//针对首行隐藏的情况要显示输入的页码
				{
					OsdView.m_chChar=0x20;
				}
			}
			//透明度用色表偏移实现
			OsdView.m_chBackColor += 32*s_chWhetherShowBackGround;
			//强制将双倍行整行拉伸
			if((chDoubleHeight)&&((OsdView.m_chCharSize&1)==0))
			{
				if(y<23)
				{
					OsdView.m_chCharSize |= 1;
				}
			}

            //change double size to double width when row 23
            if((OsdView.m_chCharSize == 0x3) && (0 == chDoubleHeight)) //double size
            {
                OsdView.m_chCharSize = 0x2; //double width
            }

            //clear double width flag when curr & next char > 0x20
			if((OsdView.m_chCharSize&0x02)
                    && (g_TTXPageShow.m_chModChar[y][x+1] > 0x20)
                    && (g_TTXPageShow.m_chModChar[y][x] > 0x20))
            {
                chDisableDW=1;
            }
            if(chDisableDW == 1)
            {
                OsdView.m_chCharSize &= 0x01;
            }

			//针对非正常的字符强制显示为空格
			if(OsdView.m_chChar<0x20)
			{
				OsdView.m_chChar=0x20;
			}

			if(OsdView.m_chConceal)
			{
				OsdView.m_chChar=0x20;
			}

			if((OsdView.m_chCharSize&0x2)&&(x==39))
			{
				OsdView.m_chCharSize&=0xfd;
			}

			osd_draw_teletext(x,y,&OsdView);

			OsdView.m_chBackColor -= 32*s_chWhetherShowBackGround;

			if( (OsdView.m_chCharSize&0x02)&&(x<39)&& (OsdView.m_chChar > 0x20))
			{
				x++;
			}
		}
        int32_t showmean = 0;
        GxBus_ConfigGetInt(GXBUS_TTX_SHOWMEAN,&showmean,GXBUS_TTX_SHOWMEAN_WRITE);
        if(showmean == GXBUS_TTX_SHOWMEAN_WRITE)
        {
            GxFBHAL_Sync(0,0);
        }

		if((chDoubleHeight)&&(y<23))
		{
			y++;
		}
		if((y==23)&&(pViewPage->m_chShow24==0)
			&&((pViewPage->m_chShowState&0x6)==0)
			&&(g_chSearchNotSetPosition==1)) //不显示X24
		{

			pViewPage->m_chModChar[24][2]=RED;
			wDataTem=g_chSearchMagNum*100+g_wSearchPageNum;
			for(x=1;x<4;x++)
			{
				if(wDataTem==s_wDefaultNavigationPage[x])
				{
					break;
				}
			}
			if(x==4)
			{
				if(g_chSearchMagNum!=0)
				{
					pViewPage->m_chModChar[24][3]=0x30+g_chSearchMagNum;
				}
				else
				{
					pViewPage->m_chModChar[24][3]=0x38;
				}
				pViewPage->m_chModChar[24][4]=0x30+g_wSearchPageNum/10;
				pViewPage->m_chModChar[24][5]=0x30+g_wSearchPageNum%10;
				pViewPage->m_PageLink[0].m_chLinkEn=1;
				pViewPage->m_PageLink[0].m_chMagazine=g_chSearchMagNum;
				pViewPage->m_PageLink[0].m_chPage=g_wSearchPageNum;
			}
			else
			{
				wDataTem=s_wDefaultNavigationPage[0]/100;
				if(wDataTem!=0)
				{
					pViewPage->m_chModChar[24][3]=0x30+wDataTem;
				}
				else
				{
					pViewPage->m_chModChar[24][3]=0x38;
				}
				wDataTem=s_wDefaultNavigationPage[0]%100;
				pViewPage->m_chModChar[24][4]=0x30+wDataTem/10;
				pViewPage->m_chModChar[24][5]=0x30+wDataTem%10;
				pViewPage->m_PageLink[0].m_chLinkEn=1;
				pViewPage->m_PageLink[0].m_chMagazine=s_wDefaultNavigationPage[0]/100;
				pViewPage->m_PageLink[0].m_chPage=wDataTem;
			}
			pViewPage->m_chModChar[24][12]=GREEN;
			pViewPage->m_chModChar[24][22]=YELLOW;
			pViewPage->m_chModChar[24][32]=BLUE;
			for(x=1;x<4;x++)
			{
				if(s_wDefaultNavigationPage[x]!=0xffff)
				{
					wDataTem=s_wDefaultNavigationPage[x]/100;
                    //gxlogd("wDataTem=s_wDefaultNavigationPage : %d\n", s_wDefaultNavigationPage[x]);
					if(wDataTem!=0)
					{
						pViewPage->m_chModChar[24][3+10*x]=0x30+wDataTem;
					}
					else
					{
						pViewPage->m_chModChar[24][3+10*x]=0x38;
					}
					wDataTem=s_wDefaultNavigationPage[x]%100;
					pViewPage->m_chModChar[24][4+10*x]=0x30+wDataTem/10;
					pViewPage->m_chModChar[24][5+10*x]=0x30+wDataTem%10;
                    //gxlogd("Set Link Enable!\n");
					pViewPage->m_PageLink[x].m_chLinkEn=1;
					pViewPage->m_PageLink[x].m_chMagazine=s_wDefaultNavigationPage[x]/100;
					pViewPage->m_PageLink[x].m_chPage=wDataTem;
				}
				else
				{
					pViewPage->m_chModChar[24][3+10*x]=pViewPage->m_chModChar[24][3];
					pViewPage->m_chModChar[24][4+10*x]=pViewPage->m_chModChar[24][4];
					pViewPage->m_chModChar[24][5+10*x]=pViewPage->m_chModChar[24][5];
					pViewPage->m_PageLink[x].m_chLinkEn=1;
					pViewPage->m_PageLink[x].m_chMagazine=pViewPage->m_PageLink[0].m_chMagazine;
					pViewPage->m_PageLink[x].m_chPage=pViewPage->m_PageLink[0].m_chPage;
				}
			}
		}

		if (chMode == 1)
		{
			return 0;
		}
	}

    return 0;
}

void draw_screen_by_line(uint8_t chDrawContent,uint8_t chLineNum)
{
    static struct osd_view OsdView;
    uint8_t x, y, t;
    struct view_page *pViewPage;
    uint8_t chVari;
    volatile uint8_t chBoxStartX,chBoxEndX;
    static uint8_t s_chCharUnDecode;
	static uint8_t chPosReSetCount;
	volatile uint8_t x26;
	volatile uint8_t y26;
    uint32_t nColorPre;
    uint8_t chDoubleHeight;
	struct x_26_pack *pDataBuffer = NULL;
	uint8_t chActivePosByX26X[208];//16*13
	uint8_t chActivePosByX26Y[208];
	uint8_t chX26Offset[208];

    pViewPage = &g_TTXPageShow;
    if(chDrawContent==1)
    {
        if(CcOpenFlag)
        {
            ttx_spp_fill_rect(g_chTtxStartAddY + 16*chLineNum,CLUT[1][0]);
            return;
        }
        OsdView.m_chG0G2LanguageSelect = language_g0_g2(pViewPage->m_chDefaultG0G2|pViewPage->m_chX28Langctrl);//
        OsdView.m_chG02LanguageSelect = language_g0_2(pViewPage->m_chDefaultG02|pViewPage->m_chX28Langctrl);//
        OsdView.m_chSetSelect = 0;
        OsdView.m_chFlash =0;
        OsdView.m_chConceal = 0;
        OsdView.m_chMosaics = 0;
        OsdView.m_chContiguousM = 0xFF;
		OsdView.m_chHoldMosaics = 0xFF;
		OsdView.m_chHoldChar = 0xFF;
        OsdView.m_chCharSize = 0;
        OsdView.m_chBackColor = CLUT[1][0];
        OsdView.m_chFrontColor = CLUT[1][0];
        OsdView.m_chChar = 0x20;
        for(x=0;x<40;x++)
        {
            osd_draw_teletext(x,chLineNum,&OsdView);
        }
        int32_t showmean = 0;
        GxBus_ConfigGetInt(GXBUS_TTX_SHOWMEAN,&showmean,GXBUS_TTX_SHOWMEAN_WRITE);
        if(showmean == GXBUS_TTX_SHOWMEAN_WRITE)
        {
            GxFBHAL_Sync(0,0);
        }
        return;
    }

    if(CcOpenFlag && (g_old_resolution != g_resolution))
    {
        g_old_resolution = g_resolution;
        ttx_spp_clear(CLUT[1][0]);
        if(g_resolution)
        {
            g_chTtxStartAddY = 30;
        }
        else
        {
            g_chTtxStartAddY = 78;
        }
    }

	memset(chActivePosByX26X,0xff,208);
	memset(chActivePosByX26Y,0xff,208);
	memset(chX26Offset,0xff,208);

	chPosReSetCount=0;
	for(y26=0;y26<16;y26++)
	{
		pDataBuffer= &pViewPage->m_X26Pack[y26][0];
		for(x26=0;x26<13;x26++)
		{
			if((pDataBuffer[x26].m_chMode>0x1f)||(pDataBuffer[x26].m_chAddress>0x3f))
				continue;
			if (pDataBuffer[x26].m_chAddress>39)
			{// row
				//gxlogd("\nTTX, row, mode = 0x%x..\n", pDataBuffer[x26].m_chMode);
				switch(pDataBuffer[x26].m_chMode)
				{
					case 0x01:              //set full row color level 2.5 3.5
						break;
					case 0x04:             //set active position
						if (pDataBuffer[x26].m_chAddress == 40)
							chActivePosByX26Y[chPosReSetCount] = 24;
						else
							chActivePosByX26Y[chPosReSetCount]= pDataBuffer[x26].m_chAddress - 40;
						//chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chData;
						chPosReSetCount++;
						break;
					case 0x07:              //display row 0
						break;
					default:
					{
						break;
					}
				}
			}
			else
			{// column
				chX26Offset[chPosReSetCount]=y26*13+x26;
				if(pDataBuffer[x26].m_chMode&0x10)
				{
					chActivePosByX26X[chPosReSetCount]=pDataBuffer[x26].m_chAddress;
					if((chActivePosByX26Y[chPosReSetCount]==0xff)&&(chPosReSetCount>0))
					{
						chActivePosByX26Y[chPosReSetCount]=chActivePosByX26Y[chPosReSetCount-1];
					}
					chPosReSetCount++;
				}
				else
				{
					switch(pDataBuffer[x26].m_chMode)
					{
						case 0x01:             // Block Mosaic Character from the G1 Set
						case 0x02:             // Line Drawing or Smoothed Mosaic Character from the G3 Set at Level 1.5
						case 0x0F:             // change the char
						case 0x10:             // @ symbol replaces * symbol at position 2/A. See P88
							if(chPosReSetCount < 1)
							{
								gxlogd("\nTTX, not standard..\n");
							}
							else
							{
								chActivePosByX26Y[chPosReSetCount] = chActivePosByX26Y[chPosReSetCount - 1];
								chActivePosByX26X[chPosReSetCount]= pDataBuffer[x26].m_chAddress;
								//= pDataBuffer[x26].m_chData;
								chPosReSetCount++;
							}
							break;

						default:
							break;
					}
				}
			}
		}
	}

    //开始解码
    chBoxStartX=40;
    chBoxEndX=39;
    //uint8_t BoxStartRecive = 0;
    //针对非标的码流，0B0B和0A0A不对应【只有0B--开始标志】，导致TTX字幕显示的时候有对于的色块显示，调整“chBoxStartX”的赋值控制。----后续还需要完善。
    for(x=0;x<40;x++)
    {
        if (pViewPage->m_chModChar[chLineNum][x]==0xb)
        {// start flag
            if(x == 39)
            {
                chBoxStartX = 40;
            }
            else if(pViewPage->m_chModChar[chLineNum][x + 1]  != 0xb)
            {
                chBoxStartX = x ;
            }
            else
            {
                chBoxStartX = x + 1;
                pViewPage->m_chModChar[chLineNum][x] = 0x20;
            }
        }
        else if((pViewPage->m_chModChar[chLineNum][x]==0xa)&&(chBoxEndX==39))
        {// and flag
            chBoxEndX=x;
        }
    }
    //BoxStartRecive = 0;

    if(pViewPage->m_chDefaultBackClut>1)
    {
        pViewPage->m_chDefaultBackClut=0;
    }
    if(pViewPage->m_chDefaultFrontClut>1)
    {
        pViewPage->m_chDefaultFrontClut=0;
    }
    //每行复位参数表
    //判断此行是否双倍字体
    OsdView.m_chCharSize = 0;
    chDoubleHeight=0;
    y=chLineNum;

    if(y<23)
    {
        for(x=0;x<40;x++)
        {
            ttx_parse_x0_x24_char(g_TTXPageShow.m_chModChar[y][x],
                    pViewPage,&OsdView);
            if((OsdView.m_chCharSize&1)!=0)
            {
                chDoubleHeight=1;
                break;
            }
        }
    }

    OsdView.m_chG0G2LanguageSelect = language_g0_g2(pViewPage->m_chDefaultG0G2|pViewPage->m_chX28Langctrl);//
    OsdView.m_chG02LanguageSelect = language_g0_2(pViewPage->m_chDefaultG02|pViewPage->m_chX28Langctrl);//
    OsdView.m_chSetSelect = 0;
    OsdView.m_chFlash =0;
    OsdView.m_chConceal = 0;
    OsdView.m_chMosaics = 0;
    OsdView.m_chContiguousM = 0xFF;
    OsdView.m_chHoldMosaics = 0xFF;
    OsdView.m_chHoldChar = 0xFF;
    OsdView.m_chCharSize = 0;
    OsdView.m_chBackColor = CLUT[0][0];
    OsdView.m_chFrontColor = CLUT[0][7];

    for(x=0;x<40;x++)
    {
        ttx_parse_x0_x24_char(g_TTXPageShow.m_chModChar[y][x],
                pViewPage,&OsdView);
        OsdView.m_chChar = g_TTXPageShow.m_chModChar[y][x];
        if(s_chCharUnDecode)
        {
            ttx_parse_x0_x24_char(g_TTXPageShow.m_chModChar[y][x-1],
                    pViewPage,&OsdView);
            s_chCharUnDecode=0;
        }
        if((OsdView.m_chHoldMosaics)&&((OsdView.m_chChar<0x8)//make sure the set after attribute
                    ||((OsdView.m_chChar>0xf)&&(OsdView.m_chChar<0x18))))
        {
            s_chCharUnDecode=1;
        }
        else
        {
            ttx_parse_x0_x24_char(OsdView.m_chChar,
                    pViewPage,&OsdView);
            s_chCharUnDecode=0;
        }

        if((OsdView.m_chMosaics == 1)
                && ((OsdView.m_chChar < 0x40) || (OsdView.m_chChar > 0x5F)))
        {
            if(OsdView.m_chContiguousM == 1)
            {
                OsdView.m_chSetSelect = 3;
            }
            else
            {
                OsdView.m_chSetSelect = 2;
            }
        }
        else
        {
            OsdView.m_chSetSelect = 0;
        }

        for(chVari=0;chVari<chPosReSetCount;chVari++)
        {
            if((y==chActivePosByX26Y[chVari])&&(x==chActivePosByX26X[chVari]))
            {
                break;
            }
        }

        if(chVari<chPosReSetCount)
        {
            //对26包进行解码
            pDataBuffer= &pViewPage->m_X26Pack[(chX26Offset[chVari]/13)][0];
            if (pDataBuffer[(chX26Offset[chVari]%13)].m_chAddress<40)
            {
                switch(pDataBuffer[(chX26Offset[chVari]%13)].m_chMode)
                {
                    case 0x00://设置前景色
                        if ((pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x60)==0)
                        {
                            t=pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x18>>3;
                            if(t>1)
                            {
                                t=0;
                            }
                            OsdView.m_chFrontColor =
                                CLUT[t][pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x7];
                        }
                        break;
                    case 0x01://选择镶嵌字符
                        OsdView.m_chSetSelect = 3;//for G1 Block Mosaics Set
                        OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                        break;
                    case 0x02://选择平滑镶嵌字符
                        OsdView.m_chSetSelect = 4;//for G3 Smooth Mosaics and Line Drawing Set
                        OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                        break;
                    case 0x03://选择背景色
                        if ((pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x60)==0)
                        {
                            t=pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x18>>3;
                            if(t>1)
                            {
                                t=0;
                            }
                            OsdView.m_chBackColor=
                                CLUT[t][pDataBuffer[(chX26Offset[chVari]%13)].m_chData&0x7];
                        }
                        break;
                    case 0x07://闪烁渐变
                        break;
                    case 0x08://语言切换
                        break;
                    case 0x09:
                        break;
                    case 0x0b:
                        break;
                    case 0x0c:
                        break;
                    case 0x0f:
                        OsdView.m_chSetSelect = 1;//for G2
                        OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                        break;
                    case 0x10:
                        if((OsdView.m_chChar == 0x2A) && (pDataBuffer[(chX26Offset[chVari]%13)].m_chData == 0x40))
                        {
                            OsdView.m_chX26ColumnMode16 = 1;//for '@' replace '*'
                            OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                        }
                        break;
                    default:
                        if (pDataBuffer[(chX26Offset[chVari]%13)].m_chMode&0x10)//替换显示，需要替换的字索引在数组中，数组索引为引用字在G0第4排中的偏移
                        {
                            OsdView.m_chX26ChangeDataEnable=1;
                            OsdView.m_chX26ChangeChar=(pDataBuffer[(chX26Offset[chVari]%13)].m_chMode&0xf)|0x40;
                            OsdView.m_chChar=pDataBuffer[(chX26Offset[chVari]%13)].m_chData;
                        }
                        break;
                }
            }
        }

        if((OsdView.m_chSetSelect&0x2)&&(OsdView.m_chHoldMosaics != 0xFF))
        {
            OsdView.m_chHoldChar = pViewPage->m_chModChar[y][x-1];
            chVari = x;
            while(chVari>0)
            {
                if((OsdView.m_chHoldChar & 0x20)==0)
                {
                    OsdView.m_chHoldChar = pViewPage->m_chModChar[y][chVari];
                    if((chVari==1)&&(OsdView.m_chHoldChar < 0x20))
                        OsdView.m_chHoldChar =pViewPage->m_chModChar[y][0];
                }
                else
                {
                    break;
                }
                chVari --;
            }
            if(OsdView.m_chHoldChar<0x20)
            {
                OsdView.m_chHoldChar=0x20;
            }
        }

        if((OsdView.m_chSetSelect&0x2)&&(OsdView.m_chHoldMosaics != 0xFF))
        {
            if(OsdView.m_chChar < 0x20)
                OsdView.m_chChar = OsdView.m_chHoldChar;
        }
        else if((OsdView.m_chSetSelect&0x2)==0)
        {
            OsdView.m_chHoldMosaics=0;
        }

        if(OsdView.m_chHoldMosaics == 0)
        {
            OsdView.m_chHoldMosaics = 0xFF;
        }

        if (OsdView.m_chChar <0x20)
        {
            OsdView.m_chChar = 0x20;
        }
        if(((x>=chBoxStartX)&&(x<=chBoxEndX)&&(y!=0))
                ||((TTX_TIME_SHOW)&&((y==0))))
                //||((TTX_TIME_SHOW)&&((y==0)&&(((x>3)&&(x<7))||(x>31)))))
        {
            //强制将双倍行整行拉伸
            if((chDoubleHeight)&&((OsdView.m_chCharSize&1)==0))
            {
                if(y<23)
                {
                    OsdView.m_chCharSize|=1;
                }
            }
            if((OsdView.m_chCharSize&0x2)&&(x==chBoxEndX))
            {
                OsdView.m_chCharSize&=0xfd;
            }
            osd_draw_teletext(x,y,&OsdView);
        }
        else
        {
            nColorPre=OsdView.m_chBackColor;
            OsdView.m_chBackColor = CLUT[1][0];
            OsdView.m_chChar = 0x20;
                        //强制将双倍行整行拉伸
            if((chDoubleHeight)&&((OsdView.m_chCharSize&1)==0))
            {
                if(y<23)
                {
                    OsdView.m_chCharSize|=1;
                }
            }
            if((OsdView.m_chCharSize&0x2)&&(x==chBoxEndX))
            {
                OsdView.m_chCharSize&=0xfd;
            }
            osd_draw_teletext(x,y,&OsdView);
            OsdView.m_chBackColor=nColorPre;
        }
        if (OsdView.m_chCharSize&0x02)
        {
            x++;
        }
    }
    int32_t showmean = 0;
    GxBus_ConfigGetInt(GXBUS_TTX_SHOWMEAN,&showmean,GXBUS_TTX_SHOWMEAN_WRITE);
    if(showmean == GXBUS_TTX_SHOWMEAN_WRITE)
    {
        GxFBHAL_Sync(0,0);
    }
}

#if SUBTITLE_IN_TTX_CHANGE_COUNTER
void com_ttx_set_refresh_count(uint8_t uc_count)
{
	gs_ucTTXRefreshCounter = uc_count;
}
#endif


