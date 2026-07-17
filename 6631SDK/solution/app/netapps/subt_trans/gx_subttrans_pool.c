#include "gxcore.h"
#include "app.h"
#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans_pool.h"
#include "gx_subttrans.h"

#define ST_DRAW_POOL_MAX (10)
typedef struct _subt_getinfo_s
{
    STDrawInfo_t subt;
    char need_show;
}subt_getinfo_t;

typedef struct _subt_drawpool_s
{
    subt_getinfo_t *infolist;
    int info_num;
    int max_num;
    handle_t mutex;
}subt_drawpool_t;

static subt_drawpool_t s_subtpool;

static int st_subt_drawpool_init(void)
{
    memset(&s_subtpool, 0, sizeof(subt_drawpool_t));
    GxCore_MutexCreate(&s_subtpool.mutex);
    if(g_STDrawPool.max <= 0)
        s_subtpool.max_num = ST_DRAW_POOL_MAX;
    else
        s_subtpool.max_num = g_STDrawPool.max;
    s_subtpool.infolist = GxCore_Calloc(s_subtpool.max_num, sizeof(subt_getinfo_t));
    if(!s_subtpool.infolist) {
        s_subtpool.max_num = 0;
        return -1;
    }
    return 0;
}

static void st_subt_drawpool_free_single(int index)
{
    if((index<0) || (index>s_subtpool.max_num))
        return;
    APP_FREE(s_subtpool.infolist[index].subt.subt_data);
    s_subtpool.infolist[index].subt.pts = 0;
    s_subtpool.infolist[index].need_show = 0;
}

static int st_subt_drawpool_destory(void)
{
    int i = 0;
    GxCore_MutexLock(s_subtpool.mutex);
    for(i=0; i<s_subtpool.max_num; i++)
    {
        st_subt_drawpool_free_single(i);
    }
    APP_FREE(s_subtpool.infolist);
    s_subtpool.max_num = 0;
    s_subtpool.info_num = 0;
    GxCore_MutexUnlock(s_subtpool.mutex);
    GxCore_MutexDelete(s_subtpool.mutex);
    return 0;
}

static void st_put_subt_drawpool(uint64_t pts, char* subt_data)
{
    int i = 0;
    if((!subt_data) || (s_subtpool.max_num<=0))
        return;
    GxCore_MutexLock(s_subtpool.mutex);
    for(i=0; i<s_subtpool.max_num; i++)
    {
        if(s_subtpool.infolist[i].need_show == 0){
            s_subtpool.infolist[i].subt.pts = pts;
            s_subtpool.infolist[i].subt.subt_data = strdup(subt_data);
            s_subtpool.infolist[i].need_show = 1;
            s_subtpool.info_num ++;
            ST_API_PRINTF("%s[%d] i=%d, max=%d\n", __func__, __LINE__, i, s_subtpool.info_num);
            break;
        }
    }
    GxCore_MutexUnlock(s_subtpool.mutex);
}

static void st_get_subt_drawpool_minpts(uint64_t *pts, char **subt_data)
{
    int i = 0;
    int min_index = -1;
    uint64_t min_pts = 0;
    if((!pts) || (s_subtpool.max_num<=0) || (s_subtpool.info_num<=0))
        return;
    GxCore_MutexLock(s_subtpool.mutex);
    min_pts = s_subtpool.infolist[i].subt.pts;
    for(i=0; i<s_subtpool.max_num; i++)
    {
        if((s_subtpool.infolist[i].need_show == 1) && (s_subtpool.infolist[i].subt.pts <= min_pts)) {
            min_pts = s_subtpool.infolist[i].subt.pts;
            min_index = i;
        }
    }
    ST_API_PRINTF("%s[%d] get min_index=%d, last_num=%d\n", __func__, __LINE__, min_index, s_subtpool.info_num-1);
    if(min_index != -1) {
        *pts = s_subtpool.infolist[min_index].subt.pts;
        *subt_data = strdup(s_subtpool.infolist[min_index].subt.subt_data);
        st_subt_drawpool_free_single(min_index);
        s_subtpool.info_num --;
    }
    GxCore_MutexUnlock(s_subtpool.mutex);
}

static int st_get_subt_drawpool_num(void)
{
    return s_subtpool.info_num;
}

GxSTDrawPool g_STDrawPool = {
    .max     = 0,
    .init    = st_subt_drawpool_init,
    .destory = st_subt_drawpool_destory,
    .put     = st_put_subt_drawpool,
    .get     = st_get_subt_drawpool_minpts,
    .get_num = st_get_subt_drawpool_num,
};

#endif
