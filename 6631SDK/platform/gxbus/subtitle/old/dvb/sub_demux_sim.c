/**
 *
 * @file        gxsubtitle_demux_pc.c
 * @brief
 * @version     1.1.0
 * @date        04/08/2010 06:06:43 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */

#include <stdio.h>
#include "sub_demux.h"
#include "subtitle/gxsubtitle.h"
#include "sub_debug.h"
#include "gx_mem.h"

#if ((SIMULATOR & DEMUX_SIMULATOR) == DEMUX_SIMULATOR)

#define TS0                 ("/mnt/usb0_1/chuangwei_subtitle_Skyworth.ts")

#define ASSERT(exp)         assert(exp)

#define TSSIZE              188

#define LEN_PACKET_BUF      (204*50)


typedef struct pes_packet_s {
    uint8_t *buffer;
    int length;
    int pos;
} pes_packet_t;

typedef struct {
    FILE*           stream;
    uint16_t        pid;
    pes_packet_t    pespacket;

} sub_sim_dmx;

static sub_sim_dmx sub_demux_simulator;

void sync_tspacket(FILE* fd)
{
    uint8_t syn_byte[204*2] = {0,};
    uint8_t pos;

    while(1) {
        fread(syn_byte, 1, 204*2, fd);

        for (pos = 0;
             pos < 204 ;
             pos++) {
            if (syn_byte[pos] == 0x47 && syn_byte[pos+204] == 0x47) {
                fseek(fd, pos, SEEK_SET);
                return;
            }
        }
    }
    return;
}

int read_tspacket(FILE* fd, uint8_t *buffer)
{
    int len;
    len = fread(buffer, 1, 204, fd);
    ASSERT(buffer[0] == 0x47);
    return len == 204 ? 188 : len;
}

uint16_t get_pid(const uint8_t *tspacket) {
    return (((uint16_t)tspacket[1] & 0x1f) << 8) + tspacket[2];
}

int get_pes_packet_length(const pes_packet_t *packet) {
    return ((uint16_t)packet->buffer[4] << 8) + packet->buffer[5] + 6;
}

int append_to_pes_packet(pes_packet_t *pespacket, const uint8_t *tspacket) {
    int pesstart = 4;

    if ((tspacket[3] & 0x10) == 0) /* has payload? */
        return 0;

    if (pespacket->buffer == NULL ||
        pespacket->pos + TSSIZE > pespacket->length) {
        pespacket->length = 2*pespacket->length + 1000;
        pespacket->buffer = av_realloc(pespacket->buffer, pespacket->length);
    }
    if (tspacket[3] & 0x20) { /* adaptation field? */

        pesstart += 1 + tspacket[4];
    }

    memcpy(pespacket->buffer + pespacket->pos, tspacket + pesstart,
           TSSIZE - pesstart);
    pespacket->pos += TSSIZE - pesstart;

    if (pespacket->pos <= 5 ||
        pespacket->pos < get_pes_packet_length(pespacket))
        return 0;

    return 1;
}


handle_t dmx_open(uint32_t demux_id)
{
    sub_demux_simulator.stream = fopen(TS0, "r");
    sync_tspacket(sub_demux_simulator.stream );
    return (handle_t)&sub_demux_simulator;
}

int32_t dmx_close(handle_t handle)
{
    fclose(sub_demux_simulator.stream);
    return E_OK;
}

static int32_t dmx_set_pid(handle_t handle, uint16_t pid)
{
    sub_demux_simulator.pid = pid;
    return E_OK;
}

static int32_t dmx_start(handle_t handle)
{
    return E_OK;
}

static int32_t dmx_stop(handle_t handle)
{
    return E_OK;
}

static size_t  dmx_get_pes(handle_t handle, uint8_t** ptr, size_t size)
{
    uint8_t         tspacket[204];
    uint16_t        pes_length;
    uint16_t        pid;
REDO:
    while (read_tspacket(sub_demux_simulator.stream, tspacket) == TSSIZE) {
        pid = get_pid(tspacket);
        if (pid != sub_demux_simulator.pid)
            continue;
/*         write_tspacket(ofd, tspacket); */
        if (append_to_pes_packet(&sub_demux_simulator.pespacket, tspacket)) {

            /* Send PES packet for subtitle decoder to handle */
            pes_length = get_pes_packet_length(&sub_demux_simulator.pespacket);
            *ptr = sub_demux_simulator.pespacket.buffer;
            sub_demux_simulator.pespacket.pos = 0;
            return pes_length;
        }
    }
    sub_demux_simulator.pespacket.pos = 0;
    fseek(sub_demux_simulator.stream, 0, SEEK_SET);
    sync_tspacket(sub_demux_simulator.stream);
    gxlogd("TS file read finished, do it again\n");
    goto REDO;
    return -1;
}

static int64_t  dmx_get_stc(handle_t handle)
{
    return 0xFFFFFFFF;
}

GxSubtitle_DemuxOps  gxsub_demux = {
                                        dmx_open,
                                        dmx_close,
                                        dmx_start,
                                        dmx_stop,
                                        dmx_get_stc,
                                        dmx_set_pid,
                                        dmx_get_pes,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL
                                    };

#endif

