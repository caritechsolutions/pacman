#ifndef __GX_PVR_H__
#define __GX_PVR_H__

#include "gxplayer_define.h"

typedef enum {
	GX_RECORD_PVR_PVR_FILE = 0x0,
	GX_RECORD_PVR_LST_FILE,
	GX_RECORD_PVR_EVT_FILE,
	GX_RECORD_PVR_SEG_FILE,
} GxRecordPVRFileType;

typedef enum {
	GX_RECORD_PVR_NOT_ENCRYPT = 0x0,
	GX_RECORD_PVR_PVR_ENCRYPT = 0x2,
	GX_RECORD_PVR_DVR_ENCRYPT = 0x3,
} GxRecordPVREncryptMode;

typedef enum {
	//set type
	GX_RECORD_PVR_NEW_SEGMENT  = 0x100,
	GX_RECORD_PVR_DELETE_SEGMENT ,
	GX_RECORD_PVR_OPEN_EVENT ,
	GX_RECORD_PVR_CLOSE_EVENT ,
	GX_RECORD_PVR_DELETE_EVENT ,
	GX_RECORD_PVR_OPEN_LIST ,
	GX_RECORD_PVR_CLOSE_LIST ,
	GX_RECORD_PVR_DELETE_LIST ,
	GX_RECORD_PVR_ADD_SEGMENT_IN_EVENT,
	GX_RECORD_PVR_REMOVE_SEGMENT_IN_EVENT,
	GX_RECORD_PVR_ADD_EVENT_IN_LIST ,
	GX_RECORD_PVR_REMOVE_EVENT_IN_LIST ,
	GX_RECORD_PVR_ADD_EVENT_LIST,
	GX_RECORD_PVR_REMOVE_EVENT_LIST,
	GX_RECORD_PVR_FREE_PVR_INFO,
	GX_RECORD_PVR_SET_KEY_DESC,

	//get type
	GX_RECORD_PVR_GET_DURATION = 0x200,
	GX_RECORD_PVR_GET_FILESIZE,
	GX_RECORD_PVR_GET_CURTIME,
	GX_RECORD_PVR_GET_MINTIME,
	GX_RECORD_PVR_GET_POS_BY_TIME,
	GX_RECORD_PVR_GET_TIME_BY_POS,
	GX_RECORD_PVR_GET_PVR_INFO,
	GX_RECORD_PVR_GET_ENCRYPT_MODE,
	GX_RECORD_PVR_GET_PVR_TYPE,
	GX_RECORD_PVR_GET_HWSAFE,
	GX_RECORD_PVR_GET_KEY_DESC,
} GxRecordPVROption;

typedef struct {
	char *prefix;
	char *desc;
	char *chapter;
	char *chapter_desc;
	char *start_time;
	char *end_time;
} GxRecordPVREventSet;

typedef struct {
	char *prefix;
	char *desc;
} GxRecordPVRListSet;

typedef struct {
	char          *prefix;
	unsigned int   elem_count;
	char         **elem_prefix;
} GxRecordPVRElem;

typedef struct {
	int64_t       timems;
	off_t         offset;
} GxRecordPVRTimePos;

typedef struct {
	char *prefix;
	char *file;
} GxRecordPVRFileElem;

typedef struct {
	const char          *desc;
	unsigned int         event_count;
	GxRecordPVRFileElem *event_elem;
} GxRecordPVRList;

typedef struct {
	const char          *desc;
	const char          *chapter;
	const char          *chapter_desc;
	const char          *start_time;
	const char          *end_time;
	int64_t              filesize;
	int64_t              duration;
	unsigned int         segment_count;
	GxRecordPVRFileElem *segment_elem;
} GxRecordPVREvent;

typedef struct {
	char    *desc;
	char    *prefix;
	int64_t  filesize;
	int64_t  duration;
} GxRecordPVRSegment;

typedef struct {
	unsigned int         list_count;
	GxRecordPVRFileElem *list_elem;
	unsigned int         event_count;
	GxRecordPVRFileElem *event_elem;
	unsigned int         segment_count;
	GxRecordPVRFileElem *segment_elem;
} GxRecordPVRFile;

typedef struct {
	GxRecordPVRFileType    type;
	union {
		GxRecordPVRFile    file;
		GxRecordPVRList    list;
		GxRecordPVREvent   event;
		GxRecordPVRSegment segment;
	};
	const char            *pvr_file;
	const char            *dir;
	const char            *prefix;
	const char            *path;
} GxRecordPVRInfo;

typedef struct {
	GxRecordPVREncryptMode mode;
	unsigned int           blocksize;
} GxRecordPVREncrypt;

typedef struct {
	unsigned char *buf;
	unsigned int   size;
} GxRecordPVRReadData;

typedef struct {
	GxRecordPVROption  opt;
	void              *arg;
} GxRecordPVRControl;

#endif
