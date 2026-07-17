#include "app.h"
#if FM_SUPPORT
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_fm.h"
#include <gx_apps.h>

static int s_fm_freq_num = 0;
static int fm_handle = -1;

static int freq_cmp(const void *a, const void *b)
{
    return ((*(int*)a) - (*(int*)b));
}

int app_fm_get_handle(void)
{
    int tuner = 0;
    if(fm_handle == -1)
    {
        tuner = app_get_fm_tuner_id();
        if(tuner >= 0)
            fm_handle = GxFeOpen(tuner);
    }

    return fm_handle;
}

int32_t app_fm_get_freq(int32_t index)
{
    int i= 0;
    char temp_buf[20]={0};
    FILE *fp = NULL;
    int32_t freq=0;

    if(((fp = fopen(FM_FREQ_DATA, "r"))) == NULL)
        return 0;

    while(fgets(temp_buf, sizeof(temp_buf), fp))
    {
        if(i == index)
        {
            freq = atoi(temp_buf);
            break;
        }
        memset(temp_buf,0,20);
        i++;
    }
    fclose(fp);
    return freq;

}

int app_fm_freq_save(uint32_t* freq_array, int freq_num)
{
    FILE *fp = NULL;
    char temp_buf[20]={0};
    int i=0;

    if((fp = fopen(FM_FREQ_DATA, "w")) == NULL)
        return -1;

    s_fm_freq_num = 0;
    qsort(freq_array, freq_num, sizeof(int), freq_cmp);
    for(i=0; i<freq_num; i++)
    {
        if(freq_array[i])
        {
            sprintf(temp_buf,"%d",(freq_array[i]));
            fwrite(temp_buf,1,strlen(temp_buf),fp);
            fwrite("\r\n",1,2,fp);
            s_fm_freq_num++;
        }
    }
    fclose(fp);

    return 0;
}

#if FM_FREQ_NAME_SUPPORT
static gx_list_t *fm_freq_name_list = NULL;
char* app_fm_get_freq_name(uint32_t freq)
{
    int i=0;
    char buff[20]= {0};

    sprintf(buff, "%.1f", freq/1000.0);
    for(i=0;i<fm_freq_name_list->item_num;i++)
    {
        if(strcmp(fm_freq_name_list->items[i].key,buff)==0)
            return fm_freq_name_list->items[i].value;
    }
    return "Unknow";
}

void app_fm_update_freq_name(FmListData* data)
{
    int i=0;
    char buff[20]= {0};

    sprintf(buff, "%.1f", data->freq/1000.0);
    for(i=0;i<fm_freq_name_list->item_num;i++)
    {
        if(strcmp(fm_freq_name_list->items[i].key,buff)==0)
        {
            gx_list_edit_item(fm_freq_name_list,i,fm_freq_name_list->items[i].key, data->name,NULL);
            gx_list_save_to_file(fm_freq_name_list, FM_USER_FREQ_NAME_LIST_PATH);
            return;
        }
    }
    gx_list_add_item(fm_freq_name_list,buff, data->name,NULL);
}

void fm_freq_name_list_destroy(void)
{
    if(fm_freq_name_list)
    {
        gx_list_save_to_file(fm_freq_name_list, FM_USER_FREQ_NAME_LIST_PATH);
        gx_list_free(fm_freq_name_list);
        fm_freq_name_list = NULL;
    }
}
#endif

void app_fm_data_init(void)
{
    FILE *fp = NULL;
    char temp_buf[20]={0};
    int num = 0;

    if(GxCore_FileExists(FM_FREQ_DATA)==GXCORE_FILE_EXIST && (fp = fopen(FM_FREQ_DATA, "r")) != NULL)
    {
        while(fgets(temp_buf, sizeof(temp_buf), fp))
        {
            num++;
        }
        s_fm_freq_num = num;
        fclose(fp);
    }
    else
    {
        s_fm_freq_num = 0;
    }
#if FM_FREQ_NAME_SUPPORT
    if(GXCORE_FILE_EXIST == GxCore_FileExists(FM_USER_FREQ_NAME_LIST_PATH))
    {
        fm_freq_name_list = gx_get_list_from_file(FM_USER_FREQ_NAME_LIST_PATH);
    }
    if(!fm_freq_name_list)
    {
        if(GXCORE_FILE_EXIST == GxCore_FileExists(FM_DEFAULT_FREQ_NAME_LIST_PATH))
        {
            fm_freq_name_list = gx_get_list_from_file(FM_DEFAULT_FREQ_NAME_LIST_PATH);
        }
        if(!fm_freq_name_list)
        {
            fm_freq_name_list = gx_list_new();
        }
        gx_list_save_to_file(fm_freq_name_list, FM_USER_FREQ_NAME_LIST_PATH);
    }
#endif
}

int app_fm_freq_num_get(void)
{
    return s_fm_freq_num;
}

int app_fm_freq_array_get(uint32_t* freq_array)
{
    int i= 0;
    char temp_buf[20]={0};
    FILE *fp = NULL;

    if((freq_array == NULL) ||((fp = fopen(FM_FREQ_DATA, "r"))) == NULL)
        return -1;

    while(fgets(temp_buf, sizeof(temp_buf), fp))
    {
        freq_array[i]=atoi(temp_buf);
        memset(temp_buf,0,20);
        i++;
    }
    fclose(fp);

    return 0;
}

int app_fm_freq_list_get(FmListData* freq_list_data)
{
    int i= 0;
    char temp_buf[20]={0};
    FILE *fp = NULL;

    if((freq_list_data == NULL) ||((fp = fopen(FM_FREQ_DATA, "r"))) == NULL)
        return -1;

    while(fgets(temp_buf, sizeof(temp_buf), fp))
    {
        freq_list_data[i].freq = atoi(temp_buf);
        freq_list_data[i].del_flag = 0;
        memset(freq_list_data[i].name,0,FM_FREQ_NAME_LEN);
#if FM_FREQ_NAME_SUPPORT
        strncpy(freq_list_data[i].name, app_fm_get_freq_name(freq_list_data[i].freq),FM_FREQ_NAME_LEN-1);
#endif
        memset(temp_buf,0,20);
        i++;
    }
    fclose(fp);

    return 0;
}

void app_fm_freq_del_file(void)
{
    if(GXCORE_FILE_EXIST == GxCore_FileExists(FM_FREQ_DATA))
        GxCore_FileDelete(FM_FREQ_DATA);

    s_fm_freq_num = 0;
    GxBus_ConfigSetInt(FM_PLAY_INDEX, FM_PLAY_INDEX_VAL);

    return;
}

bool app_fm_freq_check_exist(uint32_t freq)
{
    int i=0;
    uint32_t* freq_array=NULL;
    bool ret = false;

    freq_array =(uint32_t*)GxCore_Malloc(s_fm_freq_num*sizeof(uint32_t));
    if(freq_array)
    {
        memset(freq_array, 0, s_fm_freq_num*sizeof(uint32_t));
        app_fm_freq_array_get(freq_array);
        for(i=0;i<s_fm_freq_num;i++)
        {
            if(freq_array[i] == freq)
            {
                ret = true;
                break;
            }
        }
        GXCORE_FREE(freq_array);
    }
    return ret;
}

int app_fm_freq_del(FmListData* freq_list_data)
{
    int num = 0;
    int i = 0;
    uint32_t* freq_array = NULL;

    freq_array = (uint32_t*)GxCore_Malloc(s_fm_freq_num*sizeof(uint32_t));
    if(freq_array)
    {
        memset(freq_array, 0, s_fm_freq_num*sizeof(uint32_t));
        app_fm_freq_array_get(freq_array);
        num = app_fm_freq_num_get();
        for(i=0; i<num; i++)
        {
            if(freq_list_data[i].del_flag == true)
            {
                freq_array[i]=0;
            }
        }
        app_fm_freq_save(freq_array, num);
        GXCORE_FREE(freq_array);
    }

    return 0 ;
}

int app_fm_freq_add_one(uint32_t freq)
{
    uint32_t* freq_array=NULL;
    freq_array = (uint32_t*)GxCore_Malloc((s_fm_freq_num+1)*sizeof(uint32_t));
    if(freq_array)
    {
        memset(freq_array, 0, (s_fm_freq_num+1)*sizeof(uint32_t));
        app_fm_freq_array_get(freq_array);
        freq_array[s_fm_freq_num] = freq;
        app_fm_freq_save(freq_array, s_fm_freq_num+1);
        GXCORE_FREE(freq_array);
    }
    return 0 ;
}


#endif
