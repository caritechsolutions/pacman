/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_sub_engine.c
* Author    : 	zhangling
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include "include/si_sub_engine_private.h"
#include "sub_system/si_parse_lib/si_parse_lib.h"

#define SI_PAT_TID                           		0x00    /* Program Association Table */
#define SI_CAT_TID                           		0x01    /* Conditional Access Table */
#define SI_PMT_TID                           		0x02    /* Program Map Table */

/*两个pid一样，但是message id不一样
 * dsi message id 0x1006
 * dii message id 0x1002*/
#define SI_DSI_DII_TID                         		0x3b    /* dsi dii Table */

#define SI_DDB_TID                           		0x3c    /* ddb Table */
                                                 		    /* 0x04 to 0x3F are reserved*/
#define SI_NIT_ACTUAL_NETWORK_TID					0x40    /* Network Information Table - Actual Network*/
#define SI_NIT_OTHER_NETWORK_TID					0x41    /* Network Information Table - Other Network*/
#define SI_SDT_ACTUAL_TS_TID                 		0x42    /* Service Description Table - Actual Transport Stream*/
                                                 		    /* 0x43 to 0x45 are reserved for future use*/
#define SI_SDT_OTHER_TS_TID                  		0x46    /* Service Description Table - Other Transport Stream*/
                                                 	    	/* 0x47 to 0x49 are reserved for future use*/
#define SI_BAT_TID                           		0x4A    /* Bouquet Association Table */
#define SI_EIT_ACTUAL_PF_TID						0x4E
#define SI_EIT_OTHER_PF_TID						    0x4F
                                                     		/* 0x50 to 0x6F are used for Various EIT Table*/
#define SI_TDT_TID	                          		0x70    /* Time & Date Table */
#define SI_RST_TID	                          		0x71    /* Running Status Table */
#define SI_TOT_TID	                          		0x73    /* Time Offset Table */
#define SI_AIT_TID	                          		0x74    /* AIT Table */



#define SI_MGT_TID                                  0xc7
#define SI_TVCT_TID                                 0xc8
#define SI_CVCT_TID                                 0xc9
#define SI_RRT_TID                                  0xca
#define SI_ATSC_EIT_TID                             0xcb
#define SI_DVB_EIT_TID								0x12
#define SI_ETT_TID                                  0xcc
#define SI_STT_TID                                  0xcd
static uint32_t dvb_crc_finger[256] = 
{
    0x00000000, 0x04C11DB7, 0x9823B6E, 0xD4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 
    0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3,
    0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
    0x5BD4B01B, 0x569796C2, 0x52568B75,  0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
    0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,  0x9823B6E0, 0x9CE2AB57, 0x91A18D8E,
    0x95609039,0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,  0xBE2B5B58, 0xBAEA46EF,
    0xB7A96036,0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,  0xD4326D90,
    0xD0F37027,0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
    0xF23A8028,0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A,
    0xEC7DD02D,0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C,
    0x2E003DC5,0x2AC12072,  0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x18AEB13,
    0x54BF6A4,0x808D07D, 0xCC9CDCA,  0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB,
    0x6F52C06C, 0x6211E6B5, 0x66D0FB02,  0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
    0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,  0xACA5C697, 0xA864DB20, 0xA527FDF9,
    0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,  0x8AAD2B2F, 0x8E6C3698,
    0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,  0xE0B41DE7,
    0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,  
    0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED,
    0xD8FBA05A,  0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85,
    0x738AAD5C, 0x774BB0EB,  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
    0x58C1663D, 0x558240E4, 0x51435D53,  0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
    0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,  0x315D626, 0x7D4CB91, 0xA97ED48, 
    0xE56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,  0xF12F560E, 0xF5EE4BB9,
    0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,  0xD727BBB6,
    0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
    0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC,
    0xA379DD7B,  0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD,
    0x81B02D74, 0x857130C3,  0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645,
    0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,  0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 
    0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,  0x119B4BE9, 0x155A565E, 0x18197087,
    0x1CD86D30, 0x29F3D35, 0x65E2082, 0xB1D065B, 0xFDC1BEC,  0x3793A651, 0x3352BBE6,
    0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,  0xC5A92679,
    0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 
    0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673,
    0xFDE69BC4,  0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 
    0x933EB0BB, 0x97FFAD0C,  0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,0xBCB4666D, 
    0xB8757BDA, 0xB5365D03, 0xB1F740B4, 
};   

 
void callback_too_long_point(int32_t start_time,int32_t end_time)       
{                                                                       
    if(end_time - start_time >= 5)                                      
    {                                                                   
#ifdef GX_SI_ENGINE_ERR_DBUG                                            
        SI_ENGINE_ERR_PRINTF("callback func exec too long\n");          
#endif                                                                  
        //while(1);                                                       
    }                                                                   
    else if((end_time - start_time >1)&&                                
            (end_time - start_time < 5))                                
    {                                                                   
#ifdef GX_SI_ENGINE_ERR_DBUG                                            
        SI_ENGINE_PRINTF("callback func so long\n");                    
#endif                                                                  
    }                                                                   

}                                                                       


	
void * GxSubsystm_SiParseLibDsiDiiEntry(uint8_t * sec,uint32_t size)
{
    uint16_t message_id= 0;
    message_id = SI_TODATA0_15BIT(sec[10],sec[11]);
    if(message_id == 0x1006)
    {
        return (void*)GxSubsystm_SiParseLibDsiParse(sec,size);
    }
    else if(message_id == 0x1002)
    {
        return (void*)GxSubsystm_SiParseLibDiiParse(sec,size);
    }
    return NULL;
}

int32_t GxSubsystm_SiReleaseLibDsiDiiEntry(void * table) 
{
    uint16_t message_id = 0;
    int32_t ret = 0;
    message_id = SI_TODATA0_15BIT(((uint8_t*)table)[12],((uint8_t*)table)[13]);
    if(message_id == 0x1006)
    {
        ret = GxSubsystm_SiParseLibDsiRelease(table);
    }
    else if(message_id == 0x1002)
    {
        ret = GxSubsystm_SiParseLibDiiRelease(table);
    }
    return ret;
}

void * GxSubsystm_SiParseLibNITEntry(uint8_t * sec,uint32_t size,uint32_t flag)
{
    if(flag == GX_SEARCH_FRONT_DVB_DTMB)
    {
        return GxSubsystm_SiParseLibDtmbnitParse(sec,size);
    }
    else
    {
        return GxSubsystm_SiParseLibTnitParse(sec,size);
    }
}

int32_t GxSubsystm_SiReleaseLibNITEntry(void * table,uint32_t flag) 
{
    int32_t ret = 0;
    if(flag == GX_SEARCH_FRONT_DVB_DTMB)
    {
        ret = GxSubsystm_SiParseLibDtmbnitRelease(table);
    }
    else
    {
        ret = GxSubsystm_SiParseLibTnitRelease(table);
    }
    return ret;
}

GxSubsystmSiEngineParseFuncTable DVB_Parse[] = {
    {SI_DDB_TID,(void*)GxSubsystm_SiParseLibDdbParse,GxSubsystm_SiParseLibDdbRelease,NULL,NULL},
    {SI_AIT_TID,(void*)GxSubsystm_SiParseLibAitParse,GxSubsystm_SiParseLibAitRelease,NULL,NULL},
    {SI_TOT_TID,(void*)GxSubsystm_SiParseLibTotParse,GxSubsystm_SiParseLibTotRelease,NULL,NULL},
    {SI_TDT_TID,(void*)GxSubsystm_SiParseLibTdtParse,GxSubsystm_SiParseLibTdtRelease,NULL,NULL},
    //{SI_EIT_OTHER_PF_TID,(void*)GxSubsystm_SiParseLibEitParse,GxSubsystm_SiParseLibEitRelease},
    //{SI_RST_TID,(void*)GxSubsystm_SiParseLibRstParse,GxSubsystm_SiParseLibRstRelease},
    //{SI_EIT_ACTUAL_PF_TID,(void*)GxSubsystm_SiParseLibEitParse,GxSubsystm_SiParseLibEitRelease},
    //{SI_BAT_TID,(void*)GxSubsystm_SiParseLibBatParse,GxSubsystm_SiParseLibBatRelease},
    {SI_SDT_OTHER_TS_TID,(void*)GxSubsystm_SiParseLibSdtParse,GxSubsystm_SiParseLibSdtRelease,NULL,NULL},
    {SI_SDT_ACTUAL_TS_TID,(void*)GxSubsystm_SiParseLibSdtParse,GxSubsystm_SiParseLibSdtRelease,NULL,NULL},
    {SI_NIT_OTHER_NETWORK_TID,NULL,NULL,GxSubsystm_SiParseLibNITEntry,GxSubsystm_SiReleaseLibNITEntry},
    {SI_NIT_ACTUAL_NETWORK_TID,NULL,NULL,GxSubsystm_SiParseLibNITEntry,GxSubsystm_SiReleaseLibNITEntry},
    {SI_PMT_TID,(void*)GxSubsystm_SiParseLibPmtParse,GxSubsystm_SiParseLibPmtRelease,NULL,NULL},
    //{SI_CAT_TID,(void*)GxSubsystm_SiParseLibCatParse,GxSubsystm_SiParseLibCatRelease},
    {0,(void*)GxSubsystm_SiParseLibPatParse,GxSubsystm_SiParseLibPatRelease,NULL,NULL},
    {SI_DSI_DII_TID,GxSubsystm_SiParseLibDsiDiiEntry,GxSubsystm_SiReleaseLibDsiDiiEntry,NULL,NULL},
    {SI_DVB_EIT_TID,(void*)GxSubsystm_DvbSiParseLibEitParse,GxSubsystm_DvbSiParseLibEitRelease,NULL,NULL},       
    {-1,NULL,NULL,NULL,NULL},
};

GxSubsystmSiEngineParseFuncTable ATSC_Parse[] = {
    {SI_MGT_TID,(void*)GxSubsystm_SiParseLibMgtParse,GxSubsystm_SiParseLibMgtRelease,NULL,NULL},
    {SI_ATSC_EIT_TID,(void*)GxSubsystm_AtscSiParseLibEitParse,GxSubsystm_AtscSiParseLibEitRelease,NULL,NULL},       
    {SI_TVCT_TID,(void*)GxSubsystm_SiParseLibTvctParse,GxSubsystm_SiParseLibTvctRelease,NULL,NULL},
    {SI_CVCT_TID,(void *)GxSubsystm_SiParseLibCvctParse,GxSubsystm_SiParseLibCvctRelease,NULL,NULL},
    {SI_ETT_TID,(void*)GxSubsystm_SiParseLibEttParse,GxSubsystm_SiParseLibEttRelease,NULL,NULL},
    {SI_STT_TID,(void *)GxSubsystm_SiParseLibSttParse,GxSubsystm_SiParseLibSttRelease,NULL,NULL},
    {SI_RRT_TID,(void*)GxSubsystm_SiParseLibRrtParse,GxSubsystm_SiParseLibRrtRelease,NULL,NULL}, 
    {SI_PMT_TID,(void*)GxSubsystm_SiParseLibPmtParse,GxSubsystm_SiParseLibPmtRelease,NULL,NULL},
    {SI_PAT_TID,(void*)GxSubsystm_SiParseLibPatParse,GxSubsystm_SiParseLibPatRelease,NULL,NULL},       
    {-1,NULL,NULL,NULL,NULL},
};

static uint32_t si_sub_crc32(uint8_t* pBuffer, uint32_t nSize)
{
    uint32_t nResult = 0xFFFFFFFF;

    while (nSize--)
        nResult = (nResult << 8) ^ dvb_crc_finger[(nResult >> 24) ^ *pBuffer++];

    return nResult;
}
int32_t si_sub_crc32_check(uint8_t *pSection)
{
	uint16_t nlength = 0;
	uint32_t nCrc32 = 0;
	uint32_t nCrc32Result = 0;
	uint8_t *pdata = NULL;
	uint8_t *pndata = NULL;
	uint8_t	chTableId = 0;
	uint8_t	chSectionNumber          = 0;
	//uint8_t	chLastSectionNumber     = 0;

	pdata					= pSection;
	chTableId				= pdata[0];
	nlength					= (uint16_t)((pdata[1]&0x0F)<<8)|pdata[2];
    chSectionNumber			= pdata[6];
	//chLastSectionNumber		= pdata[7];


	 /*TDT表没CRC校验*/
	if(0x70 == chTableId)
		return 0;
	else if ((chTableId >= 0x80) && (chTableId <= 0xFE)){
		if(((pdata[1])&0x80) == 0)
			return 0;
	}

	if(nlength<1)
		return 1;
	nlength = nlength + 3 - 4;
	nCrc32Result = si_sub_crc32(pdata, nlength);

	pndata = pdata + nlength;
	nCrc32 = ((pndata[0]<<24)&0xff000000) | ((pndata[1]<<16)&0x00ff0000)
		         | ((pndata[2]<<8)&0x0000ff00) | ((pndata[3]<<0)&0x000000ff);

	if(nCrc32Result == nCrc32)
		return  0;
	else{
		return  1;
	}
}

void* si_sub_parse_section_standard(GxSubsystemSiEnginDmx * dmx,uint8_t* sec, uint32_t size)
{
	GxSubsystemSiEnginDmx * dmx_temp = NULL;
	void * p = NULL;
	int32_t i = 0,tid = 0;

	dmx_temp = dmx;
	tid = sec[0];
	if(dmx_temp->parse_arry == DVB_Parse)
	{
		//在DVB协议中这个范围都属于eit表
		if((tid >= 0x50 && tid <= 0x6f) || (tid == 0x4e) || (tid == 0x4f))
			tid = 0x12;
	}

	while(1)
	{
		if(dmx_temp->parse_arry[i].tid == -1)//没有需要的解析库
		{
#ifdef GX_SI_ENGINE_ERR_DBUG                              
			SI_ENGINE_ERR_PRINTF("si parse can not found right func!!\n");  
#endif                                                    
			return NULL;
		}
		if(tid == dmx_temp->parse_arry[i].tid)
		{
			if(dmx_temp->parse_arry[i].parsefunc != NULL)
			{
				p = dmx_temp->parse_arry[i].parsefunc(sec,size);
			}
			else if(dmx_temp->parse_arry[i].p_parsefunc != NULL)
			{
				p = dmx_temp->parse_arry[i].p_parsefunc(sec,size,dmx_temp->front_type);
			}
			else
			{
#ifdef GX_SI_ENGINE_ERR_DBUG                              
				SI_ENGINE_ERR_PRINTF("si parse can not found right func!!\n");  
#endif                                                    
				return NULL;
			}
			break;
		}
		i++;
	}
	return p;
}


void si_sub_release_section(GxSubsystemSiEnginDmx * dmx,void * table)
{
	GxSubsystemSiEnginDmx * dmx_temp = NULL;
	int32_t i = 0,tid = 0;

	dmx_temp = dmx;
	tid = ((uint8_t *)table)[0];
	if(dmx_temp->parse_arry == DVB_Parse)
	{
		//在DVB协议中这个范围都属于eit表
		if((tid >= 0x50 && tid <= 0x6f) || (tid == 0x4e) || (tid == 0x4f))
			tid = 0x12;
	}


	while(1)
	{
		if(dmx_temp->parse_arry[i].tid == -1)//没有需要的解析库
		{
#ifdef GX_SI_ENGINE_ERR_DBUG                              
			SI_ENGINE_ERR_PRINTF("si release can not found right func!!\n");  
#endif                                                    
			return;
		}
		if(tid == dmx_temp->parse_arry[i].tid)
		{
			if(dmx_temp->parse_arry[i].releasefunc != NULL)
			{
				dmx_temp->parse_arry[i].releasefunc(table);
			}
			else if(dmx_temp->parse_arry[i].p_releasefunc != NULL)
			{
				dmx_temp->parse_arry[i].p_releasefunc(table,dmx_temp->front_type);
			}
			else
			{
#ifdef GX_SI_ENGINE_ERR_DBUG                              
				SI_ENGINE_ERR_PRINTF("si parse can not found right func!!\n");  
#endif                                                    
				return;
			}
			break;
		}
		i++;
	}
	return ;
}

GxSubsystmSiEngineParseFuncTable* si_sub_get_parser_ops(int flag)
{
	if(flag == 0)//DVB 标准
	{
		return DVB_Parse;
	}
	else//ATSC
	{
		return ATSC_Parse;
	}
}

/* End of file -------------------------------------------------------------*/

