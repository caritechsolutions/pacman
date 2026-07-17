#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app_cec_private.h"
#include "app_cec_message_tx.h"
#include "app_cec_message_rx.h"
#include "app_cec_common_str.h"
#include "app_cec_link.h"
#include "app_cec_config.h"
#include "app_cec_message_api.h"
#include "gui_key.h"
#include "module/app_log.h"

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_inquire_state(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_FOUR == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HEC_MSG: <%s> [Phy Addr=0x%02X%02X][Phy Addr=0x%02X%02X]\n", \
           cdc_opcode_str[CDC_HEC_INQUIRE_STATE], param[0], param[1], param[2], param[3]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR!(param_len=%d)\n", cdc_opcode_str[CDC_HEC_INQUIRE_STATE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_report_state(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_FIVE == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HEC_MSG: <%s> [Phy Addr=0x%02X%02X][HEC State=0x%02X][HEC Support Field=0x%02X%02X]\n",\
           cdc_opcode_str[CDC_HEC_REPORT_STATE], param[0], param[1], param[2], param[3], param[4]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HEC_REPORT_STATE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_set_state_adjacent(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_THREE == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HEC_MSG: <%s> [Phy Addr=0x%02X%02X][HEC Set State=0x%02X]\n",
           cdc_opcode_str[CDC_HEC_SET_STATE_ADJACENT], param[0], param[1], param[2]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HEC_SET_STATE_ADJACENT], \
           param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_set_state(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_ELEVEN == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HEC_MSG: <%s> [Phy Addr=0x%02X%02X][Phy Addr=0x%02X%02X][HEC Set State=0x%02X]\
           [Phy Addr=0x%02X%02X][Phy Addr=0x%02X%02X][Phy Addr=0x%02X%02X]\n",
           cec_opcode_str[CDC_HEC_SET_STATE], param[0], param[1], param[2], param[3], param[4], param[5], param[6],
           param[7], param[8], param[9], param[10]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HEC_SET_STATE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_request_deactivation(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_SIX == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HEC_MSG: <%s> [Phy Addr=0x%02X%02X][Phy Addr=0x%02X%02X]\
           [Phy Addr=0x%02X%02X]\n", cdc_opcode_str[CDC_HEC_REQUEST_DEACTIVATION], param[0], param[1], param[2],\
           param[3], param[4], param[5]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HEC_REQUEST_DEACTIVATION], \
           param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_notify_alive(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_ZERO == param_length)
    {
        app_log_debug("CDC_HEC_MSG: <%s> [None]\n", cdc_opcode_str[CDC_HEC_NOTIFY_ALIVE]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HEC_NOTIFY_ALIVE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hec_msg_discover(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_ZERO == param_length)
    {
        app_log_debug("CDC_HEC_MSG: <%s> [None]\n", cdc_opcode_str[CDC_HEC_DISCOVER]);
    }
    else
    {
        app_log_debug("CDC_HEC_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HEC_DISCOVER], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hpd_msg_set_state(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_ONE == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HPD_MSG: <%s> [Input port number=0x%X][HPD State=0x%X]\n", cdc_opcode_str[CDC_HPD_SET_STATE], \
           (param[0]>>4)&0xF, (param[0])&0xF);
    }
    else
    {
        app_log_debug("CDC_HPD_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HPD_SET_STATE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO rx_cdc_hpd_msg_report_state(unsigned char *param, unsigned char param_length)
{
    if (CEC_MSG_PARAM_LEN_ONE == param_length)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_debug("CDC_HPD_MSG: <%s> [HPD State=0x%X][CDC_HPD_Errot_Code=0x%X]\n",
           cdc_opcode_str[CDC_HPD_REPORT_STATE], (param[0]>>4)&0xF, (param[0])&0xF);
    }
    else
    {
        app_log_debug("CDC_HPD_MSG: <%s> ERR! (param_len=%d)\n", cdc_opcode_str[CDC_HPD_REPORT_STATE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

static E_CEC_MSG_ERRNO _cec_msg_cdc_message_parser(E_CDC_OPCODE cdc_opcode, unsigned char *param, unsigned char param_length)
{
    E_CEC_MSG_ERRNO    tresult = CEC_MSG_UNKNOWN;

    switch(cdc_opcode)
    {
        case CDC_HEC_INQUIRE_STATE:
            tresult = rx_cdc_hec_msg_inquire_state(param, param_length);
            break;
        case CDC_HEC_REPORT_STATE:
            tresult = rx_cdc_hec_msg_report_state(param, param_length);
            break;
        case CDC_HEC_SET_STATE_ADJACENT:
            tresult = rx_cdc_hec_msg_set_state_adjacent(param, param_length);
            break;
        case CDC_HEC_SET_STATE:
            tresult = rx_cdc_hec_msg_set_state(param, param_length);
            break;
        case CDC_HEC_REQUEST_DEACTIVATION:
            tresult = rx_cdc_hec_msg_request_deactivation(param, param_length);
            break;
        case CDC_HEC_NOTIFY_ALIVE:
            tresult = rx_cdc_hec_msg_notify_alive(param, param_length);
            break;
        case CDC_HEC_DISCOVER:
            tresult = rx_cdc_hec_msg_discover(param, param_length);
            break;
        case CDC_HPD_SET_STATE:
            tresult = rx_cdc_hpd_msg_set_state(param, param_length);
            break;
        case CDC_HPD_REPORT_STATE:
            tresult = rx_cdc_hpd_msg_report_state(param, param_length);
            break;
        default:
            app_log_debug("New CEC_CDC Command is Found, need to update LIB_CEC to support this command!\n", \
               cdc_opcode);
            tresult = CEC_MSG_OK;
            break;
    }
    if (tresult != CEC_MSG_OK)
    {
        return tresult;
    }
    return tresult;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_osd_string(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char    i = 0;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if ((param_length <= CEC_MSG_PARAM_LEN_FOURTEEN) && (param_length >= CEC_MSG_PARAM_LEN_TWO))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Display Control=0x%02X][OSD String=\"", source_address, \
               cec_opcode_str[OPCODE_SET_OSD_STRING], param[0]);
            for (i=1; i<param_length; i++)
            {
                app_log_debug("%c", param[i]);
            }
            app_log_debug("\"]\n");
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SET_OSD_STRING], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SET_OSD_STRING], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_osd_name(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_ZERO == param_length)
            {
                app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GIVE_OSD_NAME]);
                app_cec_msg_set_osd_name( source_address, (unsigned char *)"GX STB");
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
                   cec_opcode_str[OPCODE_GIVE_OSD_NAME], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X)\n", source_address, \
               cec_opcode_str[OPCODE_GIVE_OSD_NAME], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_GIVE_OSD_NAME], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_osd_name(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char    i = 0;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if ((param_length <= CEC_MSG_PARAM_LEN_FOURTEEN)&&(param_length >= CEC_MSG_PARAM_LEN_ONE))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [OSD Name=\"", source_address, cec_opcode_str[OPCODE_SET_OSD_NAME]);
            for (i=0; i<param_length; i++)
            {
                app_log_debug("%c", param[i]);
            }
            app_log_debug("\"]\n");
            //  OSD Name <=14 bytes
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_SET_OSD_NAME], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_SET_OSD_NAME], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_menu_request(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_ONE == param_length)
            {
                if (NULL == param)
                {
                    return CEC_MSG_ERR_PARAM_VALUE;
                }
                app_log_info("From(0x%X): <%s> [Menu Request Type=0x%02X]\n", source_address, \
                   cec_opcode_str[OPCODE_MENU_REQUEST], param[0]);
				memset(&opcode_handler,0x0,sizeof(opcode_handler));
				opcode_handler.opcode = OPCODE_MENU_REQUEST;
				opcode_handler.param_length = param_length;
				opcode_handler.dest_address = dest_address;
				opcode_handler.source_address = source_address;
				memcpy(opcode_handler.param, param, param_length);
				app_send_cec_link_opcode_handler_msg(&opcode_handler);
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
                   cec_opcode_str[OPCODE_MENU_REQUEST], param_length);
                //Tx_CEC_MSG_Feature_Abort(source_address, opcode, REASON_INVALID_OPRAND);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X)\n", source_address, \
               cec_opcode_str[OPCODE_MENU_REQUEST], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_MENU_REQUEST], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_menu_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ONE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_info("From(0x%X): <%s> [Menu State=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_MENU_STATUS], param[0]);
            // Store the menu status information?
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_MENU_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_MENU_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_user_control_pressed(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ONE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_info("From(0x%X): <%s> [UI Command=%s(0x%02X)]\n", source_address, \
               cec_opcode_str[OPCODE_USER_CTRL_PRESSED], cec_key_str[param[0]], param[0]);
            app_cec_set_remote_passthrough_key(param[0]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_USER_CTRL_PRESSED;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_USER_CTRL_PRESSED], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
            cec_opcode_str[OPCODE_USER_CTRL_PRESSED], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_user_control_released(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_USER_CTRL_RELEASED]);
            app_cec_set_remote_passthrough_key(0xFFFF);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_USER_CTRL_RELEASED], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_USER_CTRL_RELEASED], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }

    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_power_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GIVE_POWER_STATUS]);
            app_cec_msg_report_power_status(source_address, POW_ON);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_GIVE_POWER_STATUS], param_length);
            //AP_CEC_MSG_Feature_Abort(source_address, OPCODE_GIVE_POWER_STATUS, REASON_INVALID_OPRAND);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
            cec_opcode_str[OPCODE_GIVE_POWER_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }

    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_report_power_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ONE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Power Status=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_POWER_STATUS], param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_POWER_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REPORT_POWER_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_feature_abort(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_FEATURE_ABORT_INFO feature_abort_info;
    bool b_ret = false;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_TWO == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [opcode=%s][operand=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_FEATURE_ABORT], cec_opcode_str[param[0]], param[1]);
            feature_abort_info.opcode = param[0];
            feature_abort_info.reason = param[1];
			feature_abort_info.dest_address = dest_address;
			feature_abort_info.source_address = source_address;
            b_ret = app_cec_set_last_received_feature_abort_info(&feature_abort_info);
            if (!b_ret)
            {
                app_log_error("app_cec_set_last_received_feature_abort_info error");
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_FEATURE_ABORT], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_FEATURE_ABORT], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_abort(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    int i = 0;
#define REPORT_FEATURE_ABORT_MAX (5)

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_ABORT]);
			for (i = 0; i < REPORT_FEATURE_ABORT_MAX; i++)
			{
			    tx_cec_msg_feature_abort(source_address,  OPCODE_ABORT, REASON_UNRECOGNIZE_OPCODE);
			}
        }
        else
        {
            app_log_debug("From(0x%X): <%s?, ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_ABORT], param_length);
            app_cec_msg_feature_abort(source_address,  OPCODE_ABORT, REASON_INVALID_OPRAND);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, cec_opcode_str[OPCODE_ABORT], \
           dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_audio_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GIVE_AUDIO_STATUS]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_GIVE_AUDIO_STATUS;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address,
               cec_opcode_str[OPCODE_GIVE_AUDIO_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address,
           cec_opcode_str[OPCODE_GIVE_AUDIO_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_system_audio_mode_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, \
               cec_opcode_str[OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS], param_length);
            app_cec_msg_feature_abort(source_address,  OPCODE_ABORT, REASON_INVALID_OPRAND);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_report_audio_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ONE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Audio Status=%d]\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_AUDIO_STATUS], param[0]);
            app_cec_set_system_audio_status(param[0]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_REPORT_AUDIO_STATUS;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_AUDIO_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REPORT_AUDIO_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_report_short_audio_descriptor(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if ((CEC_MSG_PARAM_LEN_THREE == param_length)
		  ||(CEC_MSG_PARAM_LEN_SIX == param_length)
          ||(CEC_MSG_PARAM_LEN_NINE == param_length)
          ||(CEC_MSG_PARAM_LEN_TWELVE == param_length))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Short Audio Descriptor = \n", source_address, \
               cec_opcode_str[OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address,
               cec_opcode_str[OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_request_short_audio_descriptor(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length <= CEC_MSG_PARAM_LEN_FOUR)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s>\n", source_address, cec_opcode_str[OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR]);
            app_cec_msg_feature_abort(source_address, OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR, REASON_INVALID_OPRAND);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_system_audio_mode(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (param_length == CEC_MSG_PARAM_LEN_ONE)
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        app_log_info("From(0x%X): <%s> [System Audio Status=%s]\n", source_address, \
           cec_opcode_str[OPCODE_SET_SYSTEM_AUDIO_MODE], (param[0]) ? "On" : "Off");
        memset(&opcode_handler,0x0,sizeof(opcode_handler));
		opcode_handler.opcode = OPCODE_SET_SYSTEM_AUDIO_MODE;
		opcode_handler.param_length = param_length;
		opcode_handler.dest_address = dest_address;
		opcode_handler.source_address = source_address;
		memcpy(opcode_handler.param, param, param_length);
		app_send_cec_link_opcode_handler_msg(&opcode_handler);
    }
    else
    {
        app_log_error("From(0x%X): <%s, ERR! (param_len=%d)\n", source_address, \
           cec_opcode_str[OPCODE_SET_SYSTEM_AUDIO_MODE], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_system_audio_mode_request(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if ((param_length == CEC_MSG_PARAM_LEN_TWO)||(param_length == CEC_MSG_PARAM_LEN_ZERO))
        {
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
            opcode_handler.opcode = OPCODE_SYSTEM_AUDIO_MODE_REQUEST;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_info("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_REQUEST], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_info("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_REQUEST], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_system_audio_mode_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_info("From(0x%X): <%s> [System Audio Status=%s]\n", source_address, \
               cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_STATUS], (param[0])?"On":"Off");
			app_cec_set_system_audio_mode_status(param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SYSTEM_AUDIO_MODE_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_record_off(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (0 == param_length)
        {
            app_log_info("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_RECORD_OFF]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_RECORD_OFF;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			// 12 - 2, Pass Criteria: The DUT ignores the message.
			//app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_RECORD_OFF], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_RECORD_OFF], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_record_on(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char    i = 0;
	CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if ((param_length >= CEC_MSG_PARAM_LEN_ONE) && (param_length <= CEC_MSG_PARAM_LEN_EIGHT))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Record Source Type=0x%02X][Record Source Data=0x%02X]\n", source_address,\
               cec_opcode_str[OPCODE_RECORD_ON], param[0]);

            if (param_length == CEC_MSG_PARAM_LEN_ONE)
            {
                app_log_debug("From(0x%X): <%s> [Record Source Type=0x%02X]\n", source_address, \
                   cec_opcode_str[OPCODE_RECORD_ON], param[0]);
            }
            else
            {
                app_log_debug("From(0x%X): <%s> [Record Source Type=0x%02X][Record Source Data=0x%02X][CDC Param=", \
                   source_address, cec_opcode_str[OPCODE_RECORD_ON], param[0]);
                for (i = 1; i < param_length-1; i++)
                {
                    app_log_debug("0x%02X ", param[i]);
                }
                app_log_debug("]\n");
            }
			if (param_length == CEC_MSG_PARAM_LEN_EIGHT)
			{
			   /* Little-Endian to Big-Endian */
                CEC_SWAP8(param+2);
                CEC_SWAP8(param+4);
                CEC_SWAP8(param+6);
			}
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_RECORD_ON;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			// 9.4 - 1 , Pass Criteria: The DUT ignores the message.
			//app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_RECORD_ON], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_RECORD_ON], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_record_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (NULL == param)
        {
            return CEC_MSG_ERR_PARAM_VALUE;
        }
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            app_log_debug("From(0x%X): <%s> [Record Status Info=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_RECORD_STATUS], param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_RECORD_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_RECORD_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_record_tv_screen(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_RECORD_TV_SCREEN]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_RECORD_TV_SCREEN;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_RECORD_TV_SCREEN], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_RECORD_TV_SCREEN], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_clear_analogue_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ELEVEN)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][Analogue Broadcast Type=0x%02X]\
               [Analogue Frequence=0x%02X %02X][Broadcast System=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_CLEAR_ANALOG_TIMER], param[0], param[1], param[2], param[3], param[4], param[5], \
               param[6], param[7], param[8], param[9], param[10]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_CLEAR_ANALOG_TIMER], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_CLEAR_ANALOG_TIMER], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_clear_digital_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
	CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_FOURTEEN)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][Digital Service ID=0x%02X %02X %02X %02X %02X %02X \
               %02X]\n", source_address, cec_opcode_str[OPCODE_CLEAR_DIGITAL_TIMER], param[0], param[1], param[2], \
               param[3], param[4], param[5], param[6], param[7], param[8], param[9], param[10], param[11], param[12], \
               param[13]);
			/* Little-Endian to Big-Endian */
			CEC_SWAP8(param+8);
            CEC_SWAP8(param+10);
            CEC_SWAP8(param+12);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_CLEAR_DIGITAL_TIMER;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_CLEAR_DIGITAL_TIMER], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_CLEAR_DIGITAL_TIMER], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_clear_external_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length==CEC_MSG_PARAM_LEN_NINE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][External Source Specifier=0x%02X]\
               [External Plug=0x%02X]\n", source_address, cec_opcode_str[OPCODE_CLEAR_EXTERNAL_TIMER], param[0], \
               param[1], param[2], param[3], param[4], param[5], param[6], param[7], param[8]);
        }
        else if (param_length == CEC_MSG_PARAM_LEN_TEN)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][External Source Specifier=0x%02X]\
               [External Physical Addr=0x%02X %02X]\n", source_address, \
               cec_opcode_str[OPCODE_CLEAR_EXTERNAL_TIMER], param[0], param[1], param[2], param[3], param[4], \
               param[5], param[6], param[7], param[8], param[9]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_CLEAR_EXTERNAL_TIMER], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_CLEAR_EXTERNAL_TIMER], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_analogue_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ELEVEN)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][Analogue Broadcast Type=0x%02X]\
               [Analogue Frequence=0x%02X %02X][Broadcast System=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_SET_ANALOG_TIMER], param[0], param[1], param[2], param[3], param[4], param[5], \
               param[6], param[7], param[8], param[9], param[10]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SET_ANALOG_TIMER], param_length);
            //AP_CEC_MSG_Feature_Abort(source_address,  OPCODE_ABORT, REASON_INVALID_OPRAND);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SET_ANALOG_TIMER], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_digital_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
	CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_FOURTEEN)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X]\
               [Digital Service ID=0x%02X %02X %02X %02X %02X %02X %02X]\n", source_address, \
               cec_opcode_str[OPCODE_SET_DIGITAL_TIMER], param[0], param[1], param[2], param[3], param[4], \
               param[5], param[6], param[7], param[8], param[9], param[10], param[11], param[12], param[13]);
            /* Little-Endian to Big-Endian */
			CEC_SWAP8(param+8);
            CEC_SWAP8(param+10);
            CEC_SWAP8(param+12);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_SET_DIGITAL_TIMER;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SET_DIGITAL_TIMER], param_length);
            //AP_CEC_MSG_Feature_Abort(source_address,  OPCODE_ABORT, REASON_INVALID_OPRAND);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SET_DIGITAL_TIMER], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_external_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_NINE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][External Source Specifier=0x%02X]\
               [External Plug=0x%02X]\n", source_address, cec_opcode_str[OPCODE_SET_EXTERNAL_TIMER], param[0], \
               param[1], param[2], param[3], param[4], param[5], param[6], param[7], param[8]);
        }
        else if (CEC_MSG_PARAM_LEN_TEN == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Day of Month=0x%02X][Month of Year=0x%02X][Start Time=0x%02X %02X]\
               [Duration=0x%02X %02X][Recording Sequence=0x%02X][External Source Specifier=0x%02X]\
               [External Physical Addr=0x%02X %02X]\n", source_address, cec_opcode_str[OPCODE_SET_EXTERNAL_TIMER], \
               param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7], param[8], param[9]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SET_EXTERNAL_TIMER], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SET_EXTERNAL_TIMER], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_timer_program_title(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if ((param_length >= CEC_MSG_PARAM_LEN_ONE)&&(param_length <= CEC_MSG_PARAM_LEN_FOURTEEN))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Program Title=\"\n", source_address, \
               cec_opcode_str[OPCODE_SET_TIMER_PROGRAM_TITLE]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SET_TIMER_PROGRAM_TITLE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_SET_TIMER_PROGRAM_TITLE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_timer_cleared_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Timer Cleared Status Data=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_TIMER_CLEARED_STATUS], param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_TIMER_CLEARED_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_TIMER_CLEARED_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_timer_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Timer Status Data=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_TIMER_STATUS], param[0]);
        }
        else if (param_length == CEC_MSG_PARAM_LEN_THREE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Timer Status Data=0x%02X 0x%02X 0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_TIMER_STATUS], param[0], param[1], param[2]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_TIMER_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_TIMER_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_deck_control(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Deck Control_Mode=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_DECK_CONTROL], param[0]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_DECK_CONTROL;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_DECK_CONTROL], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_DECK_CONTROL], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_deck_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Deck Info=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_DECK_STATUS], param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_DECK_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_DECK_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_deck_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Status Request=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_GIVE_DECK_STATUS], param[0]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_GIVE_DECK_STATUS;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_GIVE_DECK_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_GIVE_DECK_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_play(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Play Mode=0x%02X]\n", source_address, cec_opcode_str[OPCODE_PLAY], param[0]);
			memset(&opcode_handler,0x0,sizeof(opcode_handler));
			opcode_handler.opcode = OPCODE_PLAY;
			opcode_handler.param_length = param_length;
			opcode_handler.dest_address = dest_address;
			opcode_handler.source_address = source_address;
			memcpy(opcode_handler.param, param, param_length);
			app_send_cec_link_opcode_handler_msg(&opcode_handler);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_PLAY], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_PLAY], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_audio_rate(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ONE)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Audio Rate=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_SET_AUDIO_RATE], param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_SET_AUDIO_RATE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_initiate_arc(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_INITIATE_ARC]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_INITIATE_ARC], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_INITIATE_ARC], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_report_arc_initiated(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_REPORT_ARC_INITIATED]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_ARC_INITIATED], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REPORT_ARC_INITIATED], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_report_arc_terminated(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length==CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_REPORT_ARC_TERMINATED]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_ARC_TERMINATED], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REPORT_ARC_TERMINATED], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_request_arc_initiation(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length==CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_REQUEST_ARC_INITIATION]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REQUEST_ARC_INITIATION], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REQUEST_ARC_INITIATION], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_request_arc_termination(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length==CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_REQUEST_ARC_TERMINATION]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_REQUEST_ARC_TERMINATION], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_REQUEST_ARC_TERMINATION], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_terminate_arc(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length == CEC_MSG_PARAM_LEN_ZERO)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_TERMINAE_ARC]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_TERMINAE_ARC], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_TERMINAE_ARC], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_cdc_message(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char cdc_opcode = 0;
    unsigned char *cdc_param = NULL;
	unsigned char cdc_param_length = 0;
    E_CEC_MSG_ERRNO  t_result = CEC_MSG_UNKNOWN;

    if (dest_address != CEC_LA_BROADCAST)
    {
        if ((param_length <= CEC_MSG_PARAM_LEN_FOURTEEN)&&(param_length >= CEC_MSG_PARAM_LEN_THREE))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            if (param_length == CEC_MSG_PARAM_LEN_THREE)
            {
                app_log_debug("From(0x%X): <%s> [Initiator Phy. Addr=0x%02X %02X][CDC Opcode=0x%02X]\n", \
                   source_address, cec_opcode_str[OPCODE_CDC_MESSAGE], param[0], param[1], param[2]);
            }
            else
            {
                app_log_debug("From(0x%X): <%s> [Initiator Phy. Addr=0x%02X %02X][CDC Opcode=0x%02X][CDC Param=",\
                    source_address, cec_opcode_str[OPCODE_CDC_MESSAGE], param[0], param[1], param[2]);
            }
            cdc_opcode = param[2];
            cdc_param = param+3;
            cdc_param_length = param_length-3;
            t_result = _cec_msg_cdc_message_parser(cdc_opcode, cdc_param, cdc_param_length);
            if (CEC_MSG_OK != t_result)
            {
                app_log_error("_cec_msg_cdc_message_parser error \n");
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_CDC_MESSAGE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address,
           cec_opcode_str[OPCODE_CDC_MESSAGE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}
#endif
