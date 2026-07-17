#ifndef __DEMUX_PES_H__
#define __DEMUX_PES_H__

#define PES_BLOCK_SIZE (128*1024)

#include "gx_common.h"
#include "gx_demux.h"

typedef struct {
    uint32_t    read_pos;
    uint32_t    write_pos;
    uint8_t     buffer[PES_BLOCK_SIZE];
}MpegPes;


handle_t MpegPesCreate(void);
void MpegPesDestory(handle_t handle);
int MpegPesInsertBuffer(handle_t handle, uint8_t* buffer, int size);
void MpegPesClear(handle_t handle);
int MpegPesReadEs(handle_t handle, uint8_t* buffer, int size, int64_t* pts, struct gx_ctrl_info* ctrlinfo);
#endif
