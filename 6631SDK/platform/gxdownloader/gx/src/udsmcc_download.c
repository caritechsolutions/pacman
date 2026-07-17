#include "types.h"
#ifdef NO_OS
#include "gxloader.h"
#endif
#include "dvbhal/gxdemux_hal.h"
#include "string.h"
#include "common/gxmalloc.h"
#include "common.h"
#include "gui.h"
#include "gxdlp_api.h"
#include "upgrade.h"

#define UPDATE_SECTION "update"
#define FLASH_SECTION "flash"
#define BLOCK_SIZE (64 * 1024)
#define DSI_DII_TID (0x3B)
#define DDB_TID (0x3C)
#define PAT_TID (0x0)
#define MAX_FILTER_NUM (64)
#define MAX_SEC_NUM (256)
#define SECTION_SIZE (4096*8)


#define MAX_GROUP_NUM (8)
#define MAX_MODULE_NUM (0x10)
#define MAX_DDB_SECTION_SIZE (4066)
typedef struct{
	uint32_t oui;
	uint32_t hwmodule;
	uint32_t hwversion;
	uint32_t swmodule;
	uint32_t swversion;
	uint32_t groupid;
}DsiInfo;

typedef struct{
	uint32_t downloaderid;
	uint32_t blocksize;
	uint32_t modulenum;
	struct {
		uint32_t modulesize;
		uint32_t moduleid;
	}moduleinfo[MAX_MODULE_NUM];
}DiiInfo;

typedef struct{
	uint32_t moduleid;
	uint32_t max_modulenum;
	uint32_t modulenum; //index
	uint32_t parse_size;
}DdbInfo;

int32_t dsi_start(uint32_t dsi_pid);
int32_t dii_start(uint32_t dii_pid, uint32_t groupid);
int32_t ddb_start(uint32_t ddb_pid, uint32_t downloaderid, uint32_t moduleid);

static int32_t get_data(void);
static handle_t filter[MAX_FILTER_NUM] = {0};
int dmxid  = 0;

static void free_filter(uint32_t pos)
{
	GxDmxStop(filter[pos]);
	filter[pos] = 0;
}

static DsiInfo dsi[MAX_GROUP_NUM];
static int32_t dsi_parse(uint8_t* section, uint32_t len)
{
	uint8_t* p = section;
	uint32_t i = 0, group_num = 0, adap_len = 0, size = len;
	
	if (size < (8 + 12 + 20) || !p) {
		gxloge("%s,%d err section len\n", __func__, __LINE__);
		return -1;
	}
	adap_len = p[8+9];
	//section header + message header + reserve
	p += (8 + 12 + adap_len + 20 + 2 + 2);
	size -= (8 + 12 + adap_len + 20 + 2 + 2);
	
	group_num = (p[0] << 8) | p[1];
	group_num > MAX_GROUP_NUM?(group_num = MAX_GROUP_NUM):(group_num = group_num);
	p += 2;
	size -= 2;
	memset(dsi, 0, sizeof(DsiInfo) * MAX_GROUP_NUM);
	for (i = 0; (i < group_num) && (size > 0); i++) {
		uint32_t group_count = 0, j = 0;

		dsi[i].groupid = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		group_count = (p[10] << 16) | p[11];
		//group header
		p += 12;
		size -= 12;
		
		for (j = 0; j < group_count; j++) {
			if (p[0] == 0x01){
				dsi[i].oui = ((p[2] << 24) & 0x00) | (p[3] << 16) | (p[4] << 8) | p[5];//adapter pc tool
				dsi[i].hwmodule = (p[6] << 8) | p[7];
				dsi[i].hwversion = (p[8] << 8) | p[9];
			} else if (p[0] == 0x02) {
				dsi[i].oui = ((p[2] << 24) & 0x00)| (p[3] << 16) | (p[4] << 8) | p[5];
				dsi[i].swmodule = (p[6] << 8) | p[7];
				dsi[i].swversion = (p[8]<< 8) | p[9];
			}
			//group
			p += (2 + p[1]);
			size -= (2 + p[1]);
		}
	}
	
	return 0;
}

int32_t dsi_start(uint32_t dsi_pid)
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
	handle = GxDmxStart(dmxid, params);
	if(handle != 0)
	{
		int i;
		for(i = 0; i< MAX_FILTER_NUM; i++)
		{
			if(filter[i] == 0)
			{
				filter[i] = handle;
				break;
			}
		}
	}
	return 0;
}

DiiInfo dii;

static int32_t dii_parse(uint8_t* section, uint32_t len)
{
	uint8_t* p = section;
	uint32_t i = 0, size = len, adap_len = 0;
	
	if (len < (8 + 12))
		return -1;

	adap_len = p[8+9];
	//section header + message header
	p += (8 + adap_len + 12);
	size -= (8 + adap_len + 12);

	dii.downloaderid = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	dii.blocksize = (p[4] << 8) | p[5];
	dii.modulenum = (p[18] << 8) | p[19];
	
	p += 20;
	size -= 20;

	dii.modulenum > MAX_MODULE_NUM?(dii.modulenum = MAX_MODULE_NUM):(dii.modulenum = dii.modulenum);
	for (i = 0; (i < dii.modulenum) && (size > 0); i++) {
		dii.moduleinfo[i].moduleid = (p[0] << 8) | p[1];
		dii.moduleinfo[i].modulesize = (p[2] << 24) | (p[3] << 16) | (p[4] << 8) | p[5];
		p += 13;
		size -= 13;
	}
	return 0;
}

int32_t dii_start(uint32_t dii_pid, uint32_t group_id)
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
	handle = GxDmxStart(dmxid, params);
	if (handle != -1) {
		int i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (filter[i] == 0) {
				filter[i] = handle;
				break;
			}
		}
	}
	return 0;
}

typedef struct{
	int32_t moduleid;
	uint32_t max_sec_num;
	uint32_t sec_num;
	uint8_t sec[MAX_SEC_NUM];
}sec_record;
static sec_record record[MAX_MODULE_NUM];
static int32_t ddb_sec_check(uint8_t* section)
{
	uint8_t* p = section;
	uint32_t i = 0, moduleid = 0, adap_len = 0, sec_num = 0, max_sec_num = 0;

	adap_len = p[8+9];
	max_sec_num = p[7];
	p += (8 + adap_len + 12);

	moduleid = (p[0] << 8) | p[1];
	sec_num = (p[4] << 8) | p[5];
	if (max_sec_num == 1)
		return 1;//table finish

	for (i = 0; i < MAX_MODULE_NUM; i++) {
		if (record[i].moduleid == moduleid) {
			if (record[i].sec_num == record[i].max_sec_num) {
	//			gxloge("last %d,%d,%d\n", moduleid, sec_num, record[i].max_sec_num);
				record[i].moduleid = -1;
				record[i].max_sec_num = 0;
				record[i].sec_num = 0;
				memset(record[i].sec, 0, MAX_SEC_NUM);
				return 1;
			}

			if (record[i].sec[sec_num] == 1)
			{
				//gxloge("same %d,%d,%d\n", moduleid, sec_num, record[i].max_sec_num);
				return -1;//repeat section
			}
			else {
				//gxloge("rev %d,%d,%d\n", moduleid, sec_num, record[i].max_sec_num);
				record[i].sec[sec_num] = 1;
				record[i].sec_num++;
				break;
			}
		}
	}

	if (i == MAX_MODULE_NUM) {
		for (i = 0; i < MAX_MODULE_NUM; i++) {
			if (record[i].moduleid == -1) {
				record[i].moduleid = moduleid;
				record[i].max_sec_num = max_sec_num;
				record[i].sec[sec_num] = 1;
				record[i].sec_num++;
				//gxloge("first %d,%d,%d\n", moduleid, sec_num, record[i].max_sec_num);
				break;
			}
		}
	}

	return 0;//section finish
}

static DdbInfo ddb;
uint8_t* section_data = NULL;
static int32_t  ddb_parse(uint8_t* section, uint32_t len)
{
	uint8_t *p = section;
	uint32_t ret = 0, size = len, adap_len = 0;
	uint32_t msg_len = 0;
	if (size < (8 + 12))
		return -1;
	memset(&ddb, 0, sizeof(DdbInfo));
	while(p < section+len)
	{
		ret = ddb_sec_check(p);
		if (ret == -1)
			return -1;
		adap_len = p[8+9];
		msg_len =p[18]<<8 | p[19];
		size = msg_len - adap_len;
		ddb.max_modulenum = p[7];
		p += (8 + adap_len + 12);
		ddb.moduleid = (p[0] << 8) | p[1];
		ddb.modulenum = (p[4] << 8) | p[5];

		p += 6;
		size -= 6;

		if (size > MAX_DDB_SECTION_SIZE) {
			gxloge("%s,%d,error ddb len\n", __func__, __LINE__);
			return -1;
		}
		if(ret != -1)
		{
			int m;
			for (m = 0; m < dii.modulenum; m++) {
				if (dii.moduleinfo[m].moduleid == ddb.moduleid) {
					//gxloge("\nid %d num %d \n",dii.moduleinfo[m].moduleid, ddb.modulenum[k]);
					ddb.modulenum = ddb.modulenum + (256*m);
					memcpy(section_data + (ddb.modulenum * size), p, size);
					ddb.parse_size += size;
				}
			}
		}
		p = p + 4 + size;
	}
	return ret;
}

int32_t ddb_start(uint32_t ddb_pid, uint32_t downloaderid, uint32_t moduleid)
{
	handle_t handle = 0;
	uint32_t i = 0;
	static uint32_t init_flag = 0;
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
	handle = GxDmxStart(dmxid, params);

//	memset(filter, 0, MAX_FILTER_NUM);
	if (handle != -1) {
		uint32_t i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (filter[i] == 0) {
				filter[i] = handle;
				break;
			}
		}
	}
	//ddb need to check repeat section
	if (!init_flag) {
		for (i = 0; i < MAX_MODULE_NUM; i++) {
			record[i].moduleid = -1;
			record[i].max_sec_num = 0;
			record[i].sec_num = 0;
			memset(record[i].sec, 0, MAX_SEC_NUM);
		}
		init_flag = 1;
	}
	return 0;
}


static uint8_t section_buffer[SECTION_SIZE];
static int32_t get_data(void)
{
	int32_t ret = 0;
	static int index = 0;
	uint32_t len = 0;
	while (1) {
		int32_t i = 0;
		for (i = 0; i < MAX_FILTER_NUM; i++) {
			if (filter[index] == 0)
			{
				index = (index+1)%MAX_FILTER_NUM;
				continue;
			}
			memset(section_buffer, 0, SECTION_SIZE);
			ret = GxDmxRead(filter[index], section_buffer, SECTION_SIZE, &len);
			if (ret == 0 && len > 0) {
//				gxloge("len = %d ,i = %d, b[0]= %x\n", len, index, section_buffer[0]);
				switch(section_buffer[0]){
					case DSI_DII_TID:
						{
							free_filter(index);
							index = (index+1)%MAX_FILTER_NUM;
							if (section_buffer[11] == 0x02) {
								if (dii_parse(section_buffer, len) == -1)
									return -1;
							} else if (section_buffer[11] == 0x06) {
								if (dsi_parse(section_buffer, len) == -1)
									return -1;
							} else {
								gxloge("%s,%d,%d, Error Type Table\n", __func__, __LINE__, section_buffer[0]);
								return -1;
							}

							return 0;
						}
					case DDB_TID:
						{
							ret = ddb_parse(section_buffer, len);
							if (ret == 1) {
								free_filter(index);
								index = (index+1)%MAX_FILTER_NUM;
								return 0;
							}
							else
							{
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
	}
	return -1;
}

void aldsmcc_download_data(struct download_data *udata)
{
	uint32_t groupid[MAX_GROUP_NUM] = {0}, pid = 0, n = 0, i = 0;
	int demux_id  = 0;
	int ret = 0;
	enum dmx_input ts_id = 0;
	struct GxOTA_Msg msg;

	memset(filter, 0, MAX_FILTER_NUM);
	pid = atoi(GxOem_GetValue( UPDATE_SECTION, "dmx_pid"));
	pid = 0x1e61;

	demux_id = GxDLP_Get_DmxId();
	ts_id = GxDLP_Get_DmxSource();
	GxDmxInit();
	ret = GxDmxSetSource(demux_id, ts_id);
	dmxid = demux_id;
	if (ret != 0) {
		msg.type = GXOTA_GUI_STATUS;
		msg.msg_id = GXOTA_STATUS_DMX_ERROR;
		gui_show(1, &msg, NULL);
	}
	//dsi
	if (dsi_start(pid) == 0) {

		if (get_data() == 0)
			//ota_render("dsi receive success", 20);

		for (i = 0; i < MAX_GROUP_NUM; i++) {
//			if ((dsi[i].groupid != 0) && (oui == dsi[i].oui) && (hwmodule == dsi[i].hwmodule) && (hwversion == dsi[i].hwversion) &&
//					(swmodule == dsi[i].swmodule)) {
//				if (dsi[i].swversion > swversion) {
					gxloge("will start dii!!!!\n");
					groupid[n] = dsi[i].groupid;
					n++;
//				}
//			}
		}

		if (n > 0) {
			//ota_render("dii receive success", 25);
			for (i = 0; i < MAX_GROUP_NUM; i++) {
				if (groupid[i] != 0) {
					if (dii_start(pid, groupid[i]) == 0) {
						if (get_data() == 0) { //dii
							int32_t m = 0, total_size = 0, size_tmp = 0;;
							for (m = 0; m < dii.modulenum; m++) {
								if (dii.moduleinfo[m].modulesize != 0) {
									ddb_start(pid, dii.downloaderid, dii.moduleinfo[m].moduleid);
									total_size += dii.moduleinfo[m].modulesize;
								}
							}

							section_data = GxCore_Mallocz(total_size + 1);
							size_tmp = total_size;
							while(size_tmp > 0) {
								if (get_data() == 0) //ddb
									size_tmp -= ddb.parse_size;
							}
							udata->data = section_data;
							udata->len = total_size;
						}
					}
				}
			}
		} else
			return ;
	} else
		return ;
	return ;
}
