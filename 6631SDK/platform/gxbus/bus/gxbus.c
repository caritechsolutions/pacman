#include <gxbus.h>
#include <gxmsg.h>
#include <gxbus_error.h>
#include "gxbus_version.h"
#include "module/config/gxconfig.h"

#define MAX_QUEUE_SIZE                  (140)
#define GXBUS_POOL_GATE					(4)

#define MAX_MEM_POOL_NUM                (5)
#define MEM_POOL_SIZE_1                 (24)
#define MEM_POOL_NUM_1                  (80)
#define MEM_POOL_SIZE_2                 (256)
#define MEM_POOL_NUM_2                  (32)
#define MEM_POOL_SIZE_3                 (1024)
#define MEM_POOL_NUM_3                  (20)
#define MEM_POOL_SIZE_4                 (4096)
#define MEM_POOL_NUM_4                  (16)
#define MEM_POOL_SIZE_5                 (8192)
#define MEM_POOL_NUM_5                  (16)
#define MEM_POOL_PACKET_NUM             ((MEM_POOL_NUM_1\
                                        +MEM_POOL_NUM_2\
                                        +MEM_POOL_NUM_3\
                                        +MEM_POOL_NUM_4\
                                        +MEM_POOL_NUM_5)*10)
#ifdef __GXBUS_DEBUG
#define BUS_LOCK()              do {                                            \
                                     status_t ret;                              \
                                     ret = GxCore_MutexLock(gxbus.lock);        \
                                     GXBUS_ASSERT(ret==GXCORE_SUCCESS);         \
                                }while(0)

#define BUS_UNLOCK()            do {                                            \
                                     status_t ret;                              \
                                     ret = GxCore_MutexUnlock(gxbus.lock);      \
                                     GXBUS_ASSERT(ret==GXCORE_SUCCESS);         \
                                }while(0)

#define QUEUE_LOCK(queue)       do {                                            \
                                     status_t ret;                              \
                                     ret = GxCore_MutexLock((queue)->lock);     \
                                     GXBUS_ASSERT(ret==GXCORE_SUCCESS);         \
                                }while(0)

#define QUEUE_UNLOCK(queue)     do {                                            \
                                     status_t ret;                              \
                                     ret = GxCore_MutexUnlock((queue)->lock);   \
                                     GXBUS_ASSERT(ret==GXCORE_SUCCESS);         \
                                }while(0)
#define SCHED_LOCK(sched)       do {                                            \
                                     status_t ret;                              \
                                     ret = GxCore_MutexLock((sched)->lock);     \
                                     GXBUS_ASSERT(ret==GXCORE_SUCCESS);         \
                                }while(0)
#define SCHED_UNLOCK(sched)     do {                                            \
                                     status_t ret;                              \
                                     ret = GxCore_MutexUnlock((sched)->lock);   \
                                     GXBUS_ASSERT(ret==GXCORE_SUCCESS);         \
                                }while(0)
#else
#define BUS_LOCK()                      GxCore_MutexLock(gxbus.lock)
#define BUS_UNLOCK()                    GxCore_MutexUnlock(gxbus.lock)
#define QUEUE_LOCK(queue)               GxCore_MutexLock((queue)->lock)
#define QUEUE_UNLOCK(queue)             GxCore_MutexUnlock((queue)->lock)
#define SCHED_LOCK(sched)               GxCore_MutexLock((sched)->lock)
#define SCHED_UNLOCK(sched)             GxCore_MutexUnlock((sched)->lock)
#endif


#define GET_MSG_SERVICE_LIST(msg)       (gxbus.msg_map[(msg)->msg.msg_id].service_list)
#define GET_SCHEDULER_BY_TYPE(sch, ser) ((sch)->type == GXBUS_SCHED_MSG) ?\
                                (ser)->scheduler_msg : (ser)->scheduler_console

enum scheduler_status {
	RUNNING,
	QUITING,
	QUITED
};

enum msg_status {
	PROCESSING,
	PROCESSED,
	TIMEOUT
};


struct mqueue {
	struct gxlist_head      msg_list;
	handle_t                send_sem;
	handle_t                get_sem;
	int32_t                 size;
	handle_t                lock;
	int                     working;
};

struct msg_property {
	int32_t                 priv_size;
	int32_t                 count;
	int32_t                 listen_count;
	struct gxlist_head      service_list;
};

struct service_node {
	struct gxlist_head      head;
	struct service_ext*     service;
	uint32_t status;
};

struct msg_ext {
	enum msg_prior          prior;
	handle_t                sync_lock;
	volatile uint32_t       process_count;
	handle_t                pool;
	GxMessage               msg;
};

struct msg_packet {
	struct gxlist_head      msg_head;
	struct gxlist_head      cache_head;
	struct service_ext*     dest_service;
	struct msg_ext*         msg_ext;
};

struct scheduler {
	struct gxlist_head      head;
	char                    name[NAME_SIZE];
	struct mqueue*          mqueue;
	uint32_t                refcount;
	struct gxlist_head      service_list;
	GxSchedType             type;
	enum scheduler_status   quit;
	handle_t                thread_id;
	size_t                  stack_size;
	uint8_t                 prior;
	handle_t                lock;
};

struct service_ext {
	struct gxlist_head      node_bus_ser_list;
	struct gxlist_head      node_console_ser_list;
	struct scheduler*       scheduler_msg;
	struct scheduler*       scheduler_console;
	GxServiceClass*           ops;
};

struct gxbus_property {
	handle_t                lock;
	struct gxlist_head      msg_packet_pool;//改用malloc和list缓冲
	uint32_t                packet_count;//用来控制缓冲数量应该<=MEM_POOL_PACKET_NUM
//	handle_t                pool[MAX_MEM_POOL_NUM];//所有消息内容都改用malloc
	struct gxlist_head      service_list;
	struct gxlist_head      scheduler_list;
	struct msg_property     msg_map[GXMSG_MAX_NUM_SUPPORT];
};


static struct gxbus_property gxbus = {
	GXCORE_INVALID_POINTER,
};

GxServiceMemConf service_mem = {
	.player_reserved_mem = 1024*1024*10,
};

static struct mqueue *mqueue_create(int32_t max_size)
{
	struct mqueue* queue = NULL;

	queue = (struct mqueue *)GxCore_Malloc(sizeof(struct mqueue));
	if (queue == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_NOT_ENOUGH_MEM);
		return NULL;
	}
	memset(queue, 0, sizeof(struct mqueue));

	queue->size = max_size;

	if (GxCore_SemCreate(&queue->get_sem, 0) != GXCORE_SUCCESS) {
		GxCore_Free(queue);
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return NULL;
	}

	if (GxCore_SemCreate(&queue->send_sem, max_size) != GXCORE_SUCCESS) {
		GxCore_Free(queue);
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return NULL;
	}

	if (GxCore_MutexCreate(&queue->lock) != GXCORE_SUCCESS) {
		GxCore_SemDelete(queue->get_sem);
		GxCore_SemDelete(queue->send_sem);
		GxCore_Free(queue);
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return NULL;
	}

	queue->working = 1;
	GX_INIT_LIST_HEAD(&queue->msg_list);

	return queue;
}

static void mqueue_close(struct mqueue *queue)
{
	int32_t count, i;

	queue->working = 0;
	GxCore_SemGetValue(queue->send_sem, &count);
	for (i=0; i < queue->size - count; i++)
		GxCore_SemPost(queue->send_sem);

	/*make the msg scheduler keep running*/
	GxCore_SemPost(queue->get_sem);
	GxCore_ThreadYield();

	if (GxCore_SemDelete(queue->get_sem) != GXCORE_SUCCESS) {
		GXBUS_TRACE("%s%d: GxCore_SemDelete Fault\n", __FUNCTION__, __LINE__);
	}
	if (GxCore_SemDelete(queue->send_sem) != GXCORE_SUCCESS) {
		GXBUS_TRACE("%s%d: GxCore_SemDelete Fault\n", __FUNCTION__, __LINE__);
	}

	if (GxCore_MutexDelete(queue->lock) != GXCORE_SUCCESS) {
		GXBUS_TRACE("%s%d: GxCore_MutexDelete Fault\n", __FUNCTION__, __LINE__);
	}
	GxCore_Free(queue);
}

static status_t mqueue_send(struct mqueue *queue, struct msg_packet * packet)
{
	if (packet == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	if (GxCore_SemWait(queue->send_sem) != GXCORE_SUCCESS) {
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return GXCORE_ERROR;
	}

	if (!queue->working)
		return GXCORE_ERROR;

	QUEUE_LOCK(queue);
	if (packet->msg_ext->prior == GXMSG_PRIOR_FAST) {
		gxlist_add(&packet->msg_head, &queue->msg_list);
	} else {
		gxlist_add_tail(&packet->msg_head, &queue->msg_list);
	}
	QUEUE_UNLOCK(queue);

	if(GxCore_SemPost(queue->get_sem) != GXCORE_SUCCESS) {
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

static struct msg_packet *mqueue_get(struct mqueue *queue)
{
	struct msg_packet*      msg = NULL;
	struct gxlist_head*     pos;
	struct gxlist_head*     n;
	if (GxCore_SemWait(queue->get_sem) != GXCORE_SUCCESS) {
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return NULL;
	}

	if (!queue->working)
		return NULL;

	QUEUE_LOCK(queue);
	gxlist_for_each_safe( pos, n, &queue->msg_list ) {
		msg = gxlist_entry(pos, struct msg_packet, msg_head);
		gxlist_del(pos);
		break;
	}
	QUEUE_UNLOCK(queue);

	if(GxCore_SemPost(queue->send_sem) != GXCORE_SUCCESS) {
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return NULL;
	}

	return msg;
}

static int32_t mqueue_get_num(struct mqueue *queue)
{
	int32_t value;

	if (queue == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return 0;
	}

	QUEUE_LOCK(queue);
	GxCore_SemGetValue(queue->get_sem, &value);
	QUEUE_UNLOCK(queue);

	return value;
}
static void packet_cache_close(struct gxlist_head *list)
{
	struct msg_packet* p  = NULL ;
	struct msg_packet* p_temp = NULL;

//	BUS_LOCK();//调用该接口的地方BUS_LOCK
	gxlist_for_each_entry_safe(p, p_temp, list, cache_head)
	{

		gxlist_del(&(p->cache_head));
		GxCore_Free(p);
	}
	gxbus.packet_count = 0;//缓冲数量清零
//	BUS_UNLOCK();
}

static void* packet_cache_malloc(struct gxlist_head *list)
{
	struct msg_packet* p  = NULL ;
	struct msg_packet* p_temp = NULL;

	//BUS_LOCK();
	if(gxbus.packet_count != 0)
	{
		gxlist_for_each_entry_safe(p, p_temp, list, cache_head)
		{
			gxlist_del(&(p->cache_head));
			break;
		}
		gxbus.packet_count--;//缓冲分配走一个；
	}
	else
	{
		p = GxCore_Mallocz(sizeof(struct msg_packet));
		if( p == NULL)
		{
			gxlogd("malloc packet not enougth mem!!\n");
		}
	}
	//BUS_UNLOCK();
	return (void*)p;
}

static void packet_cache_free(struct gxlist_head *list, struct msg_packet* packet)
{
	//BUS_LOCK();//调用该接口的地方BUS_LOCK
	if(gxbus.packet_count == MEM_POOL_PACKET_NUM)//缓冲已经满了 
	{
		GxCore_Free(packet);
	}
	else//缓冲没满，放入缓冲
	{
		gxlist_add(&(packet->cache_head), list);
		gxbus.packet_count++;//缓冲增加一个；
	}
	//BUS_UNLOCK();
	return;
}

#ifdef __GXBUS_DEBUG
static int32_t mempool_get_free_num(handle_t pool)
{
	struct mempool_ext* pool_ext = (struct mempool_ext*)pool;
	int32_t value;

	if (pool == GXCORE_INVALID_POINTER || pool_ext == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return 0;
	}

	GxCore_SemGetValue(pool_ext->sem, &value);
	return value;
}
#endif

static void sched_msg_thread(void *arg)
{
	struct scheduler*       sch = (struct scheduler *)arg;
	struct service_ext*     ser;
	struct msg_packet*      packet;

	GXBUS_ASSERT(sch!=NULL);

	while (sch->quit==RUNNING) {
		packet = mqueue_get(sch->mqueue);
		if (packet == NULL || packet->msg_ext == NULL) {
			continue;
		}

		ser = packet->dest_service;
		if (ser == NULL ||ser->ops == NULL || ser->ops->msg_process == NULL) {
			GxCore_ThreadDelay(20);
			continue;
		}

		ser->ops->msg_process((handle_t)ser, &packet->msg_ext->msg);
		BUS_LOCK();
		packet->msg_ext->process_count--;
		if(  packet->msg_ext->process_count==0 ) {
			if ( packet->msg_ext->sync_lock != GXCORE_INVALID_POINTER ) {
				GxCore_SemPost(packet->msg_ext->sync_lock);
			} else {
				GxCore_Free(packet->msg_ext);
			}
		}

		packet_cache_free(&(gxbus.msg_packet_pool), packet);
		BUS_UNLOCK();
	}
	sch->quit = QUITED;
}

static void sched_console_thread(void *arg)
{
	struct scheduler*   sch = (struct scheduler *)arg;
	struct gxlist_head* pos;
	struct service_ext* ser_ext;

	GXBUS_ASSERT(sch != NULL);

	while (sch->quit==RUNNING) {
		SCHED_LOCK(sch);
		if (gxlist_empty(&sch->service_list)) {
			SCHED_UNLOCK(sch);
			GxCore_ThreadDelay(100);
			continue;
		}
		gxlist_for_each(pos, &sch->service_list) {
			ser_ext = gxlist_entry(pos, struct service_ext, node_console_ser_list);
			SCHED_UNLOCK(sch);
			if (ser_ext == NULL || ser_ext->ops == NULL) {
				continue;
			}
			if (ser_ext->ops->console_process != NULL) {

				ser_ext->ops->console_process((handle_t)ser_ext);
			}
		}
	}
	sch->quit = QUITED;
}


status_t gxbus_message_free(GxMessage *message)
{
	struct msg_ext *msg;

//	BUS_LOCK();
	if (message == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	msg = gxlist_entry(message, struct msg_ext, msg);
	GxCore_Free(msg);

//	BUS_UNLOCK();
	return GXCORE_SUCCESS;
}
void GxBus_GetVersion(void)
{
	//gxlogd("\n\nGoXceed version: %d.%d.%d-%d\n\n",
	//	GXBUS_MAJOR_VERSION, GXBUS_MINOR_VERSION, GXBUS_SUB_VERSION, GXBUS_PATCH_LEVEL);
}

status_t GxBus_Init(GxServiceClass* service_list, uint32_t service_num)
{
	uint32_t        i;
	status_t        ret;

	GxBus_GetVersion();
	GxBus_ConfigMutexInit();
	GXBUS_DbgPrint("Current message num=%d\n",GXMSG_TOTAL_NUM);

	GXBUS_ASSERT(service_list != NULL);
	GXBUS_ASSERT(service_num != 0 && service_list != NULL);

	if ((service_list == NULL && service_num > 0)
			|| (service_num == 0 && service_list != NULL)) {
		return GXCORE_ERROR;
	}

	if(gxbus.lock != GXCORE_INVALID_POINTER) {
		GXBUS_WARNING("GxBus_Init() could call ONE TIME before GxBus_Destroy() had been called!");
		return GXCORE_ERROR;
	}

	memset(&gxbus, 0, sizeof(struct gxbus_property));
	ret = GxCore_MutexCreate(&gxbus.lock);
	if (ret != GXCORE_SUCCESS) {
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		return GXCORE_ERROR;
	}

	GX_INIT_LIST_HEAD(&(gxbus.msg_packet_pool));

	for ( i=0; i<GXMSG_MAX_NUM_SUPPORT; i++ ) {
		gxbus.msg_map[i].priv_size = -1;
		GX_INIT_LIST_HEAD( &gxbus.msg_map[i].service_list );
	}

	GX_INIT_LIST_HEAD(&gxbus.scheduler_list);
	GX_INIT_LIST_HEAD(&gxbus.service_list);

	for (i = 0; i < service_num; i++) {
		if (GXCORE_INVALID_POINTER == GxBus_ServiceCreate(&service_list[i])) {
			GXBUS_TRACE("Service %s initialize failure!\n", service_list[i].name);
			return GXCORE_ERROR;
		}
	}

	return GXCORE_SUCCESS;
}

status_t GxBus_Destroy(void)
{
	struct gxlist_head*         pos;
	struct gxlist_head*         n;
	struct service_ext*         ser_ext;
	struct scheduler*           sch;

	if(gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_WARNING("GxBus_Destroy() could called after GxBus_Init() had been called!");
		return GXCORE_ERROR;
	}

	gxlist_for_each_safe(pos, n, &gxbus.service_list) {
		ser_ext = gxlist_entry(pos, struct service_ext, node_bus_ser_list);
		GxBus_ServiceDestroy((handle_t)ser_ext);
	}

	gxlist_for_each_safe(pos, n, &gxbus.scheduler_list) {
		sch = gxlist_entry(pos, struct scheduler, head);
		sch->refcount = 1;
		GxBus_SchedulerDestroy((handle_t)sch);
	}

	BUS_LOCK();
	packet_cache_close(&(gxbus.msg_packet_pool));
	GX_INIT_LIST_HEAD(&(gxbus.msg_packet_pool));
	BUS_UNLOCK();

	GxCore_MutexDelete(gxbus.lock);

	gxbus.lock = GXCORE_INVALID_POINTER;

	return GXCORE_SUCCESS;
}

handle_t GxBus_SchedulerFindByName(const char *name)
{
	struct gxlist_head* pos;
	struct scheduler*   sch;

	GXBUS_ASSERT(name != NULL);
	GXBUS_ASSERT(strlen(name) < NAME_SIZE);

	if (name == NULL || strlen(name) >= NAME_SIZE ) {
		return GXCORE_INVALID_POINTER;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_INVALID_POINTER;
	}

	gxlist_for_each(pos, &gxbus.scheduler_list) {
		sch = gxlist_entry(pos, struct scheduler, head);
		if (0 == strncmp(sch->name, name, strlen(name))) {
			return (handle_t)sch;
		}
	}

	return GXCORE_INVALID_POINTER;
}

handle_t GxBus_SchedulerCreate(const char *name, GxSchedType type,
		size_t stack_size, uint8_t prior)
{
	status_t ret = GXCORE_ERROR;
	struct scheduler*       sch;

	GXBUS_ASSERT(name != NULL);
	GXBUS_ASSERT(strlen(name) < NAME_SIZE);
	GXBUS_ASSERT(type < GXBUS_SCHED_TOTAL_NUM);

	if (name == NULL
			|| strlen(name) >= NAME_SIZE
			|| type >= GXBUS_SCHED_TOTAL_NUM) {
		return GXCORE_INVALID_POINTER;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_INVALID_POINTER;
	}

	BUS_LOCK();
	sch = (struct scheduler* )GxBus_SchedulerFindByName(name);
	if (sch != (struct scheduler* )GXCORE_INVALID_POINTER) {
		SCHED_LOCK(sch);
		sch->refcount++;
		SCHED_UNLOCK(sch);
		BUS_UNLOCK();
		return (handle_t)sch;
	}
	sch = (struct scheduler *)GxCore_Malloc(sizeof(struct scheduler));
	if (sch == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_NOT_ENOUGH_MEM);
		BUS_UNLOCK();
		return GXCORE_INVALID_POINTER;
	}

	memset(sch, 0, sizeof(struct scheduler));
	if (GxCore_MutexCreate(&sch->lock) != GXCORE_SUCCESS) {
		GxCore_Free(sch);
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		BUS_UNLOCK();
		return GXCORE_INVALID_POINTER;
	}

	if (type == GXBUS_SCHED_MSG) {
		sch->mqueue = mqueue_create(MAX_QUEUE_SIZE);
		if (sch->mqueue == NULL) {
			GxCore_MutexDelete(sch->lock);
			GxCore_Free(sch);
			BUS_UNLOCK();
			return GXCORE_INVALID_POINTER;
		}
	}

	sch->type       = type;
	sch->quit       = RUNNING;
	sch->stack_size = stack_size;
	sch->prior      = prior;
	sch->refcount   = 1;

	strncpy(sch->name, name, NAME_SIZE - 1);
	GX_INIT_LIST_HEAD(&sch->service_list);
	if (type == GXBUS_SCHED_MSG){
		ret = GxCore_ThreadCreate(sch->name, &sch->thread_id,
				sched_msg_thread, (void *)sch, stack_size, prior);

	}else if (type == GXBUS_SCHED_CONSOLE){
		ret = GxCore_ThreadCreate(sch->name, &sch->thread_id,
				sched_console_thread, (void *)sch, stack_size, prior);
	}
	if (ret != GXCORE_SUCCESS) {
		GXBUS_TRACE("GxCore_ThreadCreate Failure%d\n", ret);
		mqueue_close(sch->mqueue);
		GxCore_MutexDelete(sch->lock);
		GxCore_Free(sch);
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		BUS_UNLOCK();
		return GXCORE_INVALID_POINTER;
	}
	gxlist_add( &sch->head, &gxbus.scheduler_list );
	BUS_UNLOCK();

	return (handle_t)sch;
}

status_t GxBus_SchedulerDestroy(handle_t h_scheduler)
{
	struct scheduler* sch = (struct scheduler*)h_scheduler;

	GXBUS_ASSERT(h_scheduler != GXCORE_INVALID_POINTER);
	GXBUS_ASSERT(sch != NULL);

	if (h_scheduler == GXCORE_INVALID_POINTER || sch == NULL)
		return GXCORE_ERROR;

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_ERROR;
	}

	BUS_LOCK();
	SCHED_LOCK(sch);
	if (--sch->refcount) {
		BUS_UNLOCK();
		SCHED_UNLOCK(sch);
		return GXCORE_SUCCESS;
	}
	sch->quit = QUITING;
	if (sch->mqueue != NULL) {
		mqueue_close(sch->mqueue);
		sch->mqueue = NULL;
	}
	GxCore_ThreadJoin(sch->thread_id);
	gxlist_del(&sch->head);
	SCHED_UNLOCK(sch);
	BUS_UNLOCK();

	GxCore_MutexDelete(sch->lock);
	GxCore_Free(sch);

	return GXCORE_SUCCESS;
}

handle_t GxBus_ServiceCreate(GxServiceClass *ops)
{
	struct service_ext* ser_ext;

	GXBUS_ASSERT(ops != NULL);
	GXBUS_ASSERT(strlen(ops->name) < NAME_SIZE);
	GXBUS_ASSERT(ops->init != NULL);
	GXBUS_ASSERT(ops->msg_process != NULL || ops->console_process != NULL);
	GXBUS_ASSERT(ops->destroy != NULL);

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_INVALID_POINTER;
	}

	if (ops == NULL
			|| strlen(ops->name) >= NAME_SIZE
			|| ops->init == NULL
			|| ops->destroy == NULL
			|| (ops->msg_process == NULL && ops->console_process == NULL)) {
		return GXCORE_INVALID_POINTER;
	}

	ser_ext = (struct service_ext *)GxCore_Malloc(sizeof(struct service_ext));
	if (ser_ext == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_NOT_ENOUGH_MEM);
		return GXCORE_INVALID_POINTER;
	}
	memset(ser_ext, 0, sizeof(struct service_ext));

	ser_ext->ops = (GxServiceClass*)GxCore_Malloc(sizeof(GxServiceClass));
	if (ser_ext->ops == NULL) {
		GxCore_Free(ser_ext);
		GXBUS_ERROR(GXBUS_ERROR_NOT_ENOUGH_MEM);
		return GXCORE_INVALID_POINTER;
	}
	memcpy(ser_ext->ops, ops, sizeof(GxServiceClass));

	if (ops->init((handle_t)ser_ext,ops->priority_offset) != GXCORE_SUCCESS) {

		GxCore_Free(ser_ext);
		GXBUS_DbgPrint("Service:%s init failure", ops->name);
		return GXCORE_INVALID_POINTER;
	}
	ops->self = (handle_t)ser_ext;

	BUS_LOCK();
	gxlist_add(&ser_ext->node_bus_ser_list, &gxbus.service_list);
	BUS_UNLOCK();

	return (handle_t)ser_ext;
}

status_t GxBus_ServiceDestroy(handle_t service)
{
	struct service_ext* ser_ext = (struct service_ext*)service;

	GXBUS_ASSERT(service != GXCORE_INVALID_POINTER);
	GXBUS_ASSERT(ser_ext != NULL);

	if (service == GXCORE_INVALID_POINTER
			|| ser_ext == NULL) {
		return GXCORE_ERROR;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_ERROR;
	}

	if (ser_ext->ops != NULL && ser_ext->ops->destroy != NULL){
		ser_ext->ops->destroy((handle_t)ser_ext);
	}

	BUS_LOCK();
	gxlist_del(&ser_ext->node_bus_ser_list);
	BUS_UNLOCK();

	GxCore_Free(ser_ext->ops);
	GxCore_Free(ser_ext);

	return GXCORE_SUCCESS;
}

handle_t GxBus_ServiceFindByName(const char* name)
{
	struct gxlist_head* pos;
	struct service_ext* ser;

	GXBUS_ASSERT(name != NULL);
	GXBUS_ASSERT(strlen(name) < NAME_SIZE);

	if (name == NULL || strlen(name) >= NAME_SIZE ) {
		return GXCORE_INVALID_POINTER;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_INVALID_POINTER;
	}

	gxlist_for_each(pos, &gxbus.service_list) {
		ser = gxlist_entry(pos, struct service_ext, node_bus_ser_list);
		if (0 == strncmp(ser->ops->name, name, strlen(name))) {
			return (handle_t)ser;
		}
	}

	return GXCORE_INVALID_POINTER;
}

status_t GxBus_ServiceLink(handle_t h_service, handle_t h_scheduler)
{
	struct service_ext* ser_ext = (struct service_ext*)h_service;
	struct scheduler*   sch     = (struct scheduler*)h_scheduler;
	struct scheduler*   tmp_sch;

	GXBUS_ASSERT(h_service != GXCORE_INVALID_POINTER);
	GXBUS_ASSERT(ser_ext != NULL);

	if (h_service == GXCORE_INVALID_POINTER
			|| ser_ext == NULL
			|| h_scheduler == GXCORE_INVALID_POINTER
			|| sch == NULL ) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_ERROR;
	}

	tmp_sch = GET_SCHEDULER_BY_TYPE(sch, ser_ext);

	if (tmp_sch == (struct scheduler*)sch) {
		return GXCORE_SUCCESS;

	} else if (tmp_sch != NULL) {
		GxBus_ServiceUnlink(h_service);
	}

	if (sch->type == GXBUS_SCHED_MSG && ser_ext->ops->msg_process != NULL) {
		ser_ext->scheduler_msg = sch;
		return GXCORE_SUCCESS;
	}

	if (sch->type == GXBUS_SCHED_CONSOLE
			&&ser_ext->ops->console_process!= NULL) {
		ser_ext->scheduler_console = sch;
		SCHED_LOCK(sch);
		gxlist_add_tail(&ser_ext->node_console_ser_list, &sch->service_list);
		SCHED_UNLOCK(sch);
		return GXCORE_SUCCESS;
	}

	return GXCORE_ERROR;
}

status_t GxBus_ServiceUnlink(handle_t h_service)
{
	struct service_ext* ser_ext = (struct service_ext*)h_service;

	if (h_service == GXCORE_INVALID_POINTER || ser_ext == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_ERROR;
	}


	if(ser_ext->scheduler_console != NULL) {
		GxBus_SchedulerDestroy((handle_t) ser_ext->scheduler_console);
		gxlist_del(&ser_ext->node_console_ser_list);
		ser_ext->scheduler_console = NULL;
	}

	if (ser_ext->scheduler_msg != NULL) {
		GxBus_SchedulerDestroy((handle_t) ser_ext->scheduler_msg);
		ser_ext->scheduler_msg = NULL;
	}

	return GXCORE_SUCCESS;
}

status_t GxBus_MessageRegister(GxMsgID msg_id, int32_t size)
{
	if (msg_id >= GXMSG_MAX_NUM_SUPPORT || size < 0) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_ERROR;
	}
	gxbus.msg_map[msg_id].priv_size = size;
	return GXCORE_SUCCESS;
}

void GxBus_MessageUnregister(GxMsgID msg_id)
{
	struct gxlist_head*   pos;
	struct gxlist_head*   n;

	if (msg_id >= GXMSG_MAX_NUM_SUPPORT){
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return;
	}

	gxlist_for_each_safe(pos, n, &gxbus.msg_map[msg_id].service_list) {
		gxlist_del(pos);
		GXBUS_WARNING("The service list of msg is not empty.\nPlease call GxBus_Message//有漏发的风险，UnListen() at first.");
		return;
	}

	/*The list of service list in msg_map is empty, clear the information.*/
	gxbus.msg_map[msg_id].priv_size = -1;
	return;
}

status_t GxBus_MessageListen(handle_t h_service, GxMsgID msg_id)
{
	struct service_ext*     ser_ext = (struct service_ext*)h_service;
	struct service_node*    node;
	struct gxlist_head*     pos;

	if (h_service == GXCORE_INVALID_POINTER
			|| ser_ext == NULL
			|| msg_id >= GXMSG_MAX_NUM_SUPPORT) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return GXCORE_ERROR;
	}

	BUS_LOCK();
	gxlist_for_each(pos, &gxbus.msg_map[msg_id].service_list) {
		node = gxlist_entry(pos, struct service_node, head);
		if (node->service==ser_ext) {
			node->status = 1;//表示处于监听状态
			BUS_UNLOCK();
			return GXCORE_SUCCESS;
		}
	}

	node = (struct service_node*)GxCore_Malloc( sizeof(struct service_node));
	if (node == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		BUS_UNLOCK();
		return GXCORE_ERROR;
	}
	node->service = ser_ext;

	node->status = 1;//表示处于监听状态
	gxbus.msg_map[msg_id].listen_count +=1;
	gxlist_add_tail(&node->head, &gxbus.msg_map[msg_id].service_list);
	BUS_UNLOCK();

	return GXCORE_SUCCESS;
}

status_t GxBus_MessageUnListen(handle_t h_service, GxMsgID msg_id)
{
	struct service_ext*     ser_ext = (struct service_ext*)h_service;
	struct service_node*    node;
	struct gxlist_head*     pos;
	struct gxlist_head*     n;

	if (h_service == GXCORE_INVALID_POINTER
			|| ser_ext == NULL
			|| msg_id >= GXMSG_MAX_NUM_SUPPORT) {

		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	BUS_LOCK();
	gxlist_for_each_safe(pos, n, &gxbus.msg_map[msg_id].service_list) {
		node = gxlist_entry(pos, struct service_node, head);
		if (node->service == ser_ext) {
			node->status = 0;//表示处于不监听状态
			//gxlist_del(&node->head);
			//GxCore_Free(node);//删除了会导致GxBus_MessageSendPriori接口获取到错误指针
			BUS_UNLOCK();
			gxbus.msg_map[msg_id].listen_count -=1;
			return GXCORE_SUCCESS;
		}
	}
	BUS_UNLOCK();

	GXBUS_ERROR(GXBUS_ERROR_MSG_NOT_LISTEN);
	return GXCORE_ERROR;
}

GxMessage *GxBus_MessageNew(GxMsgID msg_id)
{
	uint32_t            size;
	handle_t            pool = GXCORE_INVALID_POINTER;
	struct msg_ext*     new_msg;

	if (msg_id >= GXMSG_MAX_NUM_SUPPORT) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return NULL;
	}

	if (gxbus.lock == GXCORE_INVALID_POINTER) {
		GXBUS_ERROR(GXBUS_ERROR_BUS_NOT_INIT);
		return NULL;
	}

	if (gxbus.msg_map[msg_id].priv_size == -1) {
		GXBUS_TRACE("\n%s: msg_id=%d have not register!\n", __FUNCTION__, msg_id);
		GXBUS_ERROR(GXBUS_ERROR_MSG_NOT_REGISTER);
		return NULL;
	}

	size = gxbus.msg_map[msg_id].priv_size;
	
	new_msg = GxCore_Malloc(sizeof(struct msg_ext)+size);
	if (new_msg == NULL) {
		GXBUS_TRACE("new msg Fault!\n");
		return NULL;
	}
	memset(new_msg, 0, (sizeof(struct msg_ext)+size));
	new_msg->msg.msg_id = msg_id;
	new_msg->msg.size   = gxbus.msg_map[msg_id].priv_size;
	new_msg->sync_lock  = GXCORE_INVALID_POINTER;
	new_msg->pool       = pool;

	return &new_msg->msg;
}

status_t GxBus_MessageFree(GxMessage *message)
{
	struct msg_ext *msg;

	BUS_LOCK();
	if (message == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		BUS_UNLOCK();
		return GXCORE_ERROR;
	}

	msg = gxlist_entry(message, struct msg_ext, msg);
	GxCore_Free(msg);

	BUS_UNLOCK();
	return GXCORE_SUCCESS;
}

status_t GxBus_MessageEmpty(handle_t h_service)
{
	struct service_ext*     ser_ext = (struct service_ext*)h_service;
	struct scheduler*       sch;
	struct msg_packet*      packet;
	int32_t                 num, num_bak = 0xFFFF;
	int32_t                 i;

	if (h_service == GXCORE_INVALID_POINTER || ser_ext == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	sch = ser_ext->scheduler_msg;
	for (i = 0; num = mqueue_get_num(sch->mqueue), num = (num<num_bak?num:num_bak), num_bak = num, i < num; i++) {
		packet = mqueue_get(sch->mqueue);
		if (packet == NULL)
			continue;

		BUS_LOCK();
		packet->msg_ext->process_count--;
		if(  packet->msg_ext->process_count==0 ) {
			if ( packet->msg_ext->sync_lock != GXCORE_INVALID_POINTER )
				GxCore_SemPost(packet->msg_ext->sync_lock);
			else
				gxbus_message_free(&packet->msg_ext->msg);
		}
		packet_cache_free(&(gxbus.msg_packet_pool), packet);
		BUS_UNLOCK();
	}

	return GXCORE_SUCCESS;
}

status_t GxBus_MessageEmptyByOps(GxServiceClass* ops)
{
	return GxBus_MessageEmpty(ops->self);
}

GxMessage* GxBus_MessageTryGet(handle_t self)
{
	struct service_ext* ser_ext = (struct service_ext*)self;
	struct msg_packet*  packet = NULL;
//	static struct msg_ext msg_ext = {0};

	if (mqueue_get_num(ser_ext->scheduler_msg->mqueue) == 0)
		return NULL;

	packet = mqueue_get(ser_ext->scheduler_msg->mqueue);
	GXBUS_ASSERT(packet != NULL || packet->msg_ext != NULL);
	BUS_LOCK();
	packet->msg_ext->process_count--;
//	msg_ext = packet->msg_ext;
	/*memcpy(&msg_ext, packet->msg_ext, sizeof(struct msg_ext));*/
	/*  if (packet->msg_ext->process_count == 0)*/ {
		packet_cache_free(&(gxbus.msg_packet_pool), packet);
	BUS_UNLOCK();
	}
	return &(packet->msg_ext->msg)/* &(packet->msg_ext->msg)*/;
}

status_t GxBus_MessageTryFree(GxMessage *message)
{
	struct msg_ext*     msg = NULL;

	if (message == NULL) {
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	msg = gxlist_entry(message, struct msg_ext, msg);
	if ((msg->process_count) == 0) {
		if (msg->sync_lock != GXCORE_INVALID_POINTER) {
			GxCore_SemPost(msg->sync_lock);
		} else {
			GxCore_Free(msg);
		}
	}

	return GXCORE_SUCCESS;
}

status_t GxBus_MessageSend(GxMessage *message)
{
	return GxBus_MessageSendPriority(message, GXMSG_PRIOR_NORMAL);
}

status_t GxBus_MessageSendPriority(GxMessage *message, int priority)
{
	struct msg_ext*         msg;
	struct service_ext*     ser_ext;
	struct gxlist_head*     pos;
	struct gxlist_head*     n;
	struct gxlist_head*     listen_service_list;
	struct service_node*    node;
	struct msg_packet*      packet;
	struct scheduler*       sch;

	if (message == NULL){
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	BUS_LOCK();
	msg = gxlist_entry(message, struct msg_ext, msg);
	listen_service_list = &GET_MSG_SERVICE_LIST(msg);

	if (gxlist_empty(listen_service_list)) {
		GXBUS_WARNING("This message have no service which needed it.");
		GxCore_Free(msg);
		BUS_UNLOCK();
		return GXCORE_ERROR;
	}
	

	gxlist_for_each (pos, listen_service_list) {
		node    = gxlist_entry(pos, struct service_node, head);
		if(node->status != 0)
		{
			msg->process_count++;
		}
	}
	if(msg->process_count == 0)//无人监听
	{
		GxCore_Free(msg);
		BUS_UNLOCK();
		return GXCORE_SUCCESS;
	}
	
	gxlist_for_each_safe(pos, n, listen_service_list){
		node    = gxlist_entry(pos, struct service_node, head);
		if(node->status == 0)
		{
			continue;
		}
		ser_ext = node->service;
		sch     = ser_ext->scheduler_msg;
		gxbus.msg_map[msg->msg.msg_id].count++;

		if (sch == NULL || sch->mqueue == NULL) {
			GXBUS_ERROR(GXBUS_ERROR_SERVICE_FAULT);
			continue;
		}
		if(msg->process_count != 0)
		{
			packet = (struct msg_packet*)packet_cache_malloc(&(gxbus.msg_packet_pool));
			packet->dest_service    = ser_ext;
			packet->msg_ext         = msg;
			packet->msg_ext->prior  = priority;
			BUS_UNLOCK();
			mqueue_send(sch->mqueue, packet);
			BUS_LOCK();
		}
		else
		{
			break;
		}
	}

	BUS_UNLOCK();
	return GXCORE_SUCCESS;
}

status_t GxBus_MessageSendWait(GxMessage* message)
{
	struct msg_ext*         msg;
	struct service_ext*     ser_ext;
	struct gxlist_head*     pos;
	struct gxlist_head*     n;
	struct gxlist_head*     listen_service_list;
	struct service_node*    node;
	struct msg_packet*      packet;
	status_t                ret;

	if (message == NULL){
		GXBUS_ERROR(GXBUS_ERROR_INPUT);
		return GXCORE_ERROR;
	}

	BUS_LOCK();
	msg = gxlist_entry(message, struct msg_ext, msg);
	listen_service_list = &GET_MSG_SERVICE_LIST(msg);

	if (gxlist_empty(listen_service_list)) {
		GXBUS_DbgPrint("This message:%d have no service that needed it.\n", msg->msg.msg_id);
		BUS_UNLOCK();
		return GXCORE_ERROR;
	}

	if(GXCORE_SUCCESS != GxCore_SemCreate( &msg->sync_lock, 0 )) {
		GXBUS_ERROR(GXBUS_ERROR_OS_CORE_FAULT);
		BUS_UNLOCK();
		return GXCORE_ERROR;
	}

	gxlist_for_each (pos, listen_service_list) {
		node    = gxlist_entry(pos, struct service_node, head);
		if(node->status != 0)
		{
			msg->process_count++;
		}
	}

	gxlist_for_each_safe(pos, n, listen_service_list){
		node = gxlist_entry(pos, struct service_node, head);
		if(node->status == 0)
		{
			continue;
		}
		ser_ext = node->service;

		if (ser_ext->scheduler_msg == NULL
				|| ser_ext->scheduler_msg->mqueue == NULL) {
			GXBUS_ERROR(GXBUS_ERROR_SERVICE_FAULT);
			BUS_UNLOCK();
			return GXCORE_ERROR;
		}
		if(msg->process_count != 0)
		{
			packet = (struct msg_packet*)packet_cache_malloc(&(gxbus.msg_packet_pool));
			packet->dest_service = ser_ext;
			packet->msg_ext = msg;
			gxbus.msg_map[msg->msg.msg_id].count++;

			BUS_UNLOCK();
			ret = mqueue_send(ser_ext->scheduler_msg->mqueue, packet);
			BUS_LOCK();
			GXBUS_ASSERT(ret == GXCORE_SUCCESS);
		}
		else
		{
			break;
		}
	}
	BUS_UNLOCK();

	ret = GxCore_SemWait(msg->sync_lock);
	GXBUS_ASSERT(ret == GXCORE_SUCCESS);

	ret = GxCore_SemDelete(msg->sync_lock);
	GXBUS_ASSERT(ret == GXCORE_SUCCESS);

	return GXCORE_SUCCESS;
}

status_t GxBus_ServiceMemConfSet(GxServiceMemConf* mem)
{
	if(mem == NULL || mem->player_reserved_mem < 2*1024*1024) {
		gxlogd("mem config is errr use defaule conf/n");
		return GXCORE_ERROR;
	}

	memcpy(&service_mem, mem, sizeof(GxServiceMemConf));

	return GXCORE_SUCCESS;
}

status_t GxBus_ServiceMemConfGet(GxServiceMemConf* mem)
{
	if(mem == NULL) {
		gxlogd("mem get config is errr/n");
		return GXCORE_ERROR;
	}

	memcpy(mem, &service_mem, sizeof(GxServiceMemConf));

	return GXCORE_SUCCESS;
}
