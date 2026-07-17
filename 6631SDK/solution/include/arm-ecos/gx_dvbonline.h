#ifndef _GX_DVBONLINE_H
#define _GX_DVBONLINE_H
#include <gx_apps.h>

typedef enum gx_dvbonline_sattype_e{
	DVBONLINE_SAT_DVBS = 0,
	DVBONLINE_SAT_DVBT,
	DVBONLINE_SAT_DVBC,
	DVBONLINE_SAT_MAX
}gx_dvbonline_sattype_t;

typedef enum gx_dvbonline_region_e{
	DVBONLINE_REGION_ALL = 0,
	DVBONLINE_REGION_ASIA,
	DVBONLINE_REGION_EUROPE,
	DVBONLINE_REGION_ATLANTIC,
	DVBONLINE_REGION_AMERICA,
	DVBONLINE_REGION_MAX
}gx_dvbonline_region_t;

typedef struct gx_dvbonline_satelite_s
{
	char *name;
	char *mini_name;
	int lnb_type;
	int position;
}gx_dvbonline_satelite_t;

typedef struct gx_dvbonline_satelitelist_s
{
	int satelite_num;
	gx_dvbonline_satelite_t *satelites;
}gx_dvbonline_satelitelist_t;

typedef struct gx_dvbonline_transponder_s
{
	int frequency;
	int polarization;
	int symbol_rate;
}gx_dvbonline_transponder_t;

typedef struct gx_dvbonline_transponderlist_s
{
	int transponder_num;
	gx_dvbonline_transponder_t *transponders;
}gx_dvbonline_transponderlist_t;

void gx_dvbonline_init(void);

void gx_dvbonline_pause(void);

void gx_dvbonline_resume(void);

gxapps_ret_t gx_dvbonline_get_satelites(
							gx_dvbonline_region_t region,
							gx_dvbonline_satelitelist_t **satelitelist);

void gx_dvbonline_free_satelitelist(gx_dvbonline_satelitelist_t *satelitelist);

gxapps_ret_t gx_dvbonline_get_transponders(
							char *sat_name,
							gx_dvbonline_transponderlist_t **transponderlist);

void gx_dvbonline_free_transponderlist(gx_dvbonline_transponderlist_t *transponderlist);

void gx_dvbonline_program_play(
							gx_dvbonline_sattype_t type,
							int sat_position,
							int frequency,
							int polarization,
							int symbol_rate,
							int service_id,
							char *provider_name,
							char *prog_name);

void gx_dvbonline_program_stop(void);

#endif
