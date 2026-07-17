#include "mqueue.h"
#include "gxcore.h"
#include "gxcas_dbg.h"
#include "gxcas.h"
#include <assert.h>
#define GXCAS_QUEUE_MAX_DEPTH (100)
static handle_t gxcas_queue;
static handle_t gxcas_queue_ex;
#define GXCAS_QUEUE_SIZE      GXCAS_EVENT_MAX_LEN

typedef struct{
	unsigned long type;
	char value[GXCAS_QUEUE_SIZE];
}gxcas_queue_node;

int32_t GxCas_QueueInitEx(void)
{
	if (GxCore_QueueCreate(&gxcas_queue_ex, GXCAS_QUEUE_MAX_DEPTH, sizeof(gxcas_queue_node)) != GXCORE_SUCCESS) {
		CAS_ERR(CAS,"GxCas Queue Ex Create Failed!!!");
		return -1;
	}

	return 0;
}

int32_t GxCas_QueueGetEx(unsigned long *type, void **buf, int32_t timeout)
{
	uint32_t size;
	if (!buf || !gxcas_queue_ex) {
		CAS_ERR(CAS,"GxCas Queue Ex Get Error Param!!!");
		return -1;
	}

	int32_t ret = 0;
	uint32_t copy_size = 0;
	gxcas_queue_node node = {0};
	if ((ret = GxCore_QueueGet(gxcas_queue_ex, (void *)&node, sizeof(gxcas_queue_node), &copy_size, timeout)) != GXCORE_SUCCESS) {
		CAS_ERR(CAS,"GxCas Queue Ex Get Error:%d!!!", ret);
		return -1;
	}
	*type = node.type;
	size = copy_size - 4;
	*buf = GxCore_Mallocz(size);
	assert(*buf != NULL);
	memcpy(*buf, node.value, size);

	return 0;
}

int32_t GxCas_QueuePutEx(unsigned long type, void *buf, uint32_t size, int32_t timeout)
{
	if (gxcas_queue_ex == 0) {
		CAS_ERR(CAS, "GxCas Queue Ex Init First");
		return -1;
	}
		
	int32_t ret = 0;
	gxcas_queue_node node = {0};
	node.type = type;
	memcpy(node.value, buf, (size>GXCAS_QUEUE_SIZE?GXCAS_QUEUE_SIZE:size));
	if ((ret = GxCore_QueuePut(gxcas_queue_ex, (void *)&node, sizeof(gxcas_queue_node), timeout)) != GXCORE_SUCCESS) {
		CAS_ERR(CAS,"GxCas Queue Ex Put Error:%d!!!",ret);
		return -1;
	}

	return 0;
}

void GxCas_QueueReleaseEx(void)
{
	return;
}
int32_t GxCas_QueueInit()
{
	if (GxCore_QueueCreate(&gxcas_queue, GXCAS_QUEUE_MAX_DEPTH, sizeof(gxcas_queue_node)) != GXCORE_SUCCESS) {
		CAS_ERR(CAS,"GxCas Queue Create Failed!!!");
		return -1;
	}

	return 0;
}

int32_t GxCas_QueueGet(unsigned long *type, void **buf, int32_t timeout)
{
	uint32_t size;
	if (!buf || !gxcas_queue) {
		CAS_ERR(CAS,"GxCas Queue Get Error Param!!!");
		return -1;
	}

	int32_t ret = 0;
	uint32_t copy_size = 0;
	gxcas_queue_node node = {0};
	if ((ret = GxCore_QueueGet(gxcas_queue, (void *)&node, sizeof(gxcas_queue_node), &copy_size, timeout)) != GXCORE_SUCCESS) {
		CAS_ERR(CAS,"GxCas Queue Get Error:%d!!!", ret);
		return -1;
	}
	*type = node.type;
	size = copy_size - 4;
	*buf = GxCore_Mallocz(size);
	assert(*buf != NULL);
	memcpy(*buf, node.value, size);

	return 0;
}

int32_t GxCas_QueuePut(unsigned long type, void *buf, uint32_t size, int32_t timeout)
{
	if (gxcas_queue == 0) {
		CAS_ERR(CAS, "GxCas Queue Init First");
		return -1;
	}
		
	int32_t ret = 0;
	gxcas_queue_node node = {0};
	node.type = type;
	memcpy(node.value, buf, (size>GXCAS_QUEUE_SIZE?GXCAS_QUEUE_SIZE:size));
	if ((ret = GxCore_QueuePut(gxcas_queue, (void *)&node, sizeof(gxcas_queue_node), timeout)) != GXCORE_SUCCESS) {
		CAS_ERR(CAS,"GxCas Queue Put Error:%d!!!",ret);
		return -1;
	}

	return 0;
}

void GxCas_QueueRelease(void)
{
	return;
}
