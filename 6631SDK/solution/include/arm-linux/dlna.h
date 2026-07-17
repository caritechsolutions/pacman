#ifndef __DLNA_H__
#define __DLNA_H__

/****************************************** PUBLIC  *********************************/

/* when you operation DMPServerList & DMPContentDirList in your application,
 * please use ithread_mutex_xxxx for easy to maintain the code */
#define URL_MAX_LENGTH (2048)
#define TITLE_MAX_LENGTH (512)

#ifndef ITHREAD_H
#define ithread_mutex_lock pthread_mutex_lock
#define ithread_mutex_unlock pthread_mutex_unlock
 typedef pthread_mutex_t ithread_mutex_t;
#endif
typedef unsigned long long  uint64;

typedef enum _MediaTypeEnum {
    DLNA_VIDEO = 0,
    DLNA_AUDIO,
    DLNA_IMAGE,
    DLNA_CONTAINER,
    DLNA_MEDIA_NONE
}MediaTypeEnum;

typedef enum _DLNAErrorNoEnum {
    DLNAE_INVALID_ACTION,
    DMRE_NULL_URL,
    DMRE_INVALID_SEEKMODE,
    DMRE_NULL_SEEKTIME
}DLNAErrorNoEnum;

const char *Get_error_string(DLNAErrorNoEnum err_no);
void DLNA_enable_debug_print(int flag);

/****************************************** DMR STRUCT ******************************/
#define ENABLE_VIRTUAL_DIR 1
typedef enum _TransportStatusEnum {
    TRANS_STATUS_OK,
    TRANS_STATUS_ERROR_OCCURRED,
    TRANS_STATUS_WENDER_DEFINED
}TransportStatusEnum;

typedef enum _TransportStateEnum {
    TRANS_STATE_STOPPED,
    TRANS_STATE_PLAYING,
    TRANS_STATE_PAUSE,
    TRANS_STATE_TRANSITIONING,
    TRANS_STATE_NO_MEDIA_PRESENT
}TransportStateEnum;

typedef struct _DMRPlayInfo {
    char title[TITLE_MAX_LENGTH];
    char url[URL_MAX_LENGTH];
    MediaTypeEnum media_type;
}DMRPlayInfo;

typedef struct _DMRCtrlCallback {
  int (*error)(DLNAErrorNoEnum err_no, const char *err_str);
    /** transport ctrl**/
  int (*play)(DMRPlayInfo *info, int is_new_url);
  int (*stop)(void);
  int (*pause)(void);
  int (*seek)(uint64 time);
  int (*get_position_info)(uint64 *ptotal_time, uint64 *pcur_time, uint64 *ptotal_count);
  int (*get_transport_info)(TransportStatusEnum *status, TransportStateEnum *state);
    /** render ctrl**/
  int (*get_volume)(int *value);
  int (*set_volume)(int value);
}DMRCtrlCallback;

typedef struct _DMRConfig {
    unsigned short port;  // if you set 0, the system allocates automatically
    char *ip_address; // can't be NULL, you must set ip
    char *web_dir_path; // if you set NULL ,the default is "./dlna-web", when ENABLE_VIRTUAL_DIR == 1,it not need set.
    char *desc_doc_name; // if you set NULL,the default is "dmr_description.xml", when ENABLE_VIRTUAL_DIR == 1,it not need set.
    char *friendly_name; // if you set NULL ,the default in "dmr_description.xml"
    char *udn; // if you set NULL ,the default in "dmr_description.xml"
}DMRConfig;

/****************************************** DMR API *********************************/
int DMR_start(DMRConfig *config, DMRCtrlCallback *ctrl_callback);
int DMR_stop(void);


/****************************************** DMP STRUCT ******************************/
typedef enum _DeviceTypeEnum {
    UNKNOWN_SERVER = 0,
    MEDIA_SERVER = 1,
    SAT2IP_SERVER
}DeviceTypeEnum;

typedef struct _DMPPlayClass {
    char *duration;
    char *url;
    int size;
    int bitrate;
    int sample_frequency;
    int audio_channel_num;
}DMPPlayClass;

typedef struct _DMPContentDirNode {
    ithread_mutex_t mutex;
    char *id;
    char *parent_id;
    char *title;
    char *ex_subtitles;
    char *ex_artist;
    char *ex_genre;
    char *ex_album;
    char *ex_date;
    char *ex_orig_track_nb;
    char *ex_album_artist;
    char *ex_album_art;
    int count;
    int child_count;
    MediaTypeEnum media_type;
    DMPPlayClass *play_res;
    struct _DMPContentDirNode *prev, *next;
    struct _DMPContentDirNode *child; //for application use
    struct _DMPContentDirNode *parent_node; //for application use
}DMPContentDirNode, DMPContentDirList;

typedef struct _DMPServerNode {
    ithread_mutex_t mutex;
    DeviceTypeEnum device_type;
    char *friendly_name;
    char *udn;
    char *action_url;
    char *icon_url;
    int count;
    DMPContentDirList *dir_list; //for application use
    struct _DMPServerNode *prev, *next;
}DMPServerNode, DMPServerList;

typedef enum _DlnaEventTypeEnum {
    DLNA_EVENT_SERVER_ADD,
    DLNA_EVENT_SERVER_REMOVE,
    DLNA_EVENT_CONTENTSDIR_GET,
    DLNA_EVENT_CONTENTSDIR_NULL,
    DLNA_EVENT_ACTION_ACK_ERROR,
    DLNA_EVENT_ERROR,
}DlnaEventTypeEnum;

typedef struct _DMPConfig {
    unsigned short port;  // if you set 0, the system allocates automatically
    char *ip_address; // can't be NULL, you must set ip
    int enable_device; //you can set MEDIA_SERVER, SAT2IP_SERVER or MEDIA_SERVER|SAT2IP_SERVER
    int max_http_content; // if http content length > max_http_content,you will get data fail. if you set 0, the default is 1M.
}DMPConfig;

typedef int (*DlnaEventCallback)(DlnaEventTypeEnum event_type);

/****************************************** DMP API *********************************/

DMPServerList *DMP_start(DMPConfig *config, DlnaEventCallback event_call_back);
int DMP_stop(DMPServerList **head);
int DMP_server_remove(DMPServerList *head, DMPServerNode *server);
DMPContentDirList *DMP_directory_browse(DMPServerNode *server, const char *id, DlnaEventCallback event_call_back);
int DMP_directory_free(DMPContentDirList **head);


/****************************************** DMS STRUCT ******************************/
#define MAX_ID_STR_LEN (32)
typedef struct _DMSContentInfo {
    MediaTypeEnum media_type;
    int child_count;
    unsigned long long  size;
    char id[MAX_ID_STR_LEN];
    char parent_id[MAX_ID_STR_LEN];
    char *title;
    char *url;
}DMSContentInfo;

typedef struct _DMSCtrlCallback {
  int (*error)(DLNAErrorNoEnum err_no, const char *err_str);
  int (*get_content_array)(const char* object_id, DMSContentInfo **info_arry, int *total, int *update_id);
}DMSCtrlCallback;

typedef struct _DMSConfig {
    unsigned short port;  // if you set 0, the system allocates automatically
    char *ip_address; // can't be NULL, you must set ip
    char *web_dir_path; // if you set NULL ,the default is "./dlna-web", when ENABLE_VIRTUAL_DIR == 1,it not need set.
    char *desc_doc_name; // if you set NULL,the default is "dmr_description.xml", when ENABLE_VIRTUAL_DIR == 1,it not need set.
    char *friendly_name; // if you set NULL ,the default in "dmr_description.xml"
    char *udn; // if you set NULL ,the default in "dmr_description.xml"
    char *logo_path;  // only support png
}DMSConfig;

/****************************************** DMS API *********************************/
int DMS_start(DMSConfig *config, DMSCtrlCallback *ctrl_callback);
int DMS_stop(void);
#endif


