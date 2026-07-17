#include "string.h"
#include "common.h"
#ifdef NO_OS
#include "gxloader.h"
#endif
#include "gxdlp_api.h"
#include "gui.h"
#include "dvbhal/dvbcrc.h"
#include "dvbhal/gxdemux_hal.h"
#include "upgrade.h"

#define DDT_MAX_SECTION_LENGTH                  (1024)
#define DDT_SECTION_RECEIVED                    (0xFF)
#define DDT_SECTION_NOT_RECEIVED                (0x00)
#define OTA_DMX_MAX_BUF_SIZE                    (64*1024)

enum GxOTA_DataStatus {
	GXOTA_DATA_RECEVING,
	GXOTA_DATA_FINISH,
	GXOTA_DATA_ERROR,
};

typedef struct software_param_s {
	unsigned int crc;
	unsigned int len;
	unsigned char *data;
}BinT;

struct GxOTA_DmxParam {
	unsigned short                      pid;
	int                                 depth;
	struct dmx_filter_key               key[18];
} ;

struct GxOTA_ProtoDmxParam {
	int                                 num;
	int		                    timeout;
	struct GxOTA_DmxParam               param[31];
} ;

int   *dmx_list = NULL;
unsigned int    update_type = 0;
static int dmxid;

static BinT   *gpbin=NULL;
static unsigned char* chSectionStatus=NULL;
struct GxOTA_ProtoDmxParam      dmx_param;

static int dmx_open(struct GxOTA_DmxParam* param)
{
	unsigned int i=0;
	GxDmxFilterParams fparams;
	memset(&fparams,0,sizeof(GxDmxFilterParams));
	if (param == NULL)
		return -1;

	fparams.pid = param->pid;
	fparams.depth = param->depth;
	fparams.flags = (REPEAT_FLAG|EQ_FLAG);
	for (i = 0; i < param->depth; i++) {
		fparams.match[i] = param->key[i].value;
		fparams.mask[i] = param->key[i].mask;
	}

	return GxDmxStart(dmxid,fparams);
}

/* 设置过滤条件 */
static int get_dmx_param()
{
	dmx_param.timeout = GxDLP_Get_DmxTimeout();

	dmx_param.num = 1;
	dmx_param.param[0].pid            = GxDLP_Get_DmxPid();
	dmx_param.param[0].depth          = 1;
	dmx_param.param[0].key[0].mask    = 0xff;
	dmx_param.param[0].key[0].value   = GxDLP_Get_DmxTableId();
	return 0;
}

static int ddt_init(unsigned short section_count)
{
	static int init_flag = 0;

	if ( 1 == init_flag)
		return 0;

	gpbin = (BinT *)malloc(sizeof(BinT));
	if(gpbin == NULL) {
		gxloge("ddt_init ERR1\n");
		return -1;
	}
	memset(gpbin,0,sizeof(BinT));

	gpbin->data = (unsigned char *)malloc(sizeof(unsigned char) * DDT_MAX_SECTION_LENGTH * section_count);
	if(gpbin->data == NULL) {
		gxloge("ddt_init ERR2\n");
		return -1;
	}
	memset(gpbin->data, 0,  sizeof(unsigned char) * DDT_MAX_SECTION_LENGTH*section_count);

	chSectionStatus = (unsigned char*)malloc(sizeof(unsigned char) * section_count);
	if(chSectionStatus == NULL) {
		gxloge("ddt_init ERR4\n");
		return -1;
	}
	memset(chSectionStatus,DDT_SECTION_NOT_RECEIVED, sizeof(unsigned char) *section_count);

	init_flag = 1;

	return 0;
}

static unsigned int bincrc;
static int ddt_parse( unsigned char *pBuf )
{
	unsigned char *pData = pBuf;
	unsigned short wSectionLength = 0;
	unsigned short wSectionNumber = 0;
	unsigned short wPayloadLength = 0;
	unsigned short wLastSectionNumber = 0;
	unsigned int   nCrcResult = 0;
	unsigned int   nCrcResult2 = 0;

	wSectionLength = (((unsigned short)pData[1] << 8) | pData[2]) & 0x0fff;
	wSectionNumber = ((unsigned short)pData[6] << 8 )| pData[7];
	wLastSectionNumber = ((unsigned short)pData[8] << 8 )| pData[9];
	pData += 10;
	if (0 == wSectionNumber) {//First Section
		update_type = (*(pData+12));
		gxlogd("update_type = %x\n",update_type);
		if(5 ==update_type)
			return 0;
	} else if(wSectionNumber == wLastSectionNumber) {//Last Section
		wPayloadLength = wSectionLength - 15;//crc - 4

		bincrc = (unsigned int)(pData[wPayloadLength]<<24) |pData[wPayloadLength + 1]<<16
			|pData[wPayloadLength + 2]<<8 |pData[wPayloadLength + 3];
		gxlogd("[Downloader] bin->crc = %x \n",bincrc);

		nCrcResult = DVBCRC32(pBuf, wSectionLength+3-4);
		nCrcResult2 = (unsigned int)(pBuf[wSectionLength+3-4]<<24) |pBuf[wSectionLength+3-4 + 1]<<16
			|pBuf[wSectionLength+3-4 + 2]<<8 |pBuf[wSectionLength+3-4 + 3];
		if (nCrcResult2 != nCrcResult) {
			gxloge("wSectionNumber = %d nCrcResult2 = 0x%x nCrcResult=0x%x\n",wSectionNumber,nCrcResult2,nCrcResult);
			return -1;
		}

		memcpy((gpbin->data + ( (wSectionNumber-1)* 1010)),pData,wPayloadLength);
		gpbin->len += (wPayloadLength);


	} else {
		wPayloadLength = wSectionLength - 11;//crc - 4

		nCrcResult = DVBCRC32(pBuf, wSectionLength+3-4);
		nCrcResult2 = (unsigned int)(pBuf[wSectionLength+3-4]<<24)
			|pBuf[wSectionLength+3-4 + 1]<<16
			|pBuf[wSectionLength+3-4 + 2]<<8
			|pBuf[wSectionLength+3-4 + 3];
		if (nCrcResult2 != nCrcResult) {
			gxloge("wSectionNumber = %d nCrcResult2 = 0x%x nCrcResult=0x%x\n",wSectionNumber,nCrcResult2,nCrcResult);
			return -1;
		}
		memcpy((gpbin->data + ( (wSectionNumber-1)* wPayloadLength)),pData,wPayloadLength);
		gpbin->len += (wPayloadLength);
	}
	return 0;
}

static enum GxOTA_DataStatus ddt_is_finish(unsigned char* buf, unsigned int size)
{
	int Err;
	unsigned int  len = size;
	unsigned char *sec = buf;
	unsigned short  wSecNum= 0;
	unsigned short  wLastSecNum= 0;
	static unsigned short  wTotalSection= 0;
	unsigned int wSectionLength = 0;
	static unsigned char  old_percent = 0;
	static unsigned int  wRevSectionNum = 0;
	unsigned char  percent = 0;
	struct GxOTA_Msg msg;
	GxDLP_Language_Type tmplang = GXDLP_CHINESE;
	char buftext[512] = {0,};

	while (len > 0) {
		if (sec[0] != (dmx_param.param[0].key[0].value & 0xff)) {
			gxloge("%s %d sec[0 ] =%d  \n",__FILE__,__LINE__,sec[0 ] );
			return GXOTA_DATA_ERROR;
		}
		wSecNum = ((unsigned short)sec[6] << 8 )| sec[7];
		wLastSecNum = ((unsigned short)sec[8] << 8 )| sec[9];
		if (0 != ddt_init(wLastSecNum+1)) {
			gxloge("%s %d wLastSecNum=%d wSecNum=%d ddt_init error\n",__FILE__,__LINE__,wLastSecNum,wSecNum);
			return GXOTA_DATA_ERROR;
		}

		wSectionLength = (((unsigned short)sec[1] << 8) | sec[2]) & 0x0fff;
		if (wSectionLength > 1023 || wSectionLength < 15) {
			gxloge("%s %d wSectionLength =%d  \n",__FILE__,__LINE__,wSectionLength );
			return GXOTA_DATA_ERROR;
		}

		if (wSectionLength > len) {
			gxloge("%s %d wSectionLength =%d len=%d\n",__FILE__,__LINE__,wSectionLength,len);
			return GXOTA_DATA_ERROR;
		}

		if (wSecNum > wLastSecNum) {
			gxloge("%s %d wSecNum =%d wLastSecNum=%d\n",__FILE__,__LINE__,wSecNum,wLastSecNum);
			return GXOTA_DATA_ERROR;
		}

		if(0 == wTotalSection) {
			wTotalSection = wLastSecNum + 1 ;
			GxDLP_Get_Language(&tmplang);
			if (-1 == tmplang) {
				sprintf(buftext, "%s:%d", STR_CN_GXOTA_SECTION_NUM, wTotalSection);
			} else {
				if (tmplang == GXDLP_CHINESE)
					sprintf(buftext, "%s:%d", STR_CN_GXOTA_SECTION_NUM, wTotalSection);
				if (tmplang == GXDLP_ENGLISH)
					sprintf(buftext, "%s%d", "Section count: ", wTotalSection);
			}

			msg.type = GXOTA_GUI_STATUS;
			msg.msg_id = 0;
			gui_show(0, &msg, buftext);

			gxlogd("\n  wTotalSection=%d\n",wTotalSection );
		}

		if(DDT_SECTION_RECEIVED!=chSectionStatus[wSecNum]) {
			//gxlogd("\n Section Number=%d \n",wSecNum );

			Err = ddt_parse( sec );
			if(Err == -1)
				return GXOTA_DATA_ERROR;

			wRevSectionNum ++ ;
			percent = wRevSectionNum*100/wTotalSection;

			if(percent != old_percent) {
				old_percent = percent;

				msg.type = GXOTA_GUI_PROCESS;
				msg.percent = old_percent / 2;
				gui_show(0, &msg, NULL);
			}

			if(wRevSectionNum == wTotalSection) {
				gpbin->crc = bincrc;
				gxlogd("\n[Downloader] Received Section wRevSectionNum=%d wTotalSection=%d\n",wRevSectionNum,wTotalSection );

				msg.type = GXOTA_GUI_PROCESS;
				msg.percent = 48;
				gui_show(0, &msg, NULL);

				unsigned int nCrcResult = 0;
				nCrcResult = DVBCRC32(gpbin->data, gpbin->len);

				gxlogd("[Downloader] CRC_result = %x \n",nCrcResult);
				gxlogd("[Downloader] SoftwareCRC = %x \n",gpbin->crc);
				if(nCrcResult != gpbin->crc)
					return GXOTA_DATA_ERROR;

				gxlogd("[Downloader] software_data_defrag end\n");
				gxlogd("[Downloader] App Len=%d\n", gpbin->len );

				if(chSectionStatus)
					free(chSectionStatus);
				return GXOTA_DATA_FINISH;
			}
			chSectionStatus[wSecNum] = DDT_SECTION_RECEIVED;
		} else {
			//gxlogd("\n Number=%d already\n",wSecNum);
		}
		len -= wSectionLength + 3;
		sec += wSectionLength + 3;
	}
	return GXOTA_DATA_RECEVING;
}

int ddt_download_data(struct download_data *file)
{
	int                             i;
	int                             j;
	int                             ret = 0;
	int                             ts_lose_count = 0;
	unsigned int                    size;
	unsigned char                   buf[OTA_DMX_MAX_BUF_SIZE];
	struct GxOTA_Msg                msg;
	int demux_id  = 0;
	enum dmx_input ts_id = 0;

	demux_id = GxDLP_Get_DmxId();
	ts_id = GxDLP_Get_DmxSource();

	/*Get the demux information ( or configuration) and initialize demux*/
	GxDmxInit();
	ret= GxDmxSetSource(demux_id,ts_id);
	if( ret  != 0) {
		msg.type = GXOTA_GUI_STATUS;
		msg.msg_id = GXOTA_STATUS_DMX_ERROR;
		gui_show(1, &msg, NULL);
	}

	dmxid = demux_id;

	msg.type = GXOTA_GUI_STATUS;
	msg.msg_id = GXOTA_STATUS_DMX_START_FILTER;
	gui_show(1, &msg, NULL);

	memset(&dmx_param, 0, sizeof(struct GxOTA_ProtoDmxParam));

	get_dmx_param();
	gxlogd("[GXOTA] Filter Num=%d!\r\n", dmx_param.num);
	dmx_list = malloc(dmx_param.num * sizeof(int));

	for (i = 0; i < dmx_param.num; i++) {
		gxlogd("[GXOTA] Filter PID=%d, tableid=%d,depth=%d\r\n",
				dmx_param.param[i].pid, dmx_param.param[i].key[0].value, dmx_param.param[i].depth);
		dmx_list[i] = dmx_open(&dmx_param.param[i]);

	}
	gxlogd("[GXOTA] Filtering ...\r\n");
	do {
		for (i = 0; i < dmx_param.num; i++) {
			memset(buf, 0, OTA_DMX_MAX_BUF_SIZE);

			ret = GxDmxRead(dmx_list[i],buf,OTA_DMX_MAX_BUF_SIZE,&size);

			if(size>OTA_DMX_MAX_BUF_SIZE || 0 == size) {

				for(j = 0; j < dmx_param.timeout; j++)
				{
					ret = GxDmxRead(dmx_list[i],buf,OTA_DMX_MAX_BUF_SIZE,&size);
					if(size != 0)
						break;
				}
				if(j == dmx_param.timeout)
					return -1;
#if 0

				gxlogd("[GXOTA] Filter Type=%d, PID=%d, tableid=%d,depth=%d\r\n",
						dmx_param.param[i].type, dmx_param.param[i].pid, dmx_param.param[i].key[0].value, dmx_param.param[i].depth);
				dmx_list[i] = dmx_open(&dmx_param.param[i]);
				gxlogd("[GXOTA]dmx_open\r\n");
				GxOTA_DmxEnable(dmx_list[i]);
				gxlogd("[GXOTA]GxOTA_DmxEnable\r\n");

#endif
			}
			ts_lose_count = 0;
			ret = ddt_is_finish(buf, size);
			if (GXOTA_DATA_ERROR == ret) {
				ret = GXOTA_DATA_RECEVING;

				GxDmxReset(dmx_list[i]);
				/*调试接收数据错误*/
				//					while(1);
				GxDmxStop(dmx_list[i]);
				gxlogd("[GXOTA] Filter PID=%d, tableid=%d,depth=%d\r\n",
						dmx_param.param[i].pid, dmx_param.param[i].key[0].value, dmx_param.param[i].depth);
				dmx_list[i] = dmx_open(&dmx_param.param[i]);
				gxlogd("[GXOTA]dmx_open\r\n");

				gxlogd("\n filter restart 2 size = %d\n",size);

			}
		}
	} while (ret != GXOTA_DATA_FINISH);

	if (dmx_list != NULL) {
		for (i = 0; i <  dmx_param.num; i++)
			GxDmxStop(dmx_list[i]);

		free(dmx_list);
		dmx_list = NULL;
	}

	msg.type = GXOTA_GUI_STATUS;
	msg.msg_id = GXOTA_STATUS_DMX_DATA_FINISH;
	gui_show(1, &msg, NULL);

	file->data = gpbin->data;
	file->len = gpbin->len;
	return 0;
}

