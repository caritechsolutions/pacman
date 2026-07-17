/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2006, All right reserved
******************************************************************************

******************************************************************************
* File Name :	com_ttx_dec_sub.c
* Author    : 	hulj
* Project   :	GX6102    
* Type      :	win
******************************************************************************
* Purpose   :	模块实现文件
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008/07/24         hulj	         creation
*****************************************************************************/

#include "module/ttx/gxttx.h"
#include "av/avapi.h"
#include "gxos/gxcore_os.h"
#include "gxavdev.h"
#include "av/gxav_demux_propertytypes.h"
#include "av/gxav_module_property.h"
#include "av/gxav_event_type.h"


static int32_t dev;
static int32_t demux_handle;
static int8_t  dmx_slot_id = 0x40;     
static int8_t  dmx_filter_id = 0x40; 
	
extern uint8_t* pes_parse_buf;	
extern struct page_buffer* g_pBuffer;
extern uint8_t g_chDefaultG02;
extern struct x_29_pack g_X29PackView[8];
extern uint32_t CLUT[4][8];
extern uint8_t g_chTtxOsdEn; 
extern uint8_t g_chTtxVbiEn;
extern uint16_t g_wPageNo; 
extern uint8_t g_chSearchMagNum ;             
extern uint16_t g_wSearchPageNum ;   
extern uint8_t language_g0_g2(uint8_t data);
extern struct view_page    g_TTXPageShow ; 
extern  uint8_t*  pSppBuffer;
uint8_t chBufBlocFlag[ONE_MAGAZINE_BLOCKS*8];
uint8_t chCcBufBlocFlag[ONE_MAGAZINE_BLOCKS*8];

extern int teletext_get_magazine_cache_num(void);

#define CHECK_API_RET(api_ret, err_ret)		do{\
							if (api_ret < 0){\
								gxlogd("api ret err %s %d\n",__FUNCTION__, __LINE__);\
								return err_ret;}\
								}while(0)

/*****************************************************************************
 * Function	   : teletext_dec_8_30_1
 * Description : parse broadcast service data packet 
 * Arguments   : pPacket[IN]:the first address of this packet
 * Returns     : 0 : parse success
 				 1 : parse error
 * Other       :  
 ****************************************************************************/
#if 0
uint8_t  teletext_dec_8_30_1(uint8_t*pPacket)
{
	uint8_t chTmp1,chTmp2;
	chTmp1 = pack_x27_0_3__Designation_Code(pPacket);
	chTmp1 = hamming84(chTmp1);
	if (chTmp1 ==1||chTmp1 ==0)
	{
		//获得默认页
		chTmp1 = pack_8_30_1__page_number_tens(pPacket);
		chTmp2 = pack_8_30_1__page_number_units(pPacket);
		chTmp1 = hamming84(chTmp1);
		chTmp2 = hamming84(chTmp2);
		g_pBuffer->m_chNowPage=(chTmp1<<4)+chTmp2;
		//获得默认子页，和杂志
		chTmp1 = pack_8_30_1__s1(pPacket);
		chTmp2 = pack_8_30_1__s2_m1(pPacket);
		chTmp1 = hamming84(chTmp1);
		chTmp2 = hamming84(chTmp2);
		g_pBuffer->m_wNowSubcode = ((chTmp2&0x7)<<4)+chTmp1;
		g_pBuffer->m_chNowMagazine = chTmp2>>3;
		chTmp1 = pack_8_30_1__s3(pPacket);
		chTmp2 = pack_8_30_1__s4_m2_m3(pPacket);
		chTmp1 = hamming84(chTmp1);
		chTmp2 = hamming84(chTmp2);
		g_pBuffer->m_wNowSubcode = (((uint16_t)(((chTmp2&0x3)<<4)+(chTmp1)))<<8) 
			+ g_pBuffer->m_wNowSubcode;
        return 0;
	}
    else
    {
       return 1;
    }
}
#endif
/*****************************************************************************
 * Function	   : teletext_dec_x0
 * Description : parse packet 0 
 * Arguments   : pPacket[IN]:the first address of this packet
 				 chMagazineAddress[IN]:the magazine number this packet belonged to
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void teletext_dec_x0(uint8_t*pPacket, uint8_t chMagazineAddress)
{
	uint8_t chTmp1,i,chTmp2;
	uint16_t wSubCode;
	if((g_pBuffer == NULL) || (pPacket == NULL))
		return;
	g_pBuffer->m_chNowMagazine =  chMagazineAddress;
	chTmp1 = pack_x0__page_number_units(pPacket);
	chTmp1 = hamming84(chTmp1);
	chTmp2 = pack_x0__page_number_tens(pPacket);
	chTmp2 = hamming84(chTmp2);
	chTmp2 = chTmp2*10 + chTmp1;
	g_pBuffer->m_chNowPage= chTmp2 ;
	chTmp1 = pack_x0__s1(pPacket);
	chTmp2 = pack_x0__s2_c4(pPacket);
	chTmp1 = hamming84(chTmp1);
	chTmp2 = hamming84(chTmp2);
	wSubCode = ((chTmp2&0x7)<<4)+chTmp1;
	chTmp1 = pack_x0__s3(pPacket);
	chTmp2 = pack_x0__s4_c5_c6(pPacket);
	chTmp1 = hamming84(chTmp1);
	chTmp2 = hamming84(chTmp2);
	wSubCode = (((uint16_t)(((chTmp2&0x3)<<4)+(chTmp1)))<<8) + wSubCode;
	g_pBuffer->m_wNowSubCode= wSubCode;
	//now_state [0] :C4:擦除页信息。当置一时，表示在存储页信息时，先前传输的那页信息X/0~X/28必须先被擦除。
	chTmp1 = pack_x0__s2_c4(pPacket);
	chTmp1 = hamming84(chTmp1);
	g_pBuffer->m_chC4_C10= chTmp1>>3;
//	pViewPage->m_chShowState = chTmp1>>3;
	
	//now_state [1] :C5:当前页是新闻快报标志。当置一时，当某页被指定为新闻快报时不管该页是否含有信息置在新闻快报页中所有显示信息被加框C6:当前页是字幕。当置一时，表示当前页的将被加框显示。
	//now_state [2] :C6: 字幕标志。当某页被指定为字幕页时不管该页是否含有信息置在字幕页中所有显示信息被加框
	chTmp1 = pack_x0__s4_c5_c6(pPacket);
	chTmp1 = hamming84(chTmp1);
	g_pBuffer->m_chC4_C10 = ((chTmp1&0xc)>>1) + g_pBuffer->m_chC4_C10;
//	pViewPage->m_chShowState =((chTmp1&0xc)>>1) + pViewPage->m_chShowState;

	//now_state [3] :C7:显示抑制头部显示。当置一时，表示当前页的第一行将不被显示。
	//now_state [4] :C8:更新标志。当置一时，具有相同杂志号和页号的页的信息若部分或全部更新时置可以仅传送含有更新内容的排。
	//now_state [5] :C9:队列打断标志。当置一时，表示当前页是非按页号顺序传输的页。页头可以不显示。
	//now_state [6] :C10:抑制显示。当置一时，表示当前页中的内容不显示。
	chTmp1 = pack_x0__c7_c8_c9_c10(pPacket);
	chTmp2 = hamming84(chTmp1);
	g_pBuffer->m_chC4_C10 = ((chTmp2&0xf)<<3) + g_pBuffer->m_chC4_C10;
//	pViewPage->m_chShowState =((chTmp2&0xf)<<3) + pViewPage->m_chShowState;
	chTmp1 = pack_x0__c11_c14(pPacket);
	//now_state [7] :C11:传输方式标志.0并行传输，1串行传输。
    //X0包语言控制字段
    chTmp2 = hamming84(chTmp1); 
	g_pBuffer->m_chTransMeth = chTmp2&0x01;
    chTmp2 = ((chTmp2&0x08)>>3)|((chTmp2&0x04)>>1)|((chTmp2&0x02)<<1);
	g_pBuffer->m_chDefaultG0G2 = chTmp2;
    g_TTXPageShow.m_chX28Langctrl= g_pBuffer->m_chDefaultG0G2;
	for(i=0;i<8;i++)
	{
		g_pBuffer->m_chPageContext[0][i] = 32;
	}
	g_pBuffer->m_chPageContext[0][3] = 7;
	if(chMagazineAddress==0)
		g_pBuffer->m_chPageContext[0][4] = chMagazineAddress+0x38;
	else
		g_pBuffer->m_chPageContext[0][4] = chMagazineAddress+0x30;
	g_pBuffer->m_chPageContext[0][5] = g_pBuffer->m_chNowPage/10+0x30;
	g_pBuffer->m_chPageContext[0][6] = g_pBuffer->m_chNowPage%10+0x30;
	for(i=8;i<40;i++)
	{
		chTmp1 = pack_x0__data(pPacket, i-8);
		g_pBuffer->m_chPageContext[0][i] = ODD(chTmp1);
          
	}		
}
/*****************************************************************************
 * Function	   : teletext_dec_x27
 * Description : parse packet 27 
 * Arguments   : pPacket[IN]:the first address of this packet
 				 chMagazineAddress[IN]:the magazine number this packet belonged to
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void teletext_dec_x27(struct page_buffer*pPacket)
{
	//static uint8_t chDesignationCode;
	uint8_t chTmp1 = 0;
	uint8_t chTmp2 = 0;
	uint8_t i = 0;
	uint8_t *pLink = NULL;
	struct page_link *pPageLink = NULL;

	int Index = 0;
	unsigned short Mark = (pPacket->m_chPageContext[27][0]) | (pPacket->m_chPageContext[27][1] << 8);
	
	for(Index = 0; Index < 16; Index++)
	{
		if((Mark & (1 << Index)) == 0)
			continue;
		if (Index < 4)
		{// only deal with the link data
			pLink = &pPacket->m_chPageX27Context[Index][1];
			pPageLink = g_TTXPageShow.m_PageLink;
			for(i=0;i<4;i++)
			{
				pPageLink[i].m_chLinkEn = 1;
				chTmp1 = Link__page_number_tens(pLink);
				chTmp2 = Link__page_number_units(pLink);
				chTmp1 = hamming84(chTmp1);
				chTmp2 = hamming84(chTmp2);

                if(chTmp1 == 0xF && chTmp2 == 0xF)   //page number 0xFF : no page is specified. P38
                    pPageLink[i].m_chLinkEn = 0;

				pPageLink[i].m_chPage= chTmp1*10+chTmp2;
				//获得默认子页，和杂志
				chTmp1 = Link__s1(pLink);
				chTmp2 = Link__s2_m1(pLink);
				chTmp1 = hamming84(chTmp1);
				chTmp2 = hamming84(chTmp2);
				pPageLink[i].m_wSubcode = ((chTmp2&0x7)<<4)+chTmp1;
				pPageLink[i].m_chMagazine = chTmp2>>3;
				chTmp1 = Link__s3(pLink);
				chTmp2 = Link__s4_m2_m3(pLink);
				chTmp1 = hamming84(chTmp1);
				chTmp2 = hamming84(chTmp2);
				pPageLink[i].m_wSubcode = (((uint16_t)(((chTmp2&0x3)<<4)+(chTmp1)))<<8) + pPageLink[i].m_wSubcode;
				pPageLink[i].m_chMagazine = ((chTmp2&0xc)>>1)+pPageLink[i].m_chMagazine;
				pPageLink[i].m_chMagazine^=pPacket->m_chNowMagazine;//notice from protocol EN300706 page 38
                //gxlogd("==>> Link Magazine : %d Page : %d Subcode : %d\n", pPageLink[i].m_chMagazine, pPageLink[i].m_chPage, pPageLink[i].m_wSubcode);
				pLink = pLink + 6;
			}
			if (0 == Index ) 
			{
				chTmp1 = pPacket->m_chPageX27Context[Index][37];
				chTmp1 = hamming84(chTmp1);
				g_TTXPageShow.m_chShow24 = (chTmp1&0x8)>>3;
			}
	}
			
	}
	return;
}
/*****************************************************************************
 * Function	   : teletext_dec_x1_24
 * Description : parse packet 1 to packet 24 
 * Arguments   : pPacket[IN]:the first address of these packets
 				 chPacketAddress[IN]:the packet number
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void teletext_dec_x1_24(struct page_buffer *pPacket,uint8_t chPacketAddress)
{
	uint8_t chTmp1,i,*pModChar;
	pModChar = &g_TTXPageShow.m_chModChar[chPacketAddress][0];
	for(i=0;i<40;i++)
	{
		chTmp1 = pPacket->m_chPageContext[chPacketAddress][i];
		chTmp1 = ODD(chTmp1);
		pModChar[i]=chTmp1;
	}
	return;
}
/*****************************************************************************
 * Function	   : teletext_dec_x26
 * Description : parse packet 26 
 * Arguments   : pPacket[IN]:the first address of this packet
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void teletext_dec_x26(struct page_buffer*pPacket)
{
	uint8_t chTmp1,chTmp2,tmp3,i;
	struct x_26_pack *data_buffer;
	int X26Index = 0;
	unsigned short Mark = (pPacket->m_chPageContext[26][0]) | (pPacket->m_chPageContext[26][1] << 8);
	
	for(X26Index = 0; X26Index < 16; X26Index++)
	{
		if((Mark & (1 << X26Index)) == 0)
			continue;

		data_buffer = g_TTXPageShow.m_X26Pack[X26Index];
		//gxlogd("\nTTX, X26 index = %d\n", X26Index);
		for(i=0;i<13;i++)
		{
			chTmp1 = pPacket->m_chPageX26Context[X26Index][1+3*i];
			chTmp1 = hamming2418_1(chTmp1);

			chTmp2 = pPacket->m_chPageX26Context[X26Index][2+3*i];
			chTmp2 = hamming2418_2(chTmp2);

			tmp3 = pPacket->m_chPageX26Context[X26Index][3+3*i];
			tmp3 = hamming2418_3(tmp3);
			//按规范存储	
			data_buffer[i].m_chAddress = chTmp1 + ((chTmp2&0x3)<<4);
			data_buffer[i].m_chMode = (chTmp2&0x7C)>>2;
			data_buffer[i].m_chData = (tmp3);
		}
	}
}
/*****************************************************************************
 * Function	   : teletext_dec_x28
 * Description : parse packet 28
 * Arguments   : pPacket[IN]:the first address of this packet
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void teletext_dec_x28(struct page_buffer*pPacket)
{
	volatile uint8_t chTmp1;
	volatile uint8_t chTmp2;
	volatile uint8_t chTmp3;
	uint8_t i;
	volatile uint32_t nTmpData = 0;
	chTmp1=pPacket->m_chPageContext[28][0];
	chTmp1 = hamming84(chTmp1);
	switch(chTmp1)
	{
		case 0x00:
		case 0x03:
		case 0x04:
				//解码顺序
				//海明码tmp1->1,2,3,4 tmp2->5,6,7,8,9,10,11,tmp3->12,13,14,15,16,17,18
				//第一个数据群
			for(i = 0; i < 13; i ++)
			{
				chTmp1 = pPacket->m_chPageContext[28][1+3*i];
				chTmp1 = hamming2418_1(chTmp1);

				chTmp2 = pPacket->m_chPageContext[28][2+3*i];
				chTmp2 = hamming2418_2(chTmp2);

				chTmp3 = pPacket->m_chPageContext[28][3+3*i];
				nTmpData = hamming2418_3(chTmp3);
				nTmpData = (((nTmpData<<7)+chTmp2)<<4)+chTmp1;
				if(i==0)
				{
					g_TTXPageShow.m_chDefaultG0G2 = (uint8_t)((nTmpData&0x3c00)>>7);
				}
				if(i==12)
				{
//					g_TTXPageShow.m_chDefaultFrontClut= (uint8_t)((nTmpData&0x3000)>>12);
//					g_TTXPageShow.m_chDefaultFrontCol= (uint8_t)((nTmpData&0xe00)>>9);
				}
			}
			break;
		case 0x01:break;
		case 0x02:break;
		default:break;
	}
}

/*****************************************************************************
 * Function	   : teletext_dec_x29
 * Description : parse packet 29
 * Arguments   : pPacket[IN]:the first address of this packet
 				 chMagazineAddress[IN]:the magazine number this packet belonged to
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void teletext_dec_x29(uint8_t chMagzineNum,struct page_buffer*pPacket)
{
	uint8_t chTmp1,chTmp2,chTmp3;
	uint32_t nTmpData = 0;
	uint8_t i;
	struct x_29_pack*x_29_pack_ptr;
	x_29_pack_ptr=&g_X29PackView[chMagzineNum];
	chTmp1=pPacket->m_chPageContext[29][0];
	chTmp1 = hamming84(chTmp1);
	switch(chTmp1)
	{
		case 0x00:
				//解码顺序
				//海明码tmp1->1,2,3,4 tmp2->5,6,7,8,9,10,11,tmp3->12,13,14,15,16,17,18
				//第一个数据群
			for(i = 0; i < 13; i ++)
			{
				chTmp1 = pPacket->m_chPageContext[29][1+3*i];
				chTmp1 = hamming2418_1(chTmp1);

				chTmp2 = pPacket->m_chPageContext[29][2+3*i];
				chTmp2 = hamming2418_2(chTmp2);

				chTmp3 = pPacket->m_chPageContext[29][3+3*i];
				chTmp3 = hamming2418_3(chTmp3);
				nTmpData = (((nTmpData<<7)+chTmp2)<<4)+chTmp1;
				if(i == 0)
				{
                    		x_29_pack_ptr->m_chX29Langctrl = (uint8_t)((nTmpData&0x3c00)>>7);
	
				}
				if(i==12)
				{
					x_29_pack_ptr->m_chDefaultFrontClut= (uint8_t)((nTmpData&0x3000)>>12);
					x_29_pack_ptr->m_chDefaultFrontCol= (uint8_t)((nTmpData&0xe00)>>9);
				}
			}
			break;
		case 0x01:break;
		case 0x02:break;
		case 0x03:break;
		case 0x04:break;
		default:break;
	}
}

/*****************************************************************************
 * Function	   : ttx_parse_x0_x24_char
 * Description : decode the character from packet 0 to packet 24
 * Arguments   : chTtxCharMod[IN]:the character will be decoded
 				 pViewPage[IN]:pointer to the page buffer
 				 ppOsdView[OUT]:the character and its attribute,which will be displayed
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void ttx_parse_x0_x24_char(uint8_t chTtxCharMod, struct view_page *pViewPage,struct osd_view *ppOsdView)
{
	volatile struct osd_view *pOsdView;
	pOsdView = ppOsdView;

	switch (chTtxCharMod)
    {

        case 0x00:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][0];	//	
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x01:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][1];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x02:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][2];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x03:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][3];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x04:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][4];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x05:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][5];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x06:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][6];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x07:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][7];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 0;
                break;
            }
        case 0x08:
            {
                pOsdView->m_chFlash=1;
                break;
            }
        case 0x09:
            {
                pOsdView->m_chFlash=0;
                break;
            }
        case 0x0a://box end
            {
                break;
            }
        case 0x0b://box start
            {
                break;
            }
        case 0x0c:
            {
                pOsdView->m_chCharSize =0;
                break;
            }
        case 0x0d:
            {
                pOsdView->m_chCharSize = 1;
                break;
            }
        case 0x0e:
            {
                pOsdView->m_chCharSize = 2;
                break;
            }
        case 0x0f:
            {
                pOsdView->m_chCharSize = 3;
                break;
            }
        case 0x10:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][0];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x11:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][1];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x12:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][2];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x13:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][3];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x14:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][4];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x15:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][5];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x16:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][6];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x17:
            {
                pOsdView->m_chFrontColor = CLUT[pViewPage->m_chDefaultFrontClut][7];		
                pOsdView->m_chConceal = 0;
                pOsdView->m_chMosaics = 1;
                if(pOsdView->m_chContiguousM == 0xFF) pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x18:
            {
                pOsdView->m_chConceal = 0;
                break;
            }
        case 0x19:
            {
                pOsdView->m_chContiguousM = 1;
                break;
            }
        case 0x1a:
            {
                pOsdView->m_chContiguousM = 0;
                break;
            }
        case 0x1b:
            {
                if (pOsdView->m_chSetSelect == 0)
                    pOsdView->m_chSetSelect = 8;
                else
                    pOsdView->m_chSetSelect = 0;
                break;
            }
        case 0x1c:
            {
                pOsdView->m_chBackColor = CLUT[pViewPage->m_chDefaultBackClut][0];
                break;
            }
        case 0x1d:
            {
                pOsdView->m_chBackColor = pOsdView->m_chFrontColor;
                break;
            }
        case 0x1e:
            {
                pOsdView->m_chHoldMosaics = 1;
                break;
            }
        case 0x1f:
            {
                pOsdView->m_chHoldMosaics = 0;
                break;
            }
        default:
            break;
    }
}



/*****************************************************************************
 * Function	   : ttx_buffer_malloc
 * Description : allocate an area with fixed size in SPP
 * Arguments   : pAdd[OUT]:the first address of the area which has been allocated
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void ttx_buffer_malloc(uint8_t **pAdd)
{	
	uint32_t i;
	uint32_t nBlocks;
	*pAdd = NULL;

	if(CcOpenFlag)
	{
		return;
	}	
	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	for(i=0; i<nBlocks; i++)
	{
		if(chBufBlocFlag[i] == 0)
		{
			chBufBlocFlag[i] = 1;
			*pAdd = pSppBuffer+(i*PAGE_BUFFER_SIZE);
			memset(*pAdd,0,sizeof(struct page_buffer));
            //if((g_pBuffer->m_chNowMagazine == g_chSearchMagNum) && (g_pBuffer->m_chNowPage == g_wSearchPageNum))
            //    gxlogd("\033[32m==>> Malloc Index[%d] subcode : 0x%x\n\033[0m", i, g_pBuffer->m_wNowSubCode);
			break;
		}
	}
}

/*****************************************************************************
 * Function	   : ttx_buffer_is_full
 * Description : check ttx buffer
 * Arguments   : 1 : full 
 * Returns     : 
 * Other       :  
 ****************************************************************************/
int ttx_buffer_is_full(void)
{
	uint32_t i;
	uint32_t nBlocks;

	if(CcOpenFlag)
	{
		return 1;
	}	

	nBlocks = ONE_MAGAZINE_BLOCKS*(teletext_get_magazine_cache_num());
	for(i=0; i<nBlocks; i++)
	{
		if(chBufBlocFlag[i] == 0)
		{
            return 0;
		}
	}

    return 1;
}

/*****************************************************************************
 * Function	   : check_packet_length
 * Description : check the length of TTX packet whether is 45 bytes or 46 bytes
 * Arguments   : pStartAdd[IN]:the first address of the TTX information
 				 nBufferSize[IN]:the length of TTX information
 * Returns     : the length of TTX packet
 * Other       :  
 ****************************************************************************/
void check_packet_length(uint8_t *pStartAdd,uint32_t nBufferSize,uint8_t *pTtxPacketLen)
{
	uint32_t i;
	*pTtxPacketLen = 46;
	for(i= 0; i<nBufferSize/46;i++)
		{
		if(*((uint8_t*)pStartAdd+3+i*46) != 0xe4)
			break;
		}
	if(i==nBufferSize/46)
		{
		*pTtxPacketLen = 46;
		}
	else *pTtxPacketLen = 45;
}

/*****************************************************************************
 * Function	   : color_check
 * Description : find the index of color from given palette which is the most 
 				 similar to the color transported by TTX packet
 * Arguments   : pPalet[IN]:the first address of given palette
 				 nPaletNum[IN]:the number of color within given palette
 				 pRealCol[IN]:the 12 bits color value transported by TTX packet
 				 chRealColNum[IN]:the number of the 12 bits color
 				 pColor_idex[OUT]:the index of the most similar color in palette
 * Returns     : 
 * Other       :  
 ****************************************************************************/
void color_check(uint32_t *pPalet, uint32_t nPaletNum, uint16_t *pRealCol, uint8_t chRealColNum, uint32_t *pColor_idex)
{
	uint8_t i;
	uint16_t j;
	uint32_t nColorDistanceTem, nColorDistance = -1;
	color_pre_t pre,test;

	
	for(i=0; i<chRealColNum; i++)
	{
		test.b = (pRealCol[i]>>8)&0xf;
		test.b = test.b*17;
		test.g = (pRealCol[i]>>4)&0xf;
		test.g = test.g*17;
		test.r = pRealCol[i]&0xf;
		test.r = test.r*17;
		nColorDistance = 0xffffffff;
		for(j=0; j<nPaletNum; j++)
		{
			nColorDistanceTem = pPalet[j];
			if(nColorDistanceTem==0)
			{
				pColor_idex[8] = j;
				continue;
			}
			pre.b = (nColorDistanceTem>>16)&0xff;
			pre.g = (nColorDistanceTem>>8)&0xff;
			pre.r = nColorDistanceTem&0xff;
			nColorDistanceTem = (pre.b - test.b)*(pre.b - test.b);
			nColorDistanceTem += (pre.g - test.g)*(pre.g - test.g);
			nColorDistanceTem += (pre.r - test.r)*(pre.r - test.r);
			if(nColorDistanceTem < nColorDistance)
			{
				nColorDistance = nColorDistanceTem;
				pColor_idex[i] = j;
			}
		}
	}
}

status_t ttx_demux_close(void)
{
	int32_t api_ret;
	
	api_ret = GxAvdev_CloseModule(dev, demux_handle);
	demux_handle = -1;
	
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	api_ret = GxAvdev_DestroyDevice(dev);
	dev = -1;
	
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	return api_ret;
}

status_t ttx_demux_open(void)
{
	
	if((dev > 0) || (demux_handle > 0))
		ttx_demux_close();
	dev = GxAvdev_CreateDevice(0);

	if (dev < 0)
	{
		return GXCORE_ERROR;
	}
	demux_handle = (uint32_t)(GxAvdev_OpenModule(dev, GXAV_MOD_DEMUX, 0));

	
	if (demux_handle <= 0)
	{
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

status_t ttx_filter_setup(uint16_t ttx_pid)
{
	GxDemuxProperty_Slot dmx_slot_prop = {0};
	GxDemuxProperty_Filter dmx_filter_prop = {0};
	int32_t api_ret;
	
	memset(&dmx_filter_prop, 0, sizeof(GxDemuxProperty_Filter));
	// alloc and config dmx_slot-----------------------------------------------------
    memset(&dmx_slot_prop, 0, sizeof(GxDemuxProperty_Slot));
    dmx_slot_prop.pid = ttx_pid;
	dmx_slot_prop.type = DEMUX_SLOT_PES;
	api_ret = GxAVGetProperty(dev, demux_handle, GxDemuxPropertyID_SlotAlloc,
					(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));

	//dmx_slot_prop.slot_id already get from SlotAlloc
	dmx_slot_prop.pid = ttx_pid;
	dmx_slot_prop.type = DEMUX_SLOT_PES;
	dmx_slot_prop.flags = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
	
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_SlotConfig, 
					(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
	memset(&dmx_filter_prop, 0, sizeof(GxDemuxProperty_Filter));
	dmx_filter_prop.slot_id = dmx_slot_prop.slot_id;
		// alloc and config dmx_filter-----------------------------------------------------
	api_ret = GxAVGetProperty(dev, demux_handle, GxDemuxPropertyID_FilterAlloc, 
					(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

			//dmx_filter_prop.filter_id already get from FilterAlloc
	dmx_filter_prop.depth = 1;
	dmx_filter_prop.key[0].value = 0xbd;
	dmx_filter_prop.key[0].mask = 0xff;
	
	dmx_filter_prop.flags = DMX_EQ;
	dmx_filter_prop.flags |= DMX_CRC_IRQ;
	
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterConfig, 
						(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));


	dmx_slot_id = dmx_filter_prop.slot_id;
	dmx_filter_id = dmx_filter_prop.filter_id;


	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_SlotEnable, 
								(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterEnable, 
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	return api_ret;
	
}

status_t ttx_filter_free(void)
{
	GxDemuxProperty_Slot dmx_slot_prop = {0};
	GxDemuxProperty_Filter dmx_filter_prop = {0};
	GxDemuxProperty_FilterFifoReset dmx_fifo_reset = {0};
	int32_t api_ret;
	// free dmx_filter
	if(dmx_filter_id == 0x40 || dmx_slot_id == 0x40)
	{
		return GXCORE_ERROR;
	}

	dmx_filter_prop.filter_id = dmx_filter_id;
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterDisable,
									(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	dmx_fifo_reset.filter_id = dmx_filter_id;
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterFIFOReset, 
								(void*)&dmx_fifo_reset, sizeof(GxDemuxProperty_FilterFifoReset));	
	CHECK_API_RET(api_ret, GXCORE_ERROR);
	


	
	dmx_filter_prop.filter_id = dmx_filter_id;
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterFree, 
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

	CHECK_API_RET(api_ret, GXCORE_ERROR);


	dmx_slot_prop.slot_id = dmx_slot_id;
	dmx_slot_prop.type = DEMUX_SLOT_PES;
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_SlotFree, 
						(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	dmx_filter_id = 0x40;
	dmx_slot_id = 0x40;

	return api_ret;
}


status_t ttx_filter_resert(void)
{
	GxDemuxProperty_Filter dmx_filter_prop = {0};
	GxDemuxProperty_FilterFifoReset dmx_fifo_reset = {0};
	int32_t api_ret;

	dmx_filter_prop.filter_id = dmx_filter_id;
	dmx_fifo_reset.filter_id = dmx_filter_id;
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterDisable, 
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterFIFOReset, 
								(void*)&dmx_fifo_reset, sizeof(GxDemuxProperty_FilterFifoReset));	

	CHECK_API_RET(api_ret, GXCORE_ERROR);

	dmx_filter_prop.filter_id = dmx_filter_id;
	api_ret = GxAVSetProperty(dev, demux_handle, GxDemuxPropertyID_FilterEnable, 
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	return api_ret;
}




status_t ttx_filter_query (uint8_t* pflag)
{
	GxDemuxProperty_FilterFifoQuery status = {0};
	int32_t api_ret = 0;
	uint32_t event_ret;


	api_ret = GxAVWaitEvents(dev, demux_handle,
						EVENT_DEMUX0_FILTRATE_PES_END, 200000, &event_ret);
	//CHECK_API_RET(api_ret, GXCORE_ERROR);
	if(api_ret < 0)
	{
		*pflag = 0;
        ttx_filter_resert();
		return GXCORE_ERROR;
	}

	api_ret = GxAVGetProperty(dev, demux_handle, 
				GxDemuxPropertyID_FilterFIFOQuery, 
				(void*)&status, sizeof(GxDemuxProperty_FilterFifoQuery));
	CHECK_API_RET(api_ret, GXCORE_ERROR);
	
	*pflag = (uint8_t)((status.state>>dmx_filter_id) & 1);

	return GXCORE_SUCCESS;
}


size_t ttx_filter_read(uint8_t  *data_buf, size_t  data_len)
{
	GxDemuxProperty_FilterRead dmx_filter_read;
	int32_t api_ret;


	dmx_filter_read.filter_id = dmx_filter_id;
	dmx_filter_read.buffer = (void*)data_buf;
	dmx_filter_read.max_size = data_len;
	
	api_ret = GxAVGetProperty(dev, demux_handle, GxDemuxPropertyID_FilterRead, 
								(void*)&dmx_filter_read, sizeof(GxDemuxProperty_FilterRead));
	CHECK_API_RET(api_ret, 0);

	return dmx_filter_read.read_size;
}


void ttx_filter_handle(void)
{
	uint8_t flag = 0;
	uint32_t read_size;
	uint8_t *pData = NULL;
	uint16_t length= 0;
	
	ttx_filter_query(&flag);
	if(flag)
	{
		if(pes_parse_buf==NULL)
		{
			return;
		}
		read_size = ttx_filter_read(pes_parse_buf, PES_PARSE_BUF_SIZE);
		pData = pes_parse_buf;
		while(read_size>0)
		{	
			length = (uint16_t)(((pData[4]<<8)|pData[5]) + 6);
			if((length > read_size)||length<=8)
			{				
				return;
			}
			ttx_get_data(pData,&length);
			pData +=length;
			read_size -= length;
		}
	}

}

void character_enlarger_width(uint8_t *pPreChar,uint32_t nLength,uint8_t *pResult)
{	
	uint8_t *pTem;
	uint32_t i;
	
	for(i=0;i<nLength;i+=2)
	{
		pTem=pPreChar+i;
		*pResult=(*pTem&0x80)+((*pTem&0x80)>>1)
			+((*pTem&0x40)>>1)+((*pTem&0x40)>>2)
			+((*pTem&0x20)>>2)+((*pTem&0x20)>>3)
			+((*pTem&0x10)>>3)+((*pTem&0x10)>>4);
		pResult[1]=((*pTem&0x8)<<4)+((*pTem&0x8)<<3)
			+((*pTem&0x4)<<3)+((*pTem&0x4)<<2)
			+((*pTem&0x2)<<2)+((*pTem&0x2)<<1)
			+((*pTem&0x1)<<1)+(*pTem&0x1);
		pResult[2]=(pTem[1]&0x80)+((pTem[1]&0x80)>>1)
			+((pTem[1]&0x40)>>1)+((pTem[1]&0x40)>>2)
			+((pTem[1]&0x20)>>2)+((pTem[1]&0x20)>>3)
			+((pTem[1]&0x10)>>3)+((pTem[1]&0x10)>>4);
		pResult+=3;
	}
}

void character_enlarger_height(uint8_t *pPreChar, uint32_t nLength,uint8_t chCountPerLine,uint8_t *pResult)
{	
	uint8_t *pTem;
	uint32_t i,j;
	
	for(i=0;i<nLength;i+=chCountPerLine)
	{
		for(j=0;j<chCountPerLine;j++)
		{
			pTem=pPreChar+i;
			pResult[j]=pTem[j];
			pResult[j+chCountPerLine]=pResult[j];
		}
		pResult+=2*chCountPerLine;
	}
	
}


