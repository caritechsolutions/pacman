#include "gxcore.h"
#include "module/app_ca.h"
#include "include/descrambler.h"
#include "module/app_log.h"
#ifndef NULL
#define NULL ((void*)0)
#endif
// gx3211 can use 16 descrambler
#define MAX_DESCRAMBER_SUPPORT 8
static ControlClass thiz_descrambler_control[MAX_DESCRAMBER_SUPPORT];

int app_descrambler_init(unsigned char demux_id, ControlTypeEnum control_type, DataTypeEnum data_type)
{
    int ret = 0;
    int i = 0;
    if(control_type >= CONTROL_TOTAL)
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return 0;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if(thiz_descrambler_control[i].handle == 0)
        {
            thiz_descrambler_control[i].handle = Descrmb_Open(demux_id);
            if(thiz_descrambler_control[i].handle != 0)
            {
                thiz_descrambler_control[i].demux_id = demux_id;
                thiz_descrambler_control[i].data_type = data_type;
                thiz_descrambler_control[i].control_type = control_type;
                return thiz_descrambler_control[i].handle;
            }
            else
            {
                app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
                return 0;
            }
        }
    }
    if(i == MAX_DESCRAMBER_SUPPORT)
    {
        app_log_info("\nDESCRAMBLER, warning, descrambler is full\n");
    }
    return ret;
}

int app_descrambler_close_by_pid(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type)
{
    int ret = 0;
    int i = 0;
    if(control_type >= CONTROL_TOTAL)
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].handle != 0)
                && (thiz_descrambler_control[i].control_type == control_type)
                && (thiz_descrambler_control[i].data_type == data_type)
                && (thiz_descrambler_control[i].element_pid == pid))
        {
            ret = Descrmb_Close(thiz_descrambler_control[i].handle);
            if(ret == 0)
            {
                memset(&thiz_descrambler_control[i], 0, sizeof(ControlClass));
                break;
            }
            else
            {
                app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    if(i == MAX_DESCRAMBER_SUPPORT)
    {
        //app_log_info("\nDESCRAMBLER, warning, descrambler is not existed\n");
    }
    return ret;
}

int app_descrambler_close_by_type(ControlTypeEnum control_type, DataTypeEnum data_type)
{
    int ret = 0;
    int i = 0;
    if(control_type >= CONTROL_TOTAL)
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].handle != 0)
                && (thiz_descrambler_control[i].control_type == control_type)
                && (thiz_descrambler_control[i].data_type == data_type))
        {
            ret = Descrmb_Close(thiz_descrambler_control[i].handle);
            if(ret == 0)
            {
                memset(&thiz_descrambler_control[i], 0, sizeof(ControlClass));
            }
            else
            {
                app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    if(i == MAX_DESCRAMBER_SUPPORT)
    {
        //app_log_info("\nDESCRAMBLER, warning, descrambler is not existed\n");
    }
    return ret;
}


int app_descrambler_close_by_handle(int handle)
{
    int ret = 0;
    int i = 0;
    if(handle == 0)
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if(thiz_descrambler_control[i].handle == handle)
        {
            ret = Descrmb_Close(thiz_descrambler_control[i].handle);
            if(ret == 0)
            {
                memset(&thiz_descrambler_control[i], 0, sizeof(ControlClass));
                break;
            }
            else
            {
                app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    if(i == MAX_DESCRAMBER_SUPPORT)
    {
        //app_log_info("\nDESCRAMBLER, warning, descrambler is not existed\n");
    }
    return ret;
}

int app_descrambler_close_by_control_type(ControlTypeEnum control_type)
{
    int ret = 0;
    int i = 0;

    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].handle != 0)
                &&(thiz_descrambler_control[i].control_type == control_type))
        {
            ret = Descrmb_Close(thiz_descrambler_control[i].handle);
            if(ret == 0)
            {
                memset(&thiz_descrambler_control[i], 0, sizeof(ControlClass));
            }
            else
            {
                app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    if(i == MAX_DESCRAMBER_SUPPORT)
    {
        //app_log_info("\nDESCRAMBLER, warning, descrambler is not existed\n");
    }
    return ret;
}

int app_descrambler_close_all(void)
{
    int ret = 0;
    int i = 0;
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if(thiz_descrambler_control[i].handle != 0)
        {
            ret = Descrmb_Close(thiz_descrambler_control[i].handle);
            if(ret == 0)
            {
                memset(&thiz_descrambler_control[i], 0, sizeof(ControlClass));
                break;
            }
            else
            {
                app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    return ret;
}

int app_descrambler_set_pid(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type)
{
    int ret = 0;
    int i = 0;
    int record_index = -1;
    if((0x1fff == pid) || (CONTROL_TOTAL <= control_type) || (DATA_TYPE_UNIVERSAL <= data_type))
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].handle != 0)
            && (thiz_descrambler_control[i].control_type == control_type)
            && (thiz_descrambler_control[i].data_type == data_type)
            && (0 == thiz_descrambler_control[i].element_pid)
            && (record_index == -1))
        {
            record_index = i;
            break;
        }
        else if((thiz_descrambler_control[i].handle != 0)
                 && (thiz_descrambler_control[i].control_type == control_type)
                 && (thiz_descrambler_control[i].data_type == data_type)
                 && (pid == thiz_descrambler_control[i].element_pid))
        {
            app_log_error("\nDESCRAMBLER, warning, %s, %d\n", __FUNCTION__,__LINE__);
            return 0;
        }
    }
    if(-1 != record_index)
    {
        thiz_descrambler_control[record_index].element_pid = pid;
        Descrmb_SetStreamPID(thiz_descrambler_control[record_index].handle, pid);
    }
    else
    {
        app_log_error("\nDESCRAMBLER, warning, %s, %d\n", __FUNCTION__,__LINE__);
        return 0;
    }
    return ret;
}

int app_descrambler_get_handle_by_pid(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type)
{
    int ret = 0;
    int i = 0;
    if((0x1fff == pid) || (CONTROL_TOTAL <= control_type) || (DATA_TYPE_UNIVERSAL <= data_type))
    {
        return -1;
    }

    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].handle != 0)
            && (thiz_descrambler_control[i].control_type == control_type)
            && (thiz_descrambler_control[i].data_type == data_type)
            && (pid == thiz_descrambler_control[i].element_pid))
        {
            return thiz_descrambler_control[i].handle;
        }
    }

    return ret;
}

int app_descrambler_set_pid_by_handle(int handle, unsigned short pid)
{
    int ret = 0;
    int i = 0;
    if((0x1fff == pid) || (0 == handle))
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if(thiz_descrambler_control[i].handle == handle)
        {
            thiz_descrambler_control[i].element_pid = pid;
            Descrmb_SetStreamPID(thiz_descrambler_control[i].handle, pid);
            return 0;
        }
    }
    //app_log_error("\nDESCRAMBLER, warning, %s, %d\n", __FUNCTION__,__LINE__);
    return ret;
}

int app_descrambler_set_cw(int handle,
                                const unsigned char *even,
                                const unsigned char *odd,
                                unsigned char DecryptFlag)// 3des or cas2
{
    int ret = 0;
    int i = 0;
    if(0 == handle)
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if(thiz_descrambler_control[i].handle == handle)
        {
            char cw[8] = {0};
            if((NULL != odd) && (0 != memcmp(odd, cw, 8)))
            {
                Descrmb_SetOddKey(handle, odd, 8, DecryptFlag);
            }
            if((NULL != even) && (0 != memcmp(even, cw, 8)))
            {
                Descrmb_SetEvenKey(handle, even, 8, DecryptFlag);
            }
        }
    }
    //app_log_error("\nDESCRAMBER, warning, %s, %d\n", __FUNCTION__,__LINE__);
    return ret;
}

int app_descrambler_set_cw_by_control_type(ControlTypeEnum type,
                                const unsigned char *even,
                                const unsigned char *odd,
                                unsigned char DecryptFlag)// 3des or cas2
{
    int ret = 0;
    int i = 0;
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].control_type == type)
                &&(thiz_descrambler_control[i].handle != 0))
        {
            char cw[8] = {0};
            if((NULL != odd) && (0 != strncmp((const char*)odd, cw, 8)))
            {
                Descrmb_SetOddKey(thiz_descrambler_control[i].handle, odd, 8, DecryptFlag);
            }
            if((NULL != even) && (0 != strncmp((const char*)even, cw, 8)))
            {
                Descrmb_SetEvenKey(thiz_descrambler_control[i].handle, even, 8, DecryptFlag);
            }
        }
    }
    return ret;
}

int app_descrambler_set_cw_by_define(ControlTypeEnum type,
                                const unsigned char key_type,
                                const unsigned char *even,
                                const unsigned char *odd,
                                unsigned char DecryptFlag)// 3des or cas2
{
    int ret = 0;
    int i = 0;
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].control_type == type)
                && (thiz_descrambler_control[i].handle != 0)
                && (thiz_descrambler_control[i].data_type == key_type))
        {
            char cw[8] = {0};
            if((NULL != odd) && (0 != strncmp((const char*)odd, cw, 8)))
            {
                Descrmb_SetOddKey(thiz_descrambler_control[i].handle, odd, 8, DecryptFlag);
            }
            if((NULL != even) && (0 != strncmp((const char*)even, cw, 8)))
            {
                Descrmb_SetEvenKey(thiz_descrambler_control[i].handle, even, 8, DecryptFlag);
            }
            return 0;
        }
    }
    //app_log_error("\nDESCRAMBER, warning, %s, %d\n", __FUNCTION__,__LINE__);
    return ret;
}


int app_descramber_get_cw_by_handle(int handle,
                                        unsigned char *even,
                                        unsigned char *odd)
{
    int ret = 0;
    ret = Descrmb_GetCW(handle, odd, even);
    return ret;
}

int app_descrambler_get_cw_by_pid(unsigned short pid,
                                    ControlTypeEnum control_type,
                                    DataTypeEnum data_type,
                                    unsigned char* even,
                                    unsigned char* odd)
{
    int ret = 0;
    int i = 0;
    if((0x1fff == pid) || (CONTROL_TOTAL <= control_type) || (DATA_TYPE_UNIVERSAL <= data_type))
    {
        app_log_error("\nDESCRAMBLER, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < MAX_DESCRAMBER_SUPPORT; i++)
    {
        if((thiz_descrambler_control[i].handle != 0)
            && (thiz_descrambler_control[i].control_type == control_type)
            && (thiz_descrambler_control[i].data_type == data_type)
            && (pid == thiz_descrambler_control[i].element_pid))
        {
            ret = Descrmb_GetCW(thiz_descrambler_control[i].handle, odd, even);
            break;
        }
    }

    return ret;
}


