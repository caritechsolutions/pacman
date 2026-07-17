#ifndef __APP_NET_SEARCH_LIST_H__
#define __APP_NET_SEARCH_LIST_H__

typedef int (*ListCreateCb)(void);
typedef int (*ListKeypressCb)(int, int);
typedef int (*ListGetTotalCb)(void);
typedef int (*ListGetDataCb)(void *usrdata);
typedef int (*ListExitCb)(void);
typedef struct
{
	int ListSel;
	char* ListTitle;
	ListCreateCb	CreateCb;
    ListKeypressCb	KeypressCb;
	ListGetTotalCb	GetTotalCb;
	ListGetDataCb	GetDataCb;
	ListExitCb		ExitCb;
}CSLOps;

extern int32_t app_csl_dialog_create(CSLOps *opt);
extern void app_csl_list_row_updata(int updata_row);
extern void app_csl_list_updata(void);

#endif
