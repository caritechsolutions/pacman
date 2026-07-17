#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app.h"
#include "app_cec_private.h"
#include "app_cec_message_tx.h"
#include "app_cec_message_rx.h"
#include "app_cec_common_str.h"
#include "app_cec_link.h"
#include "app_cec_config.h"
#include "app_cec_message_api.h"
#include "app_cec_operation.h"
#include "gui_key.h"
#include "gui_core.h"
#include "module/app_pvr.h"
#include "module/app_log.h"

void CEC_SWAP8(unsigned char *p)
{
    unsigned char tmp = 0;

    if (!p)
    {
        return;
    }
    tmp = *(p + 1);
    *(p + 1) = *p;
    *p = tmp;
}

E_CEC_MSG_ERRNO rx_cec_msg_image_view_on(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address != CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_IMAGE_VIEW_ON]);
            // Ignore the MSG
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_IMAGE_VIEW_ON], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_IMAGE_VIEW_ON], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_text_view_on(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address != CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_TEXT_VIEW_ON]);
            // Ignore the MSG
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_TEXT_VIEW_ON], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_TEXT_VIEW_ON], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_active_source(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned short physical_address = 0;

    if (dest_address == CEC_LA_BROADCAST)
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_TWO == param_length)
            {
                if (NULL == param)
                {
                    return CEC_MSG_ERR_PARAM_VALUE;
                }
                app_log_debug("From(0x%X): <%s> [Phy Addr=0x%02X%02X]\n", source_address ,
                   cec_opcode_str[OPCODE_ACTIVE_SOURCE], param[0], param[1]);
                if (app_cec_get_source_active_state() == CEC_SOURCE_STATE_ACTIVE)
                {
                    app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
                }
                physical_address = ((param[0]<<8)|(param[1]));
                app_cec_update_bus_active_source_info(source_address, physical_address);
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_ACTIVE_SOURCE], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address, \
               cec_opcode_str[OPCODE_ACTIVE_SOURCE], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_ACTIVE_SOURCE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_request_active_source(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_REQUEST_ACTIVE_SOURCE]);
            if (app_cec_get_source_active_state() == CEC_SOURCE_STATE_ACTIVE)
            {
                cec_act_logical_address_allocation();
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_REQUEST_ACTIVE_SOURCE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_REQUEST_ACTIVE_SOURCE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_stream_path(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_TWO == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Phy Addr=0x%02X%02X]\n", source_address, \
               cec_opcode_str[OPCODE_SET_STREAM_PATH], param[0], param[1]);

            // ? got set stream path, destination is our STB, should become active source.
            if (app_cec_get_device_physical_address() == ((unsigned short) (param[0]<<8) | (param[1])))
            {
                app_cec_set_source_active_state(CEC_SOURCE_STATE_ACTIVE);
                app_cec_msg_active_source();
            }
            else
            {
                /* The Destination is NOT our STB*/
                app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_SET_STREAM_PATH], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_SET_STREAM_PATH], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_inactive_source(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address != CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_TWO ==  param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Phy Addr=0x%02X%02X]\n", source_address, \
               cec_opcode_str[OPCODE_INACTIVE_SOURCE], param[0], param[1]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_INACTIVE_SOURCE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_INACTIVE_SOURCE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_routing_change(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_FOUR == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Original Addr=0x%02X%02X][New Addr=0x%02X%02X]\n", source_address,
               cec_opcode_str[OPCODE_ROUTING_CHANGE], param[0], param[1], param[2], param[3]);
			cec_act_logical_address_allocation();
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_ROUTING_CHANGE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_ROUTING_CHANGE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }

    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_routing_information(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_TWO == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Phy Addr=0x%02X%02X]\n", source_address, \
               cec_opcode_str[OPCODE_ROUTING_INFORMATION], param[0], param[1]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_ROUTING_INFORMATION], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_ROUTING_INFORMATION], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_system_standby(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (CEC_MSG_PARAM_LEN_ZERO == param_length)
    {
        app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_SYSTEM_STANDBY]);
        memset(&opcode_handler,0x0,sizeof(opcode_handler));
		opcode_handler.opcode = OPCODE_SYSTEM_STANDBY;
		opcode_handler.param_length = param_length;
		opcode_handler.dest_address = dest_address;
		opcode_handler.source_address = source_address;
		memcpy(opcode_handler.param, param, param_length);
		app_send_cec_link_opcode_handler_msg(&opcode_handler);
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address,  \
           cec_opcode_str[OPCODE_SYSTEM_STANDBY], param_length);
        return CEC_MSG_ERR_PAYLOAD_LENGTH;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_get_cec_version(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GET_CEC_VERSION]);
            app_cec_msg_cec_version( source_address );
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_GET_CEC_VERSION], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_GET_CEC_VERSION], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_physical_address(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
	int i = 0;
#define REPORT_PHYSICAL_ADDR_MAX (5)

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GIVE_PHYSICAL_ADDR]);
            // 9.3 - 3: Pass Criteria: The DUT tries to re-send the message between 1-5 times and then stops transmitting the message. The time between the retries is >= 3 nominal data bit periods.
			for (i = 0; i < REPORT_PHYSICAL_ADDR_MAX; i++)
			{
			    app_cec_msg_report_physical_address();
			}
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_GIVE_PHYSICAL_ADDR], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_GIVE_PHYSICAL_ADDR], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_get_menu_language(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GET_MENU_LANGUAGE]);
            // 11.2.6 - 7, Pass Criteria: The DUT does not send a <Set Menu Language> message.
			//app_cec_msg_set_menu_language(source_address);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_GET_MENU_LANGUAGE], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_GET_MENU_LANGUAGE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_cec_version(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ONE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_info("From(0x%X): <%s> [CEC Ver=0x%02X]\n", source_address,  \
               cec_opcode_str[OPCODE_CEC_VERSION], param[0]);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address,  \
               cec_opcode_str[OPCODE_CEC_VERSION], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address,  \
           cec_opcode_str[OPCODE_CEC_VERSION], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_report_physical_address(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned short    physical_address = 0;

    if (dest_address == CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_THREE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Phy Addr=0x%02X%02X][Dev Type=0x%02X]\n", source_address, \
               cec_opcode_str[OPCODE_REPORT_PHYSICAL_ADDR], param[0], param[1], param[2]);
            physical_address = ((param[0]<<8)|(param[1]));
            app_cec_update_bus_physical_address_info(source_address, physical_address);
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_REPORT_PHYSICAL_ADDR], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_REPORT_PHYSICAL_ADDR], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_set_menu_language(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == CEC_LA_BROADCAST)
    {
        if (source_address == CEC_LA_TV)
        {
            if (CEC_MSG_PARAM_LEN_THREE == param_length)
            {
                if (NULL == param)
                {
                    return CEC_MSG_ERR_PARAM_VALUE;
                }
                app_log_info("From(0x%X): <%s> [Lang=0x%02X%02X%02X(\"%c%c%c\")]\n", source_address, \
                   cec_opcode_str[OPCODE_SET_MENU_LANGUAGE], \
                   param[0], param[1], param[2], param[0], param[1], param[2]);
				memset(&opcode_handler,0x0,sizeof(opcode_handler));
				opcode_handler.opcode = OPCODE_SET_MENU_LANGUAGE;
				opcode_handler.param_length = param_length;
				opcode_handler.dest_address = dest_address;
				opcode_handler.source_address = source_address;
				memcpy(opcode_handler.param, param, param_length);
				app_send_cec_link_opcode_handler_msg(&opcode_handler);
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_SET_MENU_LANGUAGE], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address, \
               cec_opcode_str[OPCODE_SET_MENU_LANGUAGE], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_SET_MENU_LANGUAGE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_tuner_device_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
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
                app_log_debug("From(0x%X): <%s> [Status Req=0x%02X]\n", source_address, \
                   cec_opcode_str[OPCODE_GIVE_TUNER_DEVICE_STATUS], param[0]);

                /*
                    [Status Req]:     1 = "On",  2 = "Off", 3 = "Once"
                */
				memset(&opcode_handler,0x0,sizeof(opcode_handler));
                opcode_handler.opcode = OPCODE_GIVE_TUNER_DEVICE_STATUS;
				opcode_handler.param_length = param_length;
				opcode_handler.dest_address = dest_address;
				opcode_handler.source_address = source_address;
				memcpy(opcode_handler.param, param, param_length);
				app_send_cec_link_opcode_handler_msg(&opcode_handler);
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_GIVE_TUNER_DEVICE_STATUS], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address,
               cec_opcode_str[OPCODE_GIVE_TUNER_DEVICE_STATUS], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_GIVE_TUNER_DEVICE_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_select_digital_service(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_SEVEN == param_length)
            {
                if (NULL == param)
                {
                    return CEC_MSG_ERR_PARAM_VALUE;
                }
                app_log_error("From(0x%X): <%s> [Digital Service ID=0x%02X %02X %02X %02X %02X %02X %02X]\n", \
                   source_address, cec_opcode_str[OPCODE_SELECT_DIGITAL_SERVICE], param[0], param[1], param[2], \
                   param[3], param[4], param[5], param[6]);
				/* Little-Endian to Big-Endian */
                CEC_SWAP8(param+1);
                CEC_SWAP8(param+3);
                CEC_SWAP8(param+5);
				memset(&opcode_handler,0x0,sizeof(opcode_handler));
				opcode_handler.opcode = OPCODE_SELECT_DIGITAL_SERVICE;
				opcode_handler.param_length = param_length;
				opcode_handler.dest_address = dest_address;
				opcode_handler.source_address = source_address;
				memcpy(opcode_handler.param, param, param_length);
				app_send_cec_link_opcode_handler_msg(&opcode_handler);
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_SELECT_DIGITAL_SERVICE], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address, \
               cec_opcode_str[OPCODE_SELECT_DIGITAL_SERVICE], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_SELECT_DIGITAL_SERVICE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_select_analog_service(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_FOUR == param_length)
            {
                if (NULL == param)
                {
                    return CEC_MSG_ERR_PARAM_VALUE;
                }
                app_log_debug("From(0x%X): <%s> [Analog Bcast Type=0x%02X][Analog Freq=0x%02X%02X]\
                   [Bcast Sys=0x%02X]\n", source_address, cec_opcode_str[OPCODE_SELECT_ANALOG_SERVICE], param[0],
                   param[1], param[2], param[3]);
                app_cec_msg_feature_abort(source_address, OPCODE_SELECT_ANALOG_SERVICE, REASON_REFUSED);
            }
            else
            {
                app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_SELECT_ANALOG_SERVICE], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address, \
               cec_opcode_str[OPCODE_SELECT_ANALOG_SERVICE], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_SELECT_ANALOG_SERVICE], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_tuner_step_decrement(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_ZERO == param_length)
            {
                app_log_debug("From(0x%X): <%s> [None]\n", source_address,cec_opcode_str[OPCODE_TUNER_STEP_DECREMENT]);
				memset(&opcode_handler,0x0,sizeof(opcode_handler));
				opcode_handler.opcode = OPCODE_TUNER_STEP_DECREMENT;
				opcode_handler.param_length = param_length;
				opcode_handler.dest_address = dest_address;
				opcode_handler.source_address = source_address;
				memcpy(opcode_handler.param, param, param_length);
				app_send_cec_link_opcode_handler_msg(&opcode_handler);
            }
            else
            {
                app_log_debug("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_TUNER_STEP_DECREMENT], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_debug("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address, \
               cec_opcode_str[OPCODE_TUNER_STEP_DECREMENT], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_TUNER_STEP_DECREMENT], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_tuner_step_increment(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    CEC_OPCODE_HANDLER opcode_handler;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (source_address != CEC_LA_BROADCAST)
        {
            if (CEC_MSG_PARAM_LEN_ZERO == param_length)
            {
                app_log_debug("From(0x%X): <%s> [None]\n",source_address, cec_opcode_str[OPCODE_TUNER_STEP_INCREMENT]);
				memset(&opcode_handler,0x0,sizeof(opcode_handler));
				opcode_handler.opcode = OPCODE_TUNER_STEP_INCREMENT;
				opcode_handler.param_length = param_length;
				opcode_handler.dest_address = dest_address;
				opcode_handler.source_address = source_address;
				memcpy(opcode_handler.param, param, param_length);
				app_send_cec_link_opcode_handler_msg(&opcode_handler);
            }
            else
            {
                app_log_debug("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
                   cec_opcode_str[OPCODE_TUNER_STEP_INCREMENT], param_length);
                return CEC_MSG_ERR_PAYLOAD_LENGTH;
            }
        }
        else
        {
            app_log_debug("From(0x%X): <%s> ERR! (source_addr=0x%X) \n", source_address, \
               cec_opcode_str[OPCODE_TUNER_STEP_INCREMENT], source_address);
            return CEC_MSG_ERR_SRC_ADDR;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_TUNER_STEP_INCREMENT], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_tuner_device_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_EIGHT == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Digital Tuner Dev Info=0x%02X %02X %02X %02X %02X %02X %02X %02X]\n", \
               source_address, cec_opcode_str[OPCODE_TUNER_DEVICE_STATUS], param[0], param[1], param[2], param[3],
               param[4], param[5], param[6], param[7]);
            // store digital tuner status info?
        }
        else if (CEC_MSG_PARAM_LEN_FIVE == param_length)
        {
            app_log_debug("From(0x%X): <%s> [Analog Tuner Dev Info=0x%02X %02X %02X %02X %02X %02X %02X %02X]\n", \
               source_address, cec_opcode_str[OPCODE_TUNER_DEVICE_STATUS], param[0], param[1], param[2], param[3], param[4]);
            // store analog tuner status info?
            return CEC_MSG_OK;
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_TUNER_DEVICE_STATUS], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        // strange situation , ignore this message
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, cec_opcode_str[OPCODE_TUNER_DEVICE_STATUS], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_device_vendor_id(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == CEC_LA_BROADCAST)
    {
        if (CEC_MSG_PARAM_LEN_THREE == param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Ven ID=0x%02X%02X%02X]\n", source_address, \
               cec_opcode_str[OPCODE_DEVICE_VENDOR_ID], param[0], param[1], param[2]);
            // store vendor id?
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_DEVICE_VENDOR_ID], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_address=%X) \n", source_address, \
           cec_opcode_str[OPCODE_DEVICE_VENDOR_ID], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_give_device_vendor_id(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_ZERO == param_length)
        {
            app_log_debug("From(0x%X): <%s> [None]\n", source_address, cec_opcode_str[OPCODE_GIVE_DEVICE_VENDOR_ID]);
            app_cec_msg_device_vendor_id();
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d) \n", source_address, \
               cec_opcode_str[OPCODE_GIVE_DEVICE_VENDOR_ID], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X) \n", source_address, \
           cec_opcode_str[OPCODE_GIVE_DEVICE_VENDOR_ID], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_vendor_command(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char i = 0;

    if (dest_address == app_cec_get_device_logical_address())        // Directly addressed
    {
        if ((param_length <= CEC_MSG_PARAM_LEN_FOURTEEN) && (param_length >= CEC_MSG_PARAM_LEN_ONE))
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Ven Data=", source_address, cec_opcode_str[OPCODE_VENDOR_COMMAND]);
            for (i = 0; i < param_length; i++)
            {
                app_log_debug("0x%02X ", param[i]);
            }
            app_log_debug("]\n");
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_VENDOR_COMMAND], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_VENDOR_COMMAND], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_vendor_command_with_id(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char i = 0;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (param_length <= CEC_MSG_PARAM_LEN_FOURTEEN)
        {
            if (!param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [Ven ID=0x%02X%02X%02X] [Ven data=", source_address, \
               cec_opcode_str[OPCODE_VENDOR_COMMAND_WITH_ID], param[0], param[1], param[2]);
            for (i=0; i<param_length; i++)
            {
                app_log_debug("0x%02X ", param[i]);
            }
            app_log_debug("]\n");
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_VENDOR_COMMAND_WITH_ID], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_VENDOR_COMMAND_WITH_ID], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_vendor_remote_button_down(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char i = 0;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_FOURTEEN >= param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X):<%s> [RC Code=", source_address, cec_opcode_str[OPCODE_VENDOR_REOMTE_BUTTON_DOWN]);
            for (i=0; i<param_length; i++)
            {
                app_log_debug("0x%02X ", param[i]);
            }
            app_log_debug("]\n");
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_VENDOR_REOMTE_BUTTON_DOWN], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_VENDOR_REOMTE_BUTTON_DOWN], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}

E_CEC_MSG_ERRNO rx_cec_msg_vendor_remote_button_up(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length)
{
    unsigned char i = 0;

    if (dest_address == app_cec_get_device_logical_address())
    {
        if (CEC_MSG_PARAM_LEN_FOURTEEN >= param_length)
        {
            if (NULL == param)
            {
                return CEC_MSG_ERR_PARAM_VALUE;
            }
            app_log_debug("From(0x%X): <%s> [RC Code=", source_address, cec_opcode_str[OPCODE_VENDOR_REOMTE_BUTTON_UP]);
            for (i=0; i<param_length; i++)
            {
                app_log_debug("0x%02X ", param[i]);
            }
            app_log_debug("]\n");
        }
        else
        {
            app_log_error("From(0x%X): <%s> ERR! (param_len=%d)\n", source_address, \
               cec_opcode_str[OPCODE_VENDOR_REOMTE_BUTTON_UP], param_length);
            return CEC_MSG_ERR_PAYLOAD_LENGTH;
        }
    }
    else
    {
        app_log_error("From(0x%X): <%s> ERR! (dest_addr=0x%X)\n", source_address, \
           cec_opcode_str[OPCODE_VENDOR_REOMTE_BUTTON_UP], dest_address);
        return CEC_MSG_ERR_DST_ADDR;
    }
    return CEC_MSG_OK;
}
#endif
