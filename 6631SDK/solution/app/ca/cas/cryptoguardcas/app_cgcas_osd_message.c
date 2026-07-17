/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_cgcas_message.c
* Author    :   huangwf
* Project   :   CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION      Date            AUTHOR         Description
  1.0      2021.03.24        huangwf              Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_ui_porting.h"
#include "cascam_cryptoguardcas_data.h"

extern void cascam_ui_exit_msg_ex_dialog(void);
extern void cascam_ui_create_msg_ex_dialog(CascamUiNoticeClass *para, uint8_t force);
extern int32_t cascam_cgcas_queue_put(uint32_t type, void *buf, uint32_t size, int32_t timeout);

//static event_list  *thiz_osd_timeout_timer = NULL;
static char thiz_osd_message_buffer[CASCAM_CG_OSD_MSG_LEN];

/*
static int _cgcas_osd_timeout_timer(void *usrdata)
{
    cascam_ui_exit_msg_ex_dialog();
    return 0;
}*/

// for message notify
void app_cgcas_show_message_notify(void* p)
{
    static Cascam_CGShowOsdMessage cgcas_message;
    CascamUiNoticeClass ui_notice;

    if (NULL == p)
    {
        printf("%s,%d, data is NULL!\n", __func__, __LINE__);
        return ;
    }

    memset(&cgcas_message, 0, sizeof(Cascam_CGShowOsdMessage));
    memcpy(&cgcas_message, p, sizeof(Cascam_CGShowOsdMessage));

    memset(&ui_notice, 0, sizeof(CascamUiNoticeClass));
    ui_notice.timeout_sec = cgcas_message.msg_ms_duration / 1000;
    memset(thiz_osd_message_buffer, 0, CASCAM_CG_OSD_MSG_LEN);
    ui_notice.str = (char *)cgcas_message.msg_textstring;

    cascam_ui_exit_msg_ex_dialog();
    cascam_ui_create_msg_ex_dialog(&ui_notice, 0);
/*
    if (cgcas_message.msg_ms_duration != 0)
    {
        thiz_osd_timeout_timer = cascam_create_timer(_cgcas_osd_timeout_timer, cgcas_message.msg_ms_duration, NULL, TIMER_ONCE);
    }
*/
}

void app_cgcas_hide_message_notify(void* p)
{
    if (NULL == p)
    {
        printf("%s,%d, data is NULL!\n", __func__, __LINE__);
        return ;
    }

    cascam_ui_exit_msg_ex_dialog();
}

void test_osd_message(void)
{
    #define MAX_MESSAGE_COUNT   5
    Cascam_CGShowOsdMessage osd_info;
    static int array_index = 0;
    unsigned char encoding_list[] = {1, 2, 5, 6, 8};

    unsigned char message_str_10[MAX_MESSAGE_COUNT][64] =
    {
        /* iso 8859-1 */
        {
            0x51, 0x75, 0x65, 0x73, 0x74, 0x6f, 0x20, 0xe8, 0x20, 0x75, 0x6e, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x67, 0x69, 0x6f, 0x20, 0x64, 0x69, 0x20, 0x70, 0x72, 0x6f, 0x76, 0x61
        },
        /* iso 8859-2 */
        {
            0x54, 0x6f, 0x74, 0x6f, 0x20, 0x6a, 0x65, 0x20, 0x7a, 0x6b, 0x75, 0xb9, 0x65, 0x62, 0x6e, 0xed, 0x20, 0x7a, 0x70, 0x72, 0xe1, 0x76, 0x61
        },
        /* iso 8859-5 */
        {
            0x01, 0xcd, 0xe2, 0xde, 0x20, 0xe2, 0xd5, 0xe1, 0xe2, 0xde, 0xd2, 0xde, 0xd5, 0x20, 0xe1, 0xde, 0xde, 0xd1, 0xe9, 0xd5, 0xdd, 0xd8, 0xd5, 0x00, 0x76, 0x61
        },
        /* iso 8859-6 */
        {
            0x02, 0xe7, 0xd0, 0xe7, 0x20, 0xd1, 0xd3, 0xc7, 0xe4, 0xc9, 0x20, 0xc7, 0xce, 0xca, 0xc8, 0xc7, 0xd1
        },
        /* iso 8859-8 */
        {
            0x04, 0xe6, 0xe5, 0xe4, 0xe9, 0x20, 0xe4, 0xe5, 0xe3, 0xf2, 0xfa, 0x20, 0xe1, 0xe3, 0xe9, 0xf7, 0xe4, 0x00, 0xed, 0xf5, 0xec, 0xe1, 0x20, 0xe4, 0xef, 0xea, 0xe9, 0xec, 0xde, 0xf2
        }
    };

    memset(&osd_info, 0, sizeof(Cascam_CGShowOsdMessage));
    osd_info.encoding = encoding_list[array_index];
    osd_info.msg_ms_duration = 50000;
    osd_info.msg_length = sizeof(message_str_10[array_index]);
    memset(osd_info.msg_textstring, 0, 128);
    memcpy(osd_info.msg_textstring, message_str_10[array_index], sizeof(message_str_10[array_index]));
    if (array_index == 1 /* MAX_MESSAGE_COUNT - 1 */)
        array_index = 0;
    else
        array_index++;

    if (cascam_cgcas_queue_put(CASCAM_CGCAS_OSD_MESSAGE, (void *)&osd_info, sizeof(Cascam_CGShowOsdMessage), 0) != 0)
        printf("CASCAM_MSG  CASCAM_CGCAS_OSD_MESSAGE Error\n");

    return;
}

#endif

