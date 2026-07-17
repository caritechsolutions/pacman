#ifndef __RICHEPG_LOOKUPTABLES_H_
#define __RICHEPG_LOOKUPTABLES_H_
#include "gxtype.h"
#include "richepg_json.h"

enum richepg_lookup_tables_type
{
    RICHEPG_LKTT_SERVICE = 1,
    RICHEPG_LKTT_CHANNEL,
    RICHEPG_LKTT_CONTENT,
};

typedef struct eutelsat_tier richepg_lookup_tables_tier;

int richepg_lookuptables_refresh(void);
int richepg_lookuptables_get_total(enum richepg_lookup_tables_type type);
char *richepg_lookuptables_get_name_by_id(enum richepg_lookup_tables_type type, uint16_t id);
char *richepg_lookuptables_get_name_by_pos(enum richepg_lookup_tables_type type, uint16_t pos);
int richepg_lookuptables_get_id_by_pos(enum richepg_lookup_tables_type type, uint16_t pos);

richepg_lookup_tables_tier *richepg_lookuptables_get_tier_by_id(int id);
int richepg_lookuptables_free(void);
#endif
