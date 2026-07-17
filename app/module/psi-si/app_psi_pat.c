#include "app.h"
#include "module/app_psi.h"

#define PAT_FIXED_LENGTH    (12)
#define PAT_LOOP_LENGTH    (4)

/////plz release result data if valid /////
char *app_pat_generate(unsigned short *prog_id_array, unsigned int prog_count)
{
    char *pat_data = NULL;
    unsigned short i = 0;
    unsigned short cnt = 0;
    unsigned short sec_length = 0;
    unsigned int crc_value = 0;
    GxMsgProperty_NodeByIdGet node = {0};

    if((prog_id_array == NULL) || (prog_count == 0))
        return NULL;

    pat_data = (char*)GxCore_Calloc(PAT_FIXED_LENGTH + PAT_LOOP_LENGTH * prog_count, 1);
    if(pat_data != NULL)
    {
        //table_id
        pat_data[i++] = 0x00;

        //synctax
        pat_data[i] = 0x80;
        //section length
        sec_length = PAT_FIXED_LENGTH + PAT_LOOP_LENGTH * prog_count - 3;
        pat_data[i++] |= ((sec_length >> 8) & 0x0f);
        pat_data[i++] = sec_length & 0xff;

        //ts_id
        memset(&node, 0, sizeof(GxMsgProperty_NodeByIdGet));
        node.node_type = NODE_PROG;
        node.id = prog_id_array[0];
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        {
            pat_data[i++] = (node.prog_data.ts_id >> 8) & 0xff;
            pat_data[i++] = node.prog_data.ts_id & 0xff;
        }
        else
        {
            pat_data[i++] = 0;
            pat_data[i++] = 1;
        }
        //version num / current_next_indicator
        pat_data[i++] = 1;

        //sec_num
        pat_data[i++] = 0;

        //last_sec_num
        pat_data[i++] = 0;

        for(cnt = 0; cnt < prog_count; cnt++)
        {
            memset(&node, 0, sizeof(GxMsgProperty_NodeByIdGet));
            node.node_type = NODE_PROG;
            node.id = prog_id_array[cnt];
            if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
            {
                //prog_num
                pat_data[i++] = (node.prog_data.service_id >> 8) & 0xff;
                pat_data[i++] = node.prog_data.service_id & 0xff;

                //pmt_pid
                pat_data[i++] = (node.prog_data.pmt_pid >> 8) & 0x1f;
                pat_data[i++] = node.prog_data.pmt_pid & 0xff;
            }
        }

        crc_value = dvb_crc32((unsigned char*)pat_data, i);

        //crc
        pat_data[i++] = (crc_value >> 24 & 0xff);
        pat_data[i++] = (crc_value >> 16 & 0xff);
        pat_data[i++] = (crc_value >> 8 & 0xff);
        pat_data[i++] = (crc_value & 0xff);

#ifdef APP_PSI_LOG
        sec_length = i;
        app_log_debug("\n######### PAT Length = %d #########\n", sec_length);
        for(i = 0; i < sec_length; i++)
        {
            app_log_debug("%02x ", pat_data[i]);
            if(i % 16 == 15)
                app_log_debug("\n");
        }
        app_log_debug("\n###################################\n");
#endif
    }

    return pat_data;
}

TsParseRet pat_section_proc(char *section_data, void *ret_data)
{
    int length = 0;
    int cnt = 0;
    PATInfo *pat_info = NULL;
    int prog_num = 0;
    uint16_t service_id = 0;
    char *ptr = NULL;

    if(section_data == NULL)
        return TS_PARSE_CONTINUE;

    if(ret_data == NULL)
        return TS_PARSE_OVER;

    length = ((unsigned short)(section_data[1] & 0xf)) << 8 | section_data[2];
    length += 3;

    pat_info = (PATInfo*)ret_data;
    pat_info->ts_id = ((section_data[3] << 8) | section_data[4]);
    ptr = section_data + 8;
    prog_num = (length - 12) / 4;
    pat_info->prog_data = (PATProgData*)GxCore_Calloc(prog_num, sizeof(PATProgData));
    pat_info->prog_num = 0;
    if(pat_info->prog_data != NULL)
    {
        for(cnt = 0;  cnt < prog_num; cnt++)
        {
            service_id = (ptr[4 * cnt] << 8) | ptr[4 * cnt + 1];
            if(service_id == 0)
            {
                pat_info->network_id = ((ptr[4 * cnt + 2] & 0x1f) << 8) | ptr[4 * cnt + 3];
            }
            else
            {
                pat_info->prog_data[pat_info->prog_num].service_id = service_id;
                pat_info->prog_data[pat_info->prog_num].pmt_pid = ((ptr[4 * cnt + 2] & 0x1f) << 8) | ptr[4 * cnt + 3];
                pat_info->prog_num++;
            }
        }
    }

    return TS_PARSE_OVER;
}

void pat_data_release(PATInfo *pat_info)
{
    if(pat_info == NULL)
        return;

    if(pat_info->prog_data != NULL)
    {
        GxCore_Free(pat_info->prog_data);
        pat_info->prog_data = NULL;
    }
    pat_info->prog_num = 0;
    pat_info->network_id = 0;
}

