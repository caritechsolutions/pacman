#include "gxtype.h"
#ifdef NO_OS
#include "gxloader.h"
#endif
#include "dvbhal/gxdemux_hal.h"
#include "string.h"
#include "common/gxmalloc.h"
#include "common.h"
#include "gxdlp_api.h"
#include "gxota_msg.h"
#ifndef NO_OS
#include "gxota_api.h"
#include "zlib.h"
#include "ini.h"
#endif
#include "ota_mem.h"
#include "upgrade.h"
#include "progress.h"
#include "parser.h"
#include "gxupdate_api.h"
#include "dsmcc_download.h"

#define UPDATE_SECTION "update"
#define FLASH_SECTION "flash"
#define DPT_TID (0x40)
#define DSI_DII_TID (0x3B)
#define DDB_TID (0x3C)
#define PAT_TID (0x0)
#define MAX_FILTER_NUM (20)
#define MAX_SEC_NUM (256 * 64)
#define SECTION_SIZE (4096*8*4)


#define MAX_GROUP_NUM (8)
#define MAX_MODULE_NUM (0x10)
#define MAX_BIOP (0x10)
#define MAX_DDB_SECTION_SIZE (4066)

#define DMX_FILTER_SIZE (512 * 1024)

enum DSMCC_PARSE_STATUS {
	DSMCC_STOP_PARSE,
	DSMCC_START_PARSE,
	DSMCC_DPT_DATA_PARSE,
	DSMCC_DSI_DATA_PARSE,
	DSMCC_DII_DATA_PARSE,
	DSMCC_DDB_DATA_PARSE,
	DSMCC_SUCCESS_PARSE,
	DSMCC_FAILED_PARSE
};

enum OTA_THREAD_TYPE {
	OTA_DETECT_VERSION_THREAD,
	OTA_UPGRADE_THREAD
};

typedef struct{
	uint8_t *data;
	uint32_t size;
}swap_data;
struct node
{
    uint16_t moduleid,objectKey;
    char     type[10];
    char     name[40];
    uint8_t  *data;
    uint32_t size;
    uint8_t id;
    uint8_t next[0x10];
    uint8_t numnext;
}Node[MAX_BIOP];
typedef struct{
	uint32_t componentId_tag;
	uint32_t component_data_length;
	uint32_t carouselId;
	uint32_t moduleId;
	uint32_t version_major;
	uint32_t version_minor;
	uint32_t objectKey_length;
	uint32_t objectKey_data;
}BIOPObjectLocation;
typedef struct{
	uint32_t profileId_tag;
	uint32_t profile_data_length;
}IOPtaggedProfile;
typedef struct{
	uint32_t type_id_length;
	uint32_t alignment_gap;
	uint32_t taggedProfiles_count;
	IOPtaggedProfile Iop_taggedprofile[MAX_BIOP];
}IOPIOR;
typedef struct{
	uint16_t objectinfo_length;
	uint8_t  serviceContextList_count;
	struct{
		uint32_t context_id;
		uint16_t context_data_length;
	}context_data[MAX_BIOP];
	uint32_t messageBody_length;
	uint16_t bindings_count;
	struct{
		uint8_t namecomponents_count;
		uint8_t id_length;
		uint8_t id_data[100];
		uint8_t kind_length;
		uint8_t kind_data[100];
	}BIOP_Name[MAX_BIOP];
	uint8_t bindingType;
	IOPIOR  IOP_IOR[MAX_BIOP];
	uint16_t objectinfo_length1;
}BIOPDirMessage;
typedef struct{
	uint16_t objectinfo_length;
	uint64_t dsmfilesize;
	uint8_t serviceContextList_count;
	struct{
		uint32_t context_id;
		uint16_t context_data_length;
	}context_data[MAX_BIOP];
	uint32_t messageBody_length;
	uint32_t content_length;
}BIOPFilMessage;
typedef struct{
	uint32_t magic;
	uint8_t  biop_version_major;
	uint8_t  biop_version_minor;
	uint8_t  byte_order;
	uint8_t  message_type;
	uint32_t message_size;
	uint8_t  objectKey_length;
	uint32_t objectKey_data;
	uint32_t objectKind_length;
	uint32_t objectKind_data;

}BIOPMessageHeader;
typedef struct{
	uint16_t id;
	uint16_t use;
	uint16_t association_tag;
	uint8_t selector_length;
	uint16_t selector_type;
	uint32_t transactionId;
	uint32_t timeout;
}BIOPTap;
typedef struct{
	uint32_t componentId_tag;
	uint32_t component_data_length;
	uint32_t taps_count;
	BIOPTap BIOP_Tap;
}DSMConnBinder;
typedef struct{
	uint32_t profileId_tag;
	uint32_t profile_data_length;
	uint32_t profile_data_byte_order;
	uint32_t liteComponents_count;
	BIOPObjectLocation ObjectLocation;
	DSMConnBinder ConnBinder;
}BIOPProfileBody;//循坏

typedef struct{
	uint32_t PrivateDataLen;
	uint32_t version;
	uint32_t transactionId;
	IOPIOR IOP_IOR[MAX_BIOP];
	uint8_t downloadTaps_count;
	uint8_t serviceContextList_count;
	uint16_t userInfoLength;
}DsiInfo;

typedef struct{
	uint32_t downloaderid;
	uint32_t blocksize;
	uint32_t modulenum;
	struct {
		uint32_t offset;
		uint32_t modulesize;
		uint32_t moduleid;
		uint32_t moduleversion;
		uint32_t moduleinfolength;
	}moduleinfo[MAX_MODULE_NUM];
}DiiInfo;
typedef struct{
	int16_t moduleid_major;
	uint32_t size;
	uint8_t  *data;
}Receive_Info;
typedef struct{
	uint32_t moduleid_major;
	uint32_t objectKey_major;
	struct {
		uint32_t moduleid_minor;
		uint32_t objectKey_minor;
		uint8_t  type[20];
		uint32_t type_len;
		uint8_t  name[40];
		uint32_t name_len;
		uint32_t size;
		uint8_t  *data;
	}key_minor[MAX_MODULE_NUM];
}Parse_Info;
typedef struct{
	uint32_t moduletimeout;
	uint32_t blocktimeout;
	uint32_t minblocktime;
	uint8_t  taps_count;
	struct{
		uint16_t id;
		uint16_t use;
		uint16_t association_tag;
		uint8_t  selector_length;
	}tap[MAX_BIOP];
	uint8_t userinfolength;
}BiopModuleInfo;
typedef	struct{
	uint8_t tag;
	uint8_t compression_method;
	uint8_t flags_check;
	uint16_t moduleid;
	uint32_t compression_data; //模块压缩前原始大小
}UserInfo;
typedef struct{
	uint32_t moduleid;
	uint32_t max_modulenum;
	uint32_t modulenum_major; //index
	uint32_t modulenum_minor; //index
	uint32_t parse_size;
}DdbInfo;

typedef struct {
	uint32_t magicnum;
	uint32_t swversion;
	uint32_t hwversion;
	uint32_t mid;
	uint8_t  ota_type;
	uint32_t sn_start;
	uint32_t sn_end;
	uint32_t oui;
	uint32_t operator_id;
}DPTInfo;

#ifndef NO_OS
typedef struct {
	uint8_t dsmcc_parse_status;
	uint16_t data_pid;
	handle_t ota_thread;
	enum OTA_THREAD_TYPE ota_thread_type;
	uint8_t ready_write_flash;
	GxUpdateDetectVersionProgress detect_progress;
}GxOTAParseInfo;
#endif

typedef struct {
	handle_t filter;
	GxDmxFilterParams params;
}DmxFilter;

static unsigned short wRevSectionNum = 0;
static unsigned short wTotalSection = 0;
static int32_t dpt_start(uint32_t dpt_pid);
static int32_t dsi_start(uint32_t dsi_pid);
static int32_t dii_start(uint32_t dii_pid, uint32_t group_id);
static int32_t ddb_start(uint32_t ddb_pid, uint32_t downloaderid, uint32_t moduleid);

static int32_t get_data(uint32_t pid, char read_once);
static DmxFilter dmx_filter[MAX_FILTER_NUM];
int dmxid  = 0;
GxUpdateImageInfo ota_image_info;
#ifndef NO_OS
static GxOTAParseInfo ota_parse = {0};
#endif
static uint8_t* section_buffer = NULL;

static void free_filter(uint32_t pos)
{
	GxDmxStop(dmx_filter[pos].filter);
	dmx_filter[pos].filter = 0;
}

Receive_Info receive_info[MAX_MODULE_NUM];
Parse_Info parse_info[MAX_BIOP];
static uint32_t index_major = 0;
static uint32_t index_keymajor = 0;
static uint32_t index_keyminor = 0;
BIOPProfileBody ProfileBody[MAX_BIOP];//循坏
static int32_t biopprofilebody_parse(uint8_t *section, uint32_t len)
{
	uint8_t* p = section;
	uint32_t i = 0,j = 0, k = 0, size = 0;
	BIOPTap BIOP_Tap2[MAX_BIOP];
	memset(ProfileBody, 0, sizeof(BIOPProfileBody) * MAX_BIOP);
	memset(BIOP_Tap2, 0 ,sizeof(BIOPTap) * MAX_BIOP);
	for(i = 0; size < len; i++)
	{
		ProfileBody[i].profileId_tag = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		ProfileBody[i].profile_data_length = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
		ProfileBody[i].profile_data_byte_order = p[8];
		ProfileBody[i].liteComponents_count = p[9];
		p += 10;
		size += 10;

		ProfileBody[i].ObjectLocation.componentId_tag = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		if(ProfileBody[i].ObjectLocation.componentId_tag != 0x49534f50)
			return 0;

		ProfileBody[i].ObjectLocation.component_data_length = p[4];
		ProfileBody[i].ObjectLocation.carouselId = (p[5] << 24) | (p[6] << 16) | (p[7] << 8) | p[8];
		ProfileBody[i].ObjectLocation.moduleId = (p[9] << 24) | p[10];

		if (index_keymajor >= MAX_BIOP || index_keyminor >= MAX_MODULE_NUM)
			return -1;
		parse_info[index_keymajor].key_minor[index_keyminor].moduleid_minor = ProfileBody[i].ObjectLocation.moduleId;
		ProfileBody[i].ObjectLocation.version_major = p[11];
		ProfileBody[i].ObjectLocation.version_minor = p[12];
		ProfileBody[i].ObjectLocation.objectKey_length = p[13];
		p += 14;
		size += 14;
		for(j = 0; j < ProfileBody[i].ObjectLocation.objectKey_length; j++)
		{
			ProfileBody[i].ObjectLocation.objectKey_data = (ProfileBody[i].ObjectLocation.objectKey_data << 8)  + *p;
			p += 1;
			size += 1;
		}
		parse_info[index_keymajor].key_minor[index_keyminor].objectKey_minor = ProfileBody[i].ObjectLocation.objectKey_data;
		ProfileBody[i].ConnBinder.componentId_tag = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		if(ProfileBody[i].ConnBinder.componentId_tag != 0x49534f40)
			return 0;

		ProfileBody[i].ConnBinder.component_data_length = p[4];
		ProfileBody[i].ConnBinder.taps_count = p[5];
		p += 6;
		size += 6;
		ProfileBody[i].ConnBinder.BIOP_Tap.id = (p[0] << 8) | p[1];
		ProfileBody[i].ConnBinder.BIOP_Tap.use = (p[2] << 8) | p[3];
		ProfileBody[i].ConnBinder.BIOP_Tap.association_tag = (p[4] << 8) | p[5];
		ProfileBody[i].ConnBinder.BIOP_Tap.selector_length = p[6];
		ProfileBody[i].ConnBinder.BIOP_Tap.selector_type = (p[7] << 8) | p[8];
		ProfileBody[i].ConnBinder.BIOP_Tap.transactionId = (p[9] << 24) | (p[10] << 16) | (p[11] << 8) | p[12];
		ProfileBody[i].ConnBinder.BIOP_Tap.timeout = (p[13] << 24) | (p[14] << 16) | (p[15] << 8) | p[16];
		p += 17;
		size += 17;
		for(j = 0; j < ProfileBody[i].ConnBinder.taps_count - 1; j++)
		{
			BIOP_Tap2[j].id = (p[0] << 8) | p[1];
			BIOP_Tap2[j].use = (p[2] << 8) | p[3];
			BIOP_Tap2[j].association_tag = (p[4] << 8) | p[5];
			BIOP_Tap2[j].selector_length = p[6];

			p += (7 + BIOP_Tap2[j].selector_length);
			size += (7 + BIOP_Tap2[j].selector_length);
		}
		for(k = 0; k < ProfileBody[i].liteComponents_count -2; k++)
		{
			p += 6;
			size += 6;
		}
	}
	return size;
}

static int32_t dpt_parse(uint8_t* section, uint32_t len)
{
	uint8_t* p = section;
	DPTInfo dpt;

	if (!p) {
		gxloge("%s,%d err section is NULL\n", __func__, __LINE__);
		return -1;
	}

	if (GxDmxCheckSectionCrc(section) != 0) {
		gxloge("%s,%d section crc error\n", __func__, __LINE__);
		return -1;
	}

	if (len < 37) {
		gxloge("%s,%d err len %d\n", __func__, __LINE__, len);
		return -1;
	}

	p += 8;

	dpt.magicnum = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	if (dpt.magicnum != 0x44505400) {
		gxloge("%s,%d dpt magicnum error %d\n", __func__, __LINE__, dpt.magicnum);
		return -1;
	}

	p += 4;
	dpt.mid = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	dpt.hwversion = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
	dpt.swversion = (p[8] << 24) | (p[9] << 16) | (p[10] << 8) | p[11];

	p += 12;
	dpt.ota_type = p[0];

	p += 1;
	dpt.sn_start = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	dpt.sn_end = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];

	p += 8;
	dpt.oui = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	dpt.operator_id = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];

	ota_image_info.download_type = GXDLP_OTA_DDTEX_DOWNLOAD;
	ota_image_info.mid = dpt.mid;
	ota_image_info.swver = dpt.swversion;
	ota_image_info.hwver = dpt.hwversion;
	ota_image_info.upgrade_mode = dpt.ota_type;
	ota_image_info.sn_start = dpt.sn_start;
	ota_image_info.sn_end = dpt.sn_end;
	ota_image_info.oui = dpt.oui;
	ota_image_info.operator_id = dpt.operator_id;

#ifndef NO_OS
	if (ota_parse.ota_thread_type == OTA_DETECT_VERSION_THREAD) {
		ota_parse.detect_progress.version_was_got = true;
		ota_parse.detect_progress.status = GXUPDATE_GOT_VERSION;
	}
#endif

	return 0;
}
static int32_t dpt_start(uint32_t dpt_pid)
{
	handle_t handle = 0;
	GxDmxFilterParams params = {0};

	params.pid = dpt_pid;
	params.flags = EQ_FLAG | REPEAT_FLAG | CRC_FLAG;
	params.match[0] = DPT_TID;//dsm_cc section header
	params.mask[0] = 0xFF;
	params.match[3] = 0x00;
	params.mask[3] = 0xFF;
	params.match[4] = 0x00;
	params.mask[4] = 0xFF;
	params.match[6] = 0x00;
	params.mask[6] = 0xFF;
	params.match[7] = 0x00;
	params.mask[7] = 0xFF;
	params.depth = 7;
#ifndef NO_OS
	if (ota_parse.ota_thread_type == OTA_DETECT_VERSION_THREAD)
		params.size = 64 * 1024;
	else
		params.size = DMX_FILTER_SIZE;
#else
	params.size = DMX_FILTER_SIZE;
#endif
	handle = GxDmxStart(dmxid, params);
	if(handle != -1) {
		int i;
		for(i = 0; i< MAX_FILTER_NUM; i++)
		{
			if(dmx_filter[i].filter == 0)
			{
				dmx_filter[i].filter = handle;
				memcpy(&dmx_filter[i].params, &params, sizeof(GxDmxFilterParams));
				break;
			}
		}
	}
	else {
		return GXOTA_DATA_ERROR;
	}
	return 0;
}

static int32_t dsi_parse(uint8_t* section, uint32_t len)
{
	uint8_t* p = section;
	static DsiInfo dsi;
	uint32_t i = 0,j = 0, adap_len = 0, size = 0;
	int32_t ret = -1;
	if (len < (8 + 12 + 20) || !p) {
		gxloge("%s,%d err section len\n", __func__, __LINE__);
		return -1;
	}
	adap_len = p[8+9];
	//section header + message header + reserve
	p += (8 + 12 + adap_len + 20 + 2);

	memset(&dsi, 0x0, sizeof(DsiInfo));
	dsi.PrivateDataLen = (p[0] << 8) | p[1];
	p += 2;
	size = 0;
	for(i = 0; size < dsi.PrivateDataLen; i++)
	{
		dsi.IOP_IOR[i].type_id_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		p += dsi.IOP_IOR[i].type_id_length + 4;
		size += (dsi.IOP_IOR[i].type_id_length + 4);
		if(dsi.IOP_IOR[i].type_id_length % 4 != 0)
		{
			p += (dsi.IOP_IOR[i].type_id_length % 4);
			size +=(dsi.IOP_IOR[i].type_id_length % 4);
		}
		dsi.IOP_IOR[i].taggedProfiles_count = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		p += 4;
		size += 4;
		for(j = 0; j < dsi.IOP_IOR[i].taggedProfiles_count; j++)
		{
			dsi.IOP_IOR[i].Iop_taggedprofile[j].profileId_tag = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			dsi.IOP_IOR[i].Iop_taggedprofile[j].profile_data_length = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
			if(dsi.IOP_IOR[i].Iop_taggedprofile[j].profileId_tag == 0x49534f06)
			{
				ret = biopprofilebody_parse(p, dsi.IOP_IOR[i].Iop_taggedprofile[j].profile_data_length);
				if (ret == -1)
					return ret;
				p += ret;
				size += ret;
			}
		}
		dsi.downloadTaps_count = p[0];
		p += 1 + dsi.downloadTaps_count;
		size += 1 + dsi.downloadTaps_count;

		dsi.serviceContextList_count = p[0];
		p += 1 + dsi.serviceContextList_count;
		size += 1 + dsi.serviceContextList_count;

		dsi.userInfoLength = (p[0] << 8) | p[1];
		p += 2 + dsi.userInfoLength;
		size += 2 + dsi.userInfoLength;
	}
	return 0;
}
static int32_t dsi_start(uint32_t dsi_pid)
{
	handle_t handle = 0;
	GxDmxFilterParams params = {0};

	params.pid = dsi_pid;
	params.flags = EQ_FLAG | REPEAT_FLAG | CRC_FLAG;
	params.match[0] = DSI_DII_TID;//dsm_cc section header
	params.mask[0] = 0xFF;
	params.match[8] = 0x11;
	params.mask[8] = 0xFF;
	params.match[9] = 0x03;
	params.mask[9] = 0xFF;
	params.match[10] = 0x10;
	params.mask[10] = 0xFF;
	params.match[11] = 0x06;
	params.mask[11] = 0xFF;
	params.depth = 12;
	params.size = DMX_FILTER_SIZE;
	handle = GxDmxStart(dmxid, params);
	if (handle != -1 || handle != -2) {
		int i;
		for(i = 0; i< MAX_FILTER_NUM; i++)
		{
			if(dmx_filter[i].filter == 0)
			{
				dmx_filter[i].filter = handle;
				memcpy(&dmx_filter[i].params, &params, sizeof(GxDmxFilterParams));
				break;
			}
		}
	}
	else {
		return GXOTA_DATA_ERROR;
	}
	return 0;
}

DiiInfo dii;
UserInfo userinfo[MAX_BIOP];
static int32_t biop_moduleinfo_message(uint8_t *section, uint8_t len,uint16_t moduleid)
{
	uint8_t *p = section;
	uint32_t i = 0, size = 0;
	BiopModuleInfo moduleinfo;
	memset(&moduleinfo, 0, sizeof(BiopModuleInfo));

	moduleinfo.moduletimeout = (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
	moduleinfo.blocktimeout = (p[4]<<24) | (p[5]<<16) | (p[6]<<8) | p[7];
	moduleinfo.minblocktime = (p[8]<<24) | (p[9]<<16) | (p[10]<<8) | p[11];
	moduleinfo.taps_count = p[12];
	p += 13;
	size += 13;

	for(i = 0; i < moduleinfo.taps_count; i++)
	{
		moduleinfo.tap[i].id = (p[0]<<8) | p[1];
		moduleinfo.tap[i].use = (p[2]<<8) | p[3]; //BIOP_OBJECT_USE 0x0017
		moduleinfo.tap[i].association_tag = (p[4]<<8) | p[5];
		moduleinfo.tap[i].selector_length = p[6]; //must set 0x00
		p += 7;
		size += 7;
	}
	moduleinfo.userinfolength = p[0];
	p += 1;
	size += 1;
	for(i = 0; i < moduleinfo.userinfolength && size < len; i++)
	{
		if(userinfo[i].moduleid == 0)
		{
			userinfo[i].tag = p[0];
			if(userinfo[i].tag != 0x09)
				return 0;

			userinfo[i].compression_method = p[1];
			userinfo[i].flags_check = p[2];
			userinfo[i].moduleid = moduleid;
			userinfo[i].compression_data = (p[3]<<24) | (p[4]<<16) | (p[5]<<8) | p[6];
			p += 7;
			size += 7;
		}
	}
	return 0;
}
static int32_t dii_parse(uint8_t* section, uint32_t len)
{
	uint8_t* p = section;
	uint32_t i = 0, size = len, adap_len = 0;
	
	if (len < (8 + 12))
		return -1;

	memset(&dii, 0, sizeof(DiiInfo));
	adap_len = p[8+9];
	//section header + message header
	p += (8 + adap_len + 12);
	size -= (8 + adap_len + 12);

	dii.downloaderid = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	dii.blocksize = (p[4] << 8) | p[5];
	dii.modulenum = (p[18] << 8) | p[19];

	p += 20;
	size -= 20;

	if (dii.modulenum > MAX_MODULE_NUM)
		return -1;
	memset(&userinfo, 0, sizeof(UserInfo) * MAX_BIOP);

	for (i = 0; (i < dii.modulenum) && (size > 0); i++) {
		dii.moduleinfo[i].moduleid = (p[0] << 8) | p[1];
		dii.moduleinfo[i].modulesize = (p[2] << 24) | (p[3] << 16) | (p[4] << 8) | p[5];
		dii.moduleinfo[i].moduleversion = p[6];
		dii.moduleinfo[i].moduleinfolength = p[7];

		biop_moduleinfo_message(p+8, dii.moduleinfo[i].moduleinfolength, dii.moduleinfo[i].moduleid);
		p += (8 + dii.moduleinfo[i].moduleinfolength);
		size -= (8 + dii.moduleinfo[i].moduleinfolength);
	}
	return 0;
}

static int32_t dii_start(uint32_t dii_pid, uint32_t group_id)
{
	handle_t handle = 0;
	GxDmxFilterParams params = {0};

	memset(&params, 0, sizeof(GxDmxFilterParams));
	params.pid = dii_pid;
	params.flags = EQ_FLAG | REPEAT_FLAG | CRC_FLAG;
	params.match[0] = DSI_DII_TID;
	params.mask[0] = 0xFF;
	params.match[8] = 0x11;
	params.mask[8] = 0xFF;
	params.match[9] = 0x03;
	params.mask[9] = 0xFF;
	params.match[10] = 0x10;
	params.mask[10] = 0xFF;
	params.match[11] = 0x02;
	params.mask[11] = 0xFF;
	params.match[12] = (group_id >> 24) & 0xFF;
	params.mask[12] = 0xFF;
	params.match[13] = (group_id >> 16) & 0xFF;
	params.mask[13] = 0xFF;
	params.match[14] = (group_id >> 8) & 0xFF;
	params.mask[14] = 0xFF;
	params.match[15] = (group_id) & 0xFF;
	params.mask[15] = 0xFF;
	params.depth = 16;
	params.size = DMX_FILTER_SIZE;
	handle = GxDmxStart(dmxid, params);
	if (handle != -1 || handle != -2) {
		int i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (dmx_filter[i].filter == 0) {
				dmx_filter[i].filter = handle;
				memcpy(&dmx_filter[i].params, &params, sizeof(GxDmxFilterParams));
				break;
			}
		}
	}
	else {
		return GXOTA_DATA_ERROR;
	}
	return 0;
}

typedef struct{
	int32_t moduleid;
	uint32_t max_sec_num;
	uint32_t sec_num;
	uint8_t sec[MAX_SEC_NUM];
	uint32_t sec_size;
}sec_record;
static sec_record record[MAX_MODULE_NUM];
static DdbInfo ddb;
uint8_t* section_data = NULL;
static int32_t ddb_sec_check(uint8_t* section)
{
	uint8_t* p = section;
	uint32_t block_size = 0;
	uint32_t i = 0, m = 0, moduleid = 0, adap_len = 0, msg_len = 0;
	uint32_t sec_majornum = 0, sec_minjornum, max_sec_num = 0, number = 0;
	uint32_t ddb_section_len = 0;

	ddb_section_len = ((p[1] & 0xf) << 8) | p[2];
	adap_len = p[8+9];
	max_sec_num = p[7];
	msg_len =p[18]<<8 | p[19];
	block_size = msg_len - adap_len - 6;
	p += (8 + adap_len + 12);

	moduleid = (p[0] << 8) | p[1];
	sec_majornum = p[4];
	sec_minjornum = p[5];
//	if (max_sec_num == 1)
//		return 1;//table finish
	number = sec_minjornum + (sec_majornum << 8);

	if (GxDmxCheckSectionCrc(section) != 0)
		return -2;
	if ((msg_len + 4 + 17) != ddb_section_len) {
		gxlogd("DDB section length = %d\n", ddb_section_len);
		gxlogd("DDB Data Header message_len = %d, error len\n", msg_len);
		return -2;
	}

	if (block_size > MAX_DDB_SECTION_SIZE) {
		gxlogd("DDB block data size = %d > %d, error ddb len\n",block_size, MAX_DDB_SECTION_SIZE);
		return -2;
	}

	for (i = 0; i < MAX_MODULE_NUM; i++) {
		if (record[i].moduleid == moduleid) {
			for (m = 0; m < dii.modulenum; m++) {
				if (receive_info[m].moduleid_major == moduleid) {
					if(record[i].sec_size >= receive_info[m].size && record[i].sec[number] == 1)
					{
						gxlogd("last %d,%d,%d,size = %d\n", moduleid, number, record[i].sec_num, record[i].sec_size);
						record[i].max_sec_num = 0;
						record[i].sec_size = 0;
						record[i].moduleid = -1;
						record[i].sec_num = 0;
						memset(record[i].sec, 0, MAX_SEC_NUM);
						return 1;
					}
					if (record[i].sec[number] == 1)
					{
						if (max_sec_num == 0 && number == 0 && (record[i].sec_size < receive_info[m].size))
							return -3; //ts was change
						return -2;//repeat section
					}
					else {
						record[i].sec[number] = 1;
						record[i].sec_num++;
						record[i].sec_size += block_size;
						return 0;
					}
				}
			}
		}
	}

	if (i == MAX_MODULE_NUM) {
		for (i = 0; i < MAX_MODULE_NUM; i++) {
			if (record[i].moduleid == -1) {
				record[i].moduleid = moduleid;
				record[i].max_sec_num = max_sec_num;
				record[i].sec[number] = 1;
				record[i].sec_num++;
				record[i].sec_size += block_size;
				gxlogd("first i = %d, %d,%d,%d, size = %d\n", i, moduleid, sec_minjornum, record[i].max_sec_num, record[i].sec_size);

				return 0;
			}
		}
	}

	return 0;//section finish
}
static int32_t  ddb_parse(uint8_t* section, uint32_t len)
{
	uint8_t *p = section;
	int32_t ret = -1, size = len, adap_len = 0;
	uint32_t msg_len = 0;
	uint32_t modulenum = 0;
	int percent = 0;
	uint32_t ddb_section_len = 0;

	if (len < (8 + 12))
		return -1;
	memset(&ddb, 0, sizeof(DdbInfo));
	while(p < section+len)
	{
		ret = ddb_sec_check(p);
		if (ret == -1)
			return -1;
		else if(ret == 1)
			return 1;
		else if (ret == -3)
			return GXOTA_DATA_ERROR;
		else if (ret == -2) {
			ddb_section_len = ((p[1] & 0xf) << 8) | p[2];
			p = p + ddb_section_len +3;
			if (ddb.parse_size > 0)
				ret = 0;
			continue;
		}

		adap_len = p[8+9];
		msg_len =p[18]<<8 | p[19];
		size = msg_len - adap_len;
		ddb.max_modulenum = p[7];
		p += (8 + adap_len + 12);
		ddb.moduleid = (p[0] << 8) | p[1];

		ddb.modulenum_major = p[4];
		ddb.modulenum_minor = p[5];
		p += 6;
		size -= 6;


		int m;

		if(ret != -1)
		{
			for (m = 0; m < dii.modulenum; m++) {
				if (receive_info[m].moduleid_major == ddb.moduleid) {
					modulenum = ddb.modulenum_minor + ddb.modulenum_major*256;
					ddb.parse_size += size;
					wRevSectionNum = wRevSectionNum + 1;
					if (wTotalSection == 0)
						wTotalSection = 1;
					if(ddb.parse_size != 0 && (wRevSectionNum * 100 % wTotalSection) > 0)
					{
						percent = wRevSectionNum * 100 / wTotalSection / 2;
						percent = percent > 50 ? 50 : percent;
						update_progress_percent(percent);
					}
					if ((modulenum * MAX_DDB_SECTION_SIZE + size) <= receive_info[m].size) {
						if (receive_info[m].data != NULL)
							memcpy(receive_info[m].data + (modulenum * MAX_DDB_SECTION_SIZE), p, size);
						else
							return GXOTA_DATA_ERROR;
					}
				}
			}
		}
		p = p + 4 + size;

	}

	return ret;
}

static int32_t ddb_start(uint32_t ddb_pid, uint32_t downloaderid, uint32_t moduleid)
{
	handle_t handle = 0;
	GxDmxFilterParams params = {0};

	params.pid = ddb_pid;
	params.flags = EQ_FLAG | REPEAT_FLAG | CRC_FLAG;
	params.match[0] = DDB_TID;
	params.mask[0] = 0xFF;
	params.match[3] = moduleid >> 8;
	params.mask[3] = 0xFF;
	params.match[4] = moduleid & 0xff;
	params.mask[4] = 0xFF;
	params.match[8] = 0x11;
	params.mask[8] = 0xFF;
	params.match[9] = 0x03;
	params.mask[9] = 0xFF;
	params.match[10] = 0x10;
	params.mask[10] = 0xFF;
	params.match[11] = 0x03;
	params.mask[11] = 0xFF;
	params.match[12] = (downloaderid >> 24)&0xFF;
	params.mask[12] = 0xFF;
	params.match[13] = (downloaderid >> 16)&0xFF;
	params.mask[13] = 0xFF;
	params.match[14] = (downloaderid >> 8)&0xFF;
	params.mask[14] = 0xFF;
	params.match[15] = (downloaderid)&0xFF;
	params.mask[15] = 0xFF;
	params.depth = 16;
	params.size = DMX_FILTER_SIZE;
	handle = GxDmxStart(dmxid, params);

	if (handle != -1 || handle != -2) {
		uint32_t i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (dmx_filter[i].filter == 0) {
				dmx_filter[i].filter = handle;
				memcpy(&dmx_filter[i].params, &params, sizeof(GxDmxFilterParams));
				break;
			}
		}
	}
	else {
		return GXOTA_DATA_ERROR;
	}

	return 0;
}
static int32_t ddb_biop_message(uint8_t* section)
{
	uint8_t *p = section;
	uint32_t size = 0;
	uint32_t i, j, ret = 0;
	BIOPMessageHeader biopmesssge;
	BIOPDirMessage dirmessage;
	BIOPDirMessage srgmessage;
	BIOPFilMessage filmessage;
	memset(&filmessage, 0, sizeof(BIOPFilMessage));
	memset(&biopmesssge, 0, sizeof(BIOPMessageHeader));
	memset(&dirmessage, 0, sizeof(BIOPDirMessage));


	biopmesssge.magic = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	if(biopmesssge.magic != 0x42494F50)
		return -1;

	biopmesssge.biop_version_major = p[4];
	biopmesssge.biop_version_minor = p[5];
	biopmesssge.byte_order = p[6];
	biopmesssge.message_type = p[7];
	biopmesssge.message_size = (p[8] << 24) | (p[9] << 16) | (p[10] << 8) | p[11];
	biopmesssge.objectKey_length = p[12];
	p += 13;
	size += 13;
	for(i = 0; i < biopmesssge.objectKey_length; i++)
	{
		biopmesssge.objectKey_data = (biopmesssge.objectKey_data << 8)  + *p;
		p += 1;
		size += 1;
	}
	index_keymajor = biopmesssge.objectKey_data;
	if (index_keymajor >= MAX_BIOP || index_keyminor >= MAX_MODULE_NUM)
		return -1;
	parse_info[index_keymajor].moduleid_major = receive_info[index_major].moduleid_major;
	parse_info[index_keymajor].objectKey_major = biopmesssge.objectKey_data;
	
	biopmesssge.objectKind_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	if( biopmesssge.objectKind_length != 0x00000004)
		return -1;

	biopmesssge.objectKind_data = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
	if(biopmesssge.objectKind_data == 0x73726700)//srg
	{
		srgmessage.objectinfo_length = (p[8] << 8) | p[9];
		p += 10 + srgmessage.objectinfo_length;
		size += (10 + srgmessage.objectinfo_length);

		srgmessage.serviceContextList_count = p[0];
		p +=1;
		size += 1;
		for(i = 0; i < srgmessage.serviceContextList_count; i++)
		{
			srgmessage.context_data[i].context_id = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			srgmessage.context_data[i].context_data_length = (p[4] << 8) | p[5];
			p += (6 + srgmessage.context_data[i].context_data_length);
			size += (6 + srgmessage.context_data[i].context_data_length);

		}
		srgmessage.messageBody_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		srgmessage.bindings_count = (p[4] << 8) | p[5];
		p += 6;
		size +=6;
		index_keyminor = 0;
		for(i = 0; i < srgmessage.bindings_count; i++)
		{
			srgmessage.BIOP_Name[i].namecomponents_count = p[0];
			p += 1;
			size += 1;
			for(j = 0; j < srgmessage.BIOP_Name[i].namecomponents_count; j++)
			{
				srgmessage.BIOP_Name[i].id_length = p[0];
				memset(parse_info[index_keymajor].key_minor[index_keyminor].name, 0x00, sizeof(parse_info[index_keymajor].key_minor[index_keyminor].name));
				memcpy(parse_info[index_keymajor].key_minor[index_keyminor].name, p + 1, srgmessage.BIOP_Name[i].id_length);
				parse_info[index_keymajor].key_minor[index_keyminor].name_len = srgmessage.BIOP_Name[i].id_length;
				p += (1 + srgmessage.BIOP_Name[i].id_length);

				srgmessage.BIOP_Name[i].kind_length = p[0];
				memset(parse_info[index_keymajor].key_minor[index_keyminor].type, 0x00, sizeof(parse_info[index_keymajor].key_minor[index_keyminor].type));
				memcpy(parse_info[index_keymajor].key_minor[index_keyminor].type, p + 1, srgmessage.BIOP_Name[i].kind_length);
				parse_info[index_keymajor].key_minor[index_keyminor].type_len = srgmessage.BIOP_Name[i].kind_length;
				p += (1 + srgmessage.BIOP_Name[i].kind_length);

				size +=(2 + srgmessage.BIOP_Name[i].id_length + srgmessage.BIOP_Name[i].kind_length);
			}

			parse_info[index_keymajor].key_minor[index_keyminor].data = receive_info[index_major].data;
			parse_info[index_keymajor].key_minor[index_keyminor].size = receive_info[index_major].size;

			srgmessage.bindingType = p[0];
			p += 1;
			size += 1;

			srgmessage.IOP_IOR[i].type_id_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += srgmessage.IOP_IOR[i].type_id_length + 4;
			size += (srgmessage.IOP_IOR[i].type_id_length + 4);
			if(srgmessage.IOP_IOR[i].type_id_length % 4 != 0)
			{
				p += (4 - srgmessage.IOP_IOR[i].type_id_length % 4);
				size +=(4 - srgmessage.IOP_IOR[i].type_id_length % 4);
			}
			srgmessage.IOP_IOR[i].taggedProfiles_count = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += 4;
			size += 4;
			for(j = 0; j < srgmessage.IOP_IOR[i].taggedProfiles_count; j++)
			{
				srgmessage.IOP_IOR[i].Iop_taggedprofile[j].profileId_tag = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
				srgmessage.IOP_IOR[i].Iop_taggedprofile[j].profile_data_length = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
				if(srgmessage.IOP_IOR[i].Iop_taggedprofile[j].profileId_tag == 0x49534f06)
				{
					ret = biopprofilebody_parse(p, srgmessage.IOP_IOR[i].Iop_taggedprofile[j].profile_data_length);
					if (ret == -1)
						return ret;
					p += ret;
					size += ret;
				}
			}
			srgmessage.objectinfo_length1 = (p[0] << 8) | p[1];
			p += 2 + srgmessage.objectinfo_length1;
			size += (2 + srgmessage.objectinfo_length1);
			index_keyminor = index_keyminor + 1;
		}
			
	}
	else if(biopmesssge.objectKind_data == 0x64697200)//dir
	{
		dirmessage.objectinfo_length = (p[8] << 8) | p[9];
		p += 10 + dirmessage.objectinfo_length;
		size += (10 + dirmessage.objectinfo_length);

		dirmessage.serviceContextList_count = p[0];
		p +=1;
		size += 1;
		for(i = 0; i < dirmessage.serviceContextList_count; i++)
		{
			dirmessage.context_data[i].context_id = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			dirmessage.context_data[i].context_data_length = (p[4] << 8) | p[5];
			p += (6 + dirmessage.context_data[i].context_data_length);
			size += (6 + dirmessage.context_data[i].context_data_length);

		}
		dirmessage.messageBody_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		dirmessage.bindings_count = (p[4] << 8) | p[5];
		p += 6;
		size +=6;
		index_keyminor = 0;
		for(i = 0; i < dirmessage.bindings_count; i++)
		{
			dirmessage.BIOP_Name[i].namecomponents_count = p[0];
			p += 1;
			size += 1;
			for(j = 0; j < dirmessage.BIOP_Name[i].namecomponents_count; j++)
			{
				dirmessage.BIOP_Name[i].id_length = p[0];
				memset(parse_info[index_keymajor].key_minor[index_keyminor].name, 0x00, sizeof(parse_info[index_keymajor].key_minor[index_keyminor].name));
				memcpy(parse_info[index_keymajor].key_minor[index_keyminor].name, p + 1, dirmessage.BIOP_Name[i].id_length);
				parse_info[index_keymajor].key_minor[index_keyminor].name_len = dirmessage.BIOP_Name[i].id_length;
				p += (1 + dirmessage.BIOP_Name[i].id_length);

				dirmessage.BIOP_Name[i].kind_length = p[0];
				memset(parse_info[index_keymajor].key_minor[index_keyminor].type, 0x00, sizeof(parse_info[index_keymajor].key_minor[index_keyminor].type));
				memcpy(parse_info[index_keymajor].key_minor[index_keyminor].type, p + 1, dirmessage.BIOP_Name[i].kind_length);
				parse_info[index_keymajor].key_minor[index_keyminor].type_len = dirmessage.BIOP_Name[i].kind_length;
				p += (1 + dirmessage.BIOP_Name[i].kind_length);

				size +=(2 + dirmessage.BIOP_Name[i].id_length + dirmessage.BIOP_Name[i].kind_length);
			}
			parse_info[index_keymajor].key_minor[index_keyminor].data = receive_info[index_major].data;
			parse_info[index_keymajor].key_minor[index_keyminor].size = receive_info[index_major].size;

			dirmessage.bindingType = p[0];
			p += 1;
			size += 1;

			dirmessage.IOP_IOR[i].type_id_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += dirmessage.IOP_IOR[i].type_id_length + 4;
			size += (dirmessage.IOP_IOR[i].type_id_length + 4);
			if(dirmessage.IOP_IOR[i].type_id_length % 4 != 0)
			{
				p += (dirmessage.IOP_IOR[i].type_id_length % 4);
				size +=(dirmessage.IOP_IOR[i].type_id_length % 4);
			}
			dirmessage.IOP_IOR[i].taggedProfiles_count = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += 4;
			size += 4;
			for(j = 0; j < dirmessage.IOP_IOR[i].taggedProfiles_count; j++)
			{
				dirmessage.IOP_IOR[i].Iop_taggedprofile[j].profileId_tag = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
				dirmessage.IOP_IOR[i].Iop_taggedprofile[j].profile_data_length = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
				if(dirmessage.IOP_IOR[i].Iop_taggedprofile[j].profileId_tag == 0x49534f06)
				{
					ret = biopprofilebody_parse(p, dirmessage.IOP_IOR[i].Iop_taggedprofile[j].profile_data_length);
					if (ret == -1)
						return ret;
					p += ret;
					size += ret;
				}
			}
			dirmessage.objectinfo_length1 = (p[0] << 8) | p[1];
			p += 2 + dirmessage.objectinfo_length1;
			size += (2 + dirmessage.objectinfo_length1);
			index_keyminor = index_keyminor + 1;
		}
	}
	else if(biopmesssge.objectKind_data == 0x66696C00)//fil
	{
		index_keyminor = 0;
		filmessage.objectinfo_length = (p[8] << 16) | p[9];
		p += 10;
		size += 10;

		filmessage.dsmfilesize = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];//8 B只取后4B 以及能够满足16M的flash
		p += (filmessage.objectinfo_length);
		size += (filmessage.objectinfo_length);

		filmessage.serviceContextList_count = p[0];
		p += 1;
		size += 1;
		for(i = 0; i < filmessage.serviceContextList_count; i++)
		{
			filmessage.context_data[i].context_id = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			filmessage.context_data[i].context_data_length = (p[4] << 8) | p[5];
			p += (6 +filmessage.context_data[i].context_data_length);
			size += (6 +filmessage.context_data[i].context_data_length);
		}
		filmessage.messageBody_length = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		filmessage.content_length = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
		p += (8);
		size += (8);
		parse_info[index_keymajor].key_minor[index_keyminor].data = receive_info[index_major].data + size;
		parse_info[index_keymajor].key_minor[index_keyminor].size = receive_info[index_major].size - size;
	}
	return size;
}

static uint32_t get_system_run_time_second(void)
{
#ifdef NO_OS
	return gx_time_get_s();
#else
	GxTime time;

	GxCore_GetLocalTime(&time);

	return time.seconds;
#endif
}

static int dmx_filter_restart(int index)
{
	free_filter(index);
	dmx_filter[index].filter = GxDmxStart(dmxid, dmx_filter[index].params);
	if (dmx_filter[index].filter == -1  || dmx_filter[index].filter == -2) {
		dmx_filter[index].filter = 0;
		return -1;
	}

	return 0;
}

static int32_t get_data(uint32_t pid, char read_once)
{
	int32_t ret = 0;
	static int index = 0;
	uint32_t len = 0;
	uint32_t timeout = 0;
	static uint32_t timeout_count = 0;
	static uint32_t pre_time = 0;

	timeout = GxDLP_Get_DmxTimeout();

	while (1) {
		int32_t i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (dmx_filter[index].filter == 0)
			{
				index = (index+1)%MAX_FILTER_NUM;
				continue;
			}

			if (get_system_run_time_second() != pre_time) {
				pre_time = get_system_run_time_second();
				timeout_count++;
				if(timeout_count == timeout) {
#ifndef NO_OS
					if (ota_parse.ota_thread_type == OTA_UPGRADE_THREAD)
						update_progress_error(GXUPDATE_DMX_FILTER_TIMEOUT);
#endif
					gxloge("get data time out : %d s\n",timeout_count);
					timeout_count = 0;
					return GXOTA_DATA_ERROR;
				}

				if (timeout_count > 1 && ((timeout_count - 1) % 5) == 0) {
					gxlogd("read dmx 5 seconds no data timeout\n");
					if (dmx_filter_restart(index) != 0) {
						return GXOTA_DATA_ERROR;
					}
				}
			}

			memset(section_buffer, 0, SECTION_SIZE);
			ret = GxDmxRead(dmx_filter[index].filter, section_buffer, SECTION_SIZE, &len);
			if (ret == -3) { //信道错误，demux_filter需要重新开启
				gxlogd("ret == -3\n");
				if (dmx_filter_restart(index) != 0) {
					return GXOTA_DATA_ERROR;
				}
				continue;
			}
			if (ret == 0 && len > 0) {
				switch(section_buffer[0]){
					case DPT_TID:
					{
						timeout_count = 0;
						free_filter(index);
						memset(&dmx_filter[index], 0x00, sizeof(DmxFilter));
						index = (index+1)%MAX_FILTER_NUM;

						if (dpt_parse(section_buffer, len) == -1) {
							return GXOTA_DATA_ERROR;
						}

						return 0;
					}
					case DSI_DII_TID:
					{
						timeout_count = 0;
						free_filter(index);
						memset(&dmx_filter[index], 0x00, sizeof(DmxFilter));
						index = (index+1)%MAX_FILTER_NUM;
						if (section_buffer[11] == 0x02) {
							if (dii_parse(section_buffer, len) == -1) {
								return GXOTA_DATA_ERROR;
							}
						} else if (section_buffer[11] == 0x06) {
							if (dsi_parse(section_buffer, len) == -1) {
								return GXOTA_DATA_ERROR;
							}
						} else {
							gxlogd("%s,%d,%d, Error Type Table\n", __func__, __LINE__, section_buffer[0]);
							return GXOTA_DATA_ERROR;
						}

						return 0;
					}
					case DDB_TID:
					{
						ret = ddb_parse(section_buffer, len);
						if (ret == 1) {
							gxlogd("free_ddb\n");
							free_filter(index);
							index = (index+1)%MAX_FILTER_NUM;
							timeout_count = 0;
							return 0;
						}
						else
						{
							if (ret == 0)
								timeout_count = 0;
							index = (index+1)%MAX_FILTER_NUM;
							return ret;
						}
					}
					default:
						index = (index+1)%MAX_FILTER_NUM;
						break;
				}
			}

			index = (index+1)%MAX_FILTER_NUM;
		}

		if (read_once)
			break;
	}
	return -1;
}

static void dsmcc_data_find(int id)    //释放内存，找到文件对应的数据以及大小
{
	int i;
	if(id)
	{
		if(strncmp(Node[id].type, "dir", 3) == 0 || strncmp(Node[id].type, "fil", 3) == 0)
		{
			if(Node[id].data != NULL)
			{
				Node[id].data = NULL;//只需要释放原始数据，指向原始数据的指针置空
			}
		}
		if(strncmp(Node[id].type, "fil", 3) == 0)
		{
			Node[id].data =  Node[Node[id].next[0]].data;
			Node[id].size =  Node[Node[id].next[0]].size;
		}
	}
	else
		return ;
	for(i=0;i<Node[id].numnext;i++)
	{
		dsmcc_data_find(Node[id].next[i]);
	}
	return ;
}
static void dsmcc_data_free(int id)    //释放版本对不上的目录及其子文件和目录的内存
{
	int i, j;
	if(id)
	{
		if(Node[id].data != NULL)
		{
			for(j = 0; j < MAX_MODULE_NUM; j++)
			{
				if(receive_info[j].moduleid_major == Node[id].moduleid && receive_info[j].data != NULL)
				{
					download_memhole_free(receive_info[j].data); //原数据存放的位置，指针没有跳转过
					receive_info[j].data = NULL;
					Node[id].data = NULL;
				}
			}
			if(Node[id].data != NULL)
			{
				Node[id].data = NULL;
			}
		}
		memset(Node[id].type, 0, 10);
		memset(Node[id].name, 0, 30);
		Node[id].size = 0;
	}
	else
		return ;
	for(i=0;i<Node[id].numnext;i++)
	{
		dsmcc_data_free(Node[id].next[i]);
	}
	return ;
}
static int dsmcc_data_process(struct download_data *ddata)
{
	uint16_t num = 0, i, j, fatherid;
	uint16_t n_id[10] = {0};
	int ret = -1;
	Parse_Info A;
	int force_upgrade = GxDLP_Get_ForceUpgradeFLag(); //when force_upgrade == 1, ignore software version

	memset(Node, 0, sizeof(Node));
	for(i = 0; i < MAX_BIOP; i++)
	{
		A = parse_info[i];
		struct node N ;
		memset(&N, 0, sizeof(struct node));
		fatherid = 0;
		for(j=1;j<=num;j++)
		{
			if(Node[j].moduleid == A.moduleid_major && Node[j].objectKey == A.objectKey_major)
			{
				fatherid = j;
				break;
			}
		}
		for(j=0;;j++)
		{
			if((j && A.key_minor[j].moduleid_minor == 0 && A.key_minor[j].objectKey_minor == 0) || num >= MAX_BIOP - 1)
				break;
			N.moduleid = A.key_minor[j].moduleid_minor;
			N.objectKey = A.key_minor[j].objectKey_minor;

			memcpy(N.type, A.key_minor[j].type, A.key_minor[j].type_len);
			memcpy(N.name, A.key_minor[j].name, A.key_minor[j].name_len);
			
			N.size = A.key_minor[j].size;
			N.data = A.key_minor[j].data;
			N.id = num+1;
			Node[fatherid].next[Node[fatherid].numnext++] = N.id;
			Node[++num] = N;
		}
	}

	//此处当根下面存在多个升级的目录时，能选择对应版本升级
	for(i = 0; i < Node[0].numnext; i++)
	{
		n_id[i] = Node[0].next[i];
		if(strncmp(Node[n_id[i]].type,"dir", 3) != 0)
			continue;
		if(memcmp(Node[n_id[i]].name, "hw", 2) != 0)
			continue;
		if(memcmp(Node[n_id[i]].name + 13, "sw", 2) != 0)
			continue;
		if(memcmp(Node[n_id[i]].name + 26, "mid", 3) != 0)
			continue;

		if(image_version_is_ok(&ota_image_info, 1) || force_upgrade)
		{
			ret = GXOTA_DATA_RECEVING;
			dsmcc_data_find(Node[n_id[i]].id);
		} else {
#ifndef NO_OS
			update_progress_error(GXUPDATE_IMAGE_VERSION_ERROR);
#endif
			dsmcc_data_free(Node[n_id[i]].id);

			gxloge("update file version error or update file sw version is older than current STB sw version\n");
			ret = GXOTA_DATA_ERROR;
		}
	}
	for(i = 0; i < MAX_BIOP; i++)
	{
		if(Node[i].data != NULL && strcmp(Node[i].name, GxDLP_Get_UpgradeFileName()) == 0)
		{
			ddata[0].data = Node[i].data;
			ddata[0].len = Node[i].size;
		}
#if 0
		if(strncmp(Node[i].name, "data", 4) == 0)
		{
			for(j = 0; j < Node[i].numnext; j++)
			{
				if(strncmp(Node[Node[i].next[j]].type, "fil", 3) == 0)
				{
					memcpy(ddata[1+j].name, Node[Node[i].next[j]].name, 20);
					ddata[1+j].data = Node[Node[i].next[j]].data;
					ddata[1+j].len  = Node[Node[i].next[j]].size;
				}
			}
		}
#endif
		//Node[?].moduleid 没有1，因此做特殊处理
		if(receive_info[i].data != NULL && receive_info[i].moduleid_major == 1)
		{
			download_memhole_free(receive_info[i].data);
			receive_info[i].data = NULL;
		}
		//释放交换指针的指向
		for(j = 0; j < MAX_MODULE_NUM; j++)
		{
			if(parse_info[i].key_minor[j].data != NULL)
			{
				parse_info[i].key_minor[j].data = NULL;
			}
		}
	}

	if (ddata[0].data == NULL || ddata[0].len == 0) {
		gxloge("has not find file %s, please check your upgrade file name\n", GxDLP_Get_UpgradeFileName());
		ret = GXOTA_DATA_ERROR;
	}

	return ret;

}

static void free_resource(void)
{
	int i = 0;

	for (i = 0; i < MAX_MODULE_NUM; i++) {
		if (receive_info[i].data != NULL) {
			download_memhole_free(receive_info[i].data);
			memset(&receive_info[i], 0x00, sizeof(Receive_Info));
		}
	}

	for (i = 0; i < MAX_FILTER_NUM; i++) {
		if(dmx_filter[i].filter != 0) {
			free_filter(i);
			memset(&dmx_filter[i], 0x00, sizeof(DmxFilter));
		}
	}
}

#ifdef NO_OS
extern int uncompress_section(unsigned char* src_buf, unsigned int src_len, unsigned char *dst_buf, unsigned int *dst_len);
#endif
int dsmcc_download_data(struct download_data *ddata)
{
	uint32_t transactionId[MAX_BIOP] = {0}, pid = 0, n = 0, i = 0;
	int demux_id  = 0;
	int ret = 0;
	enum dmx_input ts_id = 0;

	section_buffer = GxCore_Malloc(SECTION_SIZE);
	if (section_buffer == NULL) {
		gxloge("%s,%d malloc section buffer falied\n", __func__, __LINE__);
		ret = GXOTA_DATA_ERROR;
		goto end_down;
	}
	memset(dmx_filter, 0, sizeof(dmx_filter));
	if ((pid = GxDLP_Get_DmxPid()) < 0)
		pid = 0x07d3;  //default value

	if ((demux_id = GxDLP_Get_DmxId()) < 0)
		demux_id = 0; //default value

	ts_id = GxDLP_Get_DmxSource();
	if (ts_id == -1)
		ts_id = 0;  //default value

	gxlogd("pid : 0x%x\n", pid);
	gxlogd("demux_id : 0x%x\n", demux_id);
	gxlogd("ts_id : 0x%x\n", ts_id);
	gxlogd("demux timeout filter : 0x%x\n", GxDLP_Get_DmxTimeout());

#ifdef ECOS_OS
	ota_parse.ota_thread_type = OTA_UPGRADE_THREAD;
#endif

	GxDmxInit();
	ret = GxDmxSetSource(demux_id, ts_id);
	dmxid = demux_id;
	if (ret != 0) {
		update_progress_error(GXUPDATE_DMX_CONFIG_ERROR);
		gxloge("config dmx error!!!\n");
		ret = GXOTA_DATA_ERROR;
		goto end_down;
	}

	update_progress_status(GXUPDATE_DOWNLOAD_DATA);

	if (dpt_start(pid) == 0) {
		ret = get_data(pid, 0);
		if (ret == 0){
			if (!image_version_is_ok(&ota_image_info, 1) && !GxDLP_Get_ForceUpgradeFLag()) {
				gxloge("update file version error or update file sw version is older than current STB sw version\n");
				ret = GXOTA_DATA_ERROR;
				goto end_down;
			}
		}
		else if(ret == GXOTA_DATA_ERROR) {
			gxloge("get dpt data error!!!\n");
			ret = GXOTA_DATA_ERROR;
			goto end_down;
		}
	}
	else {
		gxloge("dpt start failed!!! please check pid is right, current pid set is: %d\n", pid);
		ret = GXOTA_DATA_ERROR;
		goto end_down;
	}

	//dsi
	if (dsi_start(pid) == 0) {
		ret = get_data(pid, 0);
		if (ret == 0){
			for (i = 0; i < MAX_BIOP; i++) {
				if(ProfileBody[i].ConnBinder.BIOP_Tap.transactionId != 0)
				{
					gxlogd("will start dii!!!!\n");
					transactionId[n] = ProfileBody[i].ConnBinder.BIOP_Tap.transactionId;
					n++;
				}
			}
		}
		else if(ret == GXOTA_DATA_ERROR) {
			gxloge("get dsi data error!!!\n");
			ret = GXOTA_DATA_ERROR;
			goto end_down;
		}
	}
	else {
		gxloge("dsi start failed!!! please check pid is right, current pid set is: %d\n", pid);
		ret = GXOTA_DATA_ERROR;
		goto end_down;
	}

	for(i = 0; i < MAX_MODULE_NUM; i++)
	{
		receive_info[i].moduleid_major = -1;
		receive_info[i].size = 0;
		receive_info[i].data = NULL;
	}
	if (n > 0) {
		for (i = 0; i < MAX_BIOP; i++) {
			if (transactionId[i] != 0) {
				if (dii_start(pid, transactionId[i]) == 0) {
					ret = get_data(pid, 0);
					if (ret == 0) { //dii
						int32_t m = 0, size_tmp = 0, total_size = 0;
						for (m = 0; m < dii.modulenum; m++) {
							if (dii.moduleinfo[m].modulesize != 0) {
								if(ddb_start(pid, dii.downloaderid, dii.moduleinfo[m].moduleid) != 0) {
									gxloge("ddb start failed!!!\n");
									ret = GXOTA_DATA_ERROR;
									goto end_down;
								}

								total_size += dii.moduleinfo[m].modulesize;

								receive_info[m].size = dii.moduleinfo[m].modulesize;
								receive_info[m].moduleid_major = dii.moduleinfo[m].moduleid;

								receive_info[m].data = download_memhole_malloc(receive_info[m].size + 1);
								if (receive_info[m].data == NULL) {
									gxloge("malloc receive_info data falied!!!\n");
									ret = GXOTA_DATA_ERROR;
									goto end_down;
								}
								memset(receive_info[m].data, 0x0, receive_info[m].size + 1);
							}
						}
						if(0 == wTotalSection) {
							wTotalSection = total_size / MAX_DDB_SECTION_SIZE;
							if (total_size % MAX_DDB_SECTION_SIZE != 0)
								wTotalSection += 1;

							gxlogd("\n  wTotalSection=%d\n",wTotalSection );
						}

						int32_t i = 0;
						//ddb need to check repeat section
						for (i = 0; i < MAX_MODULE_NUM; i++) {
							record[i].moduleid = -1;
							record[i].max_sec_num = 0;
							record[i].sec_num = 0;
							memset(record[i].sec, 0, MAX_SEC_NUM);
							record[i].sec_size = 0;
						}

						size_tmp = 0;
						gxlogd("total_size = %d\n", total_size);
						while(size_tmp < total_size) {
							ret = get_data(pid, 0); //ddb

							size_tmp += ddb.parse_size;

							if(ret == GXOTA_DATA_ERROR) {
								gxloge("get ddb data error\n");
								ret = GXOTA_DATA_ERROR;
								goto end_down;
							}
						}

						for (i = 0; i < MAX_FILTER_NUM; i++) {
							if(dmx_filter[i].filter != 0) {
								free_filter(i);
							}
						}
						swap_data file_data[MAX_MODULE_NUM];
						for (m = 0; m < dii.modulenum; m++) {
							if (userinfo[m].tag == 0x09) {
								file_data[m].data = download_memhole_malloc(userinfo[m].compression_data);
								if(file_data[m].data == NULL) {
									gxloge("malloc file data falied!!!\n");
									ret = GXOTA_DATA_ERROR;
									goto end_down;
								}

								memset(file_data[m].data, 0x0, userinfo[m].compression_data);
								file_data[m].size = userinfo[m].compression_data; //zlib解压缩需要传入存放解压缩数据buf的长度,此长度必须大于等于解压缩后的数据长度
#ifdef NO_OS
								extern int uncompress_section(unsigned char* src_buf, unsigned int src_len,unsigned char *dst_buf, unsigned int *dst_len);

								ret = uncompress_section(receive_info[m].data, receive_info[m].size, file_data[m].data, &file_data[m].size);
#else
								uncompress(file_data[m].data, (unsigned long*)&file_data[m].size, receive_info[m].data, receive_info[m].size);
#endif
								if (ret != 0) {
									gxloge("uncompress module %d error\n", m);
									ret = GXOTA_DATA_ERROR;
									goto end_down;
								}

								if(receive_info[m].data != NULL)
								{
									download_memhole_free(receive_info[m].data);
									receive_info[m].data = file_data[m].data;
									receive_info[m].size = file_data[m].size;
								}
							}

							index_major = m;
							uint8_t *pdata = NULL;
							pdata = receive_info[m].data;
							while(1)
							{
								uint32_t ret_size = 0;
								ret_size = ddb_biop_message(pdata);
								if(ret_size == -1)
								{
									break;
								}
								pdata = pdata + ret_size;
							}
						}
						update_progress_status(GXUPDATE_DOWNLOAD_DATA_OK);
						ret = dsmcc_data_process(ddata);
						if(ret == GXOTA_DATA_ERROR)
							goto end_down;

						ret = GXOTA_DATA_FINISH;
						goto end_down;
					}
					else if(ret == GXOTA_DATA_ERROR) {
						ret = GXOTA_DATA_ERROR;
						goto end_down;
					}
				}
				else {
					ret = GXOTA_DATA_ERROR;
					goto end_down;
				}
			}
		}
	}
	else {
		ret = GXOTA_DATA_ERROR;
		goto end_down;
	}

end_down:
	if (section_buffer != NULL) {
		GxCore_Free(section_buffer);
		section_buffer = NULL;
	}
	if (ret == GXOTA_DATA_ERROR)
		free_resource();
	return ret;
}
#ifndef NO_OS
static handle_t dsmcc_ota_lock;
static void GxOTA_Parse_Dsmcc(void);
static GxOTAUpgradePackageInfo ota_upgrade_package_info;

bool GxOTA_isTrigger(void)
{
	return image_version_is_ok(&ota_image_info, 0);
}

int GxOTA_GetDetectVersionProgress(GxUpdateDetectVersionProgress *progress)
{
	if (progress == NULL)
		return -2;
	memcpy(progress, &ota_parse.detect_progress, sizeof(GxUpdateDetectVersionProgress));
	progress->version_is_ok = GxOTA_isTrigger();

	return 0;
}

int GxOTA_Get_UpgradePackageInfo(GxOTAUpgradePackageInfo *info)
{
	memcpy(info, &ota_upgrade_package_info, sizeof(GxOTAUpgradePackageInfo));
	gxlogd("is_upgrade_all : %d\n", info->is_upgrade_all);
	gxlogd("partition_name : %s\n", info->part[0].partition_name);
	gxlogd("fstype : %d\n", info->part[0].fstype);
	gxlogd("pinfo->part[i].size : %d\n", info->part[0].size);
	gxlogd("partition_name : %s\n", info->part[1].partition_name);
	gxlogd("fstype : %d\n", info->part[1].fstype);
	gxlogd("pinfo->part[i].size : %d\n", info->part[1].size);
	gxlogd("total_upgrade_size : %d\n", info->total_upgrade_size);
	gxlogd("total_size : %d\n", info->total_size);
	gxlogd("upgrade_part_num : %d\n", info->upgrade_part_num);

	return 0;
}

static int ota_set_upgrade_packge_info (struct upgrade_data *udata, struct download_data *ddata, uint32_t upgrade_part_num)
{
	GxOTAUpgradePackageInfo *pinfo = &ota_upgrade_package_info;
	int i = 0;

	if (ddata == NULL || udata == NULL) {
		gxloge("data is NULL\n");
		return -1;
	}

	memset(pinfo, 0x00, sizeof(GxOTAUpgradePackageInfo));

	pinfo->upgrade_part_num = upgrade_part_num;
	pinfo->total_size = ddata->len;
	for (i = 0; i < upgrade_part_num; i++) {
		pinfo->total_upgrade_size += udata[i].size;
		if (udata[i].partition == NULL)
			pinfo->part[i].fstype = RAW;
		else
			pinfo->part[i].fstype = udata[i].partition->file_system_type;
		pinfo->part[i].size = udata[i].size;
		memcpy(pinfo->part[i].partition_name, udata[i].name, FLASH_PARTITION_NAME_SIZE);
	}
	if (upgrade_part_num == 1 && strcmp(udata[0].name, "ALL") == 0)
		pinfo->is_upgrade_all = 1;
	pinfo->hwver = ota_image_info.hwver;
	pinfo->swver = ota_image_info.swver;
	pinfo->mid = ota_image_info.mid;

	return 0;
}


int GxOTA_Get_OTA_SW_Version(uint32_t *sw)
{
	int ret = 0;

	*sw = ota_image_info.swver;

	return ret;
}

int GxOTA_Get_OTA_HW_Version(uint32_t *hw)
{
	int ret = 0;

	*hw = ota_image_info.hwver;

	return ret;
}

int GxOTA_Get_OTA_Mid(uint32_t *mid)
{
	int ret = 0;
	*mid = ota_image_info.mid;
	return ret;
}

int GxOTA_GetSNRange(GxOTASNRange *range)
{
	if (range == NULL) {
		gxloge("param range == NULL\n");
		return -1;
	}

	range->start = ota_image_info.sn_start;
	range->end = ota_image_info.sn_end;

	return 0;
}

int GxOTA_GetType(GxOTAType *type)
{
	if (type == NULL) {
		gxloge("param type == NULL\n");
		return -1;
	}

	*type = ota_image_info.upgrade_mode;

	return 0;
}

void GxOTA_Start_Write_Flash(void)
{
	GxCore_MutexLock(dsmcc_ota_lock);
	ota_parse.ready_write_flash = 1;
	GxCore_MutexUnlock(dsmcc_ota_lock);
}

static void GxOTA_Detect_Version_Thread(void* arg)
{
	GxCore_ThreadDetach();
	while(1) {
		GxCore_MutexLock(dsmcc_ota_lock);
		GxOTA_Parse_Dsmcc();
		if (ota_parse.dsmcc_parse_status == DSMCC_STOP_PARSE) {
			gxlogd("OTA Detect thread Stop\n");
			free_resource();
			ota_parse.ota_thread = 0;
			if (section_buffer != NULL) {
				GxCore_Free(section_buffer);
				section_buffer = NULL;
			}
			GxCore_MutexUnlock(dsmcc_ota_lock);
			return;
		}
		GxCore_MutexUnlock(dsmcc_ota_lock);
		GxCore_ThreadDelay(1000);
	}
}

int GxOTA_Start_Detect_Version(uint32_t dmx_id, uint32_t dmx_source, uint16_t pid)
{
	int ret = 0;
	if (dsmcc_ota_lock == 0)
		GxCore_MutexCreate(&dsmcc_ota_lock);
	GxCore_MutexLock(dsmcc_ota_lock);

	if (ota_parse.dsmcc_parse_status == DSMCC_STOP_PARSE) {
		GxDmxInit();
		GxDmxSetSource(dmx_id, dmx_source);

		dmxid = dmx_id;
		ota_parse.data_pid = pid;
		ota_parse.dsmcc_parse_status = DSMCC_START_PARSE;

		if (ota_parse.ota_thread == 0) {
			update_progress_status(GXUPDATE_IDLE);
			memset(&ota_image_info, 0x00, sizeof(GxUpdateImageInfo));
			memset(&ota_parse.detect_progress, 0x00, sizeof(GxUpdateDetectVersionProgress));
			ret = GxCore_ThreadCreate("dsmcc detect version", &ota_parse.ota_thread, GxOTA_Detect_Version_Thread, NULL,100*1024,GXOS_DEFAULT_PRIORITY);
			if (ret != 0) {
				gxloge("OTA Detect thread create failed!\n");
				ota_parse.dsmcc_parse_status = DSMCC_STOP_PARSE;
			}
			ota_parse.ota_thread_type = OTA_DETECT_VERSION_THREAD;
		}
	}

	GxCore_MutexUnlock(dsmcc_ota_lock);

	return ret;
}

int GxOTA_Stop_Detect_Version(void)
{
	GxCore_MutexLock(dsmcc_ota_lock);
	if (ota_parse.dsmcc_parse_status != DSMCC_STOP_PARSE)
		ota_parse.dsmcc_parse_status = DSMCC_STOP_PARSE;
	GxCore_MutexUnlock(dsmcc_ota_lock);

	return 0;
}

static void GxOTA_Upgrade_Thread(void* arg)
{
	struct download_data ddata;
	struct upgrade_data  udata[MAX_FILE];
	int num = 0;

	memset(&ddata, 0x00, sizeof(struct download_data));

	GxCore_ThreadDetach();
	update_progress_status(GXUPDATE_DOWNLOAD_DATA);
	while(1) {
		GxCore_MutexLock(dsmcc_ota_lock);
		GxOTA_Parse_Dsmcc();
		GxCore_MutexUnlock(dsmcc_ota_lock);

		if (ota_parse.dsmcc_parse_status != DSMCC_SUCCESS_PARSE) {
			if (ota_parse.dsmcc_parse_status == DSMCC_FAILED_PARSE
				|| ota_parse.dsmcc_parse_status == DSMCC_STOP_PARSE) {
				goto err;
			}
			GxCore_ThreadDelay(10);
			continue;
		}

		ota_parse.ready_write_flash = 0;

		update_progress_status(GXUPDATE_PARSE);
		memset(&ddata, 0x00, sizeof(struct download_data));
		if (dsmcc_data_process(&ddata) == GXOTA_DATA_ERROR) {
			goto err;
		}

		num = parse_data(&ddata, udata, MAX_FILE);
		if (0 == num) {
			update_progress_error(GXUPDATE_IMAGE_NO_DATA_ERROR);
			GxCore_ThreadDelay(10);
			goto err;
		}

		ota_set_upgrade_packge_info(udata, &ddata, num);
		update_progress_status(GXUPDATE_PARSE_OK);

		while (ota_parse.ready_write_flash == 0) {
			if (ota_parse.dsmcc_parse_status == DSMCC_STOP_PARSE
				|| ota_parse.dsmcc_parse_status == DSMCC_START_PARSE) {
				goto err;
			}
			GxCore_ThreadDelay(10);
		}

		ota_parse.ready_write_flash = 0;

		update_progress_percent(50);
		GxCore_ThreadDelay(100);

		if (update_data(udata, num) != 0)
			goto err;

		free_resource();
		update_progress_percent(100);
		GxCore_ThreadDelay(10);
		update_progress_status(GXUPDATE_SUCCESS);
		GxDLP_Close();
		while (ota_parse.dsmcc_parse_status != DSMCC_STOP_PARSE
			&& ota_parse.dsmcc_parse_status != DSMCC_START_PARSE) {
			GxCore_ThreadDelay(10);
		}
		update_progress_clean_msg();
		GxCore_MutexLock(dsmcc_ota_lock);
		if (section_buffer != NULL) {
			GxCore_Free(section_buffer);
			section_buffer = NULL;
		}
		ota_parse.dsmcc_parse_status = DSMCC_STOP_PARSE;
		ota_parse.ota_thread = 0;
		GxCore_MutexUnlock(dsmcc_ota_lock);
		break;
err:
		free_resource();
		update_progress_error(GXUPDATE_NO_ERROR); //just want to send GXOTA_STATUS_UPDATE_FAILED
		while (ota_parse.dsmcc_parse_status != DSMCC_STOP_PARSE
			&& ota_parse.dsmcc_parse_status != DSMCC_START_PARSE) {
			GxCore_ThreadDelay(10);
		}
		GxCore_MutexLock(dsmcc_ota_lock);
		update_progress_clean_msg();
		if (ota_parse.dsmcc_parse_status == DSMCC_START_PARSE) {
			update_progress_init(GXDLP_OTA_DDTEX_DOWNLOAD, UPDATE_SEND_PROGRESS_MSG, NULL);
			GxCore_MutexUnlock(dsmcc_ota_lock);
			continue;
		}
		if (section_buffer != NULL) {
			GxCore_Free(section_buffer);
			section_buffer = NULL;
		}
		ota_parse.ota_thread = 0;
		GxCore_MutexUnlock(dsmcc_ota_lock);
		break;
	}
}

int GxOTA_Start_Upgrade(uint32_t dmx_id, uint32_t dmx_source, uint16_t pid)
{
	int ret = 0;
	if (dsmcc_ota_lock == 0)
		GxCore_MutexCreate(&dsmcc_ota_lock);

	GxCore_MutexLock(dsmcc_ota_lock);
	if (ota_parse.dsmcc_parse_status == DSMCC_STOP_PARSE) {
		dmxid = dmx_id;
		ota_parse.data_pid = pid;
		ota_parse.dsmcc_parse_status = DSMCC_START_PARSE;
		GxDmxInit();
		GxDmxSetSource(dmx_id, dmx_source);
		if (ota_parse.ota_thread == 0) {
			memset(&ota_image_info, 0x00, sizeof(GxUpdateImageInfo));
			update_progress_init(GXDLP_OTA_DDTEX_DOWNLOAD, UPDATE_SEND_PROGRESS_MSG, NULL);
			ret = GxCore_ThreadCreate("ota upgrade", &ota_parse.ota_thread, GxOTA_Upgrade_Thread, NULL,100*1024,GXOS_DEFAULT_PRIORITY);
			if (ret != 0) {
				gxloge("OTA Detect thread create failed!\n");
				ota_parse.dsmcc_parse_status = DSMCC_STOP_PARSE;
				ota_parse.ota_thread = 0;
				GxCore_MutexUnlock(dsmcc_ota_lock);
				return -1;
			}
			ota_parse.ota_thread_type = OTA_UPGRADE_THREAD;
		}
	}
	GxCore_MutexUnlock(dsmcc_ota_lock);

	return ret;
}

int GxOTA_Stop_Upgrade(void)
{
	GxCore_MutexLock(dsmcc_ota_lock);
	if (ota_parse.dsmcc_parse_status != DSMCC_STOP_PARSE)
		ota_parse.dsmcc_parse_status = DSMCC_STOP_PARSE;
	GxCore_MutexUnlock(dsmcc_ota_lock);

	return 0;
}

int GxOTA_Get_Msg(struct GxOTA_Msg *msg)
{
	GxUpdateMsg up_msg;

	if (msg == NULL) {
		gxloge("param msg is NULL\n");
		return -2;
	}

	if (GxUpdate_ReceiveMsg(&up_msg)) {
		gxlogd("get ota msg failed\n");
		return -1;
	}

	if (update_msg_to_ota_msg(&up_msg, msg))
		return -1;

	return 0;
}

void GxOTA_SetTrigger(ota_trigger_info *ota_trigger)
{
#define MAX_NUM_SIZE 10
	char buf[MAX_NUM_SIZE+1] = {0};

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, MAX_NUM_SIZE, "%d", ota_trigger->ota_u.ddtex_upgrade.dmx_pid);
	GxDLP_Set_DmxPid(buf);

	GxDLP_Set_DmxSource(ota_trigger->ota_u.ddtex_upgrade.dmx_source);

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, MAX_NUM_SIZE, "%d", ota_trigger->ota_u.ddtex_upgrade.Frequency);
	GxDLP_Set_FeFrequency(buf);

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, MAX_NUM_SIZE, "%d", ota_trigger->ota_u.ddtex_upgrade.Symbolrate);
	GxDLP_Set_FeSymbolrate(buf);

	GxDLP_Set_FePolarity(ota_trigger->ota_u.ddtex_upgrade.Polarity);
	GxDLP_Set_FeSat22k(ota_trigger->ota_u.ddtex_upgrade.Sat22k);
	GxDLP_Set_FeDiseqc10_Port(ota_trigger->ota_u.ddtex_upgrade.Diseqc10_Port);
	GxDLP_Set_FeDiseqc11_Port(ota_trigger->ota_u.ddtex_upgrade.Diseqc11_Port);

	GxDLP_Set_FeModulation(ota_trigger->ota_u.ddtex_upgrade.Modulation);

	GxDLP_Set_FeBandwidth(ota_trigger->ota_u.ddtex_upgrade.bandwidth);

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, MAX_NUM_SIZE, "%d", ota_trigger->OTA_Repeat_Times);
	GxDLP_Set_OtaRepeatTimes(buf);

	GxDLP_Set_UpdateType(ota_trigger->OTA_Type);

	GxDLP_Sync();
}

static void GxOTA_Parse_Dsmcc(void)
{
	uint32_t transactionId = 0;
	uint32_t i = 0;
	int ret = 0;
	static int32_t size_tmp = 0;
	static int32_t total_size = 0;
	int32_t m = 0;

	if (ota_parse.dsmcc_parse_status == DSMCC_STOP_PARSE)
		return;

	if (ota_parse.dsmcc_parse_status == DSMCC_START_PARSE) {
		size_tmp = 0;
		total_size = 0;
		index_major = 0;
		index_keymajor = 0;
		index_keyminor = 0;
		wRevSectionNum = 0;
		memset(parse_info, 0x00, sizeof(parse_info));
		free_resource();
		if (section_buffer == NULL)
			section_buffer = GxCore_Malloc(SECTION_SIZE);
		if (section_buffer == NULL) {
			gxloge("%s,%d malloc section buffer falied\n", __func__, __LINE__);
			goto dsmcc_parse_reset;
		}
		gxlogd("will start dpt!!!!\n");
		ota_parse.detect_progress.status = GXUPDATE_GETTING_VERSION;
		if (dpt_start(ota_parse.data_pid) != 0)
			goto dsmcc_parse_reset;
		ota_parse.dsmcc_parse_status = DSMCC_DPT_DATA_PARSE;
	}

	ret = get_data(ota_parse.data_pid, 1);

	if (ret == 0) {
		switch (ota_parse.dsmcc_parse_status) {
			case DSMCC_DPT_DATA_PARSE:
				if (image_version_is_ok(&ota_image_info, 0) || GxDLP_Get_ForceUpgradeFLag()) {
					if (ota_parse.ota_thread_type == OTA_DETECT_VERSION_THREAD) {
						extern GxUpdateImageInfo image_info;
						memcpy(&image_info, &ota_image_info, sizeof(GxUpdateImageInfo));
						ota_parse.dsmcc_parse_status = DSMCC_STOP_PARSE;
						free_resource();
						return;
					} else
						image_version_is_ok(&ota_image_info, 1); //print version
				} else {
					if (ota_parse.ota_thread_type == OTA_UPGRADE_THREAD) {
						image_version_is_ok(&ota_image_info, 1); //print version
						update_progress_error(GXUPDATE_IMAGE_VERSION_ERROR);
					}
					goto dsmcc_parse_reset;
				}
				ota_parse.dsmcc_parse_status = DSMCC_DSI_DATA_PARSE;
				gxlogd("will start dsi!!!!\n");
				if (dsi_start(ota_parse.data_pid) != 0)
					goto dsmcc_parse_reset;
				break;
			case DSMCC_DSI_DATA_PARSE:
				for (i = 0; i < MAX_BIOP; i++) {
					if(ProfileBody[i].ConnBinder.BIOP_Tap.transactionId != 0) {
						gxlogd("will start dii!!!!\n");
						transactionId = ProfileBody[i].ConnBinder.BIOP_Tap.transactionId;
						break;
					}
				}

				if (transactionId == 0)
					goto dsmcc_parse_reset;
				if (dii_start(ota_parse.data_pid, transactionId) != 0)
					goto dsmcc_parse_reset;

				ota_parse.dsmcc_parse_status = DSMCC_DII_DATA_PARSE;
				break;

			case DSMCC_DII_DATA_PARSE:
				if (dii.modulenum > 3)
					goto dsmcc_parse_reset;
				for (m = 0; m < dii.modulenum; m++) {
					gxlogd("will start ddb : %d\n", dii.moduleinfo[m].moduleid);
					if(ddb_start(ota_parse.data_pid, dii.downloaderid, dii.moduleinfo[m].moduleid) != 0)
						goto dsmcc_parse_reset;

					total_size += dii.moduleinfo[m].modulesize;

					receive_info[m].size = dii.moduleinfo[m].modulesize;
					receive_info[m].moduleid_major = dii.moduleinfo[m].moduleid;

					receive_info[m].data = download_memhole_malloc(receive_info[m].size + 1);
					if (receive_info[m].data == NULL) {
						gxloge("%s,%d malloc falied\n", __func__, __LINE__);
						goto dsmcc_parse_reset;
					}
					memset(receive_info[m].data, 0x0, receive_info[m].size + 1);
				}

				wTotalSection = total_size / MAX_DDB_SECTION_SIZE;
				if (total_size % MAX_DDB_SECTION_SIZE != 0)
					wTotalSection += 1;
				gxlogd("\n  wTotalSection=%d\n",wTotalSection );

				//ddb need to check repeat section
				for (i = 0; i < MAX_MODULE_NUM; i++) {
					record[i].moduleid = -1;
					record[i].max_sec_num = 0;
					record[i].sec_num = 0;
					memset(record[i].sec, 0, MAX_SEC_NUM);
					record[i].sec_size = 0;
				}

				size_tmp = 0;
				gxlogd("total_size = %d\n", total_size);
				ota_parse.dsmcc_parse_status = DSMCC_DDB_DATA_PARSE;
				break;

			case DSMCC_DDB_DATA_PARSE:
				size_tmp += ddb.parse_size;
				ddb.parse_size = 0;

				if (size_tmp < total_size) {
					break;
				}

				int32_t i = 0;
				for (i = 0; i < MAX_FILTER_NUM; i++) {
					if(dmx_filter[i].filter != 0) {
						free_filter(i);
						memset(&dmx_filter[i], 0x00, sizeof(DmxFilter));
					}
				}
				swap_data file_data[MAX_MODULE_NUM];
				for (m = 0; m < dii.modulenum; m++) {
					if (userinfo[m].tag == 0x09) {
						file_data[m].data = download_memhole_malloc(userinfo[m].compression_data);
						if(file_data[m].data == NULL) {
							gxloge("%s,%d malloc falied\n", __func__, __LINE__);
							goto dsmcc_parse_reset;
						}

						memset(file_data[m].data, 0x0, userinfo[m].compression_data);

						file_data[m].size = userinfo[m].compression_data; //zlib解压缩需要传入存放解压缩数据buf的长度,此长度必须大于等于解压缩后的数据长度

						if (uncompress(file_data[m].data, (unsigned long*)&file_data[m].size, receive_info[m].data, receive_info[m].size) != 0)
						{
							gxloge("uncompress error !!!r\n");
							download_memhole_free(file_data[m].data);

							goto dsmcc_parse_reset;
						}
						if(receive_info[m].data != NULL)
						{
							download_memhole_free(receive_info[m].data);
							receive_info[m].data = file_data[m].data;
							receive_info[m].size = file_data[m].size;
						}
					}

					index_major = m;
					uint8_t *pdata = NULL;
					pdata = receive_info[m].data;
					while(1)
					{
						uint32_t ret_size = 0;
						ret_size = ddb_biop_message(pdata);
						if(ret_size == -1)
						{
							break;
						}
						pdata = pdata + ret_size;
					}
				}

				update_progress_status(GXUPDATE_DOWNLOAD_DATA_OK);
				ota_parse.dsmcc_parse_status = DSMCC_SUCCESS_PARSE;

				break;

			default:
				ota_parse.dsmcc_parse_status = DSMCC_START_PARSE;
				free_resource();
				break;
		}
	} else if (ret == GXOTA_DATA_ERROR) {
		goto dsmcc_parse_reset;
	}

	return;

dsmcc_parse_reset:
	if (ota_parse.ota_thread_type == OTA_DETECT_VERSION_THREAD)
		ota_parse.dsmcc_parse_status = DSMCC_START_PARSE;
	else
		ota_parse.dsmcc_parse_status = DSMCC_FAILED_PARSE;
	free_resource();
}
#endif
