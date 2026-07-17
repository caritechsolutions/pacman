#ifndef APP_BOOK_H
#define APP_BOOK_H
#include "app.h"
#include "app_utility.h"

#define RADIO_REC_BOOK
//#undef RADIO_REC_BOOK

#define APP_BOOK_NUM  8
#define BOOK_PROGRAM_PVR BOOK_TYPE_1

typedef struct
{
	int prog_id;
	time_t duration;
}BookProgStruct;

typedef enum
{
	BOOKTYPE_POWOFF,
	BOOKTYPE_PVR,
	BOOKTYPE_PLAY,
}BookType;

typedef enum
{
    BOOKMODE_ALL = 0,
    BOOKMODE_PLAY,
    BOOKMODE_POWER,
}BookMode;

typedef struct app_book AppBook;
struct app_book
{
	int32_t (*get)(GxBookGet*, BookMode);
	status_t (*creat)(GxBook*);
	status_t (*modify)(GxBook*);
	status_t (*remove)(GxBook*);
	status_t (*invalid_remove)(void);
	status_t (*exec)(GxBook*);
	time_t (*get_sleep_time)(time_t);
    time_t (*get_pvr_time)(time_t, int32_t);
	WeekDay (*time2wday)(time_t);
	WeekDay (*mode2wday)(uint32_t);
	uint32_t (*wday2mode)(WeekDay);
	void (*enable)(void);
	void (*disable)(time_t); //0, forever; 1,ignor sec
};
extern status_t book_popdlg_create(BookDlg *dlg);
extern status_t book_popdlg_recreate(void);
extern void book_popdlg_exec(PopDlgRet ret);
extern AppBook g_AppBook;
int app_booklist_type_set(BookMode type);
int app_booklist_type_get(BookMode *type);
GxBookType app_book_sleep_type_get(void);
#endif
