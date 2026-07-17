#ifndef _GX_ST_POOL_H
#define _GX_ST_POOL_H


typedef struct _STDrawPool_s
{
    uint32_t max;
    int (*init)(void);
    int (*destory)(void);
    void (*put)(uint64_t pts, char* subt_data);
    void (*get)(uint64_t *pts, char **subt_data);
    int (*get_num)(void);
}GxSTDrawPool;

extern GxSTDrawPool g_STDrawPool;
#endif
