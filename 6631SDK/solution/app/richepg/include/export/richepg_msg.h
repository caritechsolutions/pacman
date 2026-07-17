#ifndef __RICHEPG_MSG_H__
#define __RICHEPG_MSG_H__
#include "richepg.h"
#include "richepg_json.h"

#define RICHEPG_MSG_QUEUE_DEPTH (64)

/* The richepg message list */
enum richepg_msg_id
{
    RICHEPG_MSGID_START = 0,

    RICHEPG_INFO_UNCHANGED,
    RICHEPG_ECL_SUCCESS,
    RICHEPG_ECL_FAILED,
    RICHEPG_EPG_SUCCESS,
    RICHEPG_EPG_FAILED,

    RICHEPG_HOMETS_SUCCESS, // nit & bat & tdt
    RICHEPG_HOMETS_FAILED,
    RICHEPG_UPDATE_TSINFO_SUCCESS, // nits
    RICHEPG_UPDATE_TSINFO_FAILED,
    RICHEPG_UPDATE_CAROUSEL_SUCCESS,    // pat & pmt
    RICHEPG_UPDATE_CAROUSEL_FAILED,
    RICHEPG_UPDATE_TIME_SUCCESS, //tdt
    RICHEPG_UPDATE_TIME_FAILED,

    RICHEPG_MSGID_END
};

/* The richepg message type list */
enum richepg_msg_type
{
    RICHEPG_GUI_STATUS = 0,
    RICHEPG_DATA_PROCESS,
    RICHEPG_PARSING_PROCESS,
    RICHEPG_WRITING_PROCESS,
    RICHEPG_DATA,
};

struct richepg_msg
{
    enum richepg_msg_type type;
    enum richepg_file_type data_type;
    union
    {
        uint32_t percent;
        enum richepg_msg_id msg_id;
        uint32_t update_flag;
    };
};

int richepg_create_msg_queue(void);
int richepg_recreate_msg_queue(void);
int richepg_delete_msg_queue(void);
int richepg_put_msg(struct richepg_msg *msg);
int richepg_get_msg(struct richepg_msg *msg);
int richepg_send_receiving_progress(uint32_t percent);
int richepg_send_parsing_progress(uint32_t percent);
int richepg_send_writing_progress(uint32_t percent);
int richepg_send_status(enum richepg_msg_id msg_id);
int richepg_send_complete(enum richepg_file_type type, uint32_t update_flag);
#endif
