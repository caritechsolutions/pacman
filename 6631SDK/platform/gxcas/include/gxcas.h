#ifndef __GXCAS_H__
#define __GXCAS_H__
#include "gxtype.h"

#define GXCAS_EVENT     'e'
#define GXCAS_GET       'g'
#define GXCAS_SET       's'
#define GXCAS_QUERY     'q'
#define GXCAS_DEBUG     'd'

#define GXCAS_MASK      (0x1fff)
#define GXCAS_VOID      (0x20000000)
#define GXCAS_READ      (0x40000000)
#define GXCAS_WRITE     (0x80000000)
#define GXCAS_WR        (GXCAS_READ | GXCAS_WRITE)

#define GXCAS_IOC(input, group, num, len) \
	((unsigned long)(input | ((len & GXCAS_MASK) << 16) | (((group) << 8) | (num))))
#define GXCAS_GETINPUT(x) ((x) & 0xff000000)
#define GXCAS_GETGROUP(x) (((x) >> 8) & 0xff)
#define GXCAS_GETNUM(x)   ((x) & 0xff)

#define GXCAS_IO(g, n)        GXCAS_IOC(GXCAS_VOID, (g), n, 0)
#define GXCAS_IOR(g, n, t)    GXCAS_IOC(GXCAS_READ, (g), n, sizeof(t))
#define GXCAS_IOW(g, n, t)    GXCAS_IOC(GXCAS_WRITE, (g), n, sizeof(t))
#define GXCAS_IOWR(g, n, t)   GXCAS_IOC(GXCAS_WR, (g), n, sizeof(t))

#define GXCAS_DEBUG_MOUDULE_CAS         GXCAS_IO(GXCAS_DEBUG, 1 << 0)
#define GXCAS_DEBUG_MOUDULE_DEMUX       GXCAS_IO(GXCAS_DEBUG, 1 << 1)
#define GXCAS_DEBUG_MOUDULE_DESC        GXCAS_IO(GXCAS_DEBUG, 1 << 2)
#define GXCAS_DEBUG_MOUDULE_SMC         GXCAS_IO(GXCAS_DEBUG, 1 << 3)
#define GXCAS_DEBUG_MOUDULE_FLASH       GXCAS_IO(GXCAS_DEBUG, 1 << 4)
#define GXCAS_DEBUG_MOUDULE_SI          GXCAS_IO(GXCAS_DEBUG, 1 << 5)
#define GXCAS_DEBUG_MOUDULE_CASLIB      GXCAS_IO(GXCAS_DEBUG, 1 << 6)
#define GXCAS_DEBUG_MOUDULE_EADLIB      GXCAS_IO(GXCAS_DEBUG, 1 << 7)

typedef enum {
	GXCAS_SMC_LOW_LEVEL,
	GXCAS_SMC_HIGH_LEVEL
}GxCasSmcPol;

typedef struct{
	uint16_t pmt_pid;
	uint16_t service_id;
	uint16_t audio_pid;
	uint16_t video_pid;
}GxCasSet_SwitchChannel;

typedef struct{
	uint32_t service_handle;
	bool f_on;
}GxCasSet_SwitchMonitor;

typedef struct{
	uint16_t ecm_pid;
	uint16_t pid;
}GxCas_Service;

typedef struct{
	uint16_t audio_ecm_pid;
	uint16_t video_ecm_pid;
	uint16_t service_id;
}GxCasEvent_EcmPid;

typedef struct{
	uint16_t pmt_pid;
	uint16_t service_id;
	GxCas_Service audio;//audio.ecm_pid 0 is invalid
	GxCas_Service video;//video.ecm_pid 0 is invalid
}GxCasSet_QuickSwitchChannel;

typedef int32_t (*gxcas_minifs_rebuild_func)(void);
typedef struct{
	char name[128];
	uint32_t size;
	uint32_t offset;
	gxcas_minifs_rebuild_func func;//when minifs cas partition damage, rebuild minifs cas partition.
}GxCas_Flash;

typedef struct{
	uint32_t sci_switch; // 1:on 0:off
	GxCasSmcPol vcc_pole;
	GxCasSmcPol detect_pole;
}GxCas_SmartCard;

typedef struct{
	uint32_t dmx_id;
	uint32_t ts_id;
	GxCas_SmartCard sci;
	GxCas_Flash flash;
	GxCas_Flash backup;
}GxCasInitParam;

int32_t GxCas_WaitEvent(unsigned long *type, void **value);
int32_t GxCas_PostEvent(unsigned long type, void *value, unsigned long len);
int32_t GxCas_Set(unsigned long type, void *value);
int32_t GxCas_Get(unsigned long type, void *value);
int32_t GxCas_Enable_Debug(unsigned long type);
int32_t GxCas_Disable_Debug(unsigned long type);
#endif
