#ifdef ECOS_OS

#include "gxcore.h"
#include "bsp_panel_ops.h"

static struct CYG_SELINFO_TAG   sg_PanelSignal;
static cyg_bool _panel_select(cyg_io_handle_t handle, cyg_uint32 which, CYG_ADDRWORD info);

static int timer_id = -1;
static int timer_pid= -1;

void create_bsp_timer(timer_func func, unsigned long ms)
{
    if(timer_id == -1)
    {
        timer_id = gx_rtc_timer_create(func, 0, 0, ms);
    }
}

void delete_bsp_timer(void)
{
    if(timer_id != -1)
    {
        gx_rtc_timer_delete(timer_id);
        timer_id = -1;
    }
}

////////////////////////////////////////////////////////////////
void create_key_timer(timer_func func, unsigned long ms)
{
    if(timer_pid == -1)
    {
        timer_pid = gx_rtc_timer_create(func, 0, 0, ms);
    }
}

void delete_key_timer(void)
{
    if(timer_pid != -1)
    {
        gx_rtc_timer_delete(timer_pid);
        timer_pid = -1;
    }
}
/////////////////////////////////////////////////////////////

static Cyg_ErrNo _panel_open(struct cyg_devtab_entry **tab,
                       struct cyg_devtab_entry *sub_tab,
                       const char *name)
{
    //printf("-----------[%s]----------\n", __func__);
    if(g_BspPanel.panel_open != NULL)
    {
        g_BspPanel.panel_open();
    }
    return ENOERR;
}

static Cyg_ErrNo _panel_close(cyg_io_handle_t handle)
{
    //printf("-----------[%s]----------\n", __func__);
    if(g_BspPanel.panel_close!= NULL)
    {
        g_BspPanel.panel_close();
    }
    return ENOERR;
}

static Cyg_ErrNo _panel_read(cyg_io_handle_t handle,
                         void *buf, cyg_uint32 *len)
{
    //printf("-----------[%s]----------\n", __func__);
    if(g_BspPanel.panel_read!= NULL)
    {
        g_BspPanel.panel_read((uint32_t*)buf);
    }
    return ENOERR;
}

static Cyg_ErrNo _panel_ioctl(cyg_io_handle_t handle, cyg_uint32 key, void *buf)
{
    //printf("-----------[%s]----------\n", __func__);
    switch(key)
    {
        case PANEL_SHOW_STR:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_str != NULL))
            {
                PanelString str = *(PanelString*)buf;
                g_BspPanel.panel_ioctl->io_show_str(str);
            }
            break;

        case PANEL_SHOW_NUM:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_num != NULL))
            {
                unsigned int num = *(unsigned int*)buf;
                g_BspPanel.panel_ioctl->io_show_num(num);
            }
            break;

        case PANEL_READ_KEY:
            //printf("----------------------- panel read ----------\n");
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_read_key != NULL))
            {
                g_BspPanel.panel_ioctl->io_read_key((unsigned int*)buf);
            }
            break;

        case PANEL_SHOW_LOCK:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_lock != NULL))
            {
                unsigned int lock = *(unsigned int*)buf;
                g_BspPanel.panel_ioctl->io_show_lock(lock);
            }
            break;
        case PANEL_SHOW_STANDBY:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_standby!= NULL))
            {
                unsigned int standby = *(unsigned int*)buf;
                g_BspPanel.panel_ioctl->io_show_standby(standby);
            }
            break;
        case PANEL_SHOW_QUALITY:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_quality != NULL))
            {
                unsigned int quality = *(unsigned int*)buf;
                g_BspPanel.panel_ioctl->io_show_quality(quality);
            }
            break;

         case PANEL_SHOW_TIME:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_time!= NULL))
            {
                unsigned int time = *(unsigned int*)buf;
                g_BspPanel.panel_ioctl->io_show_time(time);
            }
            break;

        case PANEL_SET_GPIOMAP:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_set_gpiomap != NULL))
            {
                g_BspPanel.panel_ioctl->io_set_gpiomap(buf);
            }
            break;
        case PANEL_SET_KEYMAP:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_set_keymap != NULL))
            {
                g_BspPanel.panel_ioctl->io_set_keymap(buf);
            }
            break;

        case PANEL_MCU_LOWPOWER:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_mcu_lowpower != NULL))
            {
                g_BspPanel.panel_ioctl->io_mcu_lowpower();
            }
            break;
     case PANEL_SHOW_BRIGHTNESS:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_brigthness != NULL))
            {

                unsigned int brigthness = *(unsigned int*)buf;
                g_BspPanel.panel_ioctl->io_show_brigthness(brigthness);
            }
            break;
    }

    return ENOERR;
}

static Cyg_ErrNo _panel_set_config(cyg_io_handle_t handle, cyg_uint32 key,
                                          const void* buf, cyg_uint32* len)
{
    return ENOERR;
}

// 整理poll和select的函数实现，去掉原有的key的耦合关联。增加全比变量
// "thiz_slect_active"，配对poll和select的使用。
// 0: not active; 1: active
static char thiz_slect_active = 0;
void poll_wakeup(void)
{
    if(0 == thiz_slect_active)
    {
        thiz_slect_active = 1;
	    cyg_selwakeup(&sg_PanelSignal);
    }
}

static cyg_bool _panel_select(cyg_io_handle_t handle, cyg_uint32 which, CYG_ADDRWORD info)
{
   cyg_bool retval = false;

   switch(which)
   {
     case CYG_FREAD:
        if(thiz_slect_active == 0)
        {
            cyg_selrecord( info, &sg_PanelSignal);
        }
        else
        {
            thiz_slect_active = 0;
            retval = true;
        }
        break;
     case CYG_FWRITE:    //write - not support
        break;
     case 0:             //exceptions - not support
        break;
     default:
        break;
   }
     return retval;
}

static cyg_devio_table_t panel_handlers =
{
    .read = _panel_read,
    .ioctl = _panel_ioctl,
    .close = _panel_close,
    .set_config = _panel_set_config,
    .select = _panel_select,
};

static cyg_devtab_entry_t panel_dev =
{
    PANEL_NAME,
    NULL,
    &panel_handlers,
    NULL,
    _panel_open,
    NULL
};

void bsp_panel_mod_init(void)
{
    Cyg_ErrNo ret = char_dev_register(&panel_dev, PANEL_NAME, NULL);

    if(g_BspPanel.panel_init != NULL)
    {
        gx3xxx_rtc_timer_init();
        g_BspPanel.panel_init();
        thiz_slect_active = 0;
        cyg_selinit(&sg_PanelSignal);
    }
}

#endif
