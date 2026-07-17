#include "app.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam_cryptoguardcas_data.h"
#include "cascam.h"
#include "cascam_ui_porting.h"
#include "cascam_ui_common.h"
#include "gxcore.h"

extern int32_t app_ca_cascam_do_event_layer(uint32_t sub_id, void *para);
extern int32_t app_cascam_osd_send_message(uint32_t sub_id, void *para);
extern void app_cgcas_show_message_notify(void* p);
extern void app_cgcas_show_long_message_notify(void* p);
extern void app_cgcas_pin_required_notify(void* p);
extern void app_cgcas_forcetune(Cascam_CGDelivery_descriptor *info);
extern void app_cgcas_scroll_msg_notify(void *param);
extern void app_cgcas_finger_notify(void *para);

static void app_cgcas_event_thread_deal(void* arg)
{
	void *p = NULL;
	void *param = NULL;

	while (1)
    {
		uint32_t type;
		cascam_wait_event(&type, &param);
		if (type >= (CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_OSD_MESSAGE) &&
			type <= (CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_FORCETUNE))
		{
			p = GxCore_Malloc(CASCAM_CG_MSG_MAX_LEN);
			if(p == NULL){
				printf("%s,%d  --malloc error--\n", __func__, __LINE__);
				continue;
			}

			if(param !=NULL)
				memcpy(p, param, CASCAM_CG_MSG_MAX_LEN);

#if CASCAM_UI_MULTI_LAYER
			if(app_ca_cascam_do_event_layer(type,p)==1)
#endif
			{
				app_cascam_osd_send_message(type, p);
			}
		}else{
			printf("[MSG]Not Find Sub Id: %d !\n", type);
			if(param !=NULL){
				GxCore_Free(param);
				param = NULL;
			}
		}
		if(param !=NULL){
			GxCore_Free(param);
			param = NULL;
		}
		//GxCore_ThreadDelay(200);
	}
}

void app_cgcas_event_task(void)
{
	int thread = 0;
    GxCore_ThreadCreate("app_cgcas_event", &thread, app_cgcas_event_thread_deal, NULL, 32 * 1024, GXOS_DEFAULT_PRIORITY);
}

int32_t app_cgcas_do_event(CascamOsdEvent* para)
{
    uint32_t sub_id = 0;
    if(para == NULL){
        printf("%s,%d, event para is NULL!\n", __func__, __LINE__);
        return -1;
    }
    sub_id = para->msg_id - CASCAM_MSG_CGCAS_BASEID;
    switch (sub_id) {
        case CASCAM_CGCAS_FINGER_INFO:
        case CASCAM_CGCAS_SCROLL_MESSAGE:
            return 1;
        case CASCAM_CGCAS_OSD_MESSAGE:
            {
                Cascam_CGShowOsdMessage *osd_msg = (Cascam_CGShowOsdMessage*)para->data;

                app_cgcas_show_message_notify(osd_msg);
                break;
            }
        case CASCAM_CGCAS_LONG_MESSAGE:
            {
                Cascam_CGShowLongMessage *long_msg = (Cascam_CGShowLongMessage*)para->data;

                printf("%s[%d] CASCAM_CGCAS_LONG_MESSAGE\n", __func__, __LINE__);
                app_cgcas_show_long_message_notify(long_msg);
                break;
            }
        case CASCAM_CGCAS_VIDEO_RULES:
            {
                printf("%s[%d] CASCAM_CGCAS_VIDEO_RULES\n", __func__, __LINE__);
                //Cascam_CGVideoRules *rules = (Cascam_CGVideoRules*)para->data;
                //Play ts: NationalChip_810000001_fingerprint_parental.ts [TV:SVT1 Norrbotten] can show here
                //结构体中数据内容，串口打印上有显示，供参考
                break;
            }
        case CASCAM_CGCAS_OTA_TRIGGER:
            {
                //TODO
                printf("%s[%d] CASCAM_CGCAS_OTA_TRIGGER\n", __func__, __LINE__);
                //No TS has trigger
                break;
            }
        case CASCAM_CGCAS_PIN_REQUIRED:
            {
                printf("%s[%d] CASCAM_CGCAS_PIN_REQUIRED\n", __func__, __LINE__);
                Cascam_CGPinRequired *req = (Cascam_CGPinRequired*)para->data;

                app_cgcas_pin_required_notify(req);
                //Play 3 TS all has this, but should set MATURITY_RATING < 15 can show this msg;
                break;
            }
        case CASCAM_CGCAS_FORCETUNE:
            {
                printf("%s[%d] CASCAM_CGCAS_FINGER_INFO\n", __func__, __LINE__);
                Cascam_CGDelivery_descriptor *info = (Cascam_CGDelivery_descriptor*)para->data;

                app_cgcas_forcetune(info);
                break;
            }
        case CASCAM_CGCAS_CARD_IN:
            {
                printf("%s[%d] CASCAM_CGCAS_CARD_IN\n", __func__, __LINE__);
				PopDlg pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_NO_BTN;
				pop.format = POP_FORMAT_DLG;
				pop.str = "Smartcard in.";
				pop.mode = POP_MODE_UNBLOCK;
				pop.create_cb = NULL;
				pop.exit_cb = NULL;
				pop.timeout_sec = 3;
				popdlg_create(&pop);


                break;
            }
        case CASCAM_CGCAS_CARD_OUT:
            {
                printf("%s[%d] CASCAM_CGCAS_CARD_OUT\n", __func__, __LINE__);
		        PopDlg pop;
		        memset(&pop, 0, sizeof(PopDlg));
		        pop.type = POP_TYPE_NO_BTN;
		        pop.format = POP_FORMAT_DLG;
		        pop.str = "Smartcard out.";
		        pop.mode = POP_MODE_UNBLOCK;
		        pop.create_cb = NULL;
		        pop.exit_cb = NULL;
		        pop.timeout_sec = 3;
		        popdlg_create(&pop);
                break;
            }
        default:
            break;
    }
    if(para->data != NULL)
        GxCore_Free(para->data);
    return 0;
}

#if CASCAM_UI_MULTI_LAYER
int32_t app_cgcas_do_event_layer(CascamOsdEvent* para)
{
    uint32_t sub_id = 0;
    if(para == NULL){
        printf("%s,%d, event para is NULL!", __func__, __LINE__);
        return -1;
    }
    sub_id = para->msg_id - CASCAM_MSG_CGCAS_BASEID;
    switch(sub_id)
    {
        case CASCAM_CGCAS_OSD_MESSAGE:
        case CASCAM_CGCAS_LONG_MESSAGE:
        case CASCAM_CGCAS_VIDEO_RULES:
        case CASCAM_CGCAS_OTA_TRIGGER:
        case CASCAM_CGCAS_PIN_REQUIRED:
        case CASCAM_CGCAS_FORCETUNE:
        case CASCAM_CGCAS_CARD_IN:
        case CASCAM_CGCAS_CARD_OUT:
            return 1;
        case CASCAM_CGCAS_SCROLL_MESSAGE:
            {
                Cascam_CGScrollMessage *scroll_msg = (Cascam_CGScrollMessage*)para->data;

                printf("%s[%d] CASCAM_CGCAS_SCROLL_MESSAGE\n", __func__, __LINE__);
                app_cgcas_scroll_msg_notify(scroll_msg);
                break;
            }
        case CASCAM_CGCAS_FINGER_INFO:
            {
                Cascam_CGFingerInfo *info = (Cascam_CGFingerInfo*)para->data;

                printf("%s[%d] CASCAM_CGCAS_FINGER_INFO\n", __func__, __LINE__);
                app_cgcas_finger_notify(info);
                break;
            }
        default:
            break;

    }
    if(para->data != NULL)
        GxCore_Free(para->data);
    return 0;
}
#endif
#endif
