/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	gxbook.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.12.04	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include "gxcore.h"
#include "gxmsg.h"
#include "gxbus.h"
#include "service/gxbook.h"
#include "minidb.hpp"
using namespace GoXceed;

#define GXBOOK_DEBUG    (0)
/* Private types/constants ------------------------------------------------ */
#define GxBook_gxlogd(...)      gxlogd( __VA_ARGS__ )
#define BOOK_FILE_NAME    "/home/gx/book.db"

#define BOOK_ID_BUSY		(1)
#define BOOK_ID_FREE		(0)
#define BOOK_ID_EMPTY	(0xffffffff)

// 以秒为单位
#define BOOK_PRECISION	(2)

#define SECOND_PER_DAY	(60*60*24)

typedef enum
{
	BOOK_WAIT_EVENT,
	BOOK_DO_EVENT,
	BOOK_FINISH_EVENT
}BookStatus;

#define CHECK_POINT(point, ret_err)				do{\
							if (point == NULL){\
								gxlogd("null point in %s %d\n",__FUNCTION__, __LINE__);\
								return ret_err;}\
								}while(0)


using namespace GoXceed;;
/**置一则标识该下标所对应的id号为空，可以分配*/
static uint8_t  s_BookId[MAX_BOOK_NUM];
static GxBook s_BookGetBuffer[MAX_BOOK_NUM];
static uint32_t s_BookPos = 0;
static IDB * sp_BookDb = NULL;

/***/
#define BOOK_STOP    (1)
#define BOOK_START    (0)
static uint32_t s_BookStopFlag = BOOK_START;

/* mutex handle */
handle_t s_BookMutexHandle = 0;
/* Private Functions ----------------------------------------------------- */
/**
 * @brief 		book数据库初始化
 * @param
 * @return     	GXCORE_SUCCESS   执行正常
 			GXCORE_ERROR   执行失败
 */
static status_t book_db_get(GxBook *pbook);

void GxBookSetStopFlag(uint32_t flag)
{
    s_BookStopFlag = flag;
}

static status_t book_db_init(IDB *pdb)
{
	ICursor *cursor =  pdb->CreateCursor();
	assert(cursor != NULL);
	while(!cursor->Eof()) {
		uint32_t size = 0;
		GxBook *book = NULL;

		book = (GxBook*)cursor->GetRaw(&size);
		if (book == NULL)
			break;

		s_BookId[book->id] = BOOK_ID_BUSY;
		if (book->pos >= s_BookPos)
			s_BookPos = book->pos + 1;
		GxCore_Free(book);
		cursor->Next();
	}
	delete cursor;
	return GXCORE_SUCCESS;
}

/**
 * @brief 		打开数据库文件.
 * @param
 * @return     	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
static status_t book_db_open(const char * db_path, IDB ** ppdb)
{
	sp_BookDb = IDB::New();
	assert(sp_BookDb != NULL);
	if (sp_BookDb->Open(db_path) != true) {
		GxBook_gxlogd("[book]   open  book db err !\n");
		return GXCORE_ERROR;
	}

	return book_db_init(sp_BookDb);
}

static uint32_t book_db_alloc_id(void)
{
	int32_t i;

	for (i=0; i<MAX_BOOK_NUM; i++)
	{
		if (s_BookId[i] == BOOK_ID_FREE)
		{
			s_BookId[i] = BOOK_ID_BUSY;
			return i;
		}
	}

	return BOOK_ID_EMPTY;
}

static void book_db_free_id(uint32_t book_id)
{
	s_BookId[book_id] = BOOK_ID_FREE;
}

// should do it before add, modify
static void book_db_mark_stamp(GxBook *pbook)
{
	pbook->time_stamp = 0;
}

static void book_db_close(void)
{
	if(sp_BookDb != NULL)
	{
		sp_BookDb->Close();
		sp_BookDb->Delete(sp_BookDb);
		sp_BookDb = NULL;
	}
    return;
}

/*static status_t book_db_mark_stamp(GxBook *pbook)
{
#define SECOND_PER_DAY	(60*60*24)

	CHECK_POINT(pbook, GXCORE_ERROR);

	uint32_t second_within_day = pbook->trigger_time_start%SECOND_PER_DAY;

	// set book precision to BOOK_PRECISION
	second_within_day -= second_within_day%BOOK_PRECISION;
	pbook->time_stamp = second_within_day;

	return GXCORE_SUCCESS;
}

static status_t book_db_update_stamp(GxBook *pbook)
{
	int32_t ret = 0;
	DB *pdb = sp_BookDb;
	DBT data, key;

	CHECK_POINT(pbook, GXCORE_ERROR);

	if (s_BookId[pbook->id] != BOOK_ID_BUSY)
	{
		gxlogd("[book]  modify data err!\n");
	}

	memset(&data,0,sizeof(DBT));
	memset(&key,0,sizeof(DBT));

	key.data = &(pbook->id);
	key.size = sizeof(uint32_t);
	data.data = pbook;
	data.size = sizeof(GxBook);

	ret = pdb->put(pdb, &key, &data, 0);
	if(ret != 0)
	{
		GxBook_gxlogd("[book]    modify book err !\n");
		pdb->sync(pdb,0);

		return GXCORE_ERROR;
	}

	pdb->sync(pdb,0);

	return GXCORE_SUCCESS;
}*/

static void book_send_trigger_msg(GxBook * pbook)
{
	GxMessage   *new_msg = NULL;
	GxMsgProperty_BookTrigger *trigger = NULL;

	if (pbook == NULL)  return;

	new_msg = GxBus_MessageNew(GXMSG_BOOK_TRIGGER);
	trigger = GxBus_GetMsgPropertyPtr(new_msg,
	                                  			GxMsgProperty_BookTrigger);

	memcpy(trigger, pbook, sizeof(GxMsgProperty_BookTrigger));

	GxBus_MessageSend(new_msg);
}

static void book_send_finish_msg(GxBook * pbook)
{
	GxMessage   *new_msg = NULL;
	GxMsgProperty_BookFinish *finish = NULL;

	if (pbook == NULL)  return;

	new_msg = GxBus_MessageNew(GXMSG_BOOK_FINISH);
	finish = GxBus_GetMsgPropertyPtr(new_msg,
	                                  			GxMsgProperty_BookFinish);

	memcpy(finish, pbook, sizeof(GxMsgProperty_BookFinish));

	GxBus_MessageSend(new_msg);
}

#if 0
static status_t book_db_add(GxBook *book)
{
	int32_t ret = 0;
	DB *pdb = sp_BookDb;
	DBT data, key;

	book_db_mark_stamp();

	memset(&data,0,sizeof(DBT));
	memset(&key,0,sizeof(DBT));

	book->id = book_db_alloc_id();
	key.data = &(book->id);
	key.size = sizeof(uint32_t);
	data.data = book;
	data.size = sizeof(GxBook);

	ret = pdb->put(pdb, &key, &data,0);
	if(ret != 0)
	{
		GxBook_gxlogd("[book]    add book err !\n");
		pdb->sync(pdb,0);

		return GXCORE_ERROR;
	}

	s_BookId[book->id] = BOOK_ID_BUSY;

	pdb->sync(pdb,0);

	return GXCORE_SUCCESS;
}

static status_t book_db_delete(GxBook *book)
{
	int32_t ret = 0;
	DB *pdb = sp_BookDb;
	DBT data, key;

	memset(&key,0,sizeof(DBT));
	key.data = &(book->id);
	key.size = sizeof(uint32_t);

	ret = pdb->del(pdb,&key,0);
	if(ret != 0)
	{
		GxBook_gxlogd("[book]   del book err !\n");
		pdb->sync(pdb,0);
		return GXCORE_ERROR;
	}

	book_db_free_id(book->id);

	return GXCORE_SUCCESS;
}

static status_t book_db_modify(GxBook *book)
{
	int32_t ret = 0;
	DB *pdb = sp_BookDb;
	DBT data, key;

	if (s_BookId[book->id] != BOOK_ID_BUSY)
	{
		gxlogd("[book]  modify data err!\n");
	}

	book_db_mark_stamp();

	memset(&data,0,sizeof(DBT));
	memset(&key,0,sizeof(DBT));

	key.data = &(book->id);
	key.size = sizeof(uint32_t);
	data.data = book;
	data.size = sizeof(GxBook);

	ret = pdb->put(pdb, &key, &data,0);
	if(ret != 0)
	{
		GxBook_gxlogd("[book]    modify book err !\n");
		pdb->sync(pdb,0);

		return GXCORE_ERROR;
	}

	pdb->sync(pdb,0);

	return GXCORE_SUCCESS;
}
#endif
static status_t book_db_get(GxBook *pbook)
{
	uint32_t size = 0;
	IDB *pdb = sp_BookDb;
	GxBook *book_tmp = NULL;

	CHECK_POINT(pbook, GXCORE_ERROR);

	book_tmp = (GxBook *)pdb->Get(pbook->id, &size);
	if (book_tmp == NULL) {
		GxBook_gxlogd("[book]   get book err !\n");
		return GXCORE_ERROR;
	}

	memcpy(pbook, book_tmp, sizeof(GxBook));
	GxCore_Free(book_tmp);

	return GXCORE_SUCCESS;
}

/**
  * @brief  获取当前时间为星期几
  * @param  当前时间的总秒数
  * @return  返回是星期几，0~6 指星期天，星期一...星期六
  */
static uint32_t book_tm_get_wday(time_t time)
{
	struct tm *ptm;

	ptm = localtime(&time);

	return ptm->tm_wday;
}

static status_t book_create(GxMsgProperty_BookCreate book_create)
{
	IDB *pdb = sp_BookDb;

	CHECK_POINT(book_create, GXCORE_ERROR);

	if ((book_create->book_type !=BOOK_PROGRAM_PLAY
			&& book_create->book_type !=BOOK_POWER_ON
			&& book_create->book_type !=BOOK_POWER_OFF
			&& book_create->book_type !=BOOK_TYPE_1
			&& book_create->book_type !=BOOK_TYPE_2
			&& book_create->book_type !=BOOK_TYPE_3)
			|| book_create->trigger_mode > BOOK_TRIG_BY_SEGMENT
			|| book_create->repeat_mode.mode > BOOK_REPEAT_ONCE)
	{
		book_create->id = -1;
		return GXCORE_ERROR;
	}

	book_db_mark_stamp(book_create);

	book_create->id = book_db_alloc_id();
	book_create->pos = s_BookPos++; // s_BookPos keep the largest postion

	if (book_create->id == (int32_t)BOOK_ID_EMPTY)
	{
		GxBook_gxlogd("[book]    book full !\n");
		return GXCORE_ERROR;
	}

	if ((book_create->struct_size == 0) || (book_create->struct_size > MAX_BOOK_STRUCT_LEN))
	{
		book_db_free_id(book_create->id);
		return GXCORE_ERROR;
	}

	if(book_create->trigger_time_advance < 0 || book_create->trigger_time_advance >= SECOND_PER_DAY)
		book_create->trigger_time_advance = 0;

	if (pdb->Set(book_create->id, book_create, sizeof(GxBook)) != true)
	{
		GxBook_gxlogd("[book]    add book err !\n");
		pdb->Sync();

		return GXCORE_ERROR;
	}

	s_BookId[book_create->id] = BOOK_ID_BUSY;

	pdb->Sync();

	return GXCORE_SUCCESS;
}

status_t book_destroy(GxMsgProperty_BookDestroy book_destroy)
{
	IDB *pdb = sp_BookDb;

	CHECK_POINT(book_destroy, GXCORE_ERROR);

	if (pdb->Remove(book_destroy->id) != true)
	{
		GxBook_gxlogd("[book]   del book err !\n");
		pdb->Sync();
		return GXCORE_ERROR;
	}

	book_db_free_id(book_destroy->id);
	gxlogd("sp_BookDb begin============ %p\n", sp_BookDb);
	pdb->Sync();

	gxlogd("sp_BookDb end============ %p\n", sp_BookDb);
	return GXCORE_SUCCESS;
}

static status_t book_modify(GxMsgProperty_BookModify book_modify)
{
	IDB *pdb = sp_BookDb;
//	uint8_t * struct_addr;

	CHECK_POINT(book_modify, GXCORE_ERROR);

	if (s_BookId[book_modify->id] != BOOK_ID_BUSY)
	{
		gxlogd("[book]  modify data err!\n");

		return GXCORE_ERROR;
	}

	//book_db_mark_stamp(book_modify);


	if (pdb->Set(book_modify->id, book_modify, sizeof(GxBook)) != true)
	{
		GxBook_gxlogd("[book]    modify book err !\n");
		pdb->Sync();

		return GXCORE_ERROR;
	}

	pdb->Sync();

	return GXCORE_SUCCESS;
}

int book_sort(const void *a, const void *b)
{
	return (((GxBook*)a)->pos > ((GxBook*)b)->pos) ? 1:-1;
}

static status_t book_get(GxMsgProperty_BookGet book_get)
{
	IDB * pdb = sp_BookDb;
	ICursor *cursor = NULL;

	CHECK_POINT(book_get, GXCORE_ERROR);

	book_get->book_number = 0;

	//mwInit();
	cursor = pdb->CreateCursor();
	assert(cursor != NULL);
	memset(s_BookGetBuffer, '0', sizeof(GxBook) * MAX_BOOK_NUM);
	while(!cursor->Eof()) {
		uint32_t size = 0;
		GxBook *book = NULL;

		book = (GxBook*)cursor->GetRaw(&size);
		if (book->book_type & book_get->book_type) {
			memcpy(s_BookGetBuffer + book_get->book_number, book, sizeof(GxBook));
			book_get->book_number++;
			GxCore_Free(book);
		}
		cursor->Next();
	}

	book_get->book = s_BookGetBuffer;
	delete cursor;
	if(book_get->book_number != 0)
		qsort(book_get->book, book_get->book_number, sizeof(GxBook), book_sort);
	return GXCORE_SUCCESS;
}

static status_t book_reset(void)
{
	memset(s_BookId, 0 , MAX_BOOK_NUM);
	s_BookPos = 0;
	if(sp_BookDb != NULL)
	{
		sp_BookDb->Close();
		sp_BookDb->Delete(sp_BookDb);
		sp_BookDb = NULL;
	}
	if (GxCore_FileDelete(BOOK_FILE_NAME) == 0)
	{

		// if no file exist , create the file to store the book
		book_db_open(BOOK_FILE_NAME, &sp_BookDb);

		return GXCORE_SUCCESS;
	}

	return GXCORE_ERROR;
}

static BookStatus book_console_get_status(time_t local_time, GxBook *pbook)
{
	BookStatus book_status = BOOK_FINISH_EVENT;
	GxBook book = {0};
	static int32_t day = 0; //for bug 145477
	time_t temp_local_time = local_time;

	CHECK_POINT(pbook, BOOK_WAIT_EVENT);

	memcpy(&book, pbook, sizeof(GxBook));

	// TODO: TRIG_ING problem

	if ((book.repeat_mode.mode & BOOK_REPEAT_ONCE) != BOOK_REPEAT_ONCE)
	{
		// 周期性开始的book，先统一日期再作比较
		book.trigger_time_start = book.trigger_time_start%SECOND_PER_DAY;

		if(book.trigger_time_end % SECOND_PER_DAY)
		{
			book.trigger_time_end = book.trigger_time_end%SECOND_PER_DAY;
			local_time = local_time%SECOND_PER_DAY;
		}
		else
		{
			book.trigger_time_end = SECOND_PER_DAY; //00:00:00->24:00:00
			if(SECOND_PER_DAY <= (local_time%SECOND_PER_DAY + 1) || (local_time%SECOND_PER_DAY == 0))
			{
				local_time = SECOND_PER_DAY;
			}
			else
			{
				local_time = local_time%SECOND_PER_DAY;
			}
		}
	}

	if(book.trigger_mode == BOOK_TRIG_BY_POINT)
	{
		// set book precision to BOOK_PRECISION
		book.trigger_time_start -= (book.trigger_time_start%BOOK_PRECISION);

		if (local_time < book.trigger_time_start)
		{
			book_status = BOOK_WAIT_EVENT;
		}
		else if (local_time == book.trigger_time_start)
		{
			book_status = BOOK_DO_EVENT;
		}
		else
		{
			book_status = BOOK_FINISH_EVENT;
		}
	}
	else if(book.trigger_mode == BOOK_TRIG_BY_SEGMENT)
	{
		// set book precision to BOOK_PRECISION
		book.trigger_time_start -= (book.trigger_time_start%BOOK_PRECISION);
		book.trigger_time_end -= (book.trigger_time_end%BOOK_PRECISION);

		if (local_time < book.trigger_time_start)
		{
			book_status = BOOK_WAIT_EVENT;
		}
		else
		{
			if (local_time < book.trigger_time_end
					|| local_time - book.trigger_time_advance < book.trigger_time_end)
			{
				book_status = BOOK_DO_EVENT;
			}
			else
			{
				book_status = BOOK_FINISH_EVENT;
			}
		}

		if(BOOK_DO_EVENT == book_status)
			day = temp_local_time / SECOND_PER_DAY;
	}
	else if(book.trigger_mode == BOOK_TRIG_ING)
	{//shenbin 100825
		// set book precision to BOOK_PRECISION
		book.trigger_time_start -= (book.trigger_time_start%BOOK_PRECISION);
		book.trigger_time_end -= (book.trigger_time_end%BOOK_PRECISION);

		if (local_time < book.trigger_time_start
				|| day != (int32_t)(temp_local_time / SECOND_PER_DAY))
		{
			// TODO: another round , clear ing status. But time deviation may trigger more than once, so delete temporaty
			pbook->trigger_mode = BOOK_TRIG_BY_SEGMENT;
			book_modify(pbook);

			book_status = BOOK_WAIT_EVENT;
		}
		else
		{
			if (local_time < book.trigger_time_end
					|| local_time - book.trigger_time_advance < book.trigger_time_end)
			{
				book_status = BOOK_WAIT_EVENT;
			}
			else
			{
				book_status = BOOK_FINISH_EVENT;
			}
		}
	}

	return book_status;
}

static void book_console_task(time_t local_time, uint32_t wday, GxBook *pbook)
{
	BookStatus book_status;

	// get current book status
	book_status = book_console_get_status(local_time, pbook);

	if (book_status == BOOK_WAIT_EVENT)
	{
		// wait do not need do anything & trigger status don't care
		return;
	}

	//2 deal with once mode
	if ((pbook->repeat_mode.mode & BOOK_REPEAT_ONCE) == BOOK_REPEAT_ONCE)
	{
		if (book_status == BOOK_DO_EVENT)
		{
			book_send_trigger_msg(pbook);

			// support segment trigger twice shenbin 100825
			if (pbook->trigger_mode == BOOK_TRIG_BY_SEGMENT)
			{
				pbook->trigger_mode = BOOK_TRIG_ING;
			}
			else
			{
				pbook->trigger_mode = BOOK_TRIG_OFF;
			}

			book_modify(pbook);
		}
		else if (book_status == BOOK_FINISH_EVENT)
		{
			book_send_finish_msg(pbook);

			// once finish, change the trigger mode to off, app can deal it later
			pbook->trigger_mode = BOOK_TRIG_OFF;
			book_modify(pbook);
		}
	}

	//2 deal with repeat mode
	if ((pbook->repeat_mode.mode & (1<<wday)) == (uint32_t)(1<<wday))
	{
		#define MAX_DURATION_TIME	(20*60*60)

		if (book_status == BOOK_DO_EVENT)
		{
			book_send_trigger_msg(pbook);

			// support segment trigger twice shenbin 100825
			if (pbook->trigger_mode == BOOK_TRIG_BY_SEGMENT)
			{
				pbook->trigger_mode = BOOK_TRIG_ING;
                // need send finish msg
				pbook->time_stamp = 1;

                book_modify(pbook);
			}
    	}
		else if (book_status == BOOK_FINISH_EVENT)
		{
			// if already finish, needn't send finsh msg again
			if (pbook->time_stamp == 1)
			{
				book_send_finish_msg(pbook);
			}

			// support segment trigger twice shenbin 100825
			if (pbook->trigger_mode == BOOK_TRIG_ING)
			{
				pbook->trigger_mode = BOOK_TRIG_BY_SEGMENT;
                // needn't send finish msg
				pbook->time_stamp = 0;

				book_modify(pbook);
			}
		}

	}
}

/* Exported Functions ----------------------------------------------------- */
static status_t  GxBookServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxCore_MutexCreate(&s_BookMutexHandle);

	// if no file exist , create the file to store the book
	if (GXCORE_ERROR == book_db_open(BOOK_FILE_NAME, &sp_BookDb))
	{
		book_reset();
	}

	GxBus_MessageRegister(GXMSG_BOOK_TRIGGER, sizeof(GxMsgProperty_BookTrigger));
	GxBus_MessageRegister(GXMSG_BOOK_FINISH, sizeof(GxMsgProperty_BookFinish));
	GxBus_MessageRegister(GXMSG_BOOK_GET, sizeof(GxMsgProperty_BookGet));
	GxBus_MessageRegister(GXMSG_BOOK_CREATE, sizeof(GxMsgProperty_BookCreate));
	GxBus_MessageRegister(GXMSG_BOOK_MODIFY, sizeof(GxMsgProperty_BookModify));
	GxBus_MessageRegister(GXMSG_BOOK_DESTROY, sizeof(GxMsgProperty_BookDestroy));
	GxBus_MessageRegister(GXMSG_BOOK_START, 0);
	GxBus_MessageRegister(GXMSG_BOOK_STOP, 0);
	GxBus_MessageRegister(GXMSG_BOOK_RESET, 0);

	GxBus_MessageListen(self, GXMSG_BOOK_GET);
	GxBus_MessageListen(self, GXMSG_BOOK_CREATE);
	GxBus_MessageListen(self, GXMSG_BOOK_MODIFY);
	GxBus_MessageListen(self, GXMSG_BOOK_DESTROY);
	GxBus_MessageListen(self, GXMSG_BOOK_START);
	GxBus_MessageListen(self, GXMSG_BOOK_STOP);
	GxBus_MessageListen(self, GXMSG_BOOK_RESET);

	sch = GxBus_SchedulerCreate("BookMsgScheduler", GXBUS_SCHED_MSG, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("BookConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	return GXCORE_SUCCESS;
}

static void GxBookServiceDestroy(handle_t self)
{
	GxCore_MutexDelete(s_BookMutexHandle);

	GxBus_MessageUnListen(self, GXMSG_BOOK_GET);
	GxBus_MessageUnListen(self, GXMSG_BOOK_TRIGGER);
	GxBus_MessageUnListen(self, GXMSG_BOOK_FINISH);
	GxBus_MessageUnListen(self, GXMSG_BOOK_CREATE);
	GxBus_MessageUnListen(self, GXMSG_BOOK_MODIFY);
	GxBus_MessageUnListen(self, GXMSG_BOOK_DESTROY);
	GxBus_MessageUnListen(self, GXMSG_BOOK_START);
	GxBus_MessageUnListen(self, GXMSG_BOOK_STOP);
	GxBus_MessageUnListen(self, GXMSG_BOOK_RESET);


	GxBus_MessageUnregister(GXMSG_BOOK_FINISH);
	GxBus_MessageUnregister(GXMSG_BOOK_GET);
	GxBus_MessageUnregister(GXMSG_BOOK_CREATE);
	GxBus_MessageUnregister(GXMSG_BOOK_MODIFY);
	GxBus_MessageUnregister(GXMSG_BOOK_DESTROY);
	GxBus_MessageUnregister(GXMSG_BOOK_START);
	GxBus_MessageUnregister(GXMSG_BOOK_STOP);
	GxBus_MessageUnregister(GXMSG_BOOK_RESET);
    book_db_close();
	GxBus_ServiceUnlink(self);
	return;
}


static GxMsgStatus GxBookServiceRecvMsg(handle_t self, GxMessage* Msg)
{
    GxCore_MutexLock(s_BookMutexHandle);

    switch(Msg->msg_id)
	{
		case GXMSG_BOOK_GET:
			book_get(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_BookGet));
			break;

		case GXMSG_BOOK_CREATE:
			book_create(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_BookCreate));
			break;

		case GXMSG_BOOK_MODIFY:
			book_modify(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_BookModify));
			break;

		case GXMSG_BOOK_DESTROY:
			book_destroy(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_BookDestroy));
			break;

		case GXMSG_BOOK_START:
			s_BookStopFlag = BOOK_START;
			break;

		case GXMSG_BOOK_STOP:
			s_BookStopFlag = BOOK_STOP;
			break;

		case GXMSG_BOOK_RESET:
			book_reset();
			break;

		default:
			break;
	}

	GxCore_MutexUnlock(s_BookMutexHandle);

	return GXMSG_OK;
}

static void GxBookServiceConsole(handle_t self)
{
	GxBook book;
	GxTime local_time;
	uint32_t week_day;
	time_t seconds;
	uint32_t i;

	if (s_BookStopFlag == BOOK_STOP) {
        GxCore_ThreadDelay(500);
		return;
	}

	// get local time
	GxCore_GetLocalTime(&local_time);

	GxCore_MutexLock(s_BookMutexHandle);

	for (i=0; i<MAX_BOOK_NUM; i++)
	{
		if (s_BookId[i] == BOOK_ID_BUSY)
		{
			book.id = i;
			book_db_get(&book);
			if (book.trigger_mode != BOOK_TRIG_OFF)
			{
				seconds = (time_t)(local_time.seconds) + book.trigger_time_advance;

				// set book precision to BOOK_PRECISION
				seconds -= (seconds % BOOK_PRECISION);

				// get week
				week_day = book_tm_get_wday(seconds);

				book_console_task(seconds, week_day, &book);
			}
		}
	}
	GxCore_MutexUnlock(s_BookMutexHandle);
	GxCore_ThreadDelay(300);
}


GxServiceClass book_service = {
	"book service",
	GxBookServiceInit,
	GxBookServiceDestroy,
	GxBookServiceRecvMsg,
	GxBookServiceConsole,
	0,
};

/* End of file -------------------------------------------------------------*/

