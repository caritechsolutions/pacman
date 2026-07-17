/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2006, All right reserved
******************************************************************************

******************************************************************************
* File Name :	mod_ttx_show.c  
* Author    : 	hulj
* Project   :	GX6102
* Type      :	module
******************************************************************************
* Purpose   :	Ä£¿éÊµÏÖÎÄ¼þ
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008.03.25        hulj	         creation
*****************************************************************************/
#include "module/ttx/gxttx.h"
//#include "com_osd.h"
#include "module/ttx/ttx_sub.h"
//#include "gxfontlib_bin.h"

extern uint32_t g_OsdTtxLang;
//extern GXOSD_RegionHandle_t g_RegionHandle;
//extern Handle_t g_SppHandle;
extern void ttx_drawtext(int x0, int y0,
                       int xsize, int ysize,
                       int BitsPerPixel, 
                       const uint8_t   * pData, 
		               int TextColor,int BackColor);
extern uint8_t show_teletext_text_ascii(uint16_t X,uint16_t Y,struct osd_view*osd_view);
#ifdef TTX_NEW_CHARACTER_SET

extern unsigned char txtlib_bin[] ;
extern unsigned int txtlib_bin_len;
/*test for ttx .20090210 */

#define G0_CHARACTER_COUNT 96

#define LATIN_G0_START_ADD   0          

#define LATIN_NATIONAL_SUB_START_ADD   (LATIN_G0_START_ADD+576)          //96*6

#define G1_BLOCK_CONTIGUOUS_START_ADD   (LATIN_NATIONAL_SUB_START_ADD+1014) //13*13*6

#define G1_BLOCK_SEPARATED_START_ADD  (G1_BLOCK_CONTIGUOUS_START_ADD+576)


#define G3_SMOOTH_START_ADD  (G1_BLOCK_SEPARATED_START_ADD+576)//reserve

#define LATIN_G2_START_ADD	 (G3_SMOOTH_START_ADD+576)

#define CYRILLIC_G0_1  (LATIN_G2_START_ADD+576)

#define CYRILLIC_G0_2  (CYRILLIC_G0_1+576)

#define CYRILLIC_G0_3  (CYRILLIC_G0_2+576)

#define CYRILLIC_G2  (CYRILLIC_G0_3+576)

#define GREEK_G0  (CYRILLIC_G2+576)

#define GREEK_G2  (GREEK_G0+576)

#define ARABIC_G0  (GREEK_G2+576)

#define ARABIC_G2  (ARABIC_G0+576)

#define HEBREW_G0   (ARABIC_G2+576)



void character_mix(uint8_t* pData1,uint8_t * pData2, uint8_t *pResult);

extern struct view_page    g_TTXPageShow ;     //Ö¸ÏòÕýÔÚÏÔÊ¾µÄÒ³ÃæµÄ½Úµã
void osd_draw_teletext(uint8_t x,uint8_t y,struct osd_view*osd_view)
{
	uint8_t i,chLanSel = 0;
	uint16_t X,Y;
	uint32_t* p = NULL;
	uint32_t nStartAdd;
	uint8_t LatinSubCharIndex[SUBCHAR_NUMBER]=
		{G0_2_3,G0_2_4,G0_4_0,G0_5_B,G0_5_C,G0_5_D,G0_5_E,G0_5_F,G0_6_0,G0_7_B,G0_7_C,G0_7_D,G0_7_E};
	uint8_t BlockSubCharIndex[SUBCHAR_NUMBER]=
		{-1,-1,G0_4_0,G0_5_B,G0_5_C,G0_5_D,G0_5_E,G0_5_F,-1,-1,-1,-1,-1};
	uint32_t SectionAddr=0;
	uint8_t chSubLangSel;
	volatile uint32_t SectionAddrMix;

	uint8_t* pTemp=(uint8_t*)txtlib_bin;
	
	//ÅÐ¶ÎÓÐÎÞASCII×Ö¿â
	p = (uint32_t*)txtlib_bin;
	if(*p == 0xffffffff || *p == 0)
	{
		//SEARCH_PRINTF("Erro:ÏµÍ³ÎÞ×Ö¿â\n");
		return ;
	}

    //if(osd_view->m_chChar == 0x7C)    //For Debug
    //{
    //    gxlogd("==>>>>>  m_chSetSelect : %d  Lang : 0x%x GOG2 : %d GO2 : %d DefG0G2 : %d X28C : %d\n", 
    //            osd_view->m_chSetSelect,
    //            g_OsdTtxLang,
    //            osd_view->m_chG0G2LanguageSelect,
    //            osd_view->m_chG02LanguageSelect,
    //            g_TTXPageShow.m_chDefaultG0G2,
    //            g_TTXPageShow.m_chX28Langctrl);
    //}
	
	if((osd_view->m_chSetSelect==0)||(osd_view->m_chSetSelect==8)||(osd_view->m_chX26ColumnMode16==1))
	{
		 if((g_OsdTtxLang != 0xFF) && (osd_view->m_chSetSelect==0))
		 {
                    chLanSel = g_OsdTtxLang;
			switch(chLanSel)
			{
				case Arabic:
					chLanSel = 0x57;
					goto setup2;
					break;
				case Russian:
					chLanSel = Serbian_Cyrillic_2_Set;
					goto setup2;
					break;
				case Greek:
					chLanSel = Greek_Set;
					goto setup2;
					break;
				default:
					break;
			}
		 }
		 else
		 {
			if(osd_view->m_chSetSelect==0)
			{
				chLanSel=osd_view->m_chG0G2LanguageSelect;
			}
			else
			{
				//gxlogd("\nosd_view->m_chSetSelect==8\n");
				chLanSel=osd_view->m_chG02LanguageSelect;
			}
			
		 }

         if(osd_view->m_chX26ColumnMode16==1)
         {
             chLanSel = English;
         }

         switch(chLanSel)
         {
			case German://german
				chSubLangSel = 4;
				break;
			case Swedish://Swedish/Finnish/Hungarian
				chSubLangSel = 11;
				break;
			case Italian://Italian 
				chSubLangSel = 5;
				break;
			case French://French
				chSubLangSel = 3;
				break;
			case Portuguese://Portuguese/Spanish
				chSubLangSel = 8;
				break;
			case Czech:
				chSubLangSel = 0;//Czech/Slovak 
				break;
			case Turkish:
				chSubLangSel=12;//Turkish 
				break;
			case Rumanian:
				chSubLangSel=9;//Rumanian
				break;
			case Polish:
				chSubLangSel=7;
				break;
			default:
				chSubLangSel = 1;//english
				break;
		}
		for(i=0;i<SUBCHAR_NUMBER;i++)
		{
			if(osd_view->m_chChar==LatinSubCharIndex[i])
			{
				SectionAddr=*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+5));
				break;
			}
		}
		if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
		{
			if(osd_view->m_chSetSelect==0)
			{
				chLanSel=osd_view->m_chG0G2LanguageSelect;
			}
			else
			{
				chLanSel=osd_view->m_chG02LanguageSelect;
			}
setup2:		
			switch(chLanSel)
			{
				case Serbian_Cyrillic_1_Set:
					nStartAdd=CYRILLIC_G0_1;
					break;
				case Serbian_Cyrillic_2_Set:
					nStartAdd=CYRILLIC_G0_2;
					break;
				case Serbian_Cyrillic_3_Set:
					nStartAdd=CYRILLIC_G0_3;
					break;
				case Greek_Set:
					nStartAdd=GREEK_G0;
					break;
				case Hebrew_Set:
					nStartAdd=HEBREW_G0;
					break;
				case 0x47:
				case 0x57:
					nStartAdd=ARABIC_G0;
					break;
				default:
					nStartAdd=LATIN_G0_START_ADD;
					break;
			}
			
			SectionAddr=*(pTemp+6+nStartAdd+6*(osd_view->m_chChar-0x20)+2);
			SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+3));
			SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+4));
			SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+5));
		}
	}
	//G2×Ö·û¼¯
	else if(osd_view->m_chSetSelect==1)
	{
		 if(g_OsdTtxLang != 0xFF)
		 {
                    chLanSel = g_OsdTtxLang;
			switch(chLanSel)
			{
				case Arabic:
					chLanSel = 0x57;
					break;
				case Russian:
					chLanSel = Serbian_Cyrillic_2_Set;
					break;
				case Greek:
					chLanSel = Greek_Set;
					break;
				default:
					chSubLangSel = 0xff;//latin g2
					break;
			}
		 }
		 else
		 {
			chLanSel=osd_view->m_chG0G2LanguageSelect;
		 }
		
		switch(chLanSel)
		{
			case Serbian_Cyrillic_1_Set:
			case Serbian_Cyrillic_2_Set:
			case Serbian_Cyrillic_3_Set:
				nStartAdd=CYRILLIC_G2;
				break;
			case Greek_Set:
				nStartAdd=GREEK_G2;
				break;
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
			case 0x50:
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x56:
			case 0x57:
				nStartAdd=ARABIC_G2;
				break;
			default:
				nStartAdd=LATIN_G2_START_ADD;
				break;
		}
		SectionAddr=*(pTemp+6+nStartAdd+6*(osd_view->m_chChar-0x20)+2);
		SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+3));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+4));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+5));
	}
	
	//ÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect&2)
	{
		if(g_OsdTtxLang != 0XFF)
		{
			switch(g_OsdTtxLang)
			{
				case German://german
					chSubLangSel = 4;
					break;
				case Swedish://Swedish/Finnish/Hungarian
					chSubLangSel = 11;
					break;
				case Italian://Italian 
					chSubLangSel = 5;
					break;
				case French://French
					chSubLangSel = 3;
					break;
				case Portuguese://Portuguese/Spanish
					chSubLangSel = 8;
					break;
				case Czech:
					chSubLangSel = 0;//Czech/Slovak 
					break;
				case Turkish:
					chSubLangSel=12;//Turkish 
					break;
				case Rumanian:
					chSubLangSel=9;//Rumanian
					break;
				case Polish:
					chSubLangSel=7;
					break;
				case Arabic:
					osd_view->m_chG0G2LanguageSelect = Arabic_Set;
					goto setup3;
					break;
				case Russian:
					osd_view->m_chG0G2LanguageSelect = Serbian_Cyrillic_2_Set;
					goto setup3;
					break;	
				case Greek:
					osd_view->m_chG0G2LanguageSelect = Greek_Set;
					goto setup3;
					break;
				default:
					chSubLangSel = 1;//english
					break;
			}
		}
		else
		{
			switch(osd_view->m_chG0G2LanguageSelect)
			{
				case German://german
					chSubLangSel = 4;
					break;
				case Swedish://Swedish/Finnish/Hungarian
					chSubLangSel = 11;
					break;
				case Italian://Italian 
					chSubLangSel = 5;
					break;
				case French://French
					chSubLangSel = 3;
					break;
				case Portuguese://Portuguese/Spanish
					chSubLangSel = 8;
					break;
				case Czech:
					chSubLangSel = 0;//Czech/Slovak 
					break;
				case Turkish:
					chSubLangSel=12;//Turkish 
					break;
				case Rumanian:
					chSubLangSel=9;//Rumanian
					break;
				case Polish:
					chSubLangSel=7;
					break;
				default:
					chSubLangSel = 1;//english
					break;
			}
		}
		for(i=0;i<SUBCHAR_NUMBER;i++)
		{
			if(osd_view->m_chChar==BlockSubCharIndex[i])
			{
				SectionAddr=*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i+5));
				break;
			}
		}
		if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
		{
setup3:
			if((osd_view->m_chChar>0x3f)&&(osd_view->m_chChar<0x60))
			{
				switch(osd_view->m_chG0G2LanguageSelect)
				{
					case Serbian_Cyrillic_1_Set:
						nStartAdd=CYRILLIC_G0_1;
						break;
					case Serbian_Cyrillic_2_Set:
						nStartAdd=CYRILLIC_G0_2;
						break;
					case Serbian_Cyrillic_3_Set:
						nStartAdd=CYRILLIC_G0_3;
						break;
					case Greek_Set:
						nStartAdd=GREEK_G0;
						break;
					case Arabic_Set:
						nStartAdd=ARABIC_G0;
						break;
					default:
						nStartAdd=LATIN_G0_START_ADD;
						break;
				}
				SectionAddr=*(pTemp+6+nStartAdd+6*(osd_view->m_chChar-0x20)+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+5));
			}
			else if(osd_view->m_chSetSelect==2)//·ÖÁÑÏâÇ¶×Ö·û¼¯
			{
				SectionAddr=*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+5));
			}
			else
			{
				SectionAddr=*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+5));
			}

		}
	}
	//Æ½»¬ÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect==4)
	{
		SectionAddr=*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+2);
		SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+3));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+4));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+5));
	}
	X=x*s_AsciiFontWidth;
	Y=y*16;

	#ifdef CHARACTER_SIZE_ENLARGE
	uint8_t chDataTem[s_AsciiFontHeight*4*2];
	uint8_t chDataTem1[s_AsciiFontHeight*8*2];
	uint8_t chMixData[s_AsciiFontHeight*2];
	uint8_t *pData;
	if(SectionAddr >= txtlib_bin_len)
	{
		gxlogd("\nTTX, error, out the ttxlib\n");
		return ;
	}
	SectionAddr=(uint32_t)pTemp+SectionAddr;
	pData=(uint8_t*)SectionAddr;
	if(osd_view->m_chX26ChangeDataEnable)
	{
		SectionAddrMix=*(pTemp+6+LATIN_G2_START_ADD+6*(osd_view->m_chX26ChangeChar-0x20)+2);
		SectionAddrMix=(SectionAddrMix<<8)+(*(pTemp+6+LATIN_G2_START_ADD+6*(osd_view->m_chX26ChangeChar-0x20)+3));
		SectionAddrMix=(SectionAddrMix<<8)+(*(pTemp+6+LATIN_G2_START_ADD+6*(osd_view->m_chX26ChangeChar-0x20)+4));
		SectionAddrMix=(SectionAddrMix<<8)+(*(pTemp+6+LATIN_G2_START_ADD+6*(osd_view->m_chX26ChangeChar-0x20)+5));
		SectionAddrMix=(uint32_t)pTemp+SectionAddrMix;
		character_mix((uint8_t*)SectionAddrMix,(uint8_t*)SectionAddr,chMixData);
		pData=chMixData;
		osd_view->m_chX26ChangeDataEnable=0;
	}
	if(osd_view->m_chCharSize&0x2)
	{
		character_enlarger_width(pData,s_AsciiFontHeight*2, chDataTem);
        //character size : 24 * 16 
		if(osd_view->m_chCharSize&0x1)
		{
			character_enlarger_height(chDataTem, 3*s_AsciiFontHeight,3, chDataTem1);
			ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth*2,s_AsciiFontHeight*2,1, (uint8_t*)chDataTem1,osd_view->m_chFrontColor,osd_view->m_chBackColor);
		}
		else
		{
			ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth*2,s_AsciiFontHeight,1, (uint8_t*)chDataTem,osd_view->m_chFrontColor,osd_view->m_chBackColor);
		}
	}
	else if(osd_view->m_chCharSize&0x1)
	{
		character_enlarger_height(pData, 2*s_AsciiFontHeight,2, chDataTem);
		ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth,s_AsciiFontHeight*2,1, (uint8_t*)chDataTem,osd_view->m_chFrontColor,osd_view->m_chBackColor);
	}
	else
	{
		ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth, s_AsciiFontHeight,1,pData,osd_view->m_chFrontColor,osd_view->m_chBackColor);
	}
	#else
	ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth, s_AsciiFontHeight,1, (uint8_t*)SectionAddr,osd_view->m_chFrontColor,osd_view->m_chBackColor);	    
	#endif

	
}
void character_mix(uint8_t* pData1,uint8_t * pData2, uint8_t *pResult)
{
	uint8_t i;
	pData1=pData1+4;
	for(i=0;i<s_AsciiFontHeight*2;i++)
	{
		if(i>7)
		{
			*pResult=*pData2;
		}
        	else
        	{
			*pResult=*pData1;
			pData1++;
		}
		pResult++;
		pData2++;
	}
}

#else
void osd_draw_teletext(uint8_t x,uint8_t y,struct osd_view* osd_view)
{
	show_teletext_text_ascii( x<<4, y*18, osd_view);
	return;
}

uint8_t show_teletext_text_ascii(uint16_t X,uint16_t Y,struct osd_view*osd_view)
{
	uint8_t i;
	uint8_t SectionNo;
	uint8_t TotalSectionNo;
	uint16_t FontSize;
	uint8_t LatinSubCharIndex[SUBCHAR_NUMBER]=
		{G0_2_3,G0_2_4,G0_4_0,G0_5_B,G0_5_C,G0_5_D,G0_5_E,G0_5_F,G0_6_0,G0_7_B,G0_7_C,G0_7_D,G0_7_E};
	uint8_t BlockSubCharIndex[SUBCHAR_NUMBER]=
		{-1,-1,G0_4_0,G0_5_B,G0_5_C,G0_5_D,G0_5_E,G0_5_F,-1,-1,-1,-1,-1};
	uint8_t chrIndex;	
	uint8_t *SectionAddr=0;
	uint8_t* pTemp=(uint8_t*)ttx_font_data;
	
	//ÅÐ¶ÎÓÐÎÞASCII×Ö¿â
	if(*(uint32_t*)ttx_font_data == 0xffffffff || *(uint32_t*)ttx_font_data == 0)
	{
		SEARCH_PRINTF("Erro:ÏµÍ³ÎÞ×Ö¿â\n");
		return 0;
	}
	TotalSectionNo = *(pTemp+11);
	for(i=0;i<TotalSectionNo;i++)
	{
		SectionNo = *(pTemp+10);
		if(SectionNo == TeleTextSection)
		{
			break;
		}
		else
		{
			//Ìø×ªµ½ÏÂÒ»¸ö¶Î
			pTemp += *(uint32_t*)pTemp;	
			
			//ÅÐ¶Ï×Ö¿âÊÇ·ñÕý³£
			if(*(uint32_t*)pTemp == 0xffffffff || *(uint32_t*)pTemp == 0)
			{
				SEARCH_PRINTF("Error:ÏµÍ³ÎÞASCII×Ö¿â\n");
				return 0;
			}
		}
	}

	FontSize = s_AsciiFontWidth*s_AsciiFontHeight/8;
	chrIndex=((osd_view->m_chChar)>>4)+(((osd_view->m_chChar) & 0x0f)<<2)+(((osd_view->m_chChar) & 0x0f)<<1);
	chrIndex-=2;
	if(osd_view->m_chSetSelect==0)
	{		
		if(osd_view->m_chG0G2LanguageSelect==Serbian_Cyrillic_1_Set)
				SectionAddr=(uint8_t*)(pTemp+3*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG0G2LanguageSelect==Russian)
				SectionAddr=(uint8_t*)(pTemp+4*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG0G2LanguageSelect==Ukrainian)
				SectionAddr=(uint8_t*)(pTemp+5*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG0G2LanguageSelect==Greek)
				SectionAddr=(uint8_t*)(pTemp+6*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG0G2LanguageSelect==Arabic_Set)
				SectionAddr=(uint8_t*)(pTemp+7*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG0G2LanguageSelect==Hebrew_Set)			
				SectionAddr=(uint8_t*)(pTemp+8*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
		else
		{
			for(i=0;i<SUBCHAR_NUMBER;i++)
			{
				if(chrIndex==LatinSubCharIndex[i])//ÒõÓ°×Ö·û
				{
					if (osd_view->m_chG0G2LanguageSelect == 0)
					{
						//SEARCH_PRINTF("error for latin sub set \n");
						continue ;
					}
					
					chrIndex=i;
					SectionAddr=(uint8_t*)(pTemp+3*ListSize*FontSize+(osd_view->m_chG0G2LanguageSelect-1)*SUBCHAR_NUMBER*FontSize+chrIndex*FontSize+12);
					break;		
				}
			}
				
			if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
				SectionAddr=(uint8_t*)(pTemp+2*ListSize*FontSize+chrIndex*FontSize+12);
		}
		
	}
	//G2×Ö·û¼¯
	else if(osd_view->m_chSetSelect==1)
	{
		if(osd_view->m_chG0G2LanguageSelect==Latin)
			SectionAddr=(uint8_t*)(pTemp+9*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
		else if(osd_view->m_chG0G2LanguageSelect==Serbian_Cyrillic_Set)
			SectionAddr=(uint8_t*)(pTemp+10*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
		else if(osd_view->m_chG0G2LanguageSelect==Greek)
			SectionAddr=(uint8_t*)(pTemp+11*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
		else if(osd_view->m_chG0G2LanguageSelect==Arabic_Set)
			SectionAddr=(uint8_t*)(pTemp+12*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
	}
	//·ÖÁÑÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect==2)
	{
		for(i=0;i<SUBCHAR_NUMBER;i++)
		{
			if(chrIndex==BlockSubCharIndex[i])//ÒõÓ°×Ö·û
			{
				if (osd_view->m_chG0G2LanguageSelect == 0)
				{
					//SEARCH_PRINTF("error for latin sub set \n");
					continue ;
				}
				
				chrIndex=i;
				SectionAddr=(uint8_t*)(pTemp+3*ListSize*FontSize+(osd_view->m_chG0G2LanguageSelect-1)*SUBCHAR_NUMBER*FontSize+chrIndex*FontSize+12);
				break;		
			}
		}
		if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
		{
			if((((osd_view->m_chChar)>>4)==4)||(((osd_view->m_chChar)>>4)==5))
				SectionAddr=(uint8_t*)(pTemp+2*ListSize*FontSize+chrIndex*FontSize+12);
			else
				SectionAddr=(uint8_t*)(pTemp+ListSize*FontSize+chrIndex*FontSize+12);//G1Ô­±í·ÖÁÑÏâÇ¶
		}
	}
	//ÁªºÏÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect==3)
	{
		for(i=0;i<SUBCHAR_NUMBER;i++)
		{
			if(chrIndex==BlockSubCharIndex[i])//ÒõÓ°×Ö·û
			{
				if (osd_view->m_chG0G2LanguageSelect == 0)
				{
					//SEARCH_PRINTF("error for latin sub set \n");
					continue ;
				}
				
				chrIndex=i;
				SectionAddr=(uint8_t*)(pTemp+3*ListSize*FontSize+(osd_view->m_chG0G2LanguageSelect-1)*SUBCHAR_NUMBER*FontSize+chrIndex*FontSize+12);
				break;		
			}
		}
		if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
		{
			if((((osd_view->m_chChar)>>4)==4)||(((osd_view->m_chChar)>>4)==5))
				SectionAddr=(uint8_t*)(pTemp+2*ListSize*FontSize+chrIndex*FontSize+12);
			else
				//SectionAddr=(uint8_t*)(pTemp+14*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
				SectionAddr=(uint8_t*)(pTemp+chrIndex*FontSize+12);
		}
	}
	//Æ½»¬ÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect==4)
		SectionAddr=(uint8_t*)(pTemp+13*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
	//G0µÄµÚ¶þ¸ö×Ö·û¼¯
	else if(osd_view->m_chSetSelect==8)
	{
		
		if(osd_view->m_chG02LanguageSelect==Latin)
				SectionAddr=(uint8_t*)(pTemp+2*ListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG02LanguageSelect==Serbian_Cyrillic)
				SectionAddr=(uint8_t*)(pTemp+3*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG02LanguageSelect==Russian)
				SectionAddr=(uint8_t*)(pTemp+4*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG02LanguageSelect==Ukrainian)
				SectionAddr=(uint8_t*)(pTemp+5*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG02LanguageSelect==Greek)
				SectionAddr=(uint8_t*)(pTemp+6*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG02LanguageSelect==Arabic)
				SectionAddr=(uint8_t*)(pTemp+7*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
			else if(osd_view->m_chG02LanguageSelect==Hebrew)			
				SectionAddr=(uint8_t*)(pTemp+8*ListSize*FontSize+SubListSize*FontSize+chrIndex*FontSize+12);
		}
		else
		{
			for(i=0;i<SUBCHAR_NUMBER;i++)
			{
				if(chrIndex==LatinSubCharIndex[i])//ÒõÓ°×Ö·û
				{
					if (osd_view->m_chG02LanguageSelect == 0)
					{
						//SEARCH_PRINTF("error for latin sub set \n");
						continue ;
					}
						
					chrIndex=i;
					SectionAddr=(uint8_t*)(pTemp+3*ListSize*FontSize+(osd_view->m_chG02LanguageSelect-1)*SUBCHAR_NUMBER*FontSize+chrIndex*FontSize+12);
					break;		
				}
			}
				
			if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
				SectionAddr=(uint8_t*)(pTemp+2*ListSize*FontSize+chrIndex*FontSize+12);
		}
	#ifdef CHARACTER_SIZE_ENLARGE
	uint8_t chDataTem[s_AsciiFontHeight*4];
	uint8_t chDataTem1[s_AsciiFontHeight*8];


	if(osd_view->m_chCharSize&0x2)
	{
		character_enlarger_width((uint8_t*)SectionAddr, s_AsciiFontHeight*s_AsciiFontWidth/8, chDataTem);
		if(osd_view->m_chCharSize&0x1)
		{
			character_enlarger_height(chDataTem, s_AsciiFontHeight*s_AsciiFontWidth/4,s_AsciiFontWidth/4, chDataTem1);
			ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,32,s_AsciiFontHeight*2,1, (uint8_t*)chDataTem1,osd_view->m_chFrontColor,osd_view->m_chBackColor);
		}
		else
		{
			ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,32,s_AsciiFontHeight,1, (uint8_t*)chDataTem,osd_view->m_chFrontColor,osd_view->m_chBackColor);
		}
	}
	else if(osd_view->m_chCharSize&0x1)
	{
		character_enlarger_height((uint8_t*)SectionAddr, s_AsciiFontHeight*s_AsciiFontWidth/8,s_AsciiFontWidth/8, chDataTem);
		ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth,s_AsciiFontHeight*2,1, (uint8_t*)chDataTem,osd_view->m_chFrontColor,osd_view->m_chBackColor);
	}
	else
	{
		ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth, s_AsciiFontHeight,1, (uint8_t*)SectionAddr,osd_view->m_chFrontColor,osd_view->m_chBackColor);
	}
	#else
	ttx_drawtext(X+g_chTtxStartAddX,Y+g_chTtxStartAddY,s_AsciiFontWidth, s_AsciiFontHeight,1, (uint8_t*)SectionAddr,osd_view->m_chFrontColor,osd_view->m_chBackColor);	    
	#endif
	return 1;
	
}
#endif

 void ttx_drawtext(int x0, int y0,
                       int xsize, int ysize,
                       int BitsPerPixel, 
                       const uint8_t   * pData, 
		       int TextColor,
		       int BackColor)
{
	// TODO: NO PROTECTION, "BitsPerPixel" is unused
	uint32_t dwWidth = 0;
	uint32_t dwHeight = 0;
	uint32_t dwFrontColor = 0;
	uint32_t dwBackColor = 0;
	//GXOSD_DrawMode_t DrawMode; 
	//GXSPP_DrawMode_t SppDrawMode;
	dwWidth = xsize;
	dwHeight = ysize;
	//DrawMode = GXOSD_DRAWMODE_PASTE;//GXOSD_DRAWMODE_DRAW;//fixed ?
	//SppDrawMode=GXSPP_DRAWMODE_PASTE;
	dwFrontColor = TextColor;
	dwBackColor = BackColor;
	

	if(0==CcOpenFlag)
	{	
//		if((y0!=g_chTtxStartAddY)||(x0==g_chTtxStartAddX+39*12))
		if((y0==g_chTtxStartAddY)||((x0>=g_chTtxStartAddX+40*12-dwWidth)&&(dwWidth%8)))
		{
		
			ttx_api_osd_draw_text(
								x0, 
								y0, 
								dwWidth, 
								dwHeight, 
								pData, 
								dwFrontColor, 
								dwBackColor
								);
		}
		else
		{
			ttx_sib_osd_draw_text(
								x0, 
								y0, 
								dwWidth, 
								dwHeight, 
								pData, 
								dwFrontColor, 
								dwBackColor 
								);
		}
		
	}
	else
	{
		/*#ifdef SUBT_SHOW_ON_OSD
		ttx_sib_spp_expang_draw_text(g_RegionHandle, 
							x0, 
							y0, 
							dwWidth, 
							dwHeight, 
							pData, 
							dwFrontColor, 
							dwBackColor, 
							DrawMode);

		#else
		ttx_sib_spp_draw_text(g_SppHandle, 
							x0, 
							y0, 
							dwWidth, 
							dwHeight, 
							pData, 
							dwFrontColor, 
							dwBackColor, 
							DrawMode);
		#endif*/
	 ttx_spp_draw_text( x0, 
			              y0,
			              dwWidth,
			              dwHeight,
			              pData, 
			              dwFrontColor,
			              dwBackColor);



		
	}

}

