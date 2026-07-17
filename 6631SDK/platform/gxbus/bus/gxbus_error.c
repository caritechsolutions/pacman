#include "gxbus_error.h"

#ifdef __GXBUS_DEBUG

#define GXCA_MAX_ERROR_MSG_LEN              (200)

struct gxca_error
{
    int32_t  error_code;
    uint8_t  error_msg[GXCA_MAX_ERROR_MSG_LEN+1];
};

static struct gxca_error error[] = GXBUS_ERROR_LIST;


void GxBus_Error( int32_t ErrorCode )
{
    uint32_t i;
    uint32_t error_num = sizeof(error)/sizeof(struct gxca_error);

    for( i=0; i<error_num; i++ )
    {
        if( error[i].error_code==ErrorCode )
        {
            GXBUS_DbgPrint("Error Msg: %s\n", error[i].error_msg    );
        }
    }
}

#endif
