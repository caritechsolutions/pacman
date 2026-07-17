#include "gx_mem.h"
#include "com_subt.h"
#include "com_sub_def.h"

static u8 s_chPageTimeOut = 5; //s
extern struct com_subtitle com_sub;
extern int timer_clear_screen(void *userdata);
u8* g_pSubtitlePesBufferPtr;         // 存储流中的原始数据
u8* g_pSubtitleCompositionBufferPtr; // 存储构图页的buffer指针
u8* g_pSubtitlePageBufferPtr;        // 存储解析出的page页指针
u8* g_pSubtitleObjectBufferPtr;      // 存储被OBJECT填充过的REGION的所有象素点
u8* g_pStubtitlePixelBufferPtr;      // PIC层数据数组的开始位置
u8  g_chStuffingByteLen;
u8  g_chSubtitleDecState;
u16 g_wRegionUseLen[256];            // REGION实际占用的长度
u16 g_wClutUseLen[256];              // CLUT实际占用的长度
struct DVBSubtitleWindow g_display_def;
#if (YCbCrCount==13)
extern u16 YCbCr2RGB[8*1024];
#endif
#if (YCbCrCount==14)
extern u16 YCbCr2RGB[16*1024];
#endif
#if(YCbCrCount==15)
extern u16 YCbCr2RGB[32*1024];
#endif
#if(YCbCrCount==16)
extern u16 YCbCr2RGB[64*1024];
#endif

#define subt_mem_copy(s, t, l) memcpy(t, s, l)

/*****************************************************************************
 * Function    : subt_mem_move
 * Description : offset the memory
 * Arguments   : pStratPtr,pEndPtr,wLength
 * Returns     : error
 * Other       :
 ****************************************************************************/
AppErr_t subt_mem_move(u8 *pStratPtr, u8 *pEndPtr,u16 wLength)
{

	while(pStratPtr< pEndPtr) {
		*(pEndPtr + wLength - 1) = *(pEndPtr - 1) ;
		pEndPtr--;
	}
	return APP_NO_ERROR;
}

/*****************************************************************************
 * Function    : subt_mem_move_front
 * Description : offset the memory
 * Arguments   : pStratPtr,pEndPtr,wLength
 * Returns     : error
 * Other       :
 ****************************************************************************/
AppErr_t subt_mem_move_front(u8 *pStratPtr, u8 *pEndPtr,u16 wLength)
{
	while(pStratPtr< pEndPtr) {
		*(pStratPtr- wLength) = *pStratPtr ;
		pStratPtr--;
	}
	return APP_NO_ERROR;
}

/*****************************************************************************
 * Function    : subt_pic_layer_enable
 * Description : enable the pic layer
 * Arguments   : void
 * Returns     : error
 * Other       :
 ****************************************************************************/
AppErr_t subt_pic_layer_enable(void)
{
	com_sub.render->ops->show(com_sub.render->handle);
	return APP_NO_ERROR;
}

/*****************************************************************************
 * Function    : subt_pic_layer_disable
 * Description : disable the pic layer
 * Arguments   : void
 * Returns     : error
 * Other       :
 ****************************************************************************/
AppErr_t subt_pic_layer_disable(void)
{
	com_sub.render->ops->hide(com_sub.render->handle);
	return APP_NO_ERROR;
}

#define subt_memset(s, c, count) memset(s, c, count)

/*****************************************************************************
 * Function    : subt_initial
 * Description : init the subtitling decode,setup all kinds of buffer of decode
 * Arguments   : void
 * Returns     : error
 * Other       :
 ****************************************************************************/
AppErr_t subt_initial(void)
{
	g_SppBuffer = com_sub.render->ops->get_buf(com_sub.render->handle);
	g_pStubtitlePixelBufferPtr      = com_sub.PixelBufferPtr;//临时存放整个需要显示的REGION的象素点
	g_pSubtitleObjectBufferPtr      = com_sub.ObjectBufferPtr;//流中OBJECT段等数据的缓存
	g_pSubtitleCompositionBufferPtr = com_sub.CompositionBufferPtr;//流中PAGE REGION CLUT段的缓存
	g_pSubtitlePageBufferPtr        = com_sub.PageBufferPtr;
	g_chSubtitleDecState            = ST_WAIT_MODE_CHANGE;
	memset(g_wClutUseLen,   0, sizeof(g_wClutUseLen));
	memset(g_wRegionUseLen, 0, sizeof(g_wRegionUseLen));
	memset(g_wClutTable,    0, sizeof(g_wClutTable));
	memset(&g_display_def,  0, sizeof(DVBSubtitleWindow));
	g_display_def.version = 0xffffffff;//-1
	return APP_NO_ERROR;
}

/*****************************************************************************
 * Function    : subt_buf_reset
 * Description : reset the buffer pointer,and reset the state of decode
 * Arguments   : void
 * Returns     : error
 * Other       :
 ****************************************************************************/
void subt_buf_reset(void)
{
	g_pSubtitleObjectBufferPtr      = com_sub.ObjectBufferPtr;//流中OBJECT段等数据的缓存
	g_pSubtitleCompositionBufferPtr = com_sub.CompositionBufferPtr;//流中PAGE REGION CLUT段的缓存
	g_pSubtitlePageBufferPtr        = com_sub.PageBufferPtr;
	g_chSubtitleDecState            = ST_WAIT_MODE_CHANGE;
}

/*****************************************************************************
 * Function    : subt_send_to_buffer
 * Description : send the decode data to disp buffer
 * Arguments   : wPackLen
 * Returns     : error
 * Other       :
 ****************************************************************************/
AppErr_t subt_send_to_buffer(u16 wPackLen )//解析
{
	u8 chDataType,chTempData1;
	u8 *pPesDataPtr,*pSubtitlingSegmentTargetPtr,*pSubtitlingSegmentPtr,*pSegmentDataFieldPtr,*pSegmentDataFieldTargetPtr;
	u16 wTempdata2,wTempdata3,wSegmentLength;
	static u8 s_chObjectParse,chCleanFlag;
	u32 dds_version = 0;
	u32 window_hor_min,window_hor_max,window_ver_min,window_ver_max = 0;
	GxSubtitle_RenderRect   rect = {0, 0, 0, 0};

	pPesDataPtr = g_pSubtitlePesBufferPtr+9+g_chStuffingByteLen;
	wPackLen=wPackLen-3-g_chStuffingByteLen-2;
	if (PES_data_field__data_identifier(pPesDataPtr) == 0x20 && PES_data_field__subtitle_stream_id(pPesDataPtr) == 0x00 )
	{
		pSubtitlingSegmentPtr = PES_data_field__subtitling_segment_ptr(pPesDataPtr);
		while(wPackLen>5)
		{
            //COMMON_PRINTF("\nsubt packet len is %d\n",wPackLen);
			if (subtital_segment__sync_byte(pSubtitlingSegmentPtr) == 0xff)
			{
				return APP_NO_ERROR;
			}
			if (subtital_segment__sync_byte(pSubtitlingSegmentPtr) != 0x0f)
			{
				COMMON_PRINTF("\n[subtitle] sync_byte is lost!!!\n");
                wPackLen = 0;
                break;
			}
			wTempdata2 = subtital_segment__page_id_h(pSubtitlingSegmentPtr);
			wTempdata3 = subtital_segment__page_id_l(pSubtitlingSegmentPtr);
			wTempdata3 = wTempdata3 + (wTempdata2 << 8);
			if ((wTempdata3 == g_wSubtitleCompositionPageId)/*||(wTempdata3 == g_wSubtitleAncillaryPageId)*/)
			{
				wTempdata2 = subtital_segment__segment_length_h(pSubtitlingSegmentPtr);
				wTempdata3 = subtital_segment__segment_length_l(pSubtitlingSegmentPtr);
				wTempdata3 = wTempdata3 + (wTempdata2 << 8);
				wSegmentLength = wTempdata3;
				pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
				chDataType = subtital_segment__segment_type(pSubtitlingSegmentPtr);
				switch (chDataType)
				{
					case DT_DISPLAY_DEFINITION_SEGMENT://EN 300 743 V1.3.1
						if(wSegmentLength < 5)
						{
							COMMON_PRINTF("Display Definition Segment not enough! Length: %d\n",wSegmentLength);
							return APP_NO_ERROR;
						}

						dds_version = display_definition_segment__dds_version_number(pSegmentDataFieldPtr);
						if (g_display_def.version == dds_version)
						{
							COMMON_PRINTF("already have this display definition version! \n");
							break;
						}
						else
						{
                            gxlogd("\n++++++++++++++++<<Find DDS!!!>>++++++++++++++++");
							gxlogd("New display definition version! \n");
							COMMON_PRINTF("=====>> Old version: %d  Version: %d",g_display_def.version,dds_version);
							g_display_def.version = dds_version;
						}

						g_display_def.window_flag = display_definition_segment__display_window_flag(pSegmentDataFieldPtr);
						wTempdata2 = display_definition_segment__display_width_h(pSegmentDataFieldPtr);
						wTempdata3 = display_definition_segment__display_width_l(pSegmentDataFieldPtr);
						g_display_def.display_width = wTempdata3 + (wTempdata2<<8) + 1;

						wTempdata2 = display_definition_segment__display_height_h(pSegmentDataFieldPtr);
						wTempdata3 = display_definition_segment__display_height_l(pSegmentDataFieldPtr);
						g_display_def.display_height = wTempdata3 + (wTempdata2<<8) + 1;
						COMMON_PRINTF("\n[DDS]Display Flag: %d Height: %d Width: %d\n",\
								g_display_def.window_flag,g_display_def.display_height,g_display_def.display_width);

						rect.x = 0;
						rect.y = 0;
						rect.width = g_display_def.display_width;
						rect.height = g_display_def.display_height;

						if((wSegmentLength >= 13) && g_display_def.window_flag)
						{
							wTempdata2 = display_definition_segment__display_window_horizontal_min_h(pSegmentDataFieldPtr);
							wTempdata3 = display_definition_segment__display_window_horizontal_min_l(pSegmentDataFieldPtr);
							window_hor_min = wTempdata3 + (wTempdata2<<8);

							wTempdata2 = display_definition_segment__display_window_horizontal_max_h(pSegmentDataFieldPtr);
							wTempdata3 = display_definition_segment__display_window_horizontal_max_l(pSegmentDataFieldPtr);
							window_hor_max = wTempdata3 + (wTempdata2<<8);

							wTempdata2 = display_definition_segment__display_window_vertical_min_h(pSegmentDataFieldPtr);
							wTempdata3 = display_definition_segment__display_window_vertical_min_l(pSegmentDataFieldPtr);
							window_ver_min = wTempdata3 + (wTempdata2<<8);

							wTempdata2 = display_definition_segment__display_window_vertical_max_h(pSegmentDataFieldPtr);
							wTempdata3 = display_definition_segment__display_window_vertical_max_l(pSegmentDataFieldPtr);
							window_ver_max = wTempdata3 + (wTempdata2<<8);

							g_display_def.window_x = window_hor_min;
							g_display_def.window_y = window_ver_min;
							g_display_def.window_width = window_hor_max - window_hor_min + 1;
							g_display_def.window_height = window_ver_max - window_ver_min + 1;

							gxlogd("[DDS Information!]=====>> Hor Min: %d Hor Max: %d Ver Min: %d Ver Max: %d     \
									[Window]X: %d Y: %d Width: %d Height: %d",\
									window_hor_min,window_hor_max,window_ver_min,window_ver_max,\
									g_display_def.window_x,g_display_def.window_y,\
									g_display_def.window_width,g_display_def.window_height);

							rect.x = g_display_def.window_x;
							rect.y = g_display_def.window_y;
							rect.width = g_display_def.window_width;
							rect.height = g_display_def.window_height;
						}

						if((((com_sub.render->ops->get_rect(com_sub.render->handle))->x) != rect.x)
								|| (((com_sub.render->ops->get_rect(com_sub.render->handle))->y) != rect.y)
								|| (((com_sub.render->ops->get_rect(com_sub.render->handle))->width) != rect.width)
								|| (((com_sub.render->ops->get_rect(com_sub.render->handle))->height) != rect.height))
						{
							if ((((com_sub.render->ops->get_rect_max(com_sub.render->handle))->width) < rect.width)
									|| (((com_sub.render->ops->get_rect_max(com_sub.render->handle))->height) < rect.height)) {
								gxlogd("\n!!!===The rect (width:%d height:%d) is greater than supported!!===!!!\n", rect.width, rect.height);
								return 1;
							}

							COMMON_PRINTF("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!===>> Change Rect!!!!!\n");
							COMMON_PRINTF("==== rect x: %d y: %d width: %d height : %d",rect.x,rect.y,rect.width,rect.height);
							com_sub.render->ops->change_buf(com_sub.render->handle, &rect);
							subt_initial();
						}

						break;
					case DT_PAGE_COMPOSITION_SEGMENT:
                        if(1==s_chObjectParse)
                        {
                            s_chObjectParse = 0;
                            subt_wait_PTS();
                            if(chCleanFlag)
                            {
#ifdef SUBT_DEBUG_DETAIL
                                COMMON_PRINTF("\033[31m Clear screen %d !! \033[0m\n", __LINE__);
#endif
                                timer_clear_screen(NULL);
                                chCleanFlag=0;
                            }
                            subt_dec();
                        }
                        g_chSubtitleDecState = ST_MAIN_PROCESS;

						//copy  segment data to the buffer g_pSubtitlePageBufferPtr for decode in the function subt_dec()
						if(wSegmentLength + 6>PAGE_BUF_MAX_LEN)
						{
							COMMON_PRINTF("PAGE_BUF not enough! \n");
							return APP_NO_ERROR;
						}
						chTempData1=page_composition_segment__page_state(pSegmentDataFieldPtr);
						switch(g_chSubtitleDecState)
						{
							case ST_WAIT_MODE_CHANGE:
								if(chTempData1== PG_MODE_CHANGE)
								{
									chCleanFlag=1;
									subt_buf_reset();
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitlePageBufferPtr,wSegmentLength + 6);
									g_chSubtitleDecState = ST_SUBTITLE_MODE_CHANGE_PROCESS;
									g_pSubtitlePageBufferPtr = g_pSubtitlePageBufferPtr +wSegmentLength + 6;
								}
								if(chTempData1 == PG_ACQUISITION_POINT)
								{
									chCleanFlag=1;
									subt_buf_reset();
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitlePageBufferPtr,wSegmentLength + 6);
									g_chSubtitleDecState = ST_SUBTITLE_ACQUISITION_POINT_PROCESS;
									g_pSubtitlePageBufferPtr = g_pSubtitlePageBufferPtr +wSegmentLength + 6;
								}
								break;
							case ST_MAIN_PROCESS:
							case ST_SUBTITLE_MODE_CHANGE_PROCESS:
							case ST_SUBTITLE_ACQUISITION_POINT_PROCESS:
							case ST_SUBTITLE_NORML_CASE_PROCESS:
								if(chTempData1 == PG_MODE_CHANGE)
								{
									chCleanFlag=1;
									subt_buf_reset();
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitlePageBufferPtr,wSegmentLength + 6);
									g_chSubtitleDecState = ST_SUBTITLE_MODE_CHANGE_PROCESS;
									g_pSubtitlePageBufferPtr = g_pSubtitlePageBufferPtr +wSegmentLength + 6;
								}

								if(chTempData1 == PG_ACQUISITION_POINT)
								{
                                    subt_wait_PTS();
#ifdef SUBT_DEBUG_DETAIL
                                    COMMON_PRINTF("\033[31m Clear screen %d !! \033[0m\n", __LINE__);
#endif
                                    timer_clear_screen(NULL);
									chCleanFlag=1;
									subt_buf_reset();
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitlePageBufferPtr,wSegmentLength + 6);
									g_chSubtitleDecState = ST_SUBTITLE_ACQUISITION_POINT_PROCESS;
									g_pSubtitlePageBufferPtr = g_pSubtitlePageBufferPtr +wSegmentLength + 6;
								}

								if(chTempData1 == PG_NORMAL_CASE_PAGE_UPDATE)
								{
									g_pSubtitlePageBufferPtr = com_sub.PageBufferPtr;
									if(wSegmentLength>2)
										subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitlePageBufferPtr,wSegmentLength + 6);
									g_pSubtitlePageBufferPtr = g_pSubtitlePageBufferPtr +wSegmentLength + 6;
									g_chSubtitleDecState = ST_SUBTITLE_NORML_CASE_PROCESS;
								}
								break;

							default:
								g_chSubtitleDecState = ST_WAIT_MODE_CHANGE;
								break;
						}
						break;
					case DT_REGION_COMPOSITION_SEGMENT:
						//copy Region segment data to the buffer g_pSubtitleCompositionBufferPtr for decode in the function subt_creat_region
						if(g_pSubtitleCompositionBufferPtr+wSegmentLength+6>=com_sub.CompositionBufferPtr +COMPOSITION_BUF_MAX_LEN)
						{
							COMMON_PRINTF("composition_buf not enough! \n");
							subt_buf_reset();
							return APP_NO_ERROR;
						}
						switch(g_chSubtitleDecState)
						{
							case ST_WAIT_MODE_CHANGE:
								//丢弃数据
								break;
							case ST_MAIN_PROCESS:
								//丢弃数据
								break;
							case ST_SUBTITLE_ACQUISITION_POINT_PROCESS:
							case ST_SUBTITLE_MODE_CHANGE_PROCESS:
							case ST_SUBTITLE_NORML_CASE_PROCESS:

								wTempdata2=region_compostion_segment__region_id(pSegmentDataFieldPtr);
								pSubtitlingSegmentTargetPtr=subt_get_region_ptr(wTempdata2);
								//NULL:Don't have this segment data in the buffer g_pSubtitleCompositionBufferPtr
								if (pSubtitlingSegmentTargetPtr !=NULL)
								{
									wTempdata3=g_wRegionUseLen[wTempdata2];
									if (wSegmentLength <= wTempdata3)
									{
										pSegmentDataFieldTargetPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentTargetPtr);
										if (region_compostion_segment__region_version_number(pSegmentDataFieldTargetPtr) != region_compostion_segment__region_version_number(pSegmentDataFieldPtr))
										{
											subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);
											memset((pSubtitlingSegmentTargetPtr+wSegmentLength + 6),0,(wTempdata3-wSegmentLength));

										}
									}
									else
									{
										subt_mem_move(pSubtitlingSegmentTargetPtr+wTempdata3+6,g_pSubtitleCompositionBufferPtr,wSegmentLength-wTempdata3);
										g_pSubtitleCompositionBufferPtr+=(wSegmentLength-wTempdata3);
										g_wRegionUseLen[wTempdata2]=wSegmentLength;
										subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);

									}
								}
								else
								{
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitleCompositionBufferPtr,wSegmentLength + 6);
									g_pSubtitleCompositionBufferPtr = g_pSubtitleCompositionBufferPtr +wSegmentLength + 6;
									g_wRegionUseLen[wTempdata2]=wSegmentLength;
								}
								break;
							default:
								g_chSubtitleDecState = ST_WAIT_MODE_CHANGE;
								break;
						}

						break;
					case DT_CLUT_DEFINITION_SEGMENT:
						//copy CLUT segment data to the buffer g_pSubtitleCompositionBufferPtr for decode in the function subt_creat_CLUT
						if(g_pSubtitleCompositionBufferPtr+wSegmentLength+6>=com_sub.CompositionBufferPtr +COMPOSITION_BUF_MAX_LEN)
						{
							COMMON_PRINTF("composition_buf not enough! \n");
							subt_buf_reset();
							return APP_NO_ERROR;
						}
						switch(g_chSubtitleDecState)
						{
							case ST_WAIT_MODE_CHANGE:
								break;
							case ST_MAIN_PROCESS:
								break;
							case ST_SUBTITLE_ACQUISITION_POINT_PROCESS:
							case ST_SUBTITLE_MODE_CHANGE_PROCESS:
							case ST_SUBTITLE_NORML_CASE_PROCESS:
								wTempdata2=CLUT_definition_segment__CLUT_id(pSegmentDataFieldPtr);
								pSubtitlingSegmentTargetPtr=subt_get_CLUT_ptr(wTempdata2);
								if (pSubtitlingSegmentTargetPtr !=NULL)
								{
									wTempdata3=g_wClutUseLen[wTempdata2];
									if (wSegmentLength <= wTempdata3)
									{
										pSegmentDataFieldTargetPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentTargetPtr);
										if (CLUT_definition_segment__CLUT_version_number(pSegmentDataFieldTargetPtr) != CLUT_definition_segment__CLUT_version_number(pSegmentDataFieldPtr))
										{
											subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);
											memset((pSubtitlingSegmentTargetPtr+wSegmentLength + 6),0,(wTempdata3-wSegmentLength));
										}
									}
									else
									{
										subt_mem_move(pSubtitlingSegmentTargetPtr+wTempdata3+6,g_pSubtitleCompositionBufferPtr,wSegmentLength-wTempdata3);
										g_pSubtitleCompositionBufferPtr+=(wSegmentLength-wTempdata3);
										g_wClutUseLen[wTempdata2]=wSegmentLength;
										subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);

									}
								}
								else
								{
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitleCompositionBufferPtr,wSegmentLength + 6);
									g_pSubtitleCompositionBufferPtr = g_pSubtitleCompositionBufferPtr +wSegmentLength+ 6;
									g_wClutUseLen[wTempdata2]=wSegmentLength;
								}
								break;

							default:
								g_chSubtitleDecState = ST_WAIT_MODE_CHANGE;
								break;
						}
						break;
					case DT_OBJECT_DATA_SEGMENT:
						//copy Object segment data to the buffer g_pSubtitleObjectBufferPtr for decode in the function subt_creat_object_data
						if(g_pSubtitleObjectBufferPtr+wSegmentLength+6>=com_sub.ObjectBufferPtr + OBJECT_BUF_MAX_LEN)
						{
							COMMON_PRINTF("object_buf  not enough! \n");
							subt_buf_reset();
							return APP_NO_ERROR;
						}
						switch(g_chSubtitleDecState)
						{
							case ST_WAIT_MODE_CHANGE:
								break;
							case ST_MAIN_PROCESS:
								break;
							case ST_SUBTITLE_MODE_CHANGE_PROCESS:
							case ST_SUBTITLE_ACQUISITION_POINT_PROCESS:
							case ST_SUBTITLE_NORML_CASE_PROCESS:
								s_chObjectParse = 1;
								wTempdata2 = object_data_segment__object_id_h(pSegmentDataFieldPtr);
								wTempdata3 = object_data_segment__object_id_l(pSegmentDataFieldPtr);
								wTempdata3 = wTempdata3 + (wTempdata2<<8);
								pSubtitlingSegmentTargetPtr=subt_get_object_ptr(wTempdata3);
								if (pSubtitlingSegmentTargetPtr !=NULL)
								{
									wTempdata2 = subtital_segment__segment_length_h(pSubtitlingSegmentTargetPtr);
									wTempdata3 = subtital_segment__segment_length_l(pSubtitlingSegmentTargetPtr) + (wTempdata2<<8);
									if (wSegmentLength == wTempdata3)
									{
										pSegmentDataFieldTargetPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentTargetPtr);
										//if (object_data_segment__object_version_number(pSegmentDataFieldTargetPtr) != object_data_segment__object_version_number(pSegmentDataFieldTargetPtr))
										if (object_data_segment__object_version_number(pSegmentDataFieldTargetPtr) != object_data_segment__object_version_number(pSegmentDataFieldPtr))
										{
											//subt_mem_copy(pSubtitlingSegmentPtr, pSegmentDataFieldTargetPtr,wSegmentLength + 6);
											subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);

										}
									}

									//------------------------------------------------add by hulj 081017-----------------------------------

									else if(wSegmentLength > wTempdata3)
									{
										subt_mem_move(pSubtitlingSegmentTargetPtr+wTempdata3+6,g_pSubtitleObjectBufferPtr,wSegmentLength-wTempdata3);
										subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);
										g_pSubtitleObjectBufferPtr+=(wSegmentLength-wTempdata3);
									}
									else
									{
										//subt_mem_move_front(pSubtitlingSegmentTargetPtr+wTempdata3+6,g_pSubtitleObjectBufferPtr,wSegmentLength-wTempdata3);
										subt_mem_move_front(pSubtitlingSegmentTargetPtr+wTempdata3+6,g_pSubtitleObjectBufferPtr,wTempdata3-wSegmentLength);
										subt_mem_copy(pSubtitlingSegmentPtr, pSubtitlingSegmentTargetPtr,wSegmentLength + 6);
										//g_pSubtitleObjectBufferPtr-=(wSegmentLength-wTempdata3);
										g_pSubtitleObjectBufferPtr-=(wTempdata3-wSegmentLength);
									}


									//----------------------------------------------------------------------------------------------

								}
								else
								{
									subt_mem_copy(pSubtitlingSegmentPtr, g_pSubtitleObjectBufferPtr,wSegmentLength + 6);
									g_pSubtitleObjectBufferPtr = g_pSubtitleObjectBufferPtr +wSegmentLength + 6;
								}
								break;
							default:
								g_chSubtitleDecState = ST_WAIT_MODE_CHANGE;
								break;
						}
						break;
					case DT_END_OF_THE_DISPLAY_SEGMENT:
						switch(g_chSubtitleDecState)
						{
							case ST_WAIT_MODE_CHANGE:
								break;
							case ST_MAIN_PROCESS:
								break;
							case ST_SUBTITLE_NORML_CASE_PROCESS:
								if(1==s_chObjectParse)
								{
                                    s_chObjectParse = 0;
                                    subt_wait_PTS();
                                    if(chCleanFlag)
                                    {
#ifdef SUBT_DEBUG_DETAIL
                                        COMMON_PRINTF("\033[31m Clear screen %d !! \033[0m\n", __LINE__);
#endif
                                        timer_clear_screen(NULL);
                                        chCleanFlag=0;
                                    }
                                    subt_dec();
								}
                                else
                                {
									subt_wait_PTS();
#ifdef SUBT_DEBUG_DETAIL
                                    COMMON_PRINTF("\033[31m Clear screen %d !! \033[0m\n", __LINE__);
#endif
                                    timer_clear_screen(NULL);
                                }
								g_chSubtitleDecState = ST_MAIN_PROCESS;
								break;
							case ST_SUBTITLE_MODE_CHANGE_PROCESS:
							case ST_SUBTITLE_ACQUISITION_POINT_PROCESS:
								if(1==s_chObjectParse)
								{
									s_chObjectParse = 0;
                                    subt_wait_PTS();
#ifdef SUBT_DEBUG_DETAIL
                                    COMMON_PRINTF("\033[31m Clear screen %d !! \033[0m\n", __LINE__);
#endif
                                    timer_clear_screen(NULL);
                                    chCleanFlag=0;
									subt_dec();
								}
								g_chSubtitleDecState = ST_MAIN_PROCESS;
								break;

							default:
								g_chSubtitleDecState = ST_MAIN_PROCESS;
								break;
						}
						break;
					case DT_STUFFING_SEGMENT:
						break;
					default:
						break;
				}
				pSubtitlingSegmentPtr = pSubtitlingSegmentPtr +6 + wSegmentLength;
			}
			else
			{
				wTempdata2 = subtital_segment__segment_length_h(pSubtitlingSegmentPtr);
				wTempdata3 = subtital_segment__segment_length_l(pSubtitlingSegmentPtr);
				wTempdata3 = wTempdata3 + (wTempdata2 << 8);
				wSegmentLength = wTempdata3;
				pSubtitlingSegmentPtr = pSubtitlingSegmentPtr +6 + wSegmentLength;
			}
			wPackLen=wPackLen-wSegmentLength-6;
		}
		if(s_chObjectParse==1)
        {
            s_chObjectParse = 0;
            subt_wait_PTS();
            if(ST_SUBTITLE_NORML_CASE_PROCESS!=g_chSubtitleDecState)
            {
                //subt_wait_PTS();
#ifdef SUBT_DEBUG_DETAIL
                COMMON_PRINTF("\033[31m Clear screen %d !! \033[0m\n", __LINE__);
#endif
                timer_clear_screen(NULL);
            }
            chCleanFlag=0;
            subt_dec();
        }
	}
	else
	{
		COMMON_PRINTF("Subtitle receive err!\n");
		return APP_NO_ERROR;
	}
	return APP_NO_ERROR;
}

u8* subt_get_region_ptr(u8 chRegionId)
{
	u8* pSubtitlingSegmentPtr = com_sub.CompositionBufferPtr;
	u8* pSegmentDataFieldPtr = NULL;
	u16 wTemp,wLength;
	//快速搜索
	while(pSubtitlingSegmentPtr<g_pSubtitleCompositionBufferPtr)
	{
		if(subtital_segment__sync_byte(pSubtitlingSegmentPtr)==0x0f)
		{
			if (subtital_segment__segment_type(pSubtitlingSegmentPtr) == 0x11)
			{
				pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
				if(region_compostion_segment__region_id(pSegmentDataFieldPtr)== chRegionId)
					return pSubtitlingSegmentPtr;
				else
				{
					wTemp=region_compostion_segment__region_id(pSegmentDataFieldPtr);
					wLength=g_wRegionUseLen[wTemp];
					pSubtitlingSegmentPtr = pSubtitlingSegmentPtr + 6 + wLength;
				}
			}
			else
			{
				if(subtital_segment__segment_type(pSubtitlingSegmentPtr) != 0x12)
				{
					pSubtitlingSegmentPtr++;
				}
				else
				{
					pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
					wTemp=CLUT_definition_segment__CLUT_id(pSegmentDataFieldPtr);
					wLength=g_wClutUseLen[wTemp];
					pSubtitlingSegmentPtr = pSubtitlingSegmentPtr + 6 + wLength;
				}
			}
		}
		else
		{
			break;
		}
	}
	return NULL;
}

u8* subt_get_CLUT_ptr(u8 chClutId)
{
	u8* pSubtitlingSegmentPtr = com_sub.CompositionBufferPtr;
	u8* pSegmentDataFieldPtr  =NULL;
	u16 wTemp,wLength;
	//快速搜索
	while(pSubtitlingSegmentPtr<g_pSubtitleCompositionBufferPtr)
	{
		if(subtital_segment__sync_byte(pSubtitlingSegmentPtr)==0x0f)
		{
			if (subtital_segment__segment_type(pSubtitlingSegmentPtr) == 0x12)
			{
				pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
				wTemp=CLUT_definition_segment__CLUT_id(pSegmentDataFieldPtr);
				if(wTemp == chClutId)
					return pSubtitlingSegmentPtr;
				else
				{
					wTemp=CLUT_definition_segment__CLUT_id(pSegmentDataFieldPtr);
					wLength=g_wClutUseLen[wTemp];
					pSubtitlingSegmentPtr = pSubtitlingSegmentPtr + 6 + wLength;
				}
			}
			else
			{
				if(subtital_segment__segment_type(pSubtitlingSegmentPtr) != 0x11)
				{
					pSubtitlingSegmentPtr++;
				}
				else
				{
					pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
					wTemp=region_compostion_segment__region_id(pSegmentDataFieldPtr);
					wLength=g_wRegionUseLen[wTemp];
					pSubtitlingSegmentPtr = pSubtitlingSegmentPtr + 6 + wLength;
				}
			}
		}
		else
		{
			break;
		}
	}
	return NULL;
}

u8* subt_get_object_ptr(u16 wObjectId)
{
	u8* pSubtitlingSegmentPtr = com_sub.ObjectBufferPtr;

	u8* pSegmentDataFieldPtr;
	u16 wTemp1,wTemp2;
	while(pSubtitlingSegmentPtr<g_pSubtitleObjectBufferPtr)
	{
		pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
		wTemp1= object_data_segment__object_id_h(pSegmentDataFieldPtr);
		wTemp2 = object_data_segment__object_id_l(pSegmentDataFieldPtr) + (wTemp1<<8);
		if(wTemp2 == wObjectId)
			return pSubtitlingSegmentPtr;
		else
			pSubtitlingSegmentPtr = pSubtitlingSegmentPtr + 6 + (subtital_segment__segment_length_h(pSubtitlingSegmentPtr)<<8) + subtital_segment__segment_length_l(pSubtitlingSegmentPtr);
	}
	return NULL;
}

u8 subt_wait_PTS(void)
{
    int64_t nPts=0;
    nPts  = (pes_packet_head__PTS_H(g_pSubtitlePesBufferPtr))<<29;
    nPts |= (pes_packet_head__PTS_MH(g_pSubtitlePesBufferPtr))<<22;
    nPts |= (pes_packet_head__PTS_ML(g_pSubtitlePesBufferPtr))<<14;
    nPts |= (pes_packet_head__PTS_LH(g_pSubtitlePesBufferPtr))<<7;
    nPts |= (pes_packet_head__PTS_LL(g_pSubtitlePesBufferPtr))>>1;
    return 1;
}

void subt_dec(void)
{
    u8 chPageTimeOut;
	u8 chRegionId;
	u8 *pSubtitlingSegmentPtr,*pRegionDataFieldPtr,*pSegmentDataFieldPtr;
	u16 wSegmentLength ,wTemp1,wTemp2;
	struct subtitle_region SubtitleRegions;

	GxCore_MutexLock(com_sub.lock);
	pSubtitlingSegmentPtr = com_sub.PageBufferPtr;
	g_pStubtitlePixelBufferPtr = com_sub.PixelBufferPtr;
	pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
    chPageTimeOut = page_composition_segment__page_time_out(pSegmentDataFieldPtr);
    if(chPageTimeOut != 0)
        s_chPageTimeOut = chPageTimeOut;

	pRegionDataFieldPtr = page_composition_segment__region_data_field_start_ptr(pSegmentDataFieldPtr);
	wTemp1 = subtital_segment__segment_length_h(pSubtitlingSegmentPtr);
	wTemp2 = subtital_segment__segment_length_l(pSubtitlingSegmentPtr);
	wTemp2 = wTemp2 + (wTemp1 << 8);
	wSegmentLength = wTemp2 -2;
	COMMON_PRINTF("\n-----------subt decode--------\n");
	//COMMON_PRINTF("==>> Page Time Out : %ds !!\n", s_chPageTimeOut);
	while (wSegmentLength > 5)
	{
		chRegionId = page_composition_segment__region_id(pRegionDataFieldPtr);
		wTemp1 = page_composition_segment__region_horizontal_address_h(pRegionDataFieldPtr);
		wTemp2 = page_composition_segment__region_horizontal_address_l(pRegionDataFieldPtr);
		wTemp2 = wTemp2 +(wTemp1<<8);
		SubtitleRegions.region_left = wTemp2;
		wTemp1 = page_composition_segment__region_vertical_address_h(pRegionDataFieldPtr);
		wTemp2 = page_composition_segment__region_vertical_address_l(pRegionDataFieldPtr);
		wTemp2 = wTemp2 +(wTemp1<<8);
		SubtitleRegions.region_top = wTemp2;
#ifdef SUBT_DEBUG
		COMMON_PRINTF("\n=====>> Region ID: %d Region Horizontal: %d Region Vertical: %d\n",chRegionId,SubtitleRegions.region_left,SubtitleRegions.region_top);
#endif
		SubtitleRegions.bit_per_pix = (u8)0x0;
		SubtitleRegions.CLUT_id = (u8)0xffff;
		SubtitleRegions.region_bottom = SubtitleRegions.region_top;
		SubtitleRegions.region_right= SubtitleRegions.region_left;
		SubtitleRegions.top_end = 0x0;
		SubtitleRegions.top_start_ptr =0x0;
		SubtitleRegions.bottom_start_ptr =0x0;
		SubtitleRegions.bottom_end =0x0;
		SubtitleRegions.bottom_start_ptr=0x0;
		SubtitleRegions.region_visible = 0;
		subt_creat_region(chRegionId,&SubtitleRegions);
		wSegmentLength= wSegmentLength - 6;
		pRegionDataFieldPtr = pRegionDataFieldPtr + 6;
	}
	GxCore_MutexUnlock(com_sub.lock);
}

AppErr_t subt_creat_region(u8 chRegionId,struct subtitle_region *pSubtitleRegions)
{
	u8 chObjectType;
	u8 *pSegmentDataFieldPtr,*pSubtitlingSegmentPtr,*pObjectDataFieldPtr,*pObjectPtr,*pRegionCleanPtr;
	u16 wObjectId,wRegionWidthPixel,wRegionHeightPixel,wObjectPositionH,wObjectPositionV,wTemp1,wTemp2,wSegmentLength;
	u16 *pMemWritePtr;
	u32 nObjectDataLength;
    AppErr_t ret = APP_NO_ERROR;

	//得到所需region的地址
	pSubtitlingSegmentPtr = subt_get_region_ptr(chRegionId);
	if (pSubtitlingSegmentPtr!=NULL)
	{
		wTemp1 = subtital_segment__segment_length_h(pSubtitlingSegmentPtr);
		wTemp2 = subtital_segment__segment_length_l(pSubtitlingSegmentPtr);
		wTemp2 = wTemp2 + (wTemp1 << 8);
		wSegmentLength = wTemp2;
		pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
		//得到region宽度，右边界
		wTemp1 = region_compostion_segment__region_width_h(pSegmentDataFieldPtr);
		wTemp2 = region_compostion_segment__region_width_l(pSegmentDataFieldPtr);
		wRegionWidthPixel = wTemp2 + (wTemp1<<8);
	//	pSubtitleRegions->region_right = pSubtitleRegions->region_left + wRegionWidthPixel - 1;
		pSubtitleRegions->region_right = com_sub.render->ops->get_rect(com_sub.render->handle)->width;
		//得到region底边坐标
		wTemp1 = region_compostion_segment__region_height_h(pSegmentDataFieldPtr);
		wTemp2 = region_compostion_segment__region_height_l(pSegmentDataFieldPtr);
		wRegionHeightPixel = wTemp2 +(wTemp1<<8);
		pSubtitleRegions->region_bottom = pSubtitleRegions->region_top + wRegionHeightPixel - 1;
#ifdef SUBT_DEBUG
		COMMON_PRINTF("x: %d, y: %d, width: %d, height: %d, %d, %d, %d, %d\n",
			(com_sub.render->ops->get_rect(com_sub.render->handle))->x,
			(com_sub.render->ops->get_rect(com_sub.render->handle))->y,
			(com_sub.render->ops->get_rect(com_sub.render->handle))->width,
			(com_sub.render->ops->get_rect(com_sub.render->handle))->height,
			wRegionWidthPixel, wRegionHeightPixel,
			pSubtitleRegions->region_right+1, pSubtitleRegions->region_bottom+1);
#endif
		pSubtitleRegions->region_depth = region_compostion_segment__region_depth(pSegmentDataFieldPtr);
        pSubtitleRegions->bit_per_pix = region_compostion_segment__region_level_of_compatibility(pSegmentDataFieldPtr);
#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> Region Depth:%d\n",region_compostion_segment__region_depth(pSegmentDataFieldPtr));
#endif
		//得到region数据行宽和位深度
		switch (pSubtitleRegions->region_depth)
		{
			case 0x1://2bits表示一个像素点，720个像素需要720/4/2个双字节来储存
				if (wRegionWidthPixel&0x7)// pSubtitleRegions->bit_16_per_line表示每行有16个像素点的字符的个数
					pSubtitleRegions->bit_16_per_line = (wRegionWidthPixel>>3)+1;
				else
					pSubtitleRegions->bit_16_per_line = (wRegionWidthPixel>>3);//双字节对齐
				break;
			case 0x2://4bits表示一个像素点，720个像素需要720/2/2个双字节来储存
				if (wRegionWidthPixel&0x3)
					pSubtitleRegions->bit_16_per_line = (wRegionWidthPixel>>2)+1;
				else
					pSubtitleRegions->bit_16_per_line = wRegionWidthPixel>>2;//双字节对齐
				break;
			case 0x3://8bits表示一个像素点，720个像素需要720/2个双字节来储存
				//if (wRegionWidthPixel&0xf)//for bit 3
				if (wRegionWidthPixel&0x1)
					pSubtitleRegions->bit_16_per_line = (wRegionWidthPixel>>1)+1;
				else
					pSubtitleRegions->bit_16_per_line =wRegionWidthPixel>>1;//双字节对齐
				break;
			default:
				break;
				;//gxlogd("尚未使用\n");
		}
#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> bit_per_pix: %d bit_16_per_line: %d\n",pSubtitleRegions->bit_per_pix,pSubtitleRegions->bit_16_per_line);
#endif
		pSubtitleRegions->CLUT_id = region_compostion_segment__CLUT_id(pSegmentDataFieldPtr);
		pSubtitleRegions->clut_ptr = subt_creat_CLUT(pSubtitleRegions->CLUT_id);
		pSubtitleRegions->region_visible = 1;//在PAGE中列出的region是需要显示的region
		pSubtitleRegions->top_start_ptr = g_pStubtitlePixelBufferPtr;
		// pixel_buf 保护
        if(g_pStubtitlePixelBufferPtr+pSubtitleRegions->bit_16_per_line * wRegionHeightPixel *2>=com_sub.PixelBufferPtr+PIXEL_BUF_MAX_LEN)
        {
            gxlogd("--- pixel_buf protect! Pixel buffer is not enough!---\n");
            return APP_NO_ERROR;
        }
		if (pSubtitleRegions->region_top&0x1)
		{
			if (wRegionHeightPixel&0x1)
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * (wRegionHeightPixel + 1);
			else
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * wRegionHeightPixel;
		}
		else
		{
			if (wRegionHeightPixel&0x1)
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * (wRegionHeightPixel + 1);
			else
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * wRegionHeightPixel;
		}

		pSubtitleRegions->top_end = g_pStubtitlePixelBufferPtr - 2;//end不应该等于0
		pSubtitleRegions->bottom_start_ptr =g_pStubtitlePixelBufferPtr ;
		if (pSubtitleRegions->region_top&0x1)
		{
			if (wRegionHeightPixel&0x1)
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * (wRegionHeightPixel + 1);
			else
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * wRegionHeightPixel;
		}
		else
		{
			if (wRegionHeightPixel&0x1)
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * (wRegionHeightPixel + 1);
			else
				g_pStubtitlePixelBufferPtr = g_pStubtitlePixelBufferPtr + pSubtitleRegions->bit_16_per_line * wRegionHeightPixel ;
		}
		pSubtitleRegions->bottom_end = g_pStubtitlePixelBufferPtr -2;//end不应该等于0
#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> Buffer: 0x%x-0x%x:%d 0x%x-0x%x:%d\n",(uint32_t)pSubtitleRegions->top_end,(uint32_t)pSubtitleRegions->top_start_ptr,\
				pSubtitleRegions->top_end-pSubtitleRegions->top_start_ptr,(uint32_t)pSubtitleRegions->bottom_end,\
				(uint32_t)pSubtitleRegions->bottom_start_ptr,pSubtitleRegions->bottom_end-pSubtitleRegions->bottom_start_ptr);
		COMMON_PRINTF("=====>> Fill Flag:%d\n",region_compostion_segment__region_fill_flag(pSegmentDataFieldPtr));
#endif
		pRegionCleanPtr = pSubtitleRegions->top_start_ptr;
		//填充region
		//转成16位色填充背景
		if (region_compostion_segment__region_fill_flag(pSegmentDataFieldPtr))
		{
			wTemp1 =0;
			switch(pSubtitleRegions->bit_per_pix)
			{
				case 1:
					wTemp1 = region_compostion_segment__region_2bit_pixel_code(pSegmentDataFieldPtr);
					wTemp1 = wTemp1 + (wTemp1<<2);
					wTemp1 = wTemp1 + (wTemp1<<4);
					wTemp1 = wTemp1 + (wTemp1<<8);
					break;
				case 2:
					wTemp1 = region_compostion_segment__region_4bit_pixel_code(pSegmentDataFieldPtr);
					wTemp1 = wTemp1 + (wTemp1<<4);
					wTemp1 = wTemp1 + (wTemp1<<8);
					break;
				case 3:
					wTemp1 = region_compostion_segment__region_8bit_pixel_code(pSegmentDataFieldPtr);
					wTemp1 = wTemp1 + (wTemp1<<8);
					break;
				default:
					break;
			}
		}
		else
		{
			wTemp1 = 0x0000;
		}

#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> Backcolour : %d\n",wTemp1);
#endif

		//必须进行填充,buffer清零
		for (pMemWritePtr = (u16*)(pRegionCleanPtr);pMemWritePtr <= (u16*)(pSubtitleRegions->bottom_end);pMemWritePtr++)
		{
			//填充背景
			*pMemWritePtr = wTemp1;
		}

		wSegmentLength = wSegmentLength -10;
		pObjectDataFieldPtr = region_compostion_segment__object_data_field_start_ptr(pSegmentDataFieldPtr);
		while(wSegmentLength >5)
		{
			wTemp1 = region_compostion_segment__object_id_h(pObjectDataFieldPtr);
			wTemp2 = region_compostion_segment__object_id_l(pObjectDataFieldPtr);
			wTemp2 = wTemp2 +(wTemp1<<8);
			wObjectId = wTemp2;
			chObjectType = region_compostion_segment__object_type(pObjectDataFieldPtr);
#ifdef SUBT_DEBUG
			COMMON_PRINTF("=====>> ID:0x%x Data type:%d\n",wObjectId,chObjectType);
#endif
			switch (chObjectType)
			{
				case 0x2:
				case 0x0://位图数据
					{
						switch (region_compostion_segment__object_provider_flag(pObjectDataFieldPtr))
						{
							case 0x0://继续解码
								//;//gxlogd("由码流提供显示数据");
								pObjectPtr = subt_get_object_ptr(wObjectId);
								if (pObjectPtr != NULL)
								{
									wTemp1 = subtital_segment__segment_length_h(pObjectPtr);
									wTemp2 = subtital_segment__segment_length_l(pObjectPtr) ;
									nObjectDataLength= wTemp2 + (wTemp1<<8);
									wTemp1 = region_compostion_segment__object_horizontal_position_h(pObjectDataFieldPtr);
									wTemp2 = region_compostion_segment__object_horizontal_position_l(pObjectDataFieldPtr);
									wObjectPositionH= wTemp2 +(wTemp1<<8);
									wTemp1 = region_compostion_segment__object_vertical_postion_h(pObjectDataFieldPtr);
									wTemp2 = region_compostion_segment__object_vertical_postion_l(pObjectDataFieldPtr);
									wObjectPositionV= wTemp2 +(wTemp1<<8);
#ifdef SUBT_DEBUG
									COMMON_PRINTF("=====>> Object ID:%d Obiect H:%d Obiect V:%d Len : %d\n",wObjectId,wObjectPositionH,wObjectPositionV, nObjectDataLength);
#endif
									ret = subt_creat_object_data(subtital_segment__segment_data_field_ptr(pObjectPtr),nObjectDataLength,wObjectPositionH,wObjectPositionV,pSubtitleRegions);
                                    if(ret != APP_NO_ERROR)
                                        return ret;
								}
								else
								{
									COMMON_PRINTF("error!Can't find object!");
                                    return 1;
								}
								break;
							case 0x1:
								;//gxlogd("由解码器ROM提供显示数据");
								break;
							case 0x2:
								;//gxlogd("保留");
								break;
							case 0x3:
								;//gxlogd("保留");
								break;
							default:
								break;
						}
						break;
					}
				case 0x1:
					//gxlogd("字符");
					break;
				case 0x3:
                    {
                        pObjectPtr = subt_get_object_ptr(wObjectId);
                        if (pObjectPtr != NULL)
                        {
                            wTemp1 = subtital_segment__segment_length_h(pObjectPtr);
                            wTemp2 = subtital_segment__segment_length_l(pObjectPtr) ;
                            nObjectDataLength= wTemp2 + (wTemp1<<8);
                            wTemp1 = region_compostion_segment__object_horizontal_position_h(pObjectDataFieldPtr);
                            wTemp2 = region_compostion_segment__object_horizontal_position_l(pObjectDataFieldPtr);
                            wObjectPositionH= wTemp2 +(wTemp1<<8);
                            wTemp1 = region_compostion_segment__object_vertical_postion_h(pObjectDataFieldPtr);
                            wTemp2 = region_compostion_segment__object_vertical_postion_l(pObjectDataFieldPtr);
                            wObjectPositionV= wTemp2 +(wTemp1<<8);
#ifdef SUBT_DEBUG
                            COMMON_PRINTF("=====>> Object ID:%d Obiect H:%d Obiect V:%d Len : %d\n",wObjectId,wObjectPositionH,wObjectPositionV, nObjectDataLength);
#endif
                            ret = subt_creat_object_data(subtital_segment__segment_data_field_ptr(pObjectPtr),nObjectDataLength,wObjectPositionH,wObjectPositionV,pSubtitleRegions);
                            if(ret != APP_NO_ERROR)
                                return ret;
                        }
                        else
                        {
                            COMMON_PRINTF("error!Can't find object!");
                            return 1;
                        }
                    }
					break;
				default:
					break;
			}

			if ( chObjectType == 0x01 || chObjectType == 0x02)
			{

				wSegmentLength = wSegmentLength - 8;
				pObjectDataFieldPtr = pObjectDataFieldPtr + 8;
			}
			else
			{
				wSegmentLength = wSegmentLength - 6;
				pObjectDataFieldPtr = pObjectDataFieldPtr + 6;
			}
		}
#ifdef SUBT_DEBUG_DETAIL
        COMMON_PRINTF("\033[31m Create Clear screen Timer %d !! \033[0m\n", __LINE__);
#endif
        com_sub.timer_ops->ops->remove(com_sub.timer);
        com_sub.timer = com_sub.timer_ops->ops->create(timer_clear_screen,
                                                        s_chPageTimeOut * 1000,
                                                        NULL,
                                                        0);
        subt_draw_region(pSubtitleRegions);
		return ret;
	}
	else
	{
		COMMON_PRINTF("\nsubt region is not found\n");
		return 1;
	}
}

AppErr_t subt_creat_object_data (u8 *pSegmentDataFieldPtr,u16 wSegmentLength,u16 wObjectPositionH,u16 wObjectPositionV,struct subtitle_region *pSubtitleRegions )
{
	u16 wTemp1,wTemp2,wTopFieldDataBlockLength,wBottomFieldDataBlockLength;
	u8 chObjectCodingMethod,*pTopFieldDataPtr,*pBottomFieldDataPtr,*pTargetPtr;
	chObjectCodingMethod = object_data_segment__object_coding_method(pSegmentDataFieldPtr);
    AppErr_t ret = APP_NO_ERROR;

	if (chObjectCodingMethod == 0x0)//图片数据
	{
		//得到顶场数据长度
		wTemp1 = object_data_segment__top_field_data_block_length_h(pSegmentDataFieldPtr);
		wTemp2 = object_data_segment__top_field_data_block_length_l(pSegmentDataFieldPtr);
		wTopFieldDataBlockLength =(wTemp1<<8) + wTemp2;
		//得到底场数据长度
		wTemp1 = object_data_segment__bottom_field_data_block_length_h(pSegmentDataFieldPtr);
		wTemp2 = object_data_segment__bottom_field_data_block_length_l(pSegmentDataFieldPtr);
		wBottomFieldDataBlockLength = (wTemp1<<8) + wTemp2;
		//得到两场数据的指针
		pTopFieldDataPtr = object_data_segment__field_pixel_data_sub_block_ptr(pSegmentDataFieldPtr);
		pBottomFieldDataPtr = pTopFieldDataPtr + wTopFieldDataBlockLength;
#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> TopDataLen:%d BottomDataLen:%d\n",wTopFieldDataBlockLength,wBottomFieldDataBlockLength);
#endif
		//解码并填充
		//计算起始行地址
		if (pSubtitleRegions->region_top&0x1)
		{
			if (wObjectPositionV&0x1)
				pTargetPtr = pSubtitleRegions->top_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV + 1);
			else
				pTargetPtr = pSubtitleRegions->top_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV);
		}
		else
		{
			if (wObjectPositionV&0x1)
				pTargetPtr = pSubtitleRegions->top_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV +1);
			else
				pTargetPtr = pSubtitleRegions->top_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV);
		}
#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> TopFieldPtr: 0x%x Target:0x%x\n",(uint32_t)pTopFieldDataPtr,(uint32_t)pTargetPtr);
#endif
        ret = subt_creat_block_data(pTopFieldDataPtr,pTargetPtr,wObjectPositionV,wObjectPositionH,wTopFieldDataBlockLength,pSubtitleRegions);
        if(ret != APP_NO_ERROR)
            return ret;
		if (pSubtitleRegions->region_top&0x1)
		{
			if (wObjectPositionV&0x1)
				pTargetPtr = pSubtitleRegions->bottom_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV + 1);
			else
				pTargetPtr = pSubtitleRegions->bottom_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV);
		}
		else
		{
			if (wObjectPositionV&0x1)
				pTargetPtr = pSubtitleRegions->bottom_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV +1);
			else
				pTargetPtr = pSubtitleRegions->bottom_start_ptr + pSubtitleRegions->bit_16_per_line*(wObjectPositionV);
		}
#ifdef SUBT_DEBUG
		COMMON_PRINTF("=====>> BottomFieldPtr: 0x%x Target:0x%x\n",(uint32_t)pBottomFieldDataPtr,(uint32_t)pTargetPtr);
#endif
		ret = subt_creat_block_data(pBottomFieldDataPtr,pTargetPtr,wObjectPositionV,wObjectPositionH,wBottomFieldDataBlockLength,pSubtitleRegions);
        if(ret != APP_NO_ERROR)
            return ret;
	}
	else if (chObjectCodingMethod == 0x1)
	{
        gxlogd("\n[@@@][Subt]Coded as a string of characters!\n");
	}
	else
	{
		;
	}
    return ret;
}

u8 subt_creat_CLUT(u8 chClutId)
{
	u8 *pSegmentDataFieldPtr,*pSubtitlingSegmentPtr,*pCLUTDataFieldPtr;
	u8 chY,chCb,chCr,chAlpha,chEntryId,chFullRangeFlag;
	//u16 *pClutDataPtr;
	u16 wSegmentLength,wTemp1,wTemp2,wYcbcr=0;
	pSubtitlingSegmentPtr = subt_get_CLUT_ptr(chClutId);
	if (pSubtitlingSegmentPtr != NULL)
	{
		pSegmentDataFieldPtr = subtital_segment__segment_data_field_ptr(pSubtitlingSegmentPtr);
		wTemp1 = subtital_segment__segment_length_h(pSubtitlingSegmentPtr);
		wTemp2 = subtital_segment__segment_length_l(pSubtitlingSegmentPtr);
		wSegmentLength = wTemp2+(wTemp1<<8);
		pCLUTDataFieldPtr = CLUT_definition_segment__CLUT_data_field_start_ptr(pSegmentDataFieldPtr);
		wSegmentLength = wSegmentLength -2;
		while (wSegmentLength !=0)
		{
			chEntryId = CLUT_definition_segment__CLUT_entry_id(pCLUTDataFieldPtr) ;
			//pClutDataPtr = (u16*)(&g_wClutTable[chEntryId]);
			chFullRangeFlag = CLUT_definition_segment__full_range_flag(pCLUTDataFieldPtr);
			if(chFullRangeFlag)//8:8:8:8
			{
				chY=CLUT_definition_segment__Y_value_flag1(pCLUTDataFieldPtr);
				chCb = CLUT_definition_segment__Cb_value_flag1(pCLUTDataFieldPtr);
				chCr= CLUT_definition_segment__Cr_value_flag1(pCLUTDataFieldPtr);
				chAlpha = CLUT_definition_segment__T_value_flag1(pCLUTDataFieldPtr);
				wYcbcr= ((chY>>2)<<10)+((chCb>>4)<<6)+((chCr>>4)<<2)+((chAlpha)>>6);
				wSegmentLength = wSegmentLength -6;
				pCLUTDataFieldPtr = pCLUTDataFieldPtr + 6;
			}
			else
			{
				chY = CLUT_definition_segment__Y_value_flag0(pCLUTDataFieldPtr);
				chCb = CLUT_definition_segment__Cb_value_flag0(pCLUTDataFieldPtr);
				chCr= CLUT_definition_segment__Cr_value_flag0(pCLUTDataFieldPtr);
				chAlpha = CLUT_definition_segment__T_value_flag0(pCLUTDataFieldPtr);
				wYcbcr= (chY<<10)+(chCb<<6)+(chCr<<2)+chAlpha;
				wSegmentLength = wSegmentLength -4;
				pCLUTDataFieldPtr = pCLUTDataFieldPtr + 4;
			}


			if(wYcbcr<0x400)//add by hulj 080923
			{
				wYcbcr = 0;
			}


			//g_wClutTable[chEntryId]=wYcbcr;
			g_wClutTable[chEntryId]=((wYcbcr & 0xFF00) >> 8) + ((wYcbcr & 0xFF) << 8);
#ifdef SUBT_DEBUG
			COMMON_PRINTF("=====>> Clut:%d Entry ID:%d Colour:0x%x 0x%x\n",chFullRangeFlag,chEntryId,wYcbcr,g_wClutTable[chEntryId]);
#endif
		}

	}
	else
	{
		COMMON_PRINTF("\nsubt clut is not found\n");
	}
	return 0;
}

AppErr_t subt_creat_block_data(u8* pFieldPixelDataSubBlockPtr,u8 *pBufrerWritePtr,u16 wObjectPositionV,u16 wObjectPositionH,u16 wFieldDataBlockLength,struct subtitle_region* pSubtitleRegions)
{
	u8 *pPixelDataSubBlockPtr,chBpp,chDepth;
	u16 wProcessLength = 0,wDataLength,i;
	struct draw_pen MyPen;
	struct run_length DecLength;
	chBpp = pSubtitleRegions->bit_per_pix;
	chDepth = pSubtitleRegions->region_depth;
    u8 map2to4[] = { 0x00, 0x07, 0x08, 0x0f };
    u8 map2to8[] = { 0x00, 0x77, 0x88, 0xff };
    u8 map4to8[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    u8 *map_table = NULL;
    AppErr_t ret = APP_NO_ERROR;

	memset(&MyPen, 0, sizeof(struct draw_pen));
	memset(&DecLength, 0, sizeof(struct run_length));
	//起始地址计算
	switch(chDepth)
	{
		case 1://没有流可以测试，当ObjectPositionH为奇数时可能有问题
			MyPen.draw_pen_head = pBufrerWritePtr + (wObjectPositionH>>2);
			MyPen.pixel_switch = (wObjectPositionH  &0x3)*2;//0 2 4 6
			break;
		case 2:
			MyPen.draw_pen_head = pBufrerWritePtr + (wObjectPositionH>>1);
			MyPen.pixel_switch = (wObjectPositionH  &0x1)*4;//0 4
			break;
		case 3:
			MyPen.draw_pen_head = pBufrerWritePtr + wObjectPositionH;
			MyPen.pixel_switch = 0;//0
			break;
	}
	//行偏移计算要求region字节对齐
	pPixelDataSubBlockPtr =  pFieldPixelDataSubBlockPtr;
	while(wProcessLength < wFieldDataBlockLength)
	{
		wProcessLength ++;
		DecLength.dec_length =0;
		DecLength.pixel_length =0;
#ifdef SUBT_DEBUG_DETAIL
		COMMON_PRINTF("\n=====>> Draw pen head: 0x%x pixel switch: %d\n",(uint32_t)MyPen.draw_pen_head,MyPen.pixel_switch);
		COMMON_PRINTF("=====>> Sub block data type:[0x%x] Date_len:%d\n",pixel_data_sub_block__data_type(pPixelDataSubBlockPtr),wFieldDataBlockLength+3-wProcessLength);
#endif
		switch(pixel_data_sub_block__data_type(pPixelDataSubBlockPtr))
		{
			case 0x10://2-bit/pixel code string
                if(pSubtitleRegions->region_depth == 3)
                    map_table = map2to8;
                else if(pSubtitleRegions->region_depth == 2)
                    map_table = map2to4;
                else
                    map_table = NULL;

				chBpp = 2;
				DecLength = subt_run_length_dec(pPixelDataSubBlockPtr +1,MyPen.draw_pen_head,MyPen.pixel_switch,chBpp,chDepth,wFieldDataBlockLength+3-wProcessLength,map_table);
				wDataLength = DecLength.read_length -3;
				MyPen.draw_pen_head += (pSubtitleRegions->bit_16_per_line<<1);
				break;
			case 0x11://4-bit/pixel code string
                if(pSubtitleRegions->region_depth == 3)
                    map_table = map4to8;
                else
                    map_table = NULL;

				chBpp = 4;
				DecLength = subt_run_length_dec(pPixelDataSubBlockPtr + 1,MyPen.draw_pen_head,MyPen.pixel_switch,chBpp,chDepth,wFieldDataBlockLength+3-wProcessLength,map_table);
				wDataLength = DecLength.read_length -3;
				MyPen.draw_pen_head += (pSubtitleRegions->bit_16_per_line<<1);
				break;
			case 0x12://8-bit/pixel code string
				chBpp = 8;
				DecLength = subt_run_length_dec(pPixelDataSubBlockPtr + 1,MyPen.draw_pen_head,MyPen.pixel_switch,chBpp,chDepth,wFieldDataBlockLength+3-wProcessLength,map_table);
				wDataLength = DecLength.read_length -3;
				MyPen.draw_pen_head += (pSubtitleRegions->bit_16_per_line<<1);//与具体的bit depth无关，只是bit_16_per_line*2
				break;
			case 0x20://2_to_4-bit_map-table data
				wDataLength = 2;
                map2to4[0] = (*(pPixelDataSubBlockPtr+1)) >> 4;
                map2to4[1] = (*(pPixelDataSubBlockPtr+1)) & 0xf;
                map2to4[2] = (*(pPixelDataSubBlockPtr+2)) >> 4;
                map2to4[3] = (*(pPixelDataSubBlockPtr+2)) & 0xf;
                COMMON_PRINTF("\n[Subt]Map2to4:%d %d %d %d\n",map2to4[0],map2to4[1],map2to4[2],map2to4[3]);
				break;
			case 0x21://2_to_8-bit_map-table data
				wDataLength = 4;
                for (i = 0; i < 4; i++)
                    map2to8[i] = *(pPixelDataSubBlockPtr+i+1);
				break;
			case 0x22://4_to_8-bit_map-table data
				wDataLength = 16;
                for (i = 0; i < 16; i++)
                    map4to8[i] = *(pPixelDataSubBlockPtr+i+1);
				break;
			case 0xf0://end of object line code
				wDataLength = 0;
				break;
			default:
				wDataLength = 0;
                gxlogd("\033[31mCurrent object data is wrong!!\033[0m\n");
                ret = APP_ERR_NO_DATA;  //!!!
                return ret;
		}
#ifdef SUBT_DEBUG_DETAIL
		COMMON_PRINTF(" Pixel len:%d Dataleng:%d head:0x%x\n",DecLength.pixel_length,wDataLength,(uint32_t)MyPen.draw_pen_head);
#endif
		//绘制入region
		wProcessLength= wProcessLength + wDataLength;
		pPixelDataSubBlockPtr = pFieldPixelDataSubBlockPtr +wProcessLength ;
	}
    return ret;
}

void subt_draw_region(struct subtitle_region* SubtitleRegions)
{
	u8 chBpp;
	u16 wI,wX,wYcbcr;
	u8 *pReadTopPtr = SubtitleRegions->top_start_ptr;
	u8 *pReadBottomPtr= SubtitleRegions->bottom_start_ptr;
	u8 chEntryId = SubtitleRegions->clut_ptr;

	//chBpp = SubtitleRegions->bit_per_pix;
	chBpp = SubtitleRegions->region_depth;
	chEntryId=0;
#ifdef SUBT_DEBUG
	COMMON_PRINTF("\nsubt region add x=%d,y=%d\n",SubtitleRegions->region_left,SubtitleRegions->region_top);
	COMMON_PRINTF("\nStr Ptr: 0x%x 0x%x\n",(uint32_t)SubtitleRegions->top_start_ptr,(uint32_t)SubtitleRegions->bottom_start_ptr);
#endif
	if(chBpp == 1)
	{
		for(wI=0;wI<SubtitleRegions->region_bottom - SubtitleRegions->region_top + 1;wI++)
        {
#ifdef SUBT_DEBUG_DETAIL
            COMMON_PRINTF("%d top:0x%x bottom:0x%x\n",wI,(uint32_t)pReadTopPtr,(uint32_t)pReadBottomPtr);
#endif
			for(wX=0;wX<(SubtitleRegions->bit_16_per_line<<3);wX=wX+4)
			{
				if(SubtitleRegions->region_left+wX >= SubtitleRegions->region_right - SubtitleRegions->region_left + 1)
				{
					pReadTopPtr += (((SubtitleRegions->bit_16_per_line<<3)+\
								SubtitleRegions->region_left -\
								(SubtitleRegions->region_right - SubtitleRegions->region_left + 1))>>1);
					break;
				}
				wYcbcr = g_wClutTable[chEntryId + (((*(pReadTopPtr))&0xc0)>>6)];
				subt_set_pixel((SubtitleRegions->region_left + wX), SubtitleRegions->region_top + wI,wYcbcr);

				wYcbcr = g_wClutTable[chEntryId + (((*(pReadTopPtr))&0x30)>>4)];
				subt_set_pixel((SubtitleRegions->region_left + wX + 1),SubtitleRegions->region_top + wI,wYcbcr);

				wYcbcr = g_wClutTable[chEntryId + (((*(pReadTopPtr))&0x0c)>>2)];
				subt_set_pixel((SubtitleRegions->region_left + wX + 2),SubtitleRegions->region_top + wI,wYcbcr);

				wYcbcr = g_wClutTable[chEntryId + (((*(pReadTopPtr))&0x03)>>0)];
				subt_set_pixel((SubtitleRegions->region_left + wX + 3),SubtitleRegions->region_top + wI,wYcbcr);

				pReadTopPtr++;
			}
			wI++;
			if (wI<SubtitleRegions->region_bottom - SubtitleRegions->region_top + 1)
				for(wX=0;wX<(SubtitleRegions->bit_16_per_line<<3);wX=wX+4)
				{
					if(SubtitleRegions->region_left+wX>=((com_sub.render->ops->get_rect(com_sub.render->handle))->width))
					{
						pReadBottomPtr += (((SubtitleRegions->bit_16_per_line<<3)+\
									SubtitleRegions->region_left -\
									(SubtitleRegions->region_right - SubtitleRegions->region_left + 1))>>1);
						break;
					}
					wYcbcr = g_wClutTable[chEntryId + (((*(pReadBottomPtr))&0xc0)>>6)];
					subt_set_pixel((SubtitleRegions->region_left + wX), SubtitleRegions->region_top + wI,wYcbcr);

					wYcbcr = g_wClutTable[chEntryId + (((*(pReadBottomPtr))&0x30)>>4)];
					subt_set_pixel((SubtitleRegions->region_left + wX + 1),SubtitleRegions->region_top + wI,wYcbcr);

					wYcbcr = g_wClutTable[chEntryId + (((*(pReadBottomPtr))&0x0c)>>2)];
					subt_set_pixel((SubtitleRegions->region_left + wX + 2),SubtitleRegions->region_top + wI,wYcbcr);

					wYcbcr = g_wClutTable[chEntryId + (((*(pReadBottomPtr))&0x03)>>0)];
					subt_set_pixel((SubtitleRegions->region_left + wX + 3),SubtitleRegions->region_top + wI,wYcbcr);

					pReadBottomPtr++;
				}
		}
	}
	else if(chBpp == 2)
	{
		for(wI=0;wI<SubtitleRegions->region_bottom - SubtitleRegions->region_top + 1;wI++)
		{
#ifdef SUBT_DEBUG_DETAIL
			COMMON_PRINTF("%d top:0x%x bottom:0x%x\n",wI,(uint32_t)pReadTopPtr,(uint32_t)pReadBottomPtr);
#endif
			for(wX=0;wX<(SubtitleRegions->bit_16_per_line<<2);wX=wX+2)
			{
				if(SubtitleRegions->region_left+wX >= SubtitleRegions->region_right - SubtitleRegions->region_left + 1)
				{
					pReadTopPtr += (((SubtitleRegions->bit_16_per_line<<2)+\
										SubtitleRegions->region_left -\
											(SubtitleRegions->region_right - SubtitleRegions->region_left + 1))>>1);
					break;
				}
				wYcbcr = g_wClutTable[chEntryId + (((*(pReadTopPtr))&0xf0)>>4)];
				subt_set_pixel(SubtitleRegions->region_left+wX, SubtitleRegions->region_top + wI,wYcbcr);

				wYcbcr = g_wClutTable[chEntryId + ((*(pReadTopPtr))&0xf)];
				subt_set_pixel(SubtitleRegions->region_left+wX+1,SubtitleRegions->region_top + wI,wYcbcr);

				pReadTopPtr++;
			}
			wI++;
			if (wI<SubtitleRegions->region_bottom - SubtitleRegions->region_top + 1)
				for(wX=0;wX<(SubtitleRegions->bit_16_per_line<<2);wX=wX+2)
				{
					if(SubtitleRegions->region_left+wX >= SubtitleRegions->region_right - SubtitleRegions->region_left + 1)
					{
						pReadBottomPtr += (((SubtitleRegions->bit_16_per_line<<2)+\
											SubtitleRegions->region_left -\
												(SubtitleRegions->region_right - SubtitleRegions->region_left + 1))>>1);
						break;
					}
					wYcbcr = g_wClutTable[chEntryId + (((*(pReadBottomPtr))&0xf0)>>4)];
					subt_set_pixel(SubtitleRegions->region_left+wX, SubtitleRegions->region_top + wI,wYcbcr);

					wYcbcr = g_wClutTable[chEntryId + ((*(pReadBottomPtr))&0xf)];
					subt_set_pixel(SubtitleRegions->region_left+wX+1,SubtitleRegions->region_top + wI,wYcbcr);

					pReadBottomPtr++;
				}
		}
	}
	else if(chBpp == 3)
	{
		for(wI=0;wI<SubtitleRegions->region_bottom - SubtitleRegions->region_top + 1;wI++)
		{
#ifdef SUBT_DEBUG_DETAIL
			COMMON_PRINTF("%d top:0x%x bottom:0x%x\n",wI,(uint32_t)pReadTopPtr,(uint32_t)pReadBottomPtr);
#endif
			for(wX=0;wX<(SubtitleRegions->bit_16_per_line<<1);wX=wX+1)
			{
				if(SubtitleRegions->region_left+wX >= SubtitleRegions->region_right - SubtitleRegions->region_left + 1)
				{
					pReadTopPtr += ((SubtitleRegions->bit_16_per_line<<1)+\
										SubtitleRegions->region_left -\
											(SubtitleRegions->region_right - SubtitleRegions->region_left + 1));
					break;
				}
				wYcbcr = g_wClutTable[chEntryId + (*(pReadTopPtr))];
				subt_set_pixel(SubtitleRegions->region_left+wX, SubtitleRegions->region_top + wI,wYcbcr);
				pReadTopPtr++;
			}
			wI++;
			if (wI<SubtitleRegions->region_bottom - SubtitleRegions->region_top + 1)
				for(wX=0;wX<(SubtitleRegions->bit_16_per_line<<1);wX=wX+1)
				{
					if(SubtitleRegions->region_left+wX >= SubtitleRegions->region_right - SubtitleRegions->region_left + 1)
					{
					pReadBottomPtr += ((SubtitleRegions->bit_16_per_line<<1)+\
										SubtitleRegions->region_left -\
											(SubtitleRegions->region_right - SubtitleRegions->region_left + 1));
						break;
					}
					wYcbcr = g_wClutTable[chEntryId + (*(pReadBottomPtr))];
					subt_set_pixel(SubtitleRegions->region_left+wX, SubtitleRegions->region_top + wI,wYcbcr);
					pReadBottomPtr++;
				}
		}
	}
}

struct run_length subt_run_length_dec(u8* data_source_ptr,u8* data_target_ptr,u8 write_buffer_switch,u8 bpp,u8 depth,u16 wLeftBlockLength, u8* map_table)
{
	u8 *data_ptr = data_source_ptr,data_empty = 0,buffer_empty = 0,color_data=0,tmp,*dec_buffer_write_ptr =0,write_buffer =0;
	u32 run_data;
	u16 repeat_num=0;//像素点重复的次数，不包括本身
	struct run_length return_length;
	return_length.read_length = 0;
	return_length.dec_length = 0;
	run_data = ((*((u8*)data_ptr))<<24) + ((*((u8*)data_ptr + 1))<<16) + ((*((u8*)data_ptr + 2))<<8) + ((*((u8*)data_ptr +3)));//填充好buffer
	data_ptr = data_ptr + 4;
	dec_buffer_write_ptr = data_target_ptr ;
	return_length.pixel_length =0;
#ifdef SUBT_DEBUG_DETAIL
	COMMON_PRINTF("=====>> source ptr:0x%p target ptr:0x%p switch: %d leftlength:%d\n",data_source_ptr,data_target_ptr,write_buffer_switch,wLeftBlockLength);
#endif
	while(1)
	{
		switch(bpp)
		{
			case 2:
				{
					//see en300 743-11 Structure of the pixel code strings table-15
					switch(run_data&0xc0000000)        //chang by hulj 20081020
					{
						case 0x00000000://00xxxxxx
							if (run_data&0x20000000)    //chang by hulj 20081020
							{                           //00 1L LL CC --L pixels (3-10) in colour C
								//color_data = (run_data&0x30000000)>>24;
								color_data = (run_data&0x03000000)>>24;
								repeat_num = ((run_data&0x1c000000)>>26) + 2;
								data_empty = 8;
							}
							else if(run_data&0x10000000)//00 01 --one pixel in colour 0
							{
								color_data = 0;
								repeat_num = 0;
								data_empty = 4;
							}
							else
							{
								switch(run_data&0x0c000000)   //chang by hulj 20081020
								{
									case 0x0c000000://00 00 11 LL LL LL LL CC --L pixels (29..284) in colour C
										color_data = (run_data&0x30000)>>16;
										repeat_num = ((run_data&0x3fc0000)>>18) +28;
										data_empty = 16;
										break;
									case 0x08000000://00 00 10 LL LL CC --L pixels (12..27) in colour C
										color_data = (run_data&0x300000)>>20;
										repeat_num = ((run_data &0x3c00000)>>22) + 11;
										data_empty = 12;
										break;
									case 0x04000000://00 00 01 --two pixels in colour 0
										color_data = 0;
										repeat_num = 1;
										data_empty = 6;
										break;
									case 0x00000000:
										if (buffer_empty)
										{
											if(data_ptr - data_source_ptr + 1>wLeftBlockLength)//处理完毕
											{
												return_length.read_length = wLeftBlockLength;
												return return_length;
											}
											return_length.read_length = data_ptr - data_source_ptr + 1;
										}
										else
										{
											if(data_ptr - data_source_ptr + 1>wLeftBlockLength)
											{
												return_length.read_length = wLeftBlockLength + 1;
												return return_length;
											}
											return_length.read_length = data_ptr - data_source_ptr;
										}
										if (write_buffer_switch!=0)
										{
											*dec_buffer_write_ptr  = write_buffer;
											dec_buffer_write_ptr ++;
										}
										return_length.dec_length = dec_buffer_write_ptr - data_target_ptr;
										return return_length;
										break;
									default:
										break;
								}
							}
							break;
						case 0x40000000://01 --one pixel in colour 1
							color_data = 0x1;
							repeat_num = 0;
							data_empty = 2;
							break;
						case 0x80000000://10 --one pixel in colour 2
							color_data = 0x2;
							repeat_num = 0;
							data_empty = 2;
							break;
						case 0xc0000000://11 --one pixel in colour 3
							color_data = 0x3;
							repeat_num = 0;
							data_empty = 2;
							break;
						default:
							break;
					}
                    break;
				}
			case 4:
				{
					if (run_data&0xf0000000)//0000-1111 --one pixel in colour 1 to 15
					{
						color_data = (run_data>>28);
						repeat_num = 0;
						data_empty = 4;
					}
					else
					{
						if (run_data&0x08000000)
						{//switch_1
							if (run_data&0x04000000)
							{
								switch(run_data&0x03000000)
								{
									case 0x0://0000 1100 --one pixel in colour 0
										color_data = 0;
										repeat_num = 0;
										data_empty =8;
										break;
									case 0x1000000://0000 1101 --two pixels in colour 0
										color_data = 0;
										repeat_num = 1;
										data_empty = 8;
										break;
									case 0x2000000://0000 1110 LLLL CCCC --L pixels (9..24) in colour C
										color_data = (run_data&0xf0000)>>16 ;
										repeat_num = ((run_data&0xf00000)>>20) + 8;
										data_empty = 16;
										break;
									case 0x3000000://0000 1111 LLLL LLLL CCCC --L pixels (25..280) in colour C
										color_data = (run_data&0xf000)>>12;
										repeat_num = ((run_data&0xff0000)>>16) +24;
										data_empty = 20;
										break;
									default:
										break;
								}
							}
							else//0000 10LL CCCC --L pixels (4..7) in colour C
							{
								color_data = (run_data&0x00f00000)>>20;
								repeat_num = ((run_data&0x03000000)>>24) + 3;
								data_empty = 12;
							}
						}
						else
						{
							if (run_data&0xff000000)//0000 0LLL --L pixels (3..9) in colour 0 (L>0)
							{
								color_data = 0;
								repeat_num = ((run_data&0x07000000)>>24) + 1;
								data_empty = 8;
							}
							else
							{
								if (buffer_empty)
								{
									if(data_ptr - data_source_ptr + 2>wLeftBlockLength)
									{
										return_length.read_length = wLeftBlockLength;
										return return_length;
									}
									return_length.read_length = data_ptr - data_source_ptr + 2;
								}
								else
								{
									if(data_ptr - data_source_ptr + 1>wLeftBlockLength)
									{
										return_length.read_length = wLeftBlockLength;
										return return_length;
									}
									return_length.read_length = data_ptr - data_source_ptr + 1;
								}
								if (write_buffer_switch!=0)
								{
									*dec_buffer_write_ptr  = write_buffer;
									dec_buffer_write_ptr ++;
								}
								return_length.dec_length = dec_buffer_write_ptr - data_target_ptr;
								return return_length;
							}
						}
					}

					break;
				}
			case 8:
				if (run_data & 0xff000000)//00000001-11111111 --one pixel in colour 1 to 255
				{
					color_data = (run_data&0xff000000)>>24;
					repeat_num =0;
					data_empty = 8;
				}
				else
				{
					if (run_data & 0x00800000)//00000000 1LLLLLLL CCCCCCCC --L pixels (3-127) in colour C (L > 2)
					{
						color_data = (run_data&0x0000ff00)>>8;
						repeat_num = ((run_data&0x007f0000)>>16)-1;
						data_empty = 24;
					}
					else
					{
						//if (run_data&0xffff0000)//00000000 0LLLLLLL --L pixels (1-127) in colour 0 (L > 0)
						if (run_data&0x007f0000)//00000000 0LLLLLLL --L pixels (1-127) in colour 0 (L > 0)
						{
							color_data = 0;
							repeat_num = ((run_data&0x7f0000)>>16)-1;
							data_empty = 16;
						}
						else
						{
							if (buffer_empty)
							{
								if(data_ptr - data_source_ptr + 2>wLeftBlockLength)
								{
									return_length.read_length = wLeftBlockLength;
									return return_length;
								}
								return_length.read_length = data_ptr - data_source_ptr + 2;
							}
							else
							{
								if(data_ptr - data_source_ptr + 1>wLeftBlockLength)
								{
									return_length.read_length = wLeftBlockLength;
									return return_length;
								}
								return_length.read_length = data_ptr - data_source_ptr + 1;
							}
							if (write_buffer_switch!=0)
							{
								*dec_buffer_write_ptr  = write_buffer;
								dec_buffer_write_ptr ++;
							}
							return_length.dec_length = dec_buffer_write_ptr - data_target_ptr;
							return return_length;
						}
					}
				}
				break;
			default:
				break;
		}

        if(map_table != NULL)
        {
            color_data = map_table[color_data];
        }

		//预读机制,这部分的作用就上将data_ptr指向的数据不断的填入处理区run_data
		run_data = run_data<<data_empty;//处理完的数据丢弃，处理的长度是data_empty
		buffer_empty = buffer_empty +data_empty;//如果处理长度大于一个字节
		while(buffer_empty>=8)
		{
			tmp =  buffer_empty - 8 ;
			run_data = run_data >> tmp;
			run_data = (run_data&0xffffff00)|(*data_ptr);
			run_data = run_data <<tmp;
			buffer_empty = buffer_empty - 8;
			data_ptr++;
		}
		//填充buffer
		return_length.pixel_length = return_length.pixel_length + 1 + repeat_num;//return_length.pixel_length:填充之前的长度 1：本身 repeat_num：重复次数
		while(1)
		{
			switch(depth)
			{
				case 1://2bits = one pixel
					switch(write_buffer_switch)
					{
						case 0:
							write_buffer = color_data <<6;
							break;
						case 2:
							write_buffer = (write_buffer&0xcf)|(color_data<<4);
							break;
						case 4:
							write_buffer = (write_buffer&0xf3)|(color_data<<2);
							break;
						case 6:
							write_buffer = (write_buffer&0xfc)|color_data;
							break;
						default:
							break;
					}
					write_buffer_switch = write_buffer_switch +2;
					break;
				case 2://4bits = one pixel
					switch(write_buffer_switch)
					{
						case 0:
							write_buffer = color_data <<4;
							break;
						case 4:
							write_buffer = write_buffer|color_data;
							break;
						default:
							break;
					}
					write_buffer_switch = write_buffer_switch +4;
					break;
				case 3://8bits = one pixel
					write_buffer = color_data;
					write_buffer_switch = 8;
					break;
			}
			if (write_buffer_switch == 8)
			{
				*dec_buffer_write_ptr = write_buffer;
				dec_buffer_write_ptr ++;
				write_buffer_switch = 0;
				write_buffer = 0;
			}
			if (repeat_num == 0)
				break;
			else
				repeat_num --;
		}
	}
}

void subt_set_pixel(u16 wX,u16 wY,u16 wYcbcr)
{
	u32 nI;
#if (!CHIP_GX3002)
	if (com_sub.render->ops->get_ntsc(com_sub.render->handle)) {
		nI=wY*480/576;
		wY=nI;
	}
#endif

#ifdef SUBT_SHOW_ON_OSD
	if ((wYcbcr) != 0)
	{
		volatile u16 wDataTem;

		nI=2*(700*wY+wX);
		switch(wYcbcr&0x3)
		{
			case 0:
				wYcbcr|=0x3;
				break;
			case 1:
			case 2:
				wYcbcr=(wYcbcr&0xfffc)+2;
				break;
			case 3:
				wYcbcr=(wYcbcr&0xfffc)+1;
				break;
			default:
				break;
		}

#if (YCbCrCount==13)
		wDataTem=((wYcbcr&0xe000)>>3)+(wYcbcr&0x3ff);//Y:3;Cb:4;Cr4;
#endif
#if (YCbCrCount==14)
		wDataTem=((wYcbcr&0xf000)>>2)+(wYcbcr&0x3ff);//Y:4;Cb:4;Cr4;
#endif
#if (YCbCrCount==15)
		wDataTem=((wYcbcr&0xf800)>>1)+(wYcbcr&0x3ff);//Y:5;Cb:4;Cr4;
#endif
#if (YCbCrCount==16)
		wDataTem=wYcbcr;//Y:6;Cb:4;Cr4;
#endif
		* ((u16*)(g_SppBuffer+nI))=YCbCr2RGB[wDataTem];//全部不透明
	}

#else
#if 0
	if (wYcbcr != 0)
	{
		nI=2*(700*wY+wX);
		if((wYcbcr&0x300)==0)
		{
			wYcbcr|=0x300;
		}
		* ((u16*)(g_SppBuffer+nI))=wYcbcr;//|0x3;//全部不透明

	}
#else
	if (wYcbcr != 0)
	{
		uint16_t color;
		uint16_t t;
		//nI=2*(720*wY+wX);

		nI=2*(((com_sub.render->ops->get_rect(com_sub.render->handle))->width)*wY+wX);
		color = wYcbcr & 0xFCFF;
		t = wYcbcr & 0x0300;
		if (t == 0) {
			t = 0x0300;
		}
		else if(t == 0x0100)
		{
			t = 0x0200;
		}
		else if(t == 0x0200)
		{
			t = 0x0100;
		}
		else
		{
			t = 0x0;
		}
#define GX3113NRE
#ifndef GX3113NRE
		* ((u16*)(g_SppBuffer+nI)) = (*((u16*)(g_SppBuffer+nI))&0x300)|color;//|0x3;//全部不透明
		nI += 8;
		* ((u16*)(g_SppBuffer+nI)) |= t;//|0x3;//全部不透明
#else
		* ((u16*)(g_SppBuffer+nI)) = color | t;
#endif
	}
#endif
#endif
	return ;
}


#ifdef SUBT_SHOW_ON_OSD

static u32 head0;
static u32 head3;
static u32 head5;
void subt_chang_osd_buffer(void)
{
	u32 n_region_add;
	n_region_add=(*(volatile U32*)0x40005024)|0x80000000;
	head0 = *(volatile U32*)n_region_add;
	head3 = *(volatile U32*)(n_region_add+8);
	head5 = *(volatile U32*)(n_region_add+16);


	*(volatile U32*)n_region_add=0x7d9000;

	*(volatile U32*)n_region_add &= ~(1<<14);

	*(volatile U32*)(n_region_add+16)=(u32)g_SppBuffer&0x7fffffff;//*(volatile U32*)0x4000502c;
	*(volatile U32*)(n_region_add+8)=0x2bb0000;
}

void subt_recover_osd_buffer(void)
{
	u32 n_region_add;
	n_region_add=(*(volatile U32*)0x40005024)|0x80000000;
	*(volatile U32*)n_region_add=head0;
	*(volatile U32*)(n_region_add+16)=head5;
	*(volatile U32*)(n_region_add+8)=head3;
}
#endif

