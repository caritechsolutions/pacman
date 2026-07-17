#ifndef __PVR_FILE_H__
#define __PVR_FILE_H__

#define PVR_SEG_DESC         "#PVR-SEG-DESCRIPTION:"
#define PVR_SEG_ENC_DESC     "#PVR-SEG-ENCRPYT-DESCRIPTION:"
#define PVR_TS_PATH          "#PVR-TS-PATH:"
#define PVR_TS_SUFFIX        "#PVR-TS-SUFFIX:"
#define PVR_TS_APID          "#PVR-TS-AUDIO-PID:"
#define PVR_TS_ACODEC        "#PVR-TS-AUDIO-CODEC:"
#define PVR_TS_VPID          "#PVR-TS-VIDEO-PID:"
#define PVR_TS_VCODEC        "#PVR-TS-VIDEO-CODEC:"
#define PVR_TS_PREFIX        "#PVR-TS-PREFIX:"
#define PVR_TS_ENCRPYT       "#PVR-TS-ENCRPYT:"
#define PVR_TS_DURATION      "#PVR-TS-DURATION:"
#define PVR_TS_FILESIZE      "#PVR-TS-FILESIZE:"
#define PVR_TS_ISFINISH      "#PVR-TS-ISFINISH:"

#define PVR_PVR_DESC         "#PVR-DESCRIPTION:"
#define PVR_EVT_DESC         "#PVR-EVT-DESCRIPTION:"
#define PVR_EVT_CHAPTER      "#PVR-EVT-CHAPTER:"
#define PVR_EVT_CHAPTER_DESC "#PVR-EVT-CHAPTER-DESCRIPTION:"
#define PVR_EVT_START_TIME   "#PVR-EVT-START-TIME:"
#define PVR_EVT_END_TIME     "#PVR-EVT-END-TIME:"
#define PVR_SEG_PATH         "#PVR-SEG-PATH:"
#define PVR_SEG_SUFFIX       "#PVR-SEG-SUFFIX:"
#define PVR_SEG_PREFIX       "#PVR-SEG-PREFIX:"
#define PVR_SEG_XURL         "#PVR-SEG-XURL:"
#define PVR_SEG_DURATION     "#PVR-SEG-DURATION:"
#define PVR_SEG_FILESIZE     "#PVR-SEG-FILESIZE:"
#define PVR_SEG_UNVAILD      "#PVR-SEG-BEFORE-UNVAILD-SEQNO:"

#define PVR_LST_DESC         "#PVR-LST-DESCRIPTION:"
#define PVR_EVT_PATH         "#PVR-EVT-PATH:"
#define PVR_EVT_SUFFIX       "#PVR-EVT-SUFFIX:"
#define PVR_EVT_PREFIX       "#PVR-EVT-PREFIX:"
#define PVR_EVT_XURL         "#PVR-EVT-XURL:"
#define PVR_EVT_DURATION     "#PVR-EVT-DURATION:"
#define PVR_EVT_FILESIZE     "#PVR-EVT-FILESIZE:"
#define PVR_EVT_UNVAILD      "#PVR-EVT-BEFORE-UNVAILD-SEQNO:"

//特殊定义
#define PVR_SEG_BLOCK_ENCRYPT      "block.encrypt"

typedef enum {
	PVRFILE_EXIST = 0,
	PVRFILE_TOTAL,
} PVRFileAttr;

typedef enum {
	PVRFILE_NORMAL  = 0,
	PVRFILE_OVERLAP = 1,
	PVRFILE_APPEND  = 2,
} PVRFileMode;

typedef enum {
	PVRFILE_PVR_FILE = 0,
	PVRFILE_SEG_FILE,
	PVRFILE_EVT_FILE,
	PVRFILE_LST_FILE,
} PVRFileType;

typedef enum {
	PVRFILE_GET_SEQ_BYPREFIX,
	PVRFILE_GET_SEQ_BYTIME,
	PVRFILE_GET_SEQ_BYPOS,
	PVRFILE_GET_SEQ_BYSEQ,
} PVRFileSeqGetCmd;

typedef enum {
	PVRFILE_GET_PATH,
	PVRFILE_GET_DESC,
	PVRFILE_GET_SUFFIX,
	PVRFILE_GET_CHAPTER,
	PVRFILE_GET_CHAPTER_DESC,
	PVRFILE_GET_START_TIME,
	PVRFILE_GET_END_TIME,
} PVRFileArgGetCmd;

typedef struct {
	unsigned char  unvaild;
	int32_t        duration;
	int32_t        filesize;
	char          *key_desc;
	char           prefix[PVR_MAX_STS_PREFIX_LEN];
} TSVariant;

typedef struct {
	unsigned char unvaild;
	int64_t       duration;
	off_t         filesize;
	char          prefix[PVR_MAX_SEG_PREFIX_LEN];
	char         *xurl;
} SegVariant;

typedef struct {
	unsigned char unvaild;
	int64_t       duration;
	off_t         filesize;
	char         *prefix;
	char         *xurl;
	int           is_finish;
} EvtVariant;

typedef struct {
	int            mutex;
	char          *desc;
	char          *enc_desc;
	char          *path;
	char          *suffix;
	unsigned int   apid;
	unsigned int   vpid;
	char          *acodec;
	char          *vcodec;
	unsigned int   ts_c;
	unsigned int   ts_t;
	TSVariant     *var;
	char           is_finish;
} PVRSegFile;

typedef struct {
	handle_t       mutex;
	char          *desc;
	char          *path;
	char          *suffix;
	unsigned int   seg_c;
	unsigned int   seg_t;
	SegVariant    *var;
} PVRPvrFile;

typedef struct {
	handle_t       mutex;
	char          *desc;
	char          *path;
	char          *chapter;
	char          *chapter_desc;
	char          *start_time;
	char          *end_time;
	char          *suffix;
	unsigned int   seg_c;
	unsigned int   seg_t;
	SegVariant    *var;
} PVREvtFile;

typedef struct {
	handle_t       mutex;
	char          *desc;
	char          *path;
	char          *suffix;
	unsigned int   evt_c;
	unsigned int   evt_t;
	EvtVariant    *var;
} PVRLstFile;

extern void* pvr_file_create (PVRFileType type);
extern void  pvr_file_parse  (PVRFileType type, void *priv, char *line);
extern void  pvr_file_destory(PVRFileType type, void *priv);

extern int      pvr_pvrfile_get_url      (PVRPvrFile *pvr, int c, char *buf, unsigned int max_size);
extern char*    pvr_pvrfile_get_arg      (PVRPvrFile *pvr, PVRFileArgGetCmd cmd);
extern int64_t  pvr_pvrfile_get_duration (PVRPvrFile *pvr, int seqno, PVRFileAttr attr);
extern off_t    pvr_pvrfile_get_filesize (PVRPvrFile *pvr, int seqno, PVRFileAttr attr);
extern uint32_t pvr_pvrfile_get_seqcount (PVRPvrFile *pvr);
extern int      pvr_pvrfile_add_timems   (PVRPvrFile *pvr, int64_t timems);
extern int      pvr_pvrfile_add_offset   (PVRPvrFile *pvr, off_t offset);
extern int32_t  pvr_pvrfile_get_seqno    (PVRPvrFile *pvr, PVRFileSeqGetCmd cmd, void *arg);
extern off_t    pvr_pvrfile_get_startpos (PVRPvrFile *pvr, int seqno, PVRFileAttr attr);
extern int64_t  pvr_pvrfile_get_starttime(PVRPvrFile *pvr, int seqno, PVRFileAttr attr);
extern int64_t  pvr_pvrfile_get_mintime  (PVRPvrFile *pvr);

extern int      pvr_segfile_get_url      (PVRSegFile *seg, int c, char *buf, unsigned int max_size);
extern char*    pvr_segfile_get_arg      (PVRSegFile *seg, PVRFileArgGetCmd cmd);
extern int64_t  pvr_segfile_get_duration (PVRSegFile *seg, int seqno, PVRFileAttr attr);
extern off_t    pvr_segfile_get_filesize (PVRSegFile *seg, int seqno, PVRFileAttr attr);
extern uint32_t pvr_segfile_get_seqcount (PVRSegFile *seg);
extern int      pvr_segfile_add_timems   (PVRSegFile *seg, int64_t timems);
extern int      pvr_segfile_add_offset   (PVRSegFile *seg, off_t offset);
extern int32_t  pvr_segfile_get_seqno    (PVRSegFile *seg, PVRFileSeqGetCmd cmd, void *arg);
extern off_t    pvr_segfile_get_startpos (PVRSegFile *seg, int seqno, PVRFileAttr attr);
extern int64_t  pvr_segfile_get_starttime(PVRSegFile *seg, int seqno, PVRFileAttr attr);
extern char    *pvr_segfile_get_keydesc  (PVRSegFile *seg, int seqno);
extern char     pvr_segfile_get_isfinish (PVRSegFile *seg);
extern int64_t  pvr_segfile_get_mintime  (PVRSegFile *seg);

extern int      pvr_evtfile_get_url       (PVREvtFile *evt, int c, char *buf, unsigned int max_size);
extern char*    pvr_evtfile_get_arg       (PVREvtFile *evt, PVRFileArgGetCmd cmd);
extern int64_t  pvr_evtfile_get_duration  (PVREvtFile *evt, int seqno, PVRFileAttr attr);
extern off_t    pvr_evtfile_get_filesize  (PVREvtFile *evt, int seqno, PVRFileAttr attr);
extern uint32_t pvr_evtfile_get_seqcount  (PVREvtFile *evt);
extern int      pvr_evtfile_add_timems    (PVREvtFile *evt, int64_t timems);
extern int      pvr_evtfile_add_offset    (PVREvtFile *evt, off_t offset);
extern int32_t  pvr_evtfile_get_seqno     (PVREvtFile *evt, PVRFileSeqGetCmd cmd, void *arg);
extern off_t    pvr_evtfile_get_startpos  (PVREvtFile *evt, int seqno, PVRFileAttr attr);
extern int64_t  pvr_evtfile_get_starttime (PVREvtFile *evt, int seqno, PVRFileAttr attr);
extern int64_t  pvr_evtfile_get_mintime   (PVREvtFile *evt);

extern int      pvr_lstfile_get_url       (PVRLstFile *lst, int c, char *buf, unsigned int max_size);
extern char*    pvr_lstfile_get_arg       (PVRLstFile *lst, PVRFileArgGetCmd cmd);
extern int64_t  pvr_lstfile_get_duration  (PVRLstFile *lst, int seqno, PVRFileAttr attr);
extern off_t    pvr_lstfile_get_filesize  (PVRLstFile *lst, int seqno, PVRFileAttr attr);
extern uint32_t pvr_lstfile_get_seqcount  (PVRLstFile *lst);
extern int32_t  pvr_lstfile_get_seqno     (PVRLstFile *lst, PVRFileSeqGetCmd cmd, void *arg);
extern off_t    pvr_lstfile_get_startpos  (PVRLstFile *lst, int seqno, PVRFileAttr attr);
extern int64_t  pvr_lstfile_get_starttime (PVRLstFile *lst, int seqno, PVRFileAttr attr);
extern int64_t  pvr_lstfile_get_mintime   (PVRLstFile *lst);
extern int      pvr_lstfile_set_duration_byseqno(PVRLstFile *lst, int seqno, int64_t duration);
extern int      pvr_lstfile_set_filesize_byseqno(PVRLstFile *lst, int seqno, off_t filesize);
extern char     pvr_lstfile_get_isfinish_byseqno(PVRLstFile *lst, int seqno);
extern int      pvr_lstfile_set_isfinish_byseqno(PVRLstFile *lst, int seqno, char is_finish);

#endif
