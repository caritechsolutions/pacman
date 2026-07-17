#ifndef __APP_CA_H__
#define __APP_CA_H__

typedef enum _PSIDataEnum{
    DATA_PAT,
    DATA_CAT,
    DATA_NIT,
    DATA_PMT,
    DATA_EMM,
    DATA_ECM
}PSIDataEnum;

typedef enum _ControlTypeEnum{
    CONTROL_NORMAL,
    CONTROL_PVR,
    CONTROL_TOTAL,
}ControlTypeEnum;

typedef enum _DataTypeEnum{
    DATA_TYPE_VIDEO,
    DATA_TYPE_AUDIO,
    DATA_TYPE_SUBTITLE,
    DATA_TYPE_TELETEXT,
    DATA_TYPE_EPG,
    DATA_TYPE_UNIVERSAL,
}DataTypeEnum;

#define PROG_NAME_BYTES     32
typedef struct _ServiceClass{
    unsigned int longitude;
    unsigned int direction;
    unsigned int fre;
    unsigned int polar;
    unsigned int symb;
    unsigned int service_id;
    unsigned int cas_id;// if no cas, cas_id == 0
    unsigned short video_pid;
    unsigned short audio_pid;
    unsigned char name[PROG_NAME_BYTES];
}ServiceClass;

typedef struct _DataInfoClass{
    PSIDataEnum psi_type;
    unsigned int  cas_id;
    unsigned char *data;
    unsigned int data_len;
}DataInfoClass;

typedef enum _EditTypeEnum{
    EDIT_DELETE,
    EDIT_ADD,
    EDIT_MODIFY,
}EditTypeEnum;

typedef struct _EditInfoClass{
    char *module_name;
    EditTypeEnum type;
}EditInfoClass;

typedef struct _ControlClass{
    int handle;
    unsigned short demux_id;
    unsigned short pid;// ecm pid
    unsigned short element_pid;
    DataTypeEnum data_type;
    ControlTypeEnum    control_type;
}ControlClass;

int app_get_emm_status(unsigned short cas_id);

int app_get_ecm_status(unsigned short cas_id);

int app_edit_data(EditInfoClass info , ServiceClass data);

int app_send_prepare(ServiceClass *param);

int app_send_service(ServiceClass *param, ControlClass *control_info);

int app_send_psi_data(DataInfoClass *data_info, ControlClass *control_info);

int app_extend_register(void);

void app_exshift_enable(void);

void app_exshift_disable(void);

void app_exshift_set_time(int Time);//second

int app_exshift_pid_add(int Num, int* pData);

int app_exshift_pid_delete(int Num, int* pData);

int app_tsd_check(void);

int app_tsd_time_get(void);

int app_demux_param_adjust(unsigned short *ts_source, unsigned short *demux_id, unsigned char ex_flag);

void app_ca_set_flag(unsigned char index);

void app_ca_clear_flag(void);

void app_extend_send_service(unsigned int prog_id, unsigned int sat_id, unsigned int tp_id, unsigned int demux_id, ControlTypeEnum type);

void app_extend_change_audio(unsigned short cur_audio_pid, unsigned short audio_pid, ControlTypeEnum control_type);

void app_extend_add_element(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type);

int app_extend_control_pvr_change(void);

// descrambler
int app_descrambler_init(unsigned char demux_id, ControlTypeEnum control_type, DataTypeEnum data_type);

int app_descrambler_close_by_pid(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type);

int app_descrambler_close_by_handle(int handle);

int app_descrambler_close_by_control_type(ControlTypeEnum control_type);

int app_descrambler_close_by_type(ControlTypeEnum control_type, DataTypeEnum data_type);

int app_descrambler_close_all(void);

int app_descrambler_set_pid(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type);

int app_descrambler_get_handle_by_pid(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type);

int app_descrambler_set_pid_by_handle(int handle, unsigned short pid);

int app_descrambler_set_cw(int handle,
                                const unsigned char *even,
                                const unsigned char *odd,
                                unsigned char DecryptFlag);// 3des or cas2

int app_descrambler_set_cw_by_control_type(ControlTypeEnum type,
                                const unsigned char *even,
                                const unsigned char *odd,
                                unsigned char DecryptFlag);// 3des or cas2

int app_descrambler_set_cw_by_define(ControlTypeEnum type,
                                const unsigned char key_type,
                                const unsigned char *even,
                                const unsigned char *odd,
                                unsigned char DecryptFlag);// 3des or cas2

int app_descramber_get_cw_by_handle(int handle,
                                        unsigned char *even,
                                        unsigned char *odd);

int app_descrambler_get_cw_by_pid(unsigned short pid,
                                    ControlTypeEnum control_type,
                                    DataTypeEnum data_type,
                                    unsigned char* even,
                                    unsigned char* odd);

#endif
