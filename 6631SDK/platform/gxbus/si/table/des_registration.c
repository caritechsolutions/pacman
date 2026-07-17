/*****************************************************************************
 * 						   CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	des_ac3.c
 * Author    : 	shenbin
 * Project   :	GoXceed
 * Type      :
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0  	2009.10.10	      shenbin	         creation
 *****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxcore.h"
#include "gxtype.h"
#include "module/si/si_public.h"
#include "module/si/si_pmt.h"

int32_t des_pmt_registration(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	uint8_t* d = p_section_data + 2;
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);

	GxSiDes_Trace();

	memset(pmt_table->registration, 0, 4);

	if(NULL == p_section_data || len < 4)
	{
		return GXCORE_ERROR;
	}

	memcpy(pmt_table->registration, d, 4);

	return GXCORE_SUCCESS;
}

