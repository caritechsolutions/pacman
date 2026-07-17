#ifndef _APP_DLNA_H
#define _APP_DLNA_H

#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT

typedef enum _UIControlTypeEnum {
    UI_WAITING_CONNECTION,
    UI_DMR_PLAY,
    UI_DMR_SET_VOLUME,
    UI_DMP_ADD_SERVER,
    UI_DMP_REMOVE_SERVER,
    UI_DMP_UPDATE_DIR,
    UI_DMP_GOT_FOCUS,
    UI_PIC_DOWNLOAD_SUCCESS,
    UI_PIC_DOWNLOAD_ERROR,
    UI_DLNA_ERROR,
    UI_DMP_ACTION_ACK_ERROR
}UIControlTypeEnum;

typedef struct _AppMsg_DlnaUIControl{
    UIControlTypeEnum type;
    char *err_str;
    int valume;
}AppMsg_DlnaUIControl;

#endif

#endif
