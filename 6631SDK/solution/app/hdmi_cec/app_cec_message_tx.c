#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app_cec_private.h"
#include "app_cec_message_tx.h"
#include "app_cec_message_rx.h"
#include "app_cec_common_str.h"
#include "app_cec_config.h"
#include "av/hal/gxav_hal_vout.h"
#include "module/app_log.h"

int tx_cec_msg_image_view_on(void)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

	GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_TV;
	cec_msg_write.message[0] = OPCODE_IMAGE_VIEW_ON;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", CEC_LA_TV, cec_opcode_str[OPCODE_IMAGE_VIEW_ON]);	
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_text_view_on(void)
{
	GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

	GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_TV;
	cec_msg_write.message[0] = OPCODE_TEXT_VIEW_ON;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", CEC_LA_TV, cec_opcode_str[OPCODE_TEXT_VIEW_ON]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_active_source(void)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

	GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_BROADCAST;
	cec_msg_write.message[0] = OPCODE_ACTIVE_SOURCE;
	cec_msg_write.message[1] = (unsigned char) (cec_addr.physical_addr>>8);
	cec_msg_write.message[2] = (unsigned char) (cec_addr.physical_addr&0xFF);
	cec_msg_write.message_len = 4;
    app_log_debug("To(0x%X): <%s> [Phy Addr=0x%.2X%.2X]\n", CEC_LA_BROADCAST,
       cec_opcode_str[OPCODE_ACTIVE_SOURCE], cec_msg_write.message[1], cec_msg_write.message[2]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_request_active_source(void)
{
	GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

	GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_BROADCAST;
	cec_msg_write.message[0] = OPCODE_REQUEST_ACTIVE_SOURCE;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", CEC_LA_BROADCAST, cec_opcode_str[OPCODE_REQUEST_ACTIVE_SOURCE]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_cec_version(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_CEC_VERSION;
	cec_msg_write.message[1] = CEC_VER_1_4A;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [CEC Ver=0x%02X]\n", dest_address, cec_opcode_str[OPCODE_CEC_VERSION], cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_get_cec_version(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GET_CEC_VERSION;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GET_CEC_VERSION]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_physical_address(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GIVE_PHYSICAL_ADDR;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GIVE_PHYSICAL_ADDR]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_report_physical_address(void)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
    memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_BROADCAST;
	cec_msg_write.message[0] = OPCODE_REPORT_PHYSICAL_ADDR;
	cec_msg_write.message[1] = (unsigned char)(cec_addr.physical_addr>>8);
	cec_msg_write.message[2] = (unsigned char)(cec_addr.physical_addr&0xFF);
	cec_msg_write.message[3] = CEC_DEV_TUNER;
	cec_msg_write.message_len = 5;
    app_log_debug("To(0x%X): <%s> [Phy Addr=0x%02x%02x][Device Type=0x%.2x]\n", CEC_LA_BROADCAST,
       cec_opcode_str[OPCODE_REPORT_PHYSICAL_ADDR], cec_msg_write.message[1], cec_msg_write.message[2], cec_msg_write.message[3]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_get_menu_language(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GET_MENU_LANGUAGE;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GET_MENU_LANGUAGE]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_set_menu_language(E_CEC_LOGIC_ADDR dest_address, P_CEC_MENU_LANGUAGE pmenulang)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    if (NULL == pmenulang)
    {
        return -1;
    }
    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_SET_MENU_LANGUAGE;
	cec_msg_write.message[1] = pmenulang->lang[0];
	cec_msg_write.message[2] = pmenulang->lang[1];
	cec_msg_write.message[3] = pmenulang->lang[2];
	cec_msg_write.message_len = 5;
    app_log_debug("To(0x%X): <%s> [Lang=0x%02X%02X%02X(\"%c%c%c\")]\n", dest_address,
       cec_opcode_str[OPCODE_SET_MENU_LANGUAGE], cec_msg_write.message[1], cec_msg_write.message[2], cec_msg_write.message[3], cec_msg_write.message[1], cec_msg_write.message[2], cec_msg_write.message[3]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_tuner_device_status(E_CEC_LOGIC_ADDR dest_address, P_CEC_STATUS_REQUEST status_request)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    if (NULL == status_request)
    {
        return -1;
    }
    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GIVE_TUNER_DEVICE_STATUS;
	cec_msg_write.message[1] = status_request->request_type;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [Status Req=%d]\n", dest_address,
       cec_opcode_str[OPCODE_GIVE_TUNER_DEVICE_STATUS], cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_tuner_device_status(unsigned char dest_address, P_CEC_TUNER_DEVICE_INFO tuner_device_info)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    if (NULL == tuner_device_info)
    {
        return -1;
    }
    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_TUNER_DEVICE_STATUS;
	memcpy(&cec_msg_write.message[1], tuner_device_info, sizeof(CEC_TUNER_DEVICE_INFO));
    /* Little-Endian to Big-Endian */
	CEC_SWAP8(cec_msg_write.message+3);
    CEC_SWAP8(cec_msg_write.message+5);
    CEC_SWAP8(cec_msg_write.message+7);
	cec_msg_write.message_len = 10;
    app_log_debug("To(0x%X): <%s> [Tuner Dev Info=0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X]\n",
       dest_address, cec_opcode_str[OPCODE_TUNER_DEVICE_STATUS], cec_msg_write.message[1], cec_msg_write.message[2], cec_msg_write.message[3], cec_msg_write.message[4], cec_msg_write.message[5],
       cec_msg_write.message[6], cec_msg_write.message[7], cec_msg_write.message[8]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_tuner_step_increment(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_TUNER_STEP_INCREMENT;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_TUNER_STEP_INCREMENT]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_tuner_step_decrement(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_TUNER_STEP_DECREMENT;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_TUNER_STEP_DECREMENT]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_device_vendor_id(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GIVE_DEVICE_VENDOR_ID;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GIVE_DEVICE_VENDOR_ID]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_device_vendor_id(void)
{
    GxHdmiCecAddr cec_addr = {0};
    unsigned int vid = 0;
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
    vid = app_cec_get_device_vendor_id();
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_BROADCAST;
	cec_msg_write.message[0] = OPCODE_DEVICE_VENDOR_ID;
	cec_msg_write.message[1] = (vid>>16)&0xFF;
    cec_msg_write.message[2] = (vid>>8)&0xFF;
    cec_msg_write.message[3] = (vid)&0xFF;
	cec_msg_write.message_len = 5;
    app_log_debug("To(0x%X): <%s> [Ven ID=0x%06X]\n", CEC_LA_BROADCAST, cec_opcode_str[OPCODE_DEVICE_VENDOR_ID],vid);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_set_osd_string(E_CEC_LOGIC_ADDR dest_address, P_CEC_OSD_STRING osd_string)
{
    unsigned char    str_len = 0;
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    if (NULL == osd_string)
    {
        return -1;
    }
    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
    str_len = strlen((const char *)osd_string->string);
    if (str_len > CEC_OSD_STRING_LEN)
    {
        str_len= CEC_OSD_STRING_LEN;
    }
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|CEC_LA_BROADCAST;
	cec_msg_write.message[0] = OPCODE_SET_OSD_STRING;
	cec_msg_write.message[1] = osd_string->display_control;
	strncpy((char *)&cec_msg_write.message[2], (const char *)osd_string->string, str_len);
    cec_msg_write.message[15] ='\0';    // for Debug Printf only
	cec_msg_write.message_len = 3 + str_len;
    app_log_debug("To(0x%X): <%s> [Display Ctrl=0x%02X][OSD String=\"%s\"]\n", CEC_LA_TV,
       cec_opcode_str[OPCODE_SET_OSD_STRING], cec_msg_write.message[1], &cec_msg_write.message[2]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_osd_name(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GIVE_OSD_NAME;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GIVE_OSD_NAME]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_set_osd_name(E_CEC_LOGIC_ADDR dest_address, P_CEC_OSD_NAME osd_name)
{
    unsigned char    str_len = 0;
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    if (NULL == osd_name)
    {
        return -1;
    }
    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
    str_len = strlen((const char *)osd_name->name);
    if (CEC_OSD_NAME_LEN < str_len)
    {
        str_len = CEC_OSD_NAME_LEN;
    }
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_SET_OSD_NAME;
	strncpy((char *)&cec_msg_write.message[1],(const char *)osd_name->name, str_len);
    cec_msg_write.message[15] ='\0';    // for Debug Printf only
	cec_msg_write.message_len = 2 + str_len;
    app_log_debug("To(0x%X): <%s> [OSD Name=\"%s\"]\n", dest_address, cec_opcode_str[OPCODE_SET_OSD_NAME], &cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_menu_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_MENU_STATE menu_state)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_MENU_STATUS;
	cec_msg_write.message[1] = menu_state;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [Menu State=%s]\n", dest_address, cec_opcode_str[OPCODE_MENU_STATUS],
       ((menu_state==CEC_MENU_STATE_ACTIVATE) ? "[Actived]" : "[Deactived]"));
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_device_power_status(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|OPCODE_GIVE_POWER_STATUS;
	cec_msg_write.message[0] = OPCODE_MENU_STATUS;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GIVE_POWER_STATUS]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_report_power_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_MSG_POW_STAUS pwr_state)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_REPORT_POWER_STATUS;
	cec_msg_write.message[1] = pwr_state;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [Power Status=0x%02X]\n", dest_address, cec_opcode_str[OPCODE_REPORT_POWER_STATUS],
       pwr_state);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_feature_abort(E_CEC_LOGIC_ADDR dest_address, E_CEC_OPCODE feature_opcode,
    E_CEC_FEATURE_ABORT_REASON abort_reason)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_FEATURE_ABORT;
	cec_msg_write.message[1] = feature_opcode;
	cec_msg_write.message[2] = abort_reason;
	cec_msg_write.message_len = 4;
    app_log_debug("To(0x%X): <%s> [opcode=%s][reason=0x%02X]\n", dest_address, cec_opcode_str[OPCODE_FEATURE_ABORT],
       cec_opcode_str[feature_opcode], abort_reason);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_abort(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_ABORT;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_ABORT]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_user_control_pressed(E_CEC_LOGIC_ADDR dest_address, E_CEC_KEY_CODE ui_command)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_USER_CTRL_PRESSED;
	cec_msg_write.message[1] = ui_command;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [UI_Cmd=%s(0x%02X)]\n", dest_address, cec_opcode_str[OPCODE_USER_CTRL_PRESSED],
       cec_key_str[ui_command], ui_command);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_user_control_released(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_USER_CTRL_RELEASED;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_USER_CTRL_RELEASED]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_audio_status(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GIVE_AUDIO_STATUS;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GIVE_AUDIO_STATUS]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_give_system_audio_mode_status(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_report_audio_status(E_CEC_LOGIC_ADDR dest_address, unsigned char audio_status)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_REPORT_AUDIO_STATUS;
	cec_msg_write.message[1] = audio_status;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [Aud Status=0x%02X]\n", dest_address, cec_opcode_str[OPCODE_REPORT_AUDIO_STATUS],
    cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_request_short_audio_descriptor(E_CEC_LOGIC_ADDR dest_address,
    E_CEC_AUDIO_FORMAT_CODE *p_afc_code, unsigned char afc_num)
{
    unsigned char i = 0;
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
    if (NULL == p_afc_code)
    {
        return -1;
    }
    if (CEC_MAX_AFC_NUM < afc_num)
    {
        afc_num = CEC_MAX_AFC_NUM;
    }
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR;
	for (i = 0; i < afc_num; i++)
    {
        cec_msg_write.message[1+i] = p_afc_code[i]&0x3F;
    }
	cec_msg_write.message_len = 2+afc_num;
    app_log_debug("To(0x%X): <%s> ", dest_address, cec_opcode_str[OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR]);
    for (i = 0; i < afc_num; i++)
    {
        app_log_debug("AFC[%d]=0x%02X ", i, cec_msg_write.message[1+i]);
    }
    app_log_debug("\n");
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_report_short_audio_descriptor(E_CEC_LOGIC_ADDR dest_address, unsigned char *p_short_audio_desc, unsigned char short_audio_desc_num)
{
    unsigned char i = 0;
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
    if (NULL == p_short_audio_desc)
    {
        return -1;
    }
    if (CEC_MAX_SHORT_AD_NUM < short_audio_desc_num)
    {
        short_audio_desc_num = CEC_MAX_SHORT_AD_NUM;
    }
    memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR;
	for (i = 0; i < short_audio_desc_num; i++)
    {
        cec_msg_write.message[1+i*3] = p_short_audio_desc[i*3];
        cec_msg_write.message[1+i*3+1] = p_short_audio_desc[i*3+1];
        cec_msg_write.message[1+i*3+2] = p_short_audio_desc[i*3+2];
    }
	cec_msg_write.message_len = 2+short_audio_desc_num*3;
    app_log_debug("To(0x%X): <%s> ", dest_address, cec_opcode_str[OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR], cec_msg_write.message[1]);
    for (i = 0; i < short_audio_desc_num; i++)
    {
        app_log_debug("Desc[%d]={0x%02X 0x%02X 0x%02X} ", i, cec_msg_write.message[1+i*3], cec_msg_write.message[1+i*3+1], cec_msg_write.message[1+i*3+2]);
    }
    app_log_debug("\n");
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_system_audio_mode_request(E_CEC_LOGIC_ADDR dest_address, bool bsacmodeon)
{
    unsigned short physical_address = 0;
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_SYSTEM_AUDIO_MODE_REQUEST;
    if (bsacmodeon)
    {
        physical_address = app_cec_get_bus_physical_address_info(app_cec_get_bus_active_source_logical_address());
		cec_msg_write.message[1] = (physical_address>>8)&0xFF;
		cec_msg_write.message[2] = physical_address&0xFF;
		cec_msg_write.message_len = 4;
        app_log_debug("To(0x%X): <%s> [Phy Addr=0x%02x%02x]\n", dest_address, \
           cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_REQUEST], cec_msg_write.message[1], cec_msg_write.message[2]);
    }
    else
    {
        cec_msg_write.message_len = 2;
        app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_REQUEST]);
    }
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_system_audio_mode_status(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_SYSTEM_AUDIO_MODE_STATUS;
	cec_msg_write.message[1] = app_cec_get_system_audio_mode_status();
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [Sys Aud Status=%s]\n", dest_address, \
       cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_STATUS], (cec_msg_write.message[1]) ? "On" : "Off");
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_inactive_source(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_INACTIVE_SOURCE;
	cec_msg_write.message[1] = (unsigned char)(cec_addr.physical_addr>>8);
	cec_msg_write.message[2] = (unsigned char)(cec_addr.physical_addr&0xFF);
	cec_msg_write.message_len = 4;
    app_log_debug("To(0x%X): <%s> [Phy Addr=0x%.2X%.2X]\n", dest_address, cec_opcode_str[OPCODE_INACTIVE_SOURCE],
       cec_msg_write.message[1], cec_msg_write.message[2]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_system_standby(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_SYSTEM_STANDBY;
	cec_msg_write.message_len = 2;
    app_log_debug("To(0x%X): <%s> [None]\n", dest_address, cec_opcode_str[OPCODE_SYSTEM_STANDBY]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_polling_message(E_CEC_LOGIC_ADDR dest_address)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message_len = 1;
    app_log_debug("To(0x%X): <Polling Message> [None]\n", dest_address);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_set_system_audio_mode(E_CEC_LOGIC_ADDR dest_address, bool bonoff)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_SET_SYSTEM_AUDIO_MODE;
	cec_msg_write.message[1] = bonoff;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [Sys Aud Status=%s]\n", dest_address, \
       cec_opcode_str[OPCODE_SET_SYSTEM_AUDIO_MODE], (cec_msg_write.message[1]) ? "On" : "Off");
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_record_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_RECORD_STATUS_INFO record_status)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_RECORD_STATUS;
	cec_msg_write.message[1] = record_status;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [0x%x]\n", dest_address, cec_opcode_str[OPCODE_RECORD_STATUS], cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_deck_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_DECK_STATUS_INFO deck_status)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_DECK_STATUS;
	cec_msg_write.message[1] = deck_status;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [0x%x]\n", dest_address, cec_opcode_str[OPCODE_DECK_STATUS], cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_timer_status(E_CEC_LOGIC_ADDR dest_address, P_CEC_TIMER_STATUS_DATA timer_status_data)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_TIMER_STATUS;
	memcpy(&cec_msg_write.message[1], timer_status_data, sizeof(CEC_TIMER_STATUS_DATA));
	cec_msg_write.message_len = 5;
    app_log_debug("To(0x%X): <%s> [0x%x]\n", dest_address, cec_opcode_str[OPCODE_TIMER_STATUS], cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

int tx_cec_msg_timer_clear_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_TIMER_CLEAR_STATUS timer_clear_status)
{
    GxHdmiCecAddr cec_addr = {0};
	GxHdmiCecMessage cec_msg_write = {0};

    GxVoutHAL_HdmiCecGetAddr(&cec_addr);
	memset(&cec_msg_write,0x0,sizeof(cec_msg_write));
	cec_msg_write.header = (cec_addr.logical_addr<<4)|dest_address;
	cec_msg_write.message[0] = OPCODE_CLEAR_DIGITAL_TIMER;
	cec_msg_write.message[1] = timer_clear_status;
	cec_msg_write.message_len = 3;
    app_log_debug("To(0x%X): <%s> [0x%x]\n", dest_address, cec_opcode_str[OPCODE_CLEAR_DIGITAL_TIMER], cec_msg_write.message[1]);
    return GxVoutHAL_HdmiCecWrite(&cec_msg_write);
}

#endif
