#include "app_module.h"

typedef struct
{
    char *data;
    unsigned int length;
}fifoBlock;

typedef struct
{
    int fifo_head;
    int fifo_tail;
    int block_num;
    int block_total;
    unsigned int block_size;
    fifoBlock *fifo_block;
    handle_t fifo_mutex;
    handle_t  fifo_sem;
}AppFIFO;

status_t app_fifo_destroy(handle_t fifo)
{
    AppFIFO *temp_fifo = (AppFIFO*)fifo;
    int i = 0;

    if(temp_fifo == NULL)
        return GXCORE_ERROR;

    GxCore_MutexLock(temp_fifo->fifo_mutex);
    if(temp_fifo->fifo_block != NULL)
    {
        for(i = 0; i < temp_fifo->block_total; i++)
        {
            if(temp_fifo->fifo_block[i].data != NULL)
            {
                GxCore_Free(temp_fifo->fifo_block[i].data);
            }
        }
        GxCore_Free(temp_fifo->fifo_block);
    }

    GxCore_SemDelete(temp_fifo->fifo_sem);
    GxCore_MutexDelete(temp_fifo->fifo_mutex);
    GxCore_Free(temp_fifo);

    return GXCORE_SUCCESS;
}

status_t app_fifo_create(handle_t *fifo, unsigned int block_size, unsigned int block_num)
{
    AppFIFO *temp_fifo = NULL;
    int i = 0;

    if((fifo == NULL) || (block_size == 0) || (block_num == 0))
        return GXCORE_ERROR;

    if((temp_fifo = (AppFIFO*)GxCore_Calloc(1, sizeof(AppFIFO))) == NULL)
        return GXCORE_ERROR;

    if((temp_fifo->fifo_block = (fifoBlock*)GxCore_Calloc(block_num, sizeof(fifoBlock))) == NULL)
        return GXCORE_ERROR;

    GxCore_MutexCreate(&temp_fifo->fifo_mutex);
    GxCore_SemCreate(&temp_fifo->fifo_sem, 0);

    for(i = 0; i < block_num; i++)
    {
        if((temp_fifo->fifo_block[i].data = (char*)GxCore_Calloc(1, block_size)) == NULL)
        {
            app_fifo_destroy((handle_t)temp_fifo);
            app_log_error("[%s]%d fifo failed\n", __func__, __LINE__);
            return GXCORE_ERROR;
        }
    }

    temp_fifo->block_total = block_num;
    temp_fifo->block_size = block_size;
    temp_fifo->fifo_head = 0;
    temp_fifo->fifo_tail = -1;

    *fifo = (handle_t)temp_fifo;

    return GXCORE_SUCCESS;
}

unsigned int app_fifo_write(handle_t fifo, char *write_data,  unsigned int write_size)
{
    AppFIFO *temp_fifo = (AppFIFO*)fifo;
    unsigned int cp_size = 0;

    if((temp_fifo == NULL) || (write_data == NULL) || (write_size == 0))
        return 0;

    GxCore_MutexLock(temp_fifo->fifo_mutex);
    if(temp_fifo->block_num >= temp_fifo->block_total)
    {
        app_log_debug("[%s]%d, fifo full!!!\n", __func__, __LINE__);
        GxCore_MutexUnlock(temp_fifo->fifo_mutex);
        return 0;
    }

    temp_fifo->fifo_tail++;
    if(temp_fifo->fifo_tail >= temp_fifo->block_total)
        temp_fifo->fifo_tail = 0;

    (write_size >= temp_fifo->block_size)
        ? (cp_size = temp_fifo->block_size) : (cp_size = write_size);
    memcpy(temp_fifo->fifo_block[temp_fifo->fifo_tail].data, write_data, cp_size);
    temp_fifo->fifo_block[temp_fifo->fifo_tail].length = cp_size;

    temp_fifo->block_num++;

    GxCore_SemPost(temp_fifo->fifo_sem);
    GxCore_MutexUnlock(temp_fifo->fifo_mutex);

    return cp_size;
}

unsigned int app_fifo_read(handle_t fifo, char *read_data,  unsigned int read_size)
{
    AppFIFO *temp_fifo = (AppFIFO*)fifo;
    unsigned int cp_size = 0;

    if((temp_fifo == NULL) || (read_data == NULL) || (read_size == 0))
        return 0;

    GxCore_SemWait(temp_fifo->fifo_sem);
    GxCore_MutexLock(temp_fifo->fifo_mutex);

    if(temp_fifo->block_num == 0)
    {
        app_log_debug("[%s]%d, fifo empty!!!\n", __func__, __LINE__);
        GxCore_MutexUnlock(temp_fifo->fifo_mutex);
        return 0;
    }

    (read_size >= temp_fifo->fifo_block[temp_fifo->fifo_head].length)
        ? (cp_size = temp_fifo->fifo_block[temp_fifo->fifo_head].length) : (cp_size = read_size);
    memcpy(read_data, temp_fifo->fifo_block[temp_fifo->fifo_head].data, cp_size);
    read_size = temp_fifo->fifo_block[temp_fifo->fifo_head].length;

    temp_fifo->fifo_head++;
    if(temp_fifo->fifo_head >= temp_fifo->block_total)
        temp_fifo->fifo_head = 0;

    temp_fifo->block_num--;

    GxCore_MutexUnlock(temp_fifo->fifo_mutex);

    return cp_size;
}

