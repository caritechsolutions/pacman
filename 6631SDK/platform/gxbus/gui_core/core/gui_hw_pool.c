#include "gui_private.h"
#include "common/gx_hw_malloc.h"
#ifdef LINUX_OS
#include <sys/mman.h>
#endif /*LINUX_OS*/

#ifndef NO_OS
typedef struct {
	uint32_t init_flag;  // 0: uninit, 1 inited
	GxHwInfo info;
	handle_t list_mutex;
	handle_t id_mutex;
	struct gxlist_head malloc;
	struct gxlist_head free;  // sort by hot
	uint32_t using_size;   // fix hw maxmum value
	uint32_t m_count;
	uint32_t f_count;
} GuiHwPoolObj;

typedef struct {
	GxHwMallocObj obj;  // for user
	uint32_t hot;  // hot degree, for private
	void *usr_handle;   // prevant from id conflict
	GxHwMallocFlag cache;  // for cache
	struct gxlist_head head;  // for preivate
	uint8_t* file;
	uint32_t line;
}GuiHwPoolPrivate;
#endif /*NO_OS*/

static GuiHwPoolObj GuiHwMemObj;
static uint32_t GUI_MALLOC_ID = 1;//id 从1开始 0作为第一次申请标志

#define GUIPOOL_BLOCK_NR (512)
#define CHECK_NUM (4)
#define CHECK_DATA (0x50)

static handle_t g_hwpool_handle = 0;
static handle_t g_hwpool_tlsf = 0;

static void gui_hw_block_init(void)
{
	g_hwpool_handle = GxCore_MemPoolCreate(sizeof(GuiHwPoolPrivate), GUIPOOL_BLOCK_NR, 0);
}

static void *gui_hwpool_block_malloc(void)
{
	void *temp = GxCore_MemPoolAlloc(g_hwpool_handle);
	if(NULL == temp) {
		temp = GUI_MALLOCZ(sizeof(GuiHwPoolPrivate));
	}

	return (temp);
}

static void gui_hwpool_block_free(void *p)
{
	if(GxCore_MemPoolFree(g_hwpool_handle, p) == GXCORE_ERROR) {
		GUI_FREE(p);
	}
}

static void gui_hwpool_list_and_nolock(struct gxlist_head *list, GuiHwPoolPrivate *p, uint8_t flag)
{
	GuiHwPoolPrivate* temp = NULL;
	GuiHwPoolPrivate* p_temp1 = NULL;
	GuiHwPoolPrivate* p_temp2 = NULL;
	uint8_t find = 0;

	temp = gui_hwpool_block_malloc();
	if(NULL == temp) {
		gxlogd("gui pool list add error !!\n");
		return;
	}

	memcpy(temp, p, sizeof(GuiHwPoolPrivate));
	GX_INIT_LIST_HEAD(&(temp->head));

	if(0 == flag) {
		gxlist_add(&(temp->head), list);
	} else {
		gxlist_for_each_entry_safe(p_temp1, p_temp2, list, head) {
			if((p_temp1->hot <= temp->hot) && (p_temp2->hot > temp->hot)) {
				find = GUI_TRUE;
				gxlist_add(&(temp->head),&(p_temp1->head));
				break;
			} else if(p_temp1->hot > temp->hot) {
				find = GUI_TRUE;
				gxlist_add(&(temp->head),list);
				break;
			}
		}
		if(GUI_FALSE == find) {
			gxlist_add_tail(&(temp->head),list);
		}
	}
}

static void gui_hwpool_list_drop_nolock(struct gxlist_head *list, uint32_t id, void* usr_handle, uint32_t size)
{
	GuiHwPoolPrivate *p = NULL;
	GuiHwPoolPrivate *p_temp = NULL;

	gxlist_for_each_entry_safe(p, p_temp, list, head)
	{
		if(p->obj.id == id && p->usr_handle == usr_handle && p->obj.size == size)
		{
			gxlist_del(&(p->head));
			GUI_FREE(p->file);
			gui_hwpool_block_free(p);
			break;
		}
	}

	return;
}

static void gui_hwpool_list_move_from_free_to_malloc(GuiHwPoolPrivate *p, uint8_t flag)
{
	gui_hwpool_list_and_nolock(&(GuiHwMemObj.malloc), p, flag);
	gui_hwpool_list_drop_nolock(&(GuiHwMemObj.free), p->obj.id, p->usr_handle, p->obj.size);
}

static void gui_hwpool_list_move_from_malloc_to_free(GuiHwPoolPrivate *p, uint8_t flag)
{
	GxCore_MutexLock(GuiHwMemObj.list_mutex);
	gui_hwpool_list_and_nolock(&(GuiHwMemObj.free), p, flag);
	gui_hwpool_list_drop_nolock(&(GuiHwMemObj.malloc), p->obj.id, p->usr_handle, p->obj.size);
	GxCore_MutexUnlock(GuiHwMemObj.list_mutex);
}

static inline void* gui_hw_getphys(GxHwInfo* info, uint8_t *p)
{
	if (p >= (uint8_t*)(info->mmp_address) && p < (uint8_t*)(info->mmp_address + info->size) && info->size != 0 ) {
		return(void*)((uint8_t*)(info->kernel_address) + (p - (uint8_t*)(info->mmp_address)));
	}

	return (p);
}

static void gui_hw_list_add(struct gxlist_head *list, GuiHwPoolPrivate* p,uint8_t flag)
{
	GuiHwPoolPrivate* temp = NULL;
	GuiHwPoolPrivate* p_temp1 = NULL;
	GuiHwPoolPrivate* p_temp2 = NULL;
	uint8_t find = 0;

	GxCore_MutexLock(GuiHwMemObj.list_mutex);

	temp = gui_hwpool_block_malloc();
	if(temp == NULL) {
		gxlogd("gx hw list add err!!");
		GxCore_MutexUnlock(GuiHwMemObj.list_mutex);
		return;
	}

	memcpy(temp,p,sizeof(GuiHwPoolPrivate));
	GX_INIT_LIST_HEAD(&(temp->head));

	if(flag == 0) {
		gxlist_add(&(temp->head), list);
	} else {
		gxlist_for_each_entry_safe(p_temp1, p_temp2, list, head) {
			if((p_temp1->hot <= temp->hot) && (p_temp2->hot > temp->hot)) {
				find = 1;
				gxlist_add(&(temp->head),&(p_temp1->head));
				break;
			} else if(p_temp1->hot > temp->hot) {
				find = 1;
				gxlist_add(&(temp->head),list);
				break;
			}
		}
		if(find == 0) {
			gxlist_add_tail(&(temp->head),list);
		}
	}

	GxCore_MutexUnlock(GuiHwMemObj.list_mutex);
}

static GuiHwPoolPrivate* gui_hwpool_list_find_nolock(struct gxlist_head *list, uint32_t id, void* usr_handle, uint32_t size)
{
	GuiHwPoolPrivate *p = NULL;
	GuiHwPoolPrivate *p_temp = NULL;
	uint8_t find = 0;

	gxlist_for_each_entry_safe(p, p_temp, list, head) {
		if(p->obj.id == id && p->usr_handle == usr_handle && p->obj.size == size) {
			find = GUI_TRUE;
			break;
		}
	}

	if(find != GUI_TRUE) {
		p = NULL;
	}

	return (p);
}

static GuiHwPoolPrivate* gui_hwpool_list_find(struct gxlist_head *list, uint32_t id, void* usr_handle, uint32_t size)
{
	GuiHwPoolPrivate *p = NULL;

	GxCore_MutexLock(GuiHwMemObj.list_mutex);

	p = gui_hwpool_list_find_nolock(list, id, usr_handle, size);

	GxCore_MutexUnlock(GuiHwMemObj.list_mutex);

	return (p);
}

static void gui_hwpool_list_drop(struct gxlist_head *list, uint32_t id, void* usr_handle, uint32_t size)
{
	GuiHwPoolPrivate *p = NULL;
	GuiHwPoolPrivate* p_temp = NULL;

	GxCore_MutexLock(GuiHwMemObj.list_mutex);
	gxlist_for_each_entry_safe(p, p_temp, list, head) {
		if(p->obj.id == id && p->usr_handle == usr_handle && p->obj.size == size) {
			gxlist_del(&(p->head));
			gui_hwpool_block_free(p);
			break;
		}
	}
	GxCore_MutexUnlock(GuiHwMemObj.list_mutex);
}

static int32_t gui_hwpool_obj_malloc_from_free(GxHwMallocObj *p, GxHwMallocFlag flag)
{
	GuiHwPoolPrivate *temp = NULL;

	GxCore_MutexLock(GuiHwMemObj.list_mutex);
	temp = gui_hwpool_list_find_nolock(&(GuiHwMemObj.free), p->id, (void *)p, p->size);
	if(NULL != temp) {
		memcpy(p,&(temp->obj),sizeof(GxHwMallocObj));
		p->flag = CACHE;
		/*move the node to malloc list frome free list*/
		temp->hot++;
		gui_hwpool_list_move_from_free_to_malloc(temp, 0);
		GuiHwMemObj.using_size += p->size + CHECK_NUM;
		GxCore_MutexUnlock(GuiHwMemObj.list_mutex);
		return (0);
	}
	GxCore_MutexUnlock(GuiHwMemObj.list_mutex);
	return (-1);
}

static void * GUI_HW_POOL_MALLOC(unsigned int size)
{
	void* p = NULL;

	if(GuiHwMemObj.info.size != 0) {
		p = tlsf_malloc(g_hwpool_tlsf, size);
	} else {
		p = GUI_MALLOCZ(size);
	}

	if(p != NULL) {
		GuiHwMemObj.m_count++;
	}

	return (p);
}

static void GUI_HW_POOL_FREE(void *p)
{
	if(0 != GuiHwMemObj.info.size) {
		tlsf_free(g_hwpool_tlsf, p);
	} else {
		GUI_FREE(p);
	}

	GuiHwMemObj.f_count++;

	return;
}

static int8_t gui_hwpool_list_clean(struct gxlist_head *list,uint32_t size)
{
	GuiHwPoolPrivate* p = NULL;
	GuiHwPoolPrivate *p_temp = NULL;
	uint32_t free_size = 0;
	int8_t find = -1;

	GxCore_MutexLock(GuiHwMemObj.list_mutex);

	gxlist_for_each_entry_safe(p, p_temp, list, head) {
		GUI_HW_POOL_FREE(p->obj.usr_p);
		free_size += p->obj.size;
		gxlist_del(&(p->head));

		GUI_FREE(p->file);
		p->file = NULL;

		gui_hwpool_block_free(p);
		find = 0;
		if(free_size >= size) {
			break;
		}
	}

	GxCore_MutexUnlock(GuiHwMemObj.list_mutex);

	return (find);
}

static int32_t gui_hwpool_obj_malloc(GxHwMallocObj *p, GxHwMallocFlag flag)
{
	uint32_t i = 0;

	if(NULL == p) {
		gxlogd("\n p is NULL \n");
		return (-1);
	}

	if(0 != p->id) {
		GuiHwPoolPrivate* temp = gui_hwpool_list_find(&(GuiHwMemObj.malloc), p->id, (void *)p, p->size);
		if(NULL != temp) {
			gxlogd("\n id = %d has not been freed!!\n", p->id);
			gxlogd("hw malloc error!!");
//			p->usr_p = p->kernel_p = NULL;
			return (-1);
#if 0
		} else if(MALLOC_WITH_CACHE == flag) {
			return (0);
#endif
		}

		if(0 == gui_hwpool_obj_malloc_from_free(p, flag)) {
			return (1);
		}
	}

	p->usr_p = GUI_HW_POOL_MALLOC(p->size + CHECK_NUM); // 4 bytes for checksum
	if(NULL == p->usr_p) {
		while(0 == gui_hwpool_list_clean(&(GuiHwMemObj.free), p->size)) {
			p->usr_p = GUI_HW_POOL_MALLOC(p->size + CHECK_NUM);
			if(NULL != p->usr_p) {
				break;
			}
		}
		if(NULL == p->usr_p) {
			p->kernel_p = NULL;
			gxlogd("memory not enough!!");
			gxlogd("fb mem using size = 0x%x!!", GuiHwMemObj.using_size);
			return (-1);
		}
	}

	GxCore_MutexLock(GuiHwMemObj.id_mutex);
	p->id = GUI_MALLOC_ID;
	GUI_MALLOC_ID++;
	GxCore_MutexUnlock(GuiHwMemObj.id_mutex);

	p->kernel_p = gui_hw_getphys(&(GuiHwMemObj.info), p->usr_p);
	p->flag = NO_CACHE;
	for(i=0; i < CHECK_NUM; i++) {
		((uint8_t*)(p->usr_p))[p->size+i] = CHECK_DATA;
	}
	GuiHwMemObj.using_size += p->size + CHECK_NUM;

	return (0);
}

void gui_hw_pool_init(void *buf, int32_t addr, int32_t size)
{
	if(GuiHwMemObj.init_flag) {
		gxlogd("\n gui memory pool has already inited !!\n");
		return;
	}

	GuiHwMemObj.info.gx_av_handle = GxAVCreateDevice(0);
	GuiHwMemObj.info.mmp_address = buf;
	GuiHwMemObj.info.kernel_address = (void *)addr;
	GuiHwMemObj.info.size = size;

	memset(GuiHwMemObj.info.mmp_address, 0, 4);

	g_hwpool_tlsf = tlsf_init(size, GuiHwMemObj.info.mmp_address);

	GX_INIT_LIST_HEAD(&(GuiHwMemObj.malloc));
	GX_INIT_LIST_HEAD(&(GuiHwMemObj.free));
	GxCore_MutexCreate(&(GuiHwMemObj.list_mutex));
	GxCore_MutexCreate(&(GuiHwMemObj.id_mutex));

	gui_hw_block_init();

	GuiHwMemObj.init_flag = GUI_TRUE;

	return;
}

#define     MAX_ID_LIST       (1000)
static int malloc_list[MAX_ID_LIST];

static void _add_id_to_list(int id)
{
	int i = 0;

	for(i = 0; i < MAX_ID_LIST; i++) {
		if(malloc_list[i] == 0) {
			malloc_list[i] = id;
			break;
		}
	}
}

static void _remove_id_from_list(int id)
{
	int i = 0;

	for(i = 0; i < MAX_ID_LIST; i++) {
		if(malloc_list[i] == id) {
			malloc_list[i] = 0;
			break;
		}
	}
}

void print_malloc_list(void)
{
	int i = 0;

	gxlogi("[GUI HW MALLOC]{");
	for(i = 0; i < MAX_ID_LIST; i++) {
		if(malloc_list[i] != 0) {
			gxlogi("%d, ", malloc_list[i]);
		}
	}
	gxlogi("}\n");
}

void gui_hw_pool_malloc(GxHwMallocObj *p, GxHwMallocFlag flag)
{
	GuiHwPoolPrivate *temp = NULL;
	int ret = 0;

	if(0 != gui_hwpool_obj_malloc(p, flag)) {
#if 0
		if(flag == MALLOC_WITH_CACHE) {
			p->id = 0;
			p->usr_p = p->kernel_p = NULL;
			ret = gui_hwpool_obj_malloc(p, flag);
		}
		if(0 != ret) {
			gxloge("hw malloc error!!");
			return;
		}
#endif
		return;
	}

	temp = GxCore_Mallocz(sizeof(GuiHwPoolPrivate));
	memcpy(&(temp->obj),p,sizeof(GxHwMallocObj));
	temp->hot = 0;
	temp->usr_handle = (void*)p;
	temp->cache = flag;
	gui_hw_list_add(&(GuiHwMemObj.malloc), temp, 0);
	GxCore_Free(temp);
	_add_id_to_list(p->id);
	return;
}

int32_t gui_hw_pool_check(GxHwMallocObj *p)
{
	uint32_t i = 0;
	GuiHwPoolPrivate* temp = NULL;

	if(p->id == 0 || p->usr_p == NULL) {
		gxlogd("id = %d not malloc!!\n",p->id);
		gxlogd("hw check err!!");
		return (-1);
	}

	temp = gui_hwpool_list_find(&(GuiHwMemObj.malloc), p->id, (void*)p, p->size);
	if(NULL != temp) {
		for(i=0;i<CHECK_NUM;i++) {
			if(((uint8_t*)(temp->obj.usr_p))[p->size+i] != CHECK_DATA) {
				gxlogd("hw check memory out of bound!!");
				gxlogd("id = %d usr_p = %p kernel_p = %p size =%d hot = %d!!\n",
					temp->obj.id,temp->obj.usr_p, temp->obj.kernel_p, temp->obj.size,temp->hot);
				return (-1);
			}
		}
		return (0);
	}
	temp = gui_hwpool_list_find(&(GuiHwMemObj.free), p->id, (void*)p, p->size);
	if(NULL != temp) {
		gxlogd("hw check this p is free!!\n");
		for(i=0; i < CHECK_NUM; i++) {
			if(((uint8_t*)(temp->obj.usr_p))[p->size+i] != CHECK_DATA) {
				gxlogd("hw check memory out of bound!!");
				gxlogd("id = %d usr_p = %p kernel_p = %p size =%d hot = %d!!\n",
					temp->obj.id,temp->obj.usr_p, temp->obj.kernel_p, temp->obj.size,temp->hot);
				return (-1);
			}
		}
		return (0);
	}

	gxlogd("id = %d not malloc!!\n",p->id);
	gxlogd("hw check err not find this p!!");

	return (-2);
}

void gui_hw_pool_free(GxHwMallocObj *p)
{
	GuiHwPoolPrivate *temp = NULL;
	int ret = 0;

	ret = gui_hw_pool_check(p);
	if(-1 == ret) {
//		return;
		while(1);
	} else if(-2 == ret) {
		return;
	}

	if(p->id != 0) {
		temp = gui_hwpool_list_find(&(GuiHwMemObj.malloc), p->id, (void*)p, p->size);
		if(temp == NULL ) {
			gxlogd("id = %d not malloc!!\n",p->id);
			gxlogd("hw free err!!");
			return;
		}

		if((temp->cache & MALLOC_WITH_CACHE) != 0) {
			gui_hwpool_list_move_from_malloc_to_free(temp, GUI_TRUE);
		} else {
			if(temp->obj.usr_p < GuiHwMemObj.info.mmp_address ||
			   temp->obj.usr_p > (GuiHwMemObj.info.mmp_address + GuiHwMemObj.info.size)) {
				GxKFree(GuiHwMemObj.info.gx_av_handle, (uint32_t)(temp->obj.usr_p), temp->obj.size);
			} else {
				GUI_HW_POOL_FREE(temp->obj.usr_p);
			}
			gui_hwpool_list_drop(&(GuiHwMemObj.malloc),temp->obj.id, temp->usr_handle, temp->obj.size);
		}
		GuiHwMemObj.using_size -= p->size + CHECK_NUM;
	}
	_remove_id_from_list(p->id);
	return;
}

