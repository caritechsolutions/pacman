#ifndef __RICHEPG_H__
#define __RICHEPG_H__
#include "richepg_json.h"
#include "richepg_sections.h"
#include "richepg_store.h"

enum richepg_logo_type {
    RICHEPG_LOGO_CHANNEL,
    RICHEPG_LOGO_LINEUP,
    RICHEPG_LOGO_EVENT,
    RICHEPG_LOGO_EAR_CHANNEL,
    RICHEPG_LOGO_EAR_GLOBAL
};

enum RICHEPG_PROFILE_TYPE {
    RICHEPG_FULL_PROFILE,
    RICHEPG_MID_PROFILE
};

enum RICHEPG_STAGE_TYPE {
    RICHEPG_LINEUP_STAGE = 0,
    RICHEPG_EPG_STAGE,
    RICHEPG_USBUP_STAGE
};

struct richepg_flash_limit {
    size_t ecl_limit;
    size_t ecl_used;
    size_t epg_limit;
    size_t epg_used;
};

struct richepg_flash_limit *richepg_flash_limit_ctrl_get(void);
int richepg_epg_receive_epg_carousel_total_get(void);
int richepg_epg_cur_receive_epg_carousel_index_get(void);
void richepg_set_file_save_synchronization(bool sync);
void richepg_set_rich_mode(bool rich_mode);
bool richepg_rich_mode_get(void);
void richepg_set_flash_limit(struct richepg_flash_limit *limit);
int richepg_get_masterindex(struct eutelsat_masterindex **masterindex);
int richepg_get_ecf(struct eutelsat_configuration_file **ecf);
int richepg_get_ecl(struct eutelsat_channel_list **ecl);
int richepg_get_elli(struct eutelsat_lineup_list **elli);
int richepg_get_elld(struct eutelsat_lineup_data **elld);
int richepg_get_ear(struct eutelsat_additional_resources **ear);
char *richepg_get_ear_channel_logo(int id);
char *richepg_get_logo_realpath(const char *filepath, enum richepg_logo_type type);
char *richepg_get_logo_path(const char *filepath, enum richepg_logo_type type);
enum RICHEPG_PROFILE_TYPE richepg_get_profile_type(enum usb_timestamp_type usb_type);
void richepg_clean_all_flash_files(void);

/* Reference for richepg_start
 * stage_type: 0, Parse MASTERINDEX | ECF | ECL | ELLI | ELLD
 *   At this time, lineup_id: 0, install mode; > 0, maintenance mode.
 * stage_type: 1, Parse EPG | EAR. At this time, not care lineup_id.
 * stage_type: 2, Parse all but not modify data in flash. At this time, must have valid lineup_id.
 */
int richepg_add_carousels(struct richepg_pmt *pmt);
int richepg_start(uint32_t dmx_id, uint32_t dmx_source, enum RICHEPG_STAGE_TYPE stage_type, int lineup_id);
int richepg_stop(void);
int richepg_check_stop(void);
void richepg_set_lineup(int lineup_id);
int richepg_get_lineup(void);

int richepg_parse_release(void);
int richepg_switch_lineup_step1(void);
int richepg_switch_lineup_step2(int lineup_id);
int richepg_switch_lineup_step3(bool sat_change, bool force_update);
#endif
