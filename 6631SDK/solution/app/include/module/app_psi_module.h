#ifndef __APP_PSI_MODULE_H
#define __APP_PSI_MODULE_H

#include "app_config.h"

#if PSI_MODULE_SUPPORT
#include "dvbhal/gxdemux_hal.h"

typedef GxDmxFilterParams psi_module_filter;

typedef enum
{
    PSI_MODULE_SECTION_OK,
    PSI_MODULE_TABLE_OK
}psi_module_status;


typedef enum parser_psi_mode
{
    PSI_MODULE_WITH_STANDARD,   // 标准解析和私有解析
    PSI_MODULE_PRIVATE_ONLY,    //私有解析
    PSI_MODULE_STANDARD_ONLY,
    PSI_MODULE_SECTION_ONLY      //获取原始数据
}psi_module_mode;


/*
    参数1：解析完的数据
    参数2: 解析状态 PSI_MODULE_TABLE_OK ; PSI_MODULE_SECTION_OK
   */
typedef status_t (*psi_module_solved)(void*,psi_module_status);

/*
   参数1：原始数据段
   参数2: 原始数据长度
   返回解析状态
*/
typedef psi_module_status (*psi_module_private)(uint8_t*,uint32_t);


/*
   参数1：filter_parse_module返回的id.
*/
typedef int (*psi_module_timeout)(uint8_t);

typedef struct table_control_node
{
    int timeout;                       //超时重启
    bool no_stop;                      //过滤完表是否退出模块
    bool independent_thread;             //独立线程处理
    bool no_same;                       //丢弃相同数据表
    psi_module_mode mode;          //解析模式
    psi_module_private parse_priv_cb;  //私有解析
    psi_module_solved parse_solv_cb;      //数据处理
    psi_module_timeout parse_timeout_cb; //超时处理
}psi_module_node;

typedef struct table_parse_config
{
    uint16_t    demux_id;
    psi_module_filter params;        //filter参数
    psi_module_node node;
}psi_module_config;

void app_psi_module_init(void);
void app_psi_module_destroy(void);
int  app_psi_module_start(psi_module_config config);
void app_psi_module_stop(uint8_t id);
int  app_psi_module_reset(uint8_t id);
void app_psi_module_modify(uint8_t id ,psi_module_filter params);
#endif
#endif
