#ifndef __RICHEPG_SECTIONS_H__
#define __RICHEPG_SECTIONS_H__
#include "gxos/gxcore_os_core.h"
#include "common/gxmalloc.h"
#include "dvbhal/gxdemux_hal.h"
#include <time.h>

#define MAX_BIDS (3)
typedef uint16_t re_homets_bids[MAX_BIDS];

struct richepg_nit_ts
{
    uint16_t transport_stream_id;
    uint16_t original_network_id;
    uint32_t frequency;         // MHz
    uint32_t symbol_rate;       // Ksymbol/s
    int16_t  orbital_position;  // degree*10
    uint8_t  west_east_flag: 1;
    uint8_t  polar: 2;
    uint8_t  mod_system: 5;
    uint8_t  fec_inner;
};

struct richepg_homets
{
    uint8_t state;
    re_homets_bids bouquet_ids;

    struct richepg_tdt
    {
        uint8_t got;
        time_t utc_time;
    } tdt;

    uint8_t nit_count;
    struct richepg_nit
    {
        uint8_t got;
        uint8_t version;
        uint16_t network_id;
        uint16_t ts_count;
        struct richepg_nit_ts *ts;
    } *nit;

    struct richepg_bat
    {
        uint8_t got;
        uint8_t version;
        uint16_t bouquet_id;
        uint16_t service_id;
        struct richepg_homets_migration
        {
            uint8_t need;
            uint16_t ts_id;
            uint16_t on_id;
        } migration;
    } bat;
};

struct richepg_carousels
{
    uint8_t state;

    struct richepg_pat
    {
        uint8_t version;
        uint16_t service_id;
        uint16_t map_pid;
    } pat;

    struct richepg_pmt
    {
        uint8_t version;
        uint8_t carousel_num;
        struct richepg_pmt_carousel
        {
            uint16_t pid;
            uint8_t component_tag;
            uint32_t carousel_id;
        } *carousel;
    } pmt;
};

int richepg_start_homets_identifying(uint32_t dmx_id, uint32_t dmx_source, uint16_t network_id, re_homets_bids *bouquet_ids);
int richepg_start_update_carouselsinfo(uint32_t dmx_id, uint32_t dmx_source, uint16_t service_id);
int richepg_start_update_tsinfo(uint32_t dmx_id, uint32_t dmx_source, uint16_t *onids, uint8_t onid_num);
int richepg_start_time_update(uint32_t dmx_id, uint32_t dmx_source);
int richepg_stop_sections_request(void);

int richepg_get_carousels(struct richepg_carousels **carousel);
int richepg_free_carousels(void);

int richepg_get_homets(struct richepg_homets **homets);
int richepg_free_homets(void);
#endif
