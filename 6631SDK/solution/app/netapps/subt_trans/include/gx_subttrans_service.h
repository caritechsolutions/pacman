#ifndef _GX_SUBTTRANS_SERVICE_H
#define _GX_SUBTTRANS_SERVICE_H
#include <gx_apps.h>

#define LANG639_STR_MAX (4)

typedef enum {
    ST_SUBT_TYPE_TTX = 0,
    ST_SUBT_TYPE_DVB = 1,
}GxST_SubtType_e;

typedef struct {
    GxST_SubtType_e subt_type;          //字幕类型，1=DVB, 0=Teletext
    uint16_t service_id;                //当前节目的service id
    uint16_t request_id;                //区分同时发送多条请求的场景
    char source_lang[LANG639_STR_MAX];  //原始字幕的语言类型
    char target_lang[LANG639_STR_MAX];  //翻译后的字幕语言类型
    uint8_t  *subt_data;                    //字幕PES数据
    uint32_t subt_len;
}GxST_TransParams_t;

typedef struct {
    char *trans_subt;    //翻译后的字幕数据，TXT
    double pts;          //字幕对应的pts
    uint16_t request_id;
}GxST_TransGet_t;

//翻译服务交互账户初始化设置
//传入参数： 1. oem_name: Factory name
//           2. brand_name: Client name
//           3. mac: 本机mac地址
//           4. model: 本机mac地址
//返回：    0：成功, 非0: 传入参数错误
gxapps_ret_t Gx_SubtTransSeviceInit(char *oem_name, char *brand_name, char *mac, char *model);

//登录字幕翻译服务器
//传入参数： code: 订阅代码，Subscription Code
/*  - code
    There will be three types of activation codes:
    1. 7-Day Free Trial:  "7777777777"
    2. Bulk Subscribers/Pre-Loaded MAC Lists
    3. Retail Subscription Code
*/
//返回参数： expiration_date: 订阅到期日期，当返回参数==0，该参数返回订阅到期时间
//返回：    0：成功，其他：失败
gxapps_ret_t Gx_SubtTransActivate(char *code, char **expiration_date);

//翻译字幕
//传入参数：GxST_TransParams_t *param
//返回：   0：成功，其他：失败
//          返回0， GxST_TransGet_t **get_trans获得翻译后数据。
gxapps_ret_t Gx_SubtTrans_Translation(GxST_TransParams_t *param, GxST_TransGet_t **get_trans);

//翻译后的字幕数据销毁
void Gx_SubtTrans_Translation_Free(GxST_TransGet_t *trans);

//销毁账户信息和登录信息
void Gx_SubtTransSeviceDestory(void);

//获取当前账户是否登录，登录成功返回0
gxapps_ret_t Gx_SubtTransActivate_Get_Status(char *expiration_date, int len);

//测试翻译接口，用户获得翻译是否正常，以及翻译时长
gxapps_ret_t Gx_SubtTransTest(void);
#endif
