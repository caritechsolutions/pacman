#ifndef _GX_NBTSCAN_H
#define _GX_NBTSCAN_H
#include <gx_apps.h>

typedef struct gx_nbtscan_node_s
{
	char *ip_addr;
	char *computer_name;
}gx_nbtscan_node_t;

typedef struct gx_nbtscan_nodelist_s
{
	int node_num;
	gx_nbtscan_node_t *nodes;
}gx_nbtscan_nodelist_t;

gxapps_ret_t gx_nbtscan_get_nodelist(char *ipaddr, gx_nbtscan_nodelist_t **nodelist);

void gx_nbtscan_free_nodelist(gx_nbtscan_nodelist_t *nodelist);


#endif

