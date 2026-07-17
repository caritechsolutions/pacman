/**
 * @file gxupdate_stream.c
 * @author lixb
 * @brief goxceed升级架构流层定义
 */
#include "gxcore.h"
#include "gxupdate_debug.h"
#include "update/gxupdate_stream.h"

#define GXUPDATE_PROTOCOL_IOCTL_KEY_GET_TERMINAL    (0)
#define MAX_UPDATING_THREAD_STACK                   (10*1024)
#define INLINE                                      inline

#define PARTITION_OPEN(s)                   (s->partition.ops->open())
#define PARTITION_READ(s, ptr, len)         (s->partition.ops->read(s->partition.handle, ptr, len))
#define PARTITION_WRITE(s, ptr, len)        (s->partition.ops->write(s->partition.handle, ptr, len))
#define PARTITION_SIZE(s)                   (s->partition.ops->get_size(s->partition.handle))
#define PARTITION_IOCTL(s, key, ptr, size)  (s->partition.ops->ioctl(s->partition.handle, key, ptr, size))
#define PARTITION_CLOSE(s)                  (s->partition.ops->close(s->partition.handle))

#define PROTOCOL_OPEN(s)                    (s->protocol.ops->open())
#define PROTOCOL_READ(s, ptr, len)          (s->protocol.ops->read(s->protocol.handle, ptr, len))
#define PROTOCOL_WRITE(s, ptr, len)         (s->protocol.ops->write(s->protocol.handle, ptr, len))
#define PROTOCOL_TYPE(s)                    (s->protocol.ops->get_type(s->protocol.handle))
#define PROTOCOL_SIZE(s, len)               (s->protocol.ops->set_size(s->protocol.handle, len))
#define PROTOCOL_IOCTL(s, key, ptr, size)   (s->protocol.ops->ioctl(s->protocol.handle, key, ptr, size))
#define PROTOCOL_CLOSE(s)                   (s->protocol.ops->close(s->protocol.handle))

struct gxupdate_protocol {
    struct gxlist_head          node;
    GxUpdate_ProtocolOps        ops;
};

struct gxupdate_partition {
    struct gxlist_head          node;
    GxUpdate_PartitionOps       ops;
};

struct gxupdate_protocol_ext {
    GxUpdate_ProtocolOps*       ops;
    handle_t                    handle;
};

struct gxupdate_partition_ext {
    GxUpdate_PartitionOps*      ops;
    handle_t                    handle;
};

struct gxupdate_stream {
    char                                name[MAX_UPDATE_STREAM_NAME];
    volatile    bool                    stop_flag;
    int32_t                             status;
	int32_t                             read_total;
    handle_t                            thread_handle;
    handle_t                            status_sem;
    handle_t                            read_status_sem;
    struct gxupdate_protocol_ext        protocol;
    struct gxupdate_partition_ext       partition;
};

static GX_LIST_HEAD(protocol_list);
static GX_LIST_HEAD(partition_list);
static int32_t INLINE set_state(struct gxupdate_stream* stream, int32_t status)
{
    ASSERT(stream != NULL);

    DBG_UPDATE("setting\n");
    GxCore_SemWait(stream->read_status_sem);
    if (status > 0) {
        stream->status += status;
        if (stream->status > GXUPDATE_STREAM_PERCENT_RATION) {
            stream->status = GXUPDATE_STREAM_PERCENT_RATION;
        }
    } else {
        stream->status = status;
    }
    DBG_UPDATE("stream->status=%d\n", stream->status);
    return GxCore_SemPost(stream->status_sem);

}


static int32_t INLINE get_state(struct gxupdate_stream* stream,
                                 int32_t*                status)
{
    ASSERT(stream != NULL);
    ASSERT(status != NULL);

    DBG_UPDATE("reading\n");
    GxCore_SemPost(stream->read_status_sem);
    GxCore_SemWait(stream->status_sem);
    *status = stream->status;
    DBG_UPDATE("read stream->status=%d\n", stream->status);

    return E_OK;
}

static void INLINE reset_state(struct gxupdate_stream* stream)
{
    ASSERT(stream != NULL);
    stream->status = 0;
    return;
}

static int32_t protocol_add(GxUpdate_ProtocolOps* ops)
{
    struct gxupdate_protocol*    new_protocol;

    ASSERT(ops != NULL);

    if (ops->name == NULL
        || ops->open == NULL
        || ops->close == NULL
        || ops->ioctl == NULL
        || (ops->read == NULL && ops->write == NULL)){
        return E_FAILURE;
    }

    new_protocol = CALLOC(1, sizeof(struct gxupdate_protocol));
    if (new_protocol == NULL) {
        return E_FAILURE;
    }

    new_protocol->ops = *ops;

    gxlist_add_tail(&new_protocol->node, &protocol_list);

    return E_OK;
}


static int32_t protocol_open(struct gxupdate_stream*  stream,
                             const char*              name)
{
    struct gxlist_head*                 pos;
    struct gxupdate_protocol*           protocol;

    ASSERT(stream != NULL);
    ASSERT(name != NULL);

    DBG_UPDATE("%s\n", __FUNCTION__);

    gxlist_for_each(pos, &protocol_list) {
        protocol = gxlist_entry(pos, struct gxupdate_protocol, node);
        if (strcasecmp(protocol->ops.name, name) == 0) {
            stream->protocol.ops = &(protocol->ops);
            stream->protocol.handle = protocol->ops.open(name);
            if (stream->protocol.handle != E_INVALID_HANDLE) {
                return E_OK;
            }
            break;
        }
    }

    return set_state(stream, GXUPDATE_PROTOCOL_ERROR);
}

static int32_t partition_add(GxUpdate_PartitionOps* ops)
{
    struct gxupdate_partition*   new_partition;

    ASSERT(ops != NULL);

    if (ops->name == NULL
        || ops->open == NULL
        || ops->close == NULL
        || ops->read == NULL
        || ops->write == NULL){
        return E_FAILURE;
    }

    new_partition = CALLOC(1, sizeof(struct gxupdate_partition));
    if (new_partition == NULL) {
        return E_FAILURE;
    }

    new_partition->ops = *ops;

    gxlist_add_tail(&new_partition->node, &partition_list);

    return E_OK;
}


static int32_t partition_open(struct gxupdate_stream* stream, char* name)
{
    struct gxlist_head*             pos;
    struct gxupdate_partition*      partition;

    gxlist_for_each(pos, &partition_list){
        partition = gxlist_entry(pos, struct gxupdate_partition, node);
        if (strcasecmp(partition->ops.name, name) == 0) {

            stream->partition.handle = partition->ops.open(name);
            if (stream->partition.handle == E_INVALID_HANDLE) {
                break;
            }
            stream->partition.ops = &partition->ops;
            return E_OK;
        }
    }
    return E_FAILURE;
}

static int32_t INLINE updating_client(struct gxupdate_stream* stream,
                                      size_t                  size)
{
    uint8_t* buf = NULL;//[64*1024];//[GXUPDATE_STREAM_BLOCK_SIZE];
    //ssize_t write_size;
    ssize_t read_size = GXUPDATE_STREAM_BLOCK_SIZE;
	ssize_t read_block = 0x0;
	ssize_t total_block = 0x0;

    int32_t inc = (GXUPDATE_STREAM_BLOCK_SIZE*GXUPDATE_STREAM_PERCENT_RATION)/size;
    uint32_t ret = GXUPDATE_STREAM_CONTINUE;

    buf = GxCore_Malloc(64*1024);
    if(buf == NULL)
    {
        return E_FAILURE;
    }
    ASSERT(stream != NULL);
	total_block = size/GXUPDATE_STREAM_BLOCK_SIZE;
	stream->read_total=0;
    while( ret != GXUPDATE_STREAM_FINISH) {
        ret   = PROTOCOL_READ(stream, buf, &read_size);
		read_block = stream->read_total/GXUPDATE_STREAM_BLOCK_SIZE;
		stream->read_total +=read_size;
        PARTITION_WRITE(stream, buf, read_size);

     inc = ((stream->read_total/GXUPDATE_STREAM_BLOCK_SIZE)*GXUPDATE_STREAM_PERCENT_RATION)/total_block-(read_block*GXUPDATE_STREAM_PERCENT_RATION)/total_block;
        if(inc == 0)
        {
            inc = 1;
        }
        set_state(stream, inc);
    }
    GxCore_Free(buf);
    return E_OK;
}

static int32_t INLINE updating_server(struct gxupdate_stream* stream,
                                      size_t                  size)
{
    uint8_t buf[GXUPDATE_STREAM_BLOCK_SIZE];
    ssize_t write_size;
     ssize_t read_block = 0x0;
     ssize_t total_block = 0x0;

    ssize_t read_size = GXUPDATE_STREAM_BLOCK_SIZE;
    int32_t inc = (GXUPDATE_STREAM_BLOCK_SIZE*GXUPDATE_STREAM_PERCENT_RATION)/size;

	total_block = size/GXUPDATE_STREAM_BLOCK_SIZE;
    ASSERT(stream != NULL);

	stream->read_total = 0;
    while(read_size == GXUPDATE_STREAM_BLOCK_SIZE) {
        read_size   = PARTITION_READ(stream, buf, read_size);
        write_size = PROTOCOL_WRITE(stream, buf, read_size);
        if(write_size != read_size)
        {
            return E_FAILURE;
        }
		read_block = stream->read_total/GXUPDATE_STREAM_BLOCK_SIZE;
        stream->read_total +=read_size;
        inc = ((stream->read_total/GXUPDATE_STREAM_BLOCK_SIZE)*GXUPDATE_STREAM_PERCENT_RATION)/total_block-(read_block*GXUPDATE_STREAM_PERCENT_RATION)/total_block;
		set_state(stream, inc);
    }
    return E_OK;
}


static void updating(void* param)
{
    GxUpdate_Terminal           terminal;
    struct gxupdate_stream*     stream = (struct gxupdate_stream* )param;
    size_t                      size;
    int32_t                     ret = E_FAILURE;

    DBG_UPDATE("%s\n", __FUNCTION__);

    ASSERT(stream != NULL);
    ASSERT(stream->protocol.ops != NULL);
    ASSERT(stream->protocol.handle != E_INVALID_HANDLE);

    terminal = PROTOCOL_TYPE(stream);
    size     = PARTITION_SIZE(stream);
    PROTOCOL_SIZE(stream, size);

    set_state(stream, GXUPDATE_INITIALIZING);

    if (stream->stop_flag) {
        set_state(stream, GXUPDATE_STOP);
        goto UPDATING_STOP;
    }

    if (terminal == GXUPDATE_CLIENT) {
        ret = updating_client(stream, size);
    } else if (terminal == GXUPDATE_SERVER) {
        ret = updating_server(stream, size);
    } else {
        set_state(stream, GXUPDATE_TERMINAL_NOT_SUPPORT);
    }
UPDATING_STOP:
    PROTOCOL_CLOSE(stream);
    PARTITION_CLOSE(stream);
    if (ret == E_OK) {
        set_state(stream, GXUPDATE_OK);
        DBG_UPDATE("updating finished\n");
        //if (stream->status != GXUPDATE_STREAM_PERCENT_RATION) {
        //    set_state(stream, 1000);
        //}
    } else {
        set_state(stream, GXUPDATE_UPDATE_FAILURE);
    }
    return;
}

static int32_t update_start(struct gxupdate_stream*    stream)
{
    ASSERT(stream != NULL);
    DBG_UPDATE("%s\n", __FUNCTION__);
    reset_state(stream);
    return GxCore_ThreadCreate("update thread", &stream->thread_handle,
                        updating,
                        stream,
                        MAX_UPDATING_THREAD_STACK,
                        GXOS_DEFAULT_PRIORITY);
}

static int32_t update_stop(struct gxupdate_stream*    stream)
{
    ASSERT(stream != NULL);

    stream->stop_flag = TRUE;

    return GxCore_ThreadJoin(stream->thread_handle);
}



int32_t GxUpdate_StreamInit(GxUpdate_ProtocolOps**    ProtocolList,
                            GxUpdate_PartitionOps**   PartitionList)
{
    GxUpdate_ProtocolOps**      p1;
    GxUpdate_PartitionOps**     p2;

    DBG_UPDATE("%s\n", __FUNCTION__);

    for(p1 = ProtocolList; *p1 != NULL; p1++) {
        if (protocol_add(*p1) != E_OK) {
            return E_FAILURE;
        }
    }

    for(p2 = PartitionList; *p2 != NULL; p2++) {
        if (partition_add(*p2) != E_OK) {
            return E_FAILURE;
        }
    }

    return E_OK;
}

handle_t GxUpdate_StreamOpen(const GxUpdate_StreamName Name)
{
    struct gxupdate_stream*    stream;

    DBG_UPDATE("%s,%s\n", __FUNCTION__, Name);

    ASSERT(Name != NULL);


    stream = CALLOC(1, sizeof(struct gxupdate_stream));
    if (stream == NULL) {
        return E_INVALID_HANDLE;
    }

	stream->read_total = 0;
    GxCore_SemCreate(&stream->status_sem, 0);
    GxCore_SemCreate(&stream->read_status_sem, 0);

    strncpy(stream->name, Name, MAX_UPDATE_STREAM_NAME);

    return (handle_t)stream;
}

int32_t GxUpdate_StreamIoctl(handle_t       Handle,
                             int32_t        Key,
                             void*          Buf,
                             size_t         Size)
{
    int32_t                     ret    = E_FAILURE;
    struct gxupdate_stream*     stream = (struct gxupdate_stream*)Handle;
    GxUpdate_IoCtrl*            ctrl;

    DBG_UPDATE("%s,stream=%p\n", __FUNCTION__, stream);

    ASSERT(stream != NULL);

    switch(Key) {
    case GXUPDATE_STREAM_PROTOCOL_SELECT:
        ret = protocol_open(stream, (char*)Buf);
        if (ret != E_OK) {
            set_state(stream, GXUPDATE_PARTITION_ERROR);
        }
        return ret;
    case GXUPDATE_STREAM_PROTOCOL_CTRL:
        if (Size == sizeof(GxUpdate_IoCtrl)) {
            ASSERT(stream->protocol.ops != NULL);
            ctrl = Buf;
            ret = PROTOCOL_IOCTL(stream, ctrl->key, ctrl->buf, ctrl->size);
            if (ret != E_OK) {
                set_state(stream, GXUPDATE_PROTOCOL_ERROR);
            }
        }
        return ret;
    case GXUPDATE_STREAM_PARTITION_SELECT:
        ret =  partition_open(stream, (char*)Buf);
        if (ret != E_OK) {
            set_state(stream, GXUPDATE_PROTOCOL_ERROR);
        }
        return ret;
    case GXUPDATE_STREAM_PARTITION_CTRL:
        if (Size == sizeof(GxUpdate_IoCtrl)) {
            ASSERT(stream->partition.ops != NULL);
            ctrl = Buf;
            ret = PARTITION_IOCTL(stream, ctrl->key, ctrl->buf, ctrl->size);
            if (ret != E_OK) {
                set_state(stream, GXUPDATE_PARTITION_ERROR);
            }
        }
        return ret;
    case GXUPDATE_STREAM_UPDATE_START:
        ret = update_start(stream);
        if (ret != E_OK) {
            set_state(stream, GXUPDATE_UPDATE_FAILURE);
        }
        return ret;
    case GXUPDATE_STREAM_UPDATE_STOP:
        ret =  update_stop(stream);
        if (ret != E_OK) {
            set_state(stream, GXUPDATE_UPDATE_FAILURE);
        }
        return ret;
    case GXUPDATE_STREAM_UPDATE_GET_STATUS:
        if (sizeof(int32_t) == Size) {
            int32_t* status = (int32_t*)Buf;

            return get_state(stream, status);

        }
        break;
    default:
        break;
    }
    return E_FAILURE;
}



int32_t GxUpdate_StreamClose(handle_t Handle)
{
    int32_t                     ret     = E_OK;
    struct gxupdate_stream*     stream  = (struct gxupdate_stream*)Handle;

    DBG_UPDATE("%s,stream=%p\n", __FUNCTION__, stream);

    ASSERT(Handle != E_INVALID_HANDLE);

    GxCore_SemDelete(stream->status_sem);
    GxCore_SemDelete(stream->read_status_sem);
    GxCore_Free(stream);

    return ret;
}

int32_t GxUpdate_StreamClearStatus(handle_t Handle)
{
    int32_t                     ret     = E_OK;
    struct gxupdate_stream*     stream  = (struct gxupdate_stream*)Handle;

    DBG_UPDATE("%s,stream=%p\n", __FUNCTION__, stream);

    ASSERT(Handle != E_INVALID_HANDLE);
    stream->status = E_OK;
    return ret;
}

int32_t GxUpdate_StreamGetStatus(void)
{
	extern handle_t update_stream_handle;
	struct gxupdate_stream*     stream  = (struct gxupdate_stream*)update_stream_handle;

	DBG_UPDATE("%s,stream=%p\n", __FUNCTION__, stream);

	ASSERT(Handle != E_INVALID_HANDLE);
	return stream->status;
}

