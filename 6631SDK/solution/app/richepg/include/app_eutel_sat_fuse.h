#ifndef __APP_EUTEL_SAT_FUSE_H__
#define __APP_EUTEL_SAT_FUSE_H__


int app_eutel_sat_fuse_init(void);

int app_eutel_sat_fuse_build(void);

unsigned char *app_eutel_sat_get_display_name(int index,int *len);
unsigned char *app_eutel_sat_get_area_name(int index);

/**/
int app_eutel_sat_fuse_free_is_point(int free_sat_id);
int app_eutel_sat_fuse_free_sat_is_point(unsigned int sat_id);

int app_eutel_sat_fuse_get_freescan_area_count(unsigned int free_sat_id,unsigned int *pIndex);
int app_eutel_sat_fuse_get_sattv_area_count(unsigned int sattv_id,unsigned int *pIndex);

int app_eutel_sat_fuse_get_all_area_count(unsigned int sat_id,unsigned int *pIndex);

#endif

