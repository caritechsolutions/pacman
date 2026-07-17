/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_ait.c
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
#include <string.h>
#include "include/si_parse_lib_private.h"

static si_parse_lib_protect protect = {0};
static handle_t si_parse_lib_protect_mutex = 0;
/******************function******************************/

static void si_parse_lib_ait_release(GxSubsystemSiParseAIT* ait);
static void si_parse_lib_ait_add_protect(void* p);
static uint32_t si_parse_lib_ait_remove_protect(void* p);
static int32_t si_parse_lib_ait_parse_loop1(GxSubsystemSiParseAIT* ait,uint8_t* src,uint32_t len); 
static int32_t si_parse_lib_ait_parse_loop2(GxSubsystemSiParseAIT* ait,uint8_t* src,uint32_t len); 
static int32_t si_parse_lib_ait_parse_loop2_des(GxSubsystemSiParseAitLoop2* loop2,uint8_t* src,uint32_t len);
static int32_t si_parse_lib_ait_name(GxSubsystemSiParseApplicationNameDes* name,uint8_t* src,uint32_t len);
static int32_t si_parse_lib_ait_dvb_j_app(GxSubsystemSiParseDvbJApplicationDes* dvb_j_app,uint8_t* src,uint32_t len);
static void si_parse_lib_ait_release_loop2(GxSubsystemSiParseAitLoop2* loop2);

static void si_parse_lib_ait_release_loop2(GxSubsystemSiParseAitLoop2* loop2)
{
    uint32_t i = 0;
    uint32_t j = 0;
    if(loop2)
    {
        if(loop2->application)
        {
            for(i=0; i<loop2->application_count; i++)
            {
                if(loop2->application[i].profile)
                {
                    GxCore_Free(loop2->application[i].profile);
                }
            }
            GxCore_Free(loop2->application);
        }
        if(loop2->name)
        {
            for(i=0; i<loop2->name_count; i++)
            {
                if(loop2->name[i].name)
                {
                    for(j=0; j<loop2->name[i].application_name_count; j++)
                    {
                        if(loop2->name[i].name[j].name)
                        {
                            GxCore_Free(loop2->name[i].name[j].name);
                        }
                    }
                    GxCore_Free(loop2->name[i].name);
                }
            }
            GxCore_Free(loop2->name);
        }
        if(loop2->icons)
        {
            for(i=0; i<loop2->icons_count; i++)
            {
                if(loop2->icons[i].icon_locator)
                {
                    GxCore_Free(loop2->icons[i].icon_locator);
                }
                if(loop2->icons[i].reserved_future_use)
                {
                    GxCore_Free(loop2->icons[i].reserved_future_use);
                }
            }
            GxCore_Free(loop2->icons);
        }
        if(loop2->transport_protocol)
        {
            GxCore_Free(loop2->transport_protocol);
        }
        if(loop2->dvb_j_application)
        {
            for(i=0; i<loop2->dvb_j_application_count; i++)
            {
                if(loop2->dvb_j_application[i].parameter)
                {
					for(j=0; j<loop2->dvb_j_application[i].parameter_count; j++)
					{
						if(loop2->dvb_j_application[i].parameter[j].parameter)
						{
							GxCore_Free(loop2->dvb_j_application[i].parameter[j].parameter);
						}
					}
					GxCore_Free(loop2->dvb_j_application[i].parameter);
                }
            }
            GxCore_Free(loop2->dvb_j_application);
        }
        if(loop2->dvb_j_application_location)
        {
            for(i=0; i<loop2->dvb_j_application_location_count; i++)
            {
                if(loop2->dvb_j_application_location[i].base_directory)
                {
                    GxCore_Free(loop2->dvb_j_application_location[i].base_directory);
                }
				if(loop2->dvb_j_application_location[i].initial_class)
				{
					GxCore_Free(loop2->dvb_j_application_location[i].initial_class);
				}
            }
            GxCore_Free(loop2->dvb_j_application_location);
        }
	}
    return;
}
/*释放ait的空间*/
static void si_parse_lib_ait_release(GxSubsystemSiParseAIT* ait)
{
    uint32_t i = 0;    
    if(ait != NULL)
    {
        if(ait->loop1.transport_protocol != NULL)
        {
            GxCore_Free(ait->loop1.transport_protocol);
        }
        if(ait->loop2)
        {
            for(i=0; i<ait->loop2_count; i++)
            {
                si_parse_lib_ait_release_loop2(&(ait->loop2[i]));
            }
            GxCore_Free(ait->loop2);
        }
        GxCore_Free(ait);
    }
    return;
}

static void si_parse_lib_ait_add_protect(void* p)
{
    if(si_parse_lib_protect_mutex == 0)
    {
	    GxCore_MutexCreate(&si_parse_lib_protect_mutex);
    }

    GxCore_MutexLock(si_parse_lib_protect_mutex);
    si_parse_lib_add_protect(&protect,p);
    GxCore_MutexUnlock(si_parse_lib_protect_mutex);

    return;
}

static uint32_t si_parse_lib_ait_remove_protect(void* p)
{
    uint32_t ret = 0;
    if(si_parse_lib_protect_mutex == 0)
    {
	    GxCore_MutexCreate(&si_parse_lib_protect_mutex);
    }

    GxCore_MutexLock(si_parse_lib_protect_mutex);
    ret = si_parse_lib_remove_protect(&protect,p);
    GxCore_MutexUnlock(si_parse_lib_protect_mutex);

    return ret;
}
static int32_t si_parse_lib_ait_parse_loop1(GxSubsystemSiParseAIT* ait,uint8_t* src,uint32_t len) 
{
    int32_t size = len;
    uint32_t transport_protocol_count = 0;
    uint8_t* p = src;
    uint32_t descriptor_length = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_TRANSPORT_PROTOCOL_DES :
                transport_protocol_count++;
                ait->loop1.transport_protocol = GxCore_Realloc(ait->loop1.transport_protocol,sizeof(GxSubsystemSiParseTransportProtocolDes)*transport_protocol_count);
                if(ait->loop1.transport_protocol == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
                    goto err;
                }      
                ait->loop1.transport_protocol[transport_protocol_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                ait->loop1.transport_protocol[transport_protocol_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                ait->loop1.transport_protocol[transport_protocol_count-1].protocol_id = TODATA0_15BIT(p[2],p[3]);
                ait->loop1.transport_protocol[transport_protocol_count-1].protocol_label = TODATA0_7BIT(p[4]);
				if(ait->loop1.transport_protocol[transport_protocol_count-1].protocol_id == 1)//oc
				{
					ait->loop1.transport_protocol[transport_protocol_count-1].oc.remote_connection = TODATA7BIT(p[5]);
					if(ait->loop1.transport_protocol[transport_protocol_count-1].oc.remote_connection == 1)
					{
						ait->loop1.transport_protocol[transport_protocol_count-1].oc.original_network_id = TODATA0_15BIT(p[6],p[7]);
						ait->loop1.transport_protocol[transport_protocol_count-1].oc.transport_stream_id = TODATA0_15BIT(p[8],p[9]);
						ait->loop1.transport_protocol[transport_protocol_count-1].oc.service_id = TODATA0_15BIT(p[10],p[11]);
						ait->loop1.transport_protocol[transport_protocol_count-1].oc.component_tag = TODATA0_7BIT(p[12]);
					}
					else
					{
						ait->loop1.transport_protocol[transport_protocol_count-1].oc.component_tag = TODATA0_7BIT(p[6]);
					}
				}
				else if(ait->loop1.transport_protocol[transport_protocol_count-1].protocol_id == 2)//ip
				{
					;
				}
                p += 2;//跳过头
                p += ait->loop1.transport_protocol[transport_protocol_count-1].descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= ait->loop1.transport_protocol[transport_protocol_count-1].descriptor_length;
                break;

            default:
                descriptor_length = TODATA0_7BIT(p[1]);
                p += 2;//跳过头
                p += descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= descriptor_length;
                break;
        }
    }
    ait->loop1.transport_protocol_count = transport_protocol_count;
    return size;

err:
    ait->loop1.transport_protocol_count = transport_protocol_count;
    return -1;
}
static int32_t si_parse_lib_ait_name(GxSubsystemSiParseApplicationNameDes* name,uint8_t* src,uint32_t len)
{
    uint32_t descriptor_count = 0;
    uint8_t* p = src;
    int32_t size = len;
    while(size > 0)
    {
        descriptor_count++;
        name->name = GxCore_Realloc(name->name,
                sizeof(GxSubsystemSiParseApplicationNameInfo)*descriptor_count);
        if(name->name == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse application_name no memery!!");
#endif  
            goto err;
        }      
        memcpy(name->name[descriptor_count-1].language,p,3);
        name->name[descriptor_count-1].name_length = TODATA0_7BIT(p[3]);
		name->name[descriptor_count-1].name = NULL;
		if(name->name[descriptor_count-1].name_length != 0)
		{
			name->name[descriptor_count-1].name = GxCore_Malloc(name->name[descriptor_count-1].name_length);
			if(name->name[descriptor_count-1].name == NULL)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
				SI_PARSE_LIB_ERR_PRINTF("parse application_name no memery!!");
#endif  
				goto err;
			}
			memcpy(name->name[descriptor_count-1].name,
					&p[4],name->name[descriptor_count-1].name_length);
		}
		p += 4;//跳过该描述
		size -= 4;
		p += name->name[descriptor_count-1].name_length;//跳过该描述
		size -= name->name[descriptor_count-1].name_length;
	}
    name->application_name_count = descriptor_count;
    return size;
err:
    name->application_name_count = descriptor_count;
    return -1;
}
static int32_t si_parse_lib_ait_dvb_j_app(GxSubsystemSiParseDvbJApplicationDes* dvb_j_app,uint8_t* src,uint32_t len)
{
    uint32_t descriptor_count = 0;
    uint8_t* p = src;
    int32_t size = len;
    while(size > 0)
    {
        descriptor_count++;
        dvb_j_app->parameter = GxCore_Realloc(dvb_j_app->parameter,
                sizeof(GxSubsystemSiParseParameter)*descriptor_count);
        if(dvb_j_app->parameter == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse dvb_j_application no memery!!");
#endif  
            goto err;
        }      
        dvb_j_app->parameter[descriptor_count-1].parameter_length = TODATA0_7BIT(p[0]);
		dvb_j_app->parameter[descriptor_count-1].parameter = NULL;
		if(dvb_j_app->parameter[descriptor_count-1].parameter_length != 0)
		{
			dvb_j_app->parameter[descriptor_count-1].parameter = GxCore_Malloc(dvb_j_app->parameter[descriptor_count-1].parameter_length);
			if(dvb_j_app->parameter[descriptor_count-1].parameter == NULL)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
				SI_PARSE_LIB_ERR_PRINTF("parse dvb j application no memery!!");
#endif  
				goto err;
			}
			memcpy(dvb_j_app->parameter[descriptor_count-1].parameter,
					&p[1],dvb_j_app->parameter[descriptor_count-1].parameter_length);
		}
		p += 1;//跳过该描述
		size -= 1;
		p += dvb_j_app->parameter[descriptor_count-1].parameter_length;//跳过该描述
		size -= dvb_j_app->parameter[descriptor_count-1].parameter_length;
	}
    dvb_j_app->parameter_count = descriptor_count;
    return size;
err:
    dvb_j_app->parameter_count = descriptor_count;
    return -1;
}
static int32_t si_parse_lib_ait_parse_loop2_des(GxSubsystemSiParseAitLoop2* loop2,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint32_t application_count = 0;
    uint32_t name_count = 0;
    uint32_t icons_count = 0;
    uint32_t transport_protocol_count = 0;
	uint32_t dvb_j_application_count = 0;
	uint32_t dvb_j_application_location_count = 0;
    uint8_t* p = src;
    uint32_t descriptor_length = 0;
    uint32_t base_directory_length = 0;
    uint32_t classpath_extension_length = 0;
    uint32_t initial_class_length = 0;
	int32_t ret = 0;
	uint32_t i = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_APPLICATION_DES:
                application_count++;
                loop2->application = GxCore_Realloc(loop2->application,sizeof(GxSubsystemSiParseApplicationDes)*application_count);
                if(loop2->application == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif  
                    goto err;
                }      
                loop2->application[application_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->application[application_count-1].descriptor_length = TODATA0_7BIT(p[1]);
				loop2->application[application_count-1].application_profiles_length = TODATA0_7BIT(p[2]);
                loop2->application[application_count-1].profie_count = loop2->application[application_count-1].application_profiles_length/40;
				loop2->application[application_count-1].profile = NULL;
				p += 3;//跳过头
				size -= 3;
				if(loop2->application[application_count-1].profie_count != 0)
				{
					loop2->application[application_count-1].profile = GxCore_Malloc(sizeof(GxSubsystemSiParseApplicationProfile)*loop2->application[application_count-1].profie_count);
					if(loop2->application[application_count-1].profile == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
						goto err;
					}
					for(i=0; i<loop2->application[application_count-1].profie_count; i++)
					{
						loop2->application[application_count-1].profile[i].application_profile = TODATA0_15BIT(p[0],p[1]);
						loop2->application[application_count-1].profile[i].version_major = TODATA0_7BIT(p[2]);
						loop2->application[application_count-1].profile[i].version_minor = TODATA0_7BIT(p[3]);
						loop2->application[application_count-1].profile[i].version_micro = TODATA0_7BIT(p[4]);
						p += 5;//跳过这次profie描述
						size -= 5;
					}
				}
				loop2->application[application_count-1].service_bound_flag = TODATA7BIT(p[0]);
				loop2->application[application_count-1].visibility = TODATA5_6BIT(p[0]);
				loop2->application[application_count-1].application_priority = TODATA0_7BIT(p[1]);
				descriptor_length = loop2->application[application_count-1].descriptor_length - 1 -loop2->application[application_count-1].application_profiles_length - 2;//transport_protocol_label的长度；
				loop2->application[application_count-1].transport_protocol_label_length = descriptor_length;
				loop2->application[application_count-1].transport_protocol_label= NULL;
				if(descriptor_length != 0 )
				{
					loop2->application[application_count-1].transport_protocol_label= GxCore_Malloc(descriptor_length);
					if(loop2->application[application_count-1].transport_protocol_label == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
						goto err;
					}
					memcpy(loop2->application[application_count-1].transport_protocol_label,&p[2],descriptor_length);
				}
				p += 2;
				size -= 2;
				p += descriptor_length;
				size -= descriptor_length;
                break;
            case SI_PARSE_LIB_APPLICATION_NAME_DES:
				name_count ++;
                loop2->name = GxCore_Realloc(loop2->name,sizeof(GxSubsystemSiParseApplicationNameDes)*name_count);
				memset((void*)(loop2->name)+sizeof(GxSubsystemSiParseApplicationNameDes)*(name_count-1),0,sizeof(GxSubsystemSiParseApplicationNameDes));
                if(loop2->name == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
                    goto err;
                }
                loop2->name[name_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->name[name_count-1].descriptor_length = TODATA0_7BIT(p[1]);
				loop2->name[name_count-1].name = NULL;
                ret = si_parse_lib_ait_name(&(loop2->name[name_count-1]),
                            &p[2],loop2->name[name_count-1].descriptor_length);
                if(ret < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse name err!!");
#endif  
                    goto err;
                }
                p += 2;//跳过头
                p += loop2->name[name_count-1].descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= loop2->name[name_count-1].descriptor_length;
                break;

            case SI_PARSE_LIB_APPLICATION_ICONS_DES:
				icons_count++;
                loop2->icons = GxCore_Realloc(loop2->icons,sizeof(GxSubsystemSiParseApplicationIconsDes)*icons_count);
                if(loop2->icons == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
                    goto err;
                }      
                loop2->icons[icons_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->icons[icons_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->icons[icons_count-1].icon_locator_length = TODATA0_7BIT(p[2]);
				loop2->icons[icons_count-1].icon_locator = NULL;
				loop2->icons[icons_count-1].reserved_future_use_length = 0;//不管future_use
				loop2->icons[icons_count-1].reserved_future_use = NULL;
				if(loop2->icons[icons_count-1].icon_locator_length != 0)
				{
					loop2->icons[icons_count-1].icon_locator = GxCore_Malloc(loop2->icons[icons_count-1].icon_locator_length);
					if(loop2->icons[icons_count-1].icon_locator == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
						goto err;
					}
					memcpy(loop2->icons[icons_count-1].icon_locator,&p[3],loop2->icons[icons_count-1].icon_locator_length);
					p += 3;//跳过头
					size -=3;
					p += loop2->icons[icons_count-1].icon_locator_length;
					size -= loop2->icons[icons_count-1].icon_locator_length;
					loop2->icons[icons_count-1].icon_flags = TODATA0_15BIT(p[0],p[1]);
					p += 2;
					size -= 2;
					p += loop2->icons[icons_count-1].descriptor_length - 1 - loop2->icons[icons_count-1].icon_locator_length - 2;//跳过剩余字节
					size -= loop2->icons[icons_count-1].descriptor_length - 1 - loop2->icons[icons_count-1].icon_locator_length - 2;//跳过剩余字节
				}
				else
				{
					p += 2;//跳过头
					p += loop2->icons[icons_count-1].descriptor_length;//跳过该tag剩余字节
					size -= 2;
					size -= loop2->icons[icons_count-1].descriptor_length;
				}
				break;

			case SI_PARSE_LIB_TRANSPORT_PROTOCOL_DES:
                transport_protocol_count++;
                loop2->transport_protocol = GxCore_Realloc(loop2->transport_protocol,sizeof(GxSubsystemSiParseTransportProtocolDes)*transport_protocol_count);
                if(loop2->transport_protocol == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
                    goto err;
                }      
                loop2->transport_protocol[transport_protocol_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->transport_protocol[transport_protocol_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->transport_protocol[transport_protocol_count-1].protocol_id = TODATA0_15BIT(p[2],p[3]);
                loop2->transport_protocol[transport_protocol_count-1].protocol_label = TODATA0_7BIT(p[4]);
				if(loop2->transport_protocol[transport_protocol_count-1].protocol_id == 1)//oc
				{
					loop2->transport_protocol[transport_protocol_count-1].oc.remote_connection = TODATA7BIT(p[5]);
					if(loop2->transport_protocol[transport_protocol_count-1].oc.remote_connection == 1)
					{
						loop2->transport_protocol[transport_protocol_count-1].oc.original_network_id = TODATA0_15BIT(p[6],p[7]);
						loop2->transport_protocol[transport_protocol_count-1].oc.transport_stream_id = TODATA0_15BIT(p[8],p[9]);
						loop2->transport_protocol[transport_protocol_count-1].oc.service_id = TODATA0_15BIT(p[10],p[11]);
						loop2->transport_protocol[transport_protocol_count-1].oc.component_tag = TODATA0_7BIT(p[12]);
					}
					else
					{
						loop2->transport_protocol[transport_protocol_count-1].oc.component_tag = TODATA0_7BIT(p[6]);
					}
				}
				else if(loop2->transport_protocol[transport_protocol_count-1].protocol_id == 2)//ip
				{
					;
				}
                p += 2;//跳过头
                p += loop2->transport_protocol[transport_protocol_count-1].descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= loop2->transport_protocol[transport_protocol_count-1].descriptor_length;
                break;

            case SI_PARSE_LIB_DVB_J_APPLICATION_DES:
				dvb_j_application_count++;
                loop2->dvb_j_application = GxCore_Realloc(loop2->dvb_j_application,sizeof(GxSubsystemSiParseDvbJApplicationDes)*dvb_j_application_count);
				memset(((void*)loop2->dvb_j_application)+sizeof(GxSubsystemSiParseDvbJApplicationDes)*(dvb_j_application_count-1),0,sizeof(GxSubsystemSiParseDvbJApplicationDes));
                if(loop2->dvb_j_application == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
                    goto err;
                }      
                loop2->dvb_j_application[dvb_j_application_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->dvb_j_application[dvb_j_application_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                ret = si_parse_lib_ait_dvb_j_app(&(loop2->dvb_j_application[dvb_j_application_count-1]),
                            &p[2],loop2->dvb_j_application[dvb_j_application_count-1].descriptor_length);
                if(ret < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse dvb_j_application err!!");
#endif  
                    goto err;
                }
                p += 2;//跳过头
                p += loop2->dvb_j_application[dvb_j_application_count-1].descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= loop2->dvb_j_application[dvb_j_application_count-1].descriptor_length;
                break;
            case SI_PARSE_LIB_DVB_J_APPLICATION_LOCATION_DES:
                dvb_j_application_location_count++;
                loop2->dvb_j_application_location = GxCore_Realloc(loop2->dvb_j_application_location,sizeof(GxSubsystemSiParseDvbJApplicationLocationDes)*dvb_j_application_location_count);
                if(loop2->dvb_j_application_location == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
                    goto err;
                }      
                loop2->dvb_j_application_location[dvb_j_application_location_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->dvb_j_application_location[dvb_j_application_location_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length = TODATA0_7BIT(p[2]); 
                loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory = NULL;
				if(loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length != 0)
				{
					loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory = GxCore_Malloc(loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length);
					if(loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
						goto err;
					}
					memcpy(loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory,&p[3],loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length);
				}
				p += 3;
				size -= 3;
				p += loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length;
				size -= loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length;
                loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length = TODATA0_7BIT(p[0]); 
				loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension = NULL;
				if(loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length !=0)
				{
					loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension = GxCore_Malloc(loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length);
					if(loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
						goto err;
					}
					memcpy(loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension,&p[1],loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length);
				}
				p += 1;
				size -= 1;
				p += loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length;
				size -= loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length;
				base_directory_length = loop2->dvb_j_application_location[dvb_j_application_location_count-1].base_directory_length;
				classpath_extension_length = loop2->dvb_j_application_location[dvb_j_application_location_count-1].class_path_extension_length;
				descriptor_length = loop2->dvb_j_application_location[dvb_j_application_location_count-1].descriptor_length;

				initial_class_length = descriptor_length-1-base_directory_length-1-classpath_extension_length;
				loop2->dvb_j_application_location[dvb_j_application_location_count-1].initial_class_length = initial_class_length;
				loop2->dvb_j_application_location[dvb_j_application_location_count-1].initial_class = NULL;
				if(initial_class_length != 0)
				{
					loop2->dvb_j_application_location[dvb_j_application_location_count-1].initial_class = GxCore_Malloc(initial_class_length);
					if(loop2->dvb_j_application_location[dvb_j_application_location_count-1].initial_class == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif  
						goto err;
					}
					memcpy(loop2->dvb_j_application_location[dvb_j_application_location_count-1].initial_class,&p[0],initial_class_length);
				}
				p += initial_class_length;
				size -= initial_class_length;
                break;
			
			default:
                descriptor_length = TODATA0_7BIT(p[1]);
                p += 2;//跳过头
                p += descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= descriptor_length;
                break;
        }
    }
    loop2->application_count = application_count;
    loop2->name_count = name_count;
    loop2->icons_count = icons_count;
    loop2->transport_protocol_count = transport_protocol_count;
    loop2->dvb_j_application_count = dvb_j_application_count;
    loop2->dvb_j_application_location_count = dvb_j_application_location_count;
    return size;

err:
    loop2->application_count = application_count;
    loop2->name_count = name_count;
    loop2->icons_count = icons_count;
    loop2->transport_protocol_count = transport_protocol_count;
    loop2->dvb_j_application_count = dvb_j_application_count;
    loop2->dvb_j_application_location_count = dvb_j_application_location_count;
    
    return -1;
}
static int32_t si_parse_lib_ait_parse_loop2(GxSubsystemSiParseAIT* ait,uint8_t* src,uint32_t len) 
{
    int32_t size = len;
    uint32_t loop2_count = 0;
    uint8_t* p = src;
    int32_t ret = 0;

    while(size > 0)
    {
        /*解析loop2头部*/
        loop2_count++;
        ait->loop2 = GxCore_Realloc(ait->loop2,sizeof(GxSubsystemSiParseAitLoop2)*loop2_count);
		memset((void*)(ait->loop2)+sizeof(GxSubsystemSiParseAitLoop2)*(loop2_count-1),0,sizeof(GxSubsystemSiParseAitLoop2));
        if(ait->loop2 == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif              
            goto err;
        }
        ait->loop2[loop2_count-1].application_identifier.organisation_id = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
        ait->loop2[loop2_count-1].application_identifier.application_id = TODATA0_15BIT(p[4],p[5]);
        ait->loop2[loop2_count-1].application_control_code = TODATA0_7BIT(p[6]);
        ait->loop2[loop2_count-1].application_descriptors_loop_length = TODATA0_11BIT(p[7],p[8]);
        p += 9;//跳过头部
        size -= 9;
        ret = si_parse_lib_ait_parse_loop2_des(&ait->loop2[loop2_count-1],p,ait->loop2[loop2_count-1].application_descriptors_loop_length);
        if(ret < 0 )
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse loop2 des err!!");
#endif              
            goto err;
        }
        p += ait->loop2[loop2_count-1].application_descriptors_loop_length;//跳过该tag
        size -= ait->loop2[loop2_count-1].application_descriptors_loop_length;
    }
    ait->loop2_count = loop2_count;
    return size;
err:
    ait->loop2_count = loop2_count;
    return -1;
}

/**
 *  @brief      把一段section数据解析成ait表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      ait数据
 *               NULL：发生错误
 */
GxSubsystemSiParseAIT* GxSubsystm_SiParseLibAitParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseAIT* ait = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
    int32_t ret = 0;
	
	if(section == NULL || size < 10)//ait的头部108字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    ait = GxCore_Malloc(sizeof(GxSubsystemSiParseAIT));
    if(ait == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse ait no memery!!");
#endif
        goto err;
    }
    memset(ait,0,sizeof(GxSubsystemSiParseAIT));
    ait->table_id = TODATA0_7BIT(p[0]);
    ait->section_syntax_indicator = TODATA7BIT(p[1]);
    ait->section_length = TODATA0_11BIT(p[1],p[2]);
    ait->application_type = TODATA0_15BIT(p[3],p[4]);
    ait->version_number = TODATA1_5BIT(p[5]);
    ait->current_next_indicator = TODATA0BIT(p[5]);
    ait->section_number = TODATA0_7BIT(p[6]);
    ait->last_section_number = TODATA0_7BIT(p[7]);
    ait->common_descriptors_length = TODATA0_11BIT(p[8],p[9]);

    p += 10;//跳过头部
    len -= 10;//头部解析完成
    len -= 4;//去掉crc

    if(len != ait->section_length-7-4 || 
		ait->section_length > 1024)//解析出来的大小和表里面的大小不一致,并且aitlength不超过1024
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif  
        goto err;
    }


    ret = si_parse_lib_ait_parse_loop1(ait,p,ait->common_descriptors_length);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop1 err!!");
#endif
        goto err;
    }
    p += ait->common_descriptors_length;//跳过第一个循环
    len -= ait->common_descriptors_length;

	ait->application_length = TODATA0_11BIT(p[0],p[1]);

	p +=2;
	len-=2;
	if(len != ait->application_length)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif
        goto err;
	}

    ret = si_parse_lib_ait_parse_loop2(ait,p,len);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop2 err!!");
#endif
        goto err;
    }
    si_parse_lib_ait_add_protect((void*)ait);
    return ait;
err:
   si_parse_lib_ait_release(ait);
    return NULL;
}

/**
 *  @brief      释放ait的空间，应用应该在通过GxSubsystm_SiParseLibAitParse获取
 *  解析好的ait数据之后某个时刻释放ait的空间。
 *   
 *  @param       GxSubsystemSiParseAIT* ait:ait的存储空间，是通过GxSubsystm_SiParseLibAitParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibAitRelease(void* ait)
{
    uint32_t ret = 0;
    ret = si_parse_lib_ait_remove_protect(ait);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseAIT* ait is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_ait_release((GxSubsystemSiParseAIT*)ait);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

