#ifndef __APP_IPTV_LIST_API_H__
#define __APP_IPTV_LIST_API_H__

typedef enum _IptvAdTypeEnum
{
    AD_TYPE_NONE = 0,
    AD_TYPE_AV,
    AD_TYPE_PIC,
    AD_TYPE_TOTAL,
}IptvAdTypeEnum;

typedef struct _IptvAdClass
{
	char *ad_name;
	char *ad_type;
	char *ad_value;
}IptvAdClass;

typedef struct _IptvItemClass
{
	char *iptv_key;
	char *iptv_value;
	char *iptv_type;
    IptvAdClass iptv_ad;
}IptvItemClass;

typedef struct _IptvListClass
{
	int iptv_item_num;
	IptvItemClass *iptv_item_list;
}IptvListClass;

typedef struct _IptvIndexClass
{
	int iptv_index_num;
	int *iptv_index;
}IptvIndexClass;

IptvListClass *_iptv_list_new(void);
IptvIndexClass *iptv_indexlist_new(void);
void iptv_indexlist_add_item(IptvIndexClass *list, int index);
void iptv_indexlist_del_item(IptvIndexClass *list, int index);
void iptv_list_free(IptvListClass *list);
void iptv_indexlist_free(IptvIndexClass *list);
IptvListClass *iptv_get_list_by_type(IptvListClass *list, char *type);
IptvIndexClass *iptv_get_indexlist_by_type(IptvListClass *list, char *type);
IptvListClass *iptv_get_list_by_type_simple(IptvListClass *list, char *type);
IptvIndexClass *iptv_get_indexlist_by_type_simple(IptvListClass *list, char *type);
IptvListClass *iptv_get_group_list(IptvListClass *list);
IptvListClass *iptv_get_group_list_simple(IptvListClass *list);

IptvListClass* app_iptv_api_get_local_list(void);
IptvListClass* app_iptv_api_get_fav_list(void);
int app_iptv_service_update_local_list(char *file_path);

#endif
