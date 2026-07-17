#include <sys/ioctl.h>
#include <fcntl.h>
#include "module/config/gxconfig.h"
#include "gxfrontend_socket_mgr.h"
#include "gxfrontend_demux_fifo.h"
#include "gxfrontend_net.h"
#include "gxfrontend.h"

//for Multi-Point LNBF #359333
#define GXFRONTEND_LNBF_FRE_MIN    (250000)//KHz
#define GXFRONTEND_LNBF_FRE_MAX    (2600000)//KHz
#define GXFRONTEND_NORMAL_FRE_MIN  (915000)//KHz
#define GXFRONTEND_NORMAL_FRE_MAX  (2185000)//KHz

static int32_t gs_strength[FRONTEND_MAX];
static int32_t gs_frontend_status[FRONTEND_MAX];
static GxFrontend_22k gs_frontend_22k[FRONTEND_MAX];
static GxFrontend_Polar gs_frontend_polar[FRONTEND_MAX];
static GxFrontend_Dis gs_frontend_dis10[FRONTEND_MAX];
static GxFrontend_Dis gs_frontend_dis11[FRONTEND_MAX];
static GxFrontend_Dis gs_frontend_dis12[FRONTEND_MAX];
static GxFrontend_Unicable gs_frontend_unicable[FRONTEND_MAX];
static GxFrontendSignalQuality gs_frontend_quality[FRONTEND_MAX];
fronted_set_status g_frontend_set[FRONTEND_MAX];
extern void GxFrontendOccur(int32_t tuner);
static uint32_t gs_frontend_strenth_quality = 1;//默认是开启读取信号质量，信号强度的
extern GxFrontendNetOps net_ops;


static  GxFrontend_Params gs_frontend[FRONTEND_MAX] = {
	{.dev =-1,.frontend = -1, .demux = -1, .hSocket = 0, .type = FRONTEND_DVB_S2, .url = NULL},
	{.dev =-1,.frontend = -1, .demux = -1, .hSocket = 0, .type = FRONTEND_DVB_S2, .url = NULL},
	{.dev =-1,.frontend = -1, .demux = -1, .hSocket = 0, .type = FRONTEND_DVB_S2, .url = NULL},
	{.dev =-1,.frontend = -1, .demux = -1, .hSocket = 0, .type = FRONTEND_DVB_S2, .url = NULL}};
static  handle_t      gs_frontend_mutex[FRONTEND_MAX] = {-1,-1,-1,-1};
static  int32_t       gs_frontend_monitor[FRONTEND_MAX]={0,0,0,0};
uint32_t g_check_count[FRONTEND_MAX]={0,0,0,0};

enum {
	TP_LOCK_START = 1,
	TP_LOCK_END
};

static int32_t gs_tplock_flag[FRONTEND_MAX] = { TP_LOCK_END, TP_LOCK_END,TP_LOCK_END,TP_LOCK_END};

static int32_t sg_TPParamError = -1;// 1 = error; 0 = ok;
typedef struct {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
}GxFrontendDiseqcCmd;

static GxFrontendDiseqcCmd cmd[] = {
	{ { { 0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k off--port 1
	{ { { 0xe0, 0x10, 0x38, 0xf2, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k off--port 1
	{ { { 0xe0, 0x10, 0x38, 0xf1, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k on --port 1
	{ { { 0xe0, 0x10, 0x38, 0xf3, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k on --port 1
	{ { { 0xe0, 0x10, 0x38, 0xf4, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k off--port 2
	{ { { 0xe0, 0x10, 0x38, 0xf6, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k off--port 2
	{ { { 0xe0, 0x10, 0x38, 0xf5, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k on --port 2
	{ { { 0xe0, 0x10, 0x38, 0xf7, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k on --port 2
	{ { { 0xe0, 0x10, 0x38, 0xf8, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k off--port 3
	{ { { 0xe0, 0x10, 0x38, 0xfa, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k off--port 3
	{ { { 0xe0, 0x10, 0x38, 0xf9, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k on --port 3
	{ { { 0xe0, 0x10, 0x38, 0xfb, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k on --port 3
	{ { { 0xe0, 0x10, 0x38, 0xfc, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k off--port 4
	{ { { 0xe0, 0x10, 0x38, 0xfe, 0x00, 0x00 }, 4 }, 0 },// 18v, 22k off--port 4
	{ { { 0xe0, 0x10, 0x38, 0xfd, 0x00, 0x00 }, 4 }, 0 },// 13v, 22k on --port 4
	{ { { 0xe0, 0x10, 0x38, 0xff, 0x00, 0x00 }, 4 }, 0 } // 18v, 22k on --port 4
};

void GxFrontend_SetDev(int32_t tuner, int32_t dev, int32_t demux)
{
    if(tuner < 0
            || tuner >= FRONTEND_MAX
            || dev < 0
            || demux < 0)
    {
        gxlogd("!!!!!!!!![%s, %d] params error!!!!!!!!!!\n", __func__, __LINE__);
        return;
    }

    gs_frontend[tuner].dev = dev;
    gs_frontend[tuner].demux = demux;

    return;
}

int32_t GxFrontend_SetFrontendTuneMode(int32_t tuner, uint32_t mode)
{
	if((tuner >= FRONTEND_MAX) || (gs_frontend[tuner].frontend < 0))
		return -1;
	return ioctl(gs_frontend[tuner].frontend, FE_SET_FRONTEND_TUNE_MODE, mode);
}

int32_t GxFrontend_GetFrontendTuneMode(int32_t tuner, uint32_t *mode)
{
	if((tuner >= FRONTEND_MAX) || (gs_frontend[tuner].frontend < 0) || (mode == NULL))
		return -1;
	return ioctl(gs_frontend[tuner].frontend, FE_GET_FRONTEND_TUNE_MODE, mode);
}

static int gxfrontend_open(int id) {
	int fd = -1, i;
	char devname[128] = {0};
	for (i = 0; i < 4; i++) {
		snprintf(devname,  sizeof(devname), "/dev/dvb/adapter%d/frontend%d", i, id);
		if (GxCore_UpUserPermissions() != true)
		{
			goto out;
		}
		fd = open(devname, O_RDWR);
		GxCore_DownUserPermissions();
		if (fd > 0)
			goto out;

		sprintf(devname, "/dev/dvb%d.frontend%d", i, id);
		if (GxCore_UpUserPermissions() != true)
		{
			goto out;
		}
		fd = open(devname, O_RDWR|O_NONBLOCK);
		GxCore_DownUserPermissions();
		if (fd > 0)
			goto out;

		sprintf(devname, "/dev/gxfe%d", id);
		if (GxCore_UpUserPermissions() != true)
		{
			goto out;
		}
		fd = open(devname, O_RDWR);
		GxCore_DownUserPermissions();
		if (fd > 0)
			goto out;
		break;
	}
out:
	return fd;
}

static int32_t gxfrontend_diseqc_10(int32_t frontend, GxFrontendDiseqc10 *pa)
{
	int32_t ret = 0;
	if(pa->b22k == 0)// on
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chDiseqc)
			{
				case 0:
					break;
				case 1:// F1
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[2].cmd));
					break;

				case 2:// F5
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[6].cmd));
					break;

				case 3:// F9
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[10].cmd));
					break;

				case 4:// FD
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[14].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}
		}
		else // 18V
		{
			switch(pa->chDiseqc)
			{
				case 0:
					break;
				case 1:// F3
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[3].cmd));
					break;

				case 2:// F7
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[7].cmd));
					break;

				case 3:// FB
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[11].cmd));
					break;

				case 4:// FF
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[15].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}

		}
	}
	else// off
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chDiseqc)
			{
				case 0:
					break;
				case 1:// F0
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[0].cmd));
					break;

				case 2:// F4
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[4].cmd));
					break;

				case 3:// F8
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[8].cmd));
					break;

				case 4:// FC
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[12].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}
		}
		else // 18V
		{
			switch(pa->chDiseqc)
			{
				case 0:
					break;
				case 1:// F2
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[1].cmd));
					break;

				case 2:// F6
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[5].cmd));
					break;

				case 3:// FA
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[9].cmd));
					break;

				case 4:// FE
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[13].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}
		}
	}
	return ret;
}

static int32_t gxfrontend_diseqc_11(int32_t frontend, GxFrontendDiseqc11 *pa)
{
	int32_t ret = 0;
	struct dvb_diseqc_master_cmd tmp;

	if(pa->chUncom == 0)//cannot send cmd while DiSEqC1.1 is OFF
		return ret;

	if(pa->b22k == 0)
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chCom)
			{
				case 0:
					break;
				case 1:// F1
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[2].cmd));
					break;

				case 2:// F5
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[6].cmd));
					break;

				case 3:// F9
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[10].cmd));
					break;

				case 4:// FD
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[14].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}
		}
		else
		{
			switch(pa->chCom)
			{
				case 0:
					break;
				case 1:// F3
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[3].cmd));
					break;

				case 2:// F7
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[7].cmd));
					break;

				case 3:// FB
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[11].cmd));
					break;

				case 4:// FF
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[15].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}

		}
	}
	else
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chCom)
			{
				case 0:
					break;
				case 1:// F0
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[0].cmd));
					break;

				case 2:// F4
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[4].cmd));
					break;

				case 3:// F8
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[8].cmd));
					break;

				case 4:// FC
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[12].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}
		}
		else
		{
			switch(pa->chCom)
			{
				case 0:
					break;
				case 1:// F2
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[1].cmd));
					break;

				case 2:// F6
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[5].cmd));
					break;

				case 3:// FA
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[9].cmd));
					break;

				case 4:// FE
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[13].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}
		}
	}
	tmp.msg[0] = 0xE0;
	tmp.msg[1] = 0x10;
	tmp.msg[2] = 0x39;
	if(pa->chUncom > 16)
		pa->chUncom = 1;
	tmp.msg[3] = 0xf0+(pa->chUncom-1);
	tmp.msg[4] = 0x00;
	tmp.msg[5] = 0x00;
	tmp.msg_len = 4;

	ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(tmp));
	return ret;
}

#if 0
static int32_t gxfrontend_diseqc_10(int32_t frontend, GxFrontendDiseqc10 *pa)
{
	int32_t ret = 0;
	if(pa->b22k == 0)// on
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chDiseqc)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[1].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[5].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[9].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[13].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}
		}
		else
		{
			switch(pa->chDiseqc)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[0].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[4].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[8].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[12].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}

		}
	}
	else// off
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chDiseqc)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[3].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[7].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[11].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[15].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}
		}
		else
		{
			switch(pa->chDiseqc)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[2].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[6].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[10].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[14].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chDiseqc);
					ret = -1;
					break;
			}
		}
	}
	return ret;
}

static int32_t gxfrontend_diseqc_11(int32_t frontend, GxFrontendDiseqc11 *pa)
{
	int32_t ret = 0;
	struct dvb_diseqc_master_cmd tmp;
	if(pa->b22k == 0)
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chCom)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[1].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[5].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[9].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[13].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}
		}
		else
		{
			switch(pa->chCom)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[0].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[4].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[8].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[12].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}

		}
	}
	else
	{
		if(pa->bPolar == SEC_VOLTAGE_13)
		{
			switch(pa->chCom)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[3].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[7].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[11].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[15].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}
		}
		else
		{
			switch(pa->chCom)
			{
				case 0:
				case 1:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[2].cmd));
					break;

				case 2:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[6].cmd));
					break;

				case 3:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[10].cmd));
					break;

				case 4:
					ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(cmd[14].cmd));
					break;

				default:
					gxlogd("diseqc cmd = %d   err!!!!!\n",pa->chCom);
					ret = -1;
					break;
			}
		}
	}
	tmp.msg[0] = 0xE0;
	tmp.msg[1] = 0x10;
	tmp.msg[2] = 0x39;
	if((pa->chUncom == 0) || ((pa->chUncom > 16)))// even if OFF the DISEQC1.1 function, default output port 1
		pa->chUncom = 1;
	tmp.msg[3] = 0xf0+(pa->chUncom-1);
	tmp.msg[4] = 0x00;
	tmp.msg[5] = 0x00;
	tmp.msg_len = 4;

	ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(tmp));
	return ret;
}
#endif



static int32_t gxfrontend_diseqc_12(int32_t frontend, GxFrontendDiseqc12 *pa)
{
	struct dvb_diseqc_master_cmd tmp;
	uint8_t i = 0;

	if(pa->CmdCount > FRONTEND_DISEQC_MAX_CMD)
		goto err;

	for(i = 0; i < pa->CmdCount; i++)
	{
		tmp.msg[0] = 0xe0;
		tmp.msg[1] = 0x31;
		switch(pa->DiseqcControl[i])
		{
			case DISEQC_LIMIT_WEST:
				tmp.msg[2] = 0x67;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 3;
				break;

			case DISEQC_DRIVE_WEST:
				tmp.msg[2] = 0x69;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 4;
				break;

			case DISEQC_LIMIT_EAST:
				tmp.msg[2] = 0x66;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 3;
				break;

			case DISEQC_DRIVE_EAST:
				tmp.msg[2] = 0x68;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 4;
				break;

			case DISEQC_STOP:
				tmp.msg[2] = 0x60;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 3;
				break;

			case DISEQC_LIMIT_OFF:
				tmp.msg[2] = 0x63;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 3;
				break;

			case DISEQC_STORE_NN:
				tmp.msg[2] = 0x6a;
				tmp.msg[3] = pa->DiSEqC12Params[i].m_chParam0;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 4;
				break;

			case DISEQC_GOTO_NN:
				tmp.msg[2] = 0x6b;
				tmp.msg[3] = pa->DiSEqC12Params[i].m_chParam0;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 4;
				break;

			case DISEQC_GOTO_XX:
				tmp.msg[2] = 0x6e;
				tmp.msg[3] = pa->DiSEqC12Params[i].m_chParam0;
				tmp.msg[4] = pa->DiSEqC12Params[i].m_chParam1;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 5;
				break;

			case DISEQC_SET_POSNS:
				tmp.msg[2] = 0x6f;
				tmp.msg[3] = pa->DiSEqC12Params[i].m_chParam0;
				tmp.msg[4] = pa->DiSEqC12Params[i].m_chParam2;
				tmp.msg[5] = pa->DiSEqC12Params[i].m_chParam3;
				tmp.msg_len = 6;
				break;

			default:
				tmp.msg[2] = 0x60;
				tmp.msg[3] = 0x00;
				tmp.msg[4] = 0x00;
				tmp.msg[5] = 0x00;
				tmp.msg_len = 3;
				break;
		}
		if(ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &(tmp)) < 0)
		{
			goto err;
		}
	}
	return 0;
err:
	return -1;
}

int32_t GxFrontend_RegisterNet(uint32_t tuner_count, char* player_name)
{
	int32_t ret = 0;
	if(tuner_count > FRONTEND_MAX || player_name == NULL)
		return -1;
	ret = GxFrontendNet_Register(player_name);
	if(-1 == ret){
		gxloge("!!!!!!!!![%s, %d] register net frontend error %d!!!!!!!!!!!\n", __func__, __LINE__, tuner_count);
		return -1;
	}
	gs_frontend[tuner_count].frontend = 0xFFFF;
	gs_frontend[tuner_count].dev      = 0xFFFF;
	gs_frontend[tuner_count].type     = FRONTEND_DVB_NET;
	gs_frontend[tuner_count].url      = GxCore_Malloc(GXBUS_NETFRONTEND_MAX_URL_LENGTH);
	if(gs_frontend[tuner_count].url == NULL){
		gxloge("!!!!!!!!! url malloc fail !!!!!!!!!!!\n");
		return -1;
	}

	if(-1 == gs_frontend_mutex[tuner_count])
		GxCore_MutexCreate(&gs_frontend_mutex[tuner_count]);
	return 0;
}
int32_t GxFrontend_Init(uint32_t tuner_count)
{
	if(tuner_count > FRONTEND_MAX)
		return -1;

	gs_frontend_dis10[tuner_count].para.tuner = tuner_count;
	gs_frontend_dis10[tuner_count].para.diseqc.type = DiSEQC10;
	gs_frontend_dis11[tuner_count].para.tuner = tuner_count;
	gs_frontend_dis11[tuner_count].para.diseqc.type = DiSEQC11;
	gs_frontend_dis12[tuner_count].para.tuner = tuner_count;
	gs_frontend_dis12[tuner_count].para.diseqc.type = DiSEQC12;

	gs_frontend[tuner_count].frontend = gxfrontend_open(tuner_count);
	if(-1 == gs_frontend[tuner_count].frontend)
	{
		gxlogd("!!!!!!!!![%s, %d] open frontend error %d!!!!!!!!!!!\n", __func__, __LINE__, tuner_count);
		return -1;
	}

	if(-1 == gs_frontend_mutex[tuner_count])
		GxCore_MutexCreate(&gs_frontend_mutex[tuner_count]);
	return 0;
}

GxFrontendType GxFrontendGetType(int32_t tuner)
{
	return gs_frontend[tuner].type;
}

uint32_t GxFrontendGetNetTimeOut(int32_t tuner)
{
	return gxFrontendGetTimeoutCnt((void*)(gs_frontend[tuner].hSocket));
}

int GxFrontendReconnectNet(int32_t tuner)
{
	GxFrontend Param = {0};
	char ipStr[16] = {0};
	uint32_t ip = 0;
	if(gs_frontend[tuner].type == FRONTEND_DVB_NET && gs_frontend[tuner].hSocket)
	{
		ip = gs_frontend[tuner].ip;
		snprintf(ipStr, 15, "%d.%d.%d.%d", (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip)&0xFF);
		Param.type = FRONTEND_DVB_NET;
		Param.ip = ipStr;
		Param.port = gs_frontend[tuner].port;
		Param.dev = gs_frontend[tuner].dev;
		Param.demux = gs_frontend[tuner].demux;
		GxFrontend_SetTp(&Param);
	}
	return 0;
}

static void GxFrontendSet22k(int32_t tuner,int32_t sat22k)
{
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	gs_frontend_22k[tuner].sat22k = sat22k;
	gs_frontend_22k[tuner].flag = 1;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return;
}

static int32_t GxFrontendGet22k(int32_t tuner,int32_t* sat22k)
{
	int32_t flag = 0;
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	*sat22k = gs_frontend_22k[tuner].sat22k;
	flag = gs_frontend_22k[tuner].flag;
	gs_frontend_22k[tuner].flag = 0;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return flag;
}

static void GxFrontendSetPolar(int32_t tuner,fe_sec_voltage_t polar)
{
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	gs_frontend_polar[tuner].polar = polar;
	gs_frontend_polar[tuner].flag = 1;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return;
}

static int32_t GxFrontendGetPolar(int32_t tuner,fe_sec_voltage_t* polar)
{
	int32_t flag = 0;
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	*polar = gs_frontend_polar[tuner].polar;
	flag = gs_frontend_polar[tuner].flag;
	gs_frontend_polar[tuner].flag = 0;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return flag;
}
static void GxFrontendSetDis(int32_t tuner,GxFrontendDiseqc* para)
{
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	switch(para->diseqc.type)
	{
		case DiSEQC10:
			memcpy(&gs_frontend_dis10[tuner].para,para,sizeof(GxFrontendDiseqc));
			gs_frontend_dis10[tuner].flag = 1;
			break;

		case DiSEQC11:
			memcpy(&gs_frontend_dis11[tuner].para,para,sizeof(GxFrontendDiseqc));
			gs_frontend_dis11[tuner].flag = 1;
			break;

		case DiSEQC12:
			memcpy(&gs_frontend_dis12[tuner].para,para,sizeof(GxFrontendDiseqc));
			gs_frontend_dis12[tuner].flag = 1;
			break;

		default:
			gxlogd("para->type = %d   err!!!!\n",para->diseqc.type);
			break;
	}
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return;
}

static int32_t GxFrontendGetDis(int32_t tuner,GxFrontendDiseqc* para)
{
	int32_t flag = 0;
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	switch(para->diseqc.type)
	{
		case DiSEQC10:
			memcpy(para,&gs_frontend_dis10[tuner].para,sizeof(GxFrontendDiseqc));
			flag = gs_frontend_dis10[tuner].flag;
			gs_frontend_dis10[tuner].flag = 0;
			break;

		case DiSEQC11:
			memcpy(para,&gs_frontend_dis11[tuner].para,sizeof(GxFrontendDiseqc));
			flag = gs_frontend_dis11[tuner].flag;
			gs_frontend_dis11[tuner].flag = 0;
			break;

		case DiSEQC12:
			memcpy(para,&gs_frontend_dis12[tuner].para,sizeof(GxFrontendDiseqc));
			flag = gs_frontend_dis12[tuner].flag;
			gs_frontend_dis12[tuner].flag = 0;
			break;

		default:
			gxlogd("para->type = %d   err!!!!\n",para->diseqc.type);
			break;
	}
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return flag;
}

static void GxFrontendSaveUnicableParams(int32_t tuner, struct dvb_diseqc_master_cmd *cmd)
{
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	gs_frontend_unicable[tuner].flag = 1;
	memcpy(&gs_frontend_unicable[tuner].cmd, cmd, sizeof(struct dvb_diseqc_master_cmd));
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return;
}

static int32_t GxFrontendGetUnicableParams(int32_t tuner, struct dvb_diseqc_master_cmd *cmd)
{
	int32_t flag = 0;
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	memcpy(cmd, &gs_frontend_unicable[tuner].cmd, sizeof(struct dvb_diseqc_master_cmd));
	flag = gs_frontend_unicable[tuner].flag;
	gs_frontend_unicable[tuner].flag = 0;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return flag;
}

static void GxFrontendClearUnicableParams(int32_t tuner)
{
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	memset(&gs_frontend_unicable[tuner], 0, sizeof(GxFrontend_Unicable));
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
}

void GxFrontend_ResetStatusMonitor(int32_t tuner)
{
	if((tuner >= FRONTEND_MAX) || (gs_frontend[tuner].frontend < 0))
		return;
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	g_frontend_set[tuner].set_tp = 1;
	gs_frontend_dis10[tuner].flag = 1;
	gs_frontend_dis11[tuner].flag = 1;
	if ( gs_frontend_unicable[tuner].cmd.msg_len > 0)
	{
		gs_frontend_unicable[tuner].flag = 1;
	}
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
}

void GxFrontend_GetGlobalParams(int32_t tuner, GxFrontend_MonitorParams *params)
{
	if((tuner >= FRONTEND_MAX) || (gs_frontend[tuner].frontend < 0))
		return;

	params->diseqc10[tuner].para.diseqc.type = DiSEQC10;
	params->diseqc11[tuner].para.diseqc.type = DiSEQC11;
	params->diseqc12[tuner].para.diseqc.type = DiSEQC12;
	params->polar[tuner].flag    = GxFrontendGetPolar(tuner,&(params->polar[tuner].polar));
	params->tone[tuner].flag     = GxFrontendGet22k(tuner,&(params->tone[tuner].sat22k));
	params->diseqc10[tuner].flag = GxFrontendGetDis(tuner,&(params->diseqc10[tuner].para));
	params->diseqc11[tuner].flag = GxFrontendGetDis(tuner,&(params->diseqc11[tuner].para));
	params->diseqc12[tuner].flag = GxFrontendGetDis(tuner,&(params->diseqc12[tuner].para));
	params->unicable[tuner].flag = GxFrontendGetUnicableParams(tuner, &(params->unicable[tuner].cmd));
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	memcpy(&params->set_status[tuner], &g_frontend_set[tuner], sizeof(fronted_set_status));
	if(params->set_status[tuner].set_tp != 0 && params->set_status[tuner].set_unlock_ts != 1)
		g_frontend_set[tuner].set_tp = 0;

	if(((params->set_status[tuner].set_tp != 0) && (params->set_status[tuner].set_unlock_ts != 1)) ||
			(params->polar[tuner].flag) || (params->tone[tuner].flag) ||
			(params->diseqc10[tuner].flag) || (params->diseqc11[tuner].flag) || (params->diseqc12[tuner].flag)){
		memcpy(&(params->params[tuner]), &(gs_frontend[tuner]), sizeof(GxFrontend_Params));
	}
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
}

// only set diseqc, dev & module id come from app
void GxFrontend_SetDiseqcMonitor(int32_t tuner, GxFrontend_MonitorParams *params)
{
	GxFrontendDiseqc para;
	GxFrontendType type = 0;
	if (tuner>=FRONTEND_MAX || params->params[tuner].frontend < 0)
	{
		return;
	}
	type = GxFrontendGetType(tuner);
	if((type != FRONTEND_DVB_S) && (type != FRONTEND_DVB_S2))
		return;

	if(params->diseqc10[tuner].flag == 1){
		params->diseqc10[tuner].flag = 0;
		gxfrontend_diseqc_10(params->params[tuner].frontend, &(params->diseqc10[tuner].para.diseqc.u.params_10));
	}
	if(params->diseqc11[tuner].flag == 1){
		params->diseqc11[tuner].flag = 0;
		gxfrontend_diseqc_11(params->params[tuner].frontend, &(params->diseqc11[tuner].para.diseqc.u.params_11));
	}
	if(params->diseqc12[tuner].flag == 1){
		params->diseqc12[tuner].flag = 0;
		gxfrontend_diseqc_12(params->params[tuner].frontend, &(params->diseqc12[tuner].para.diseqc.u.params_12));
	}
}

void GxFrontend_SetDiseqc(GxFrontendDiseqc *para)
{
    if(para == NULL)
        return;

	if (para->tuner>=FRONTEND_MAX||
			gs_frontend[para->tuner].frontend < 0)
	{
		return;
	}

	GxFrontendSetDis(para->tuner,para);
	if(gs_frontend_monitor[para->tuner] == 1)
	{
		return;
	}
	switch(para->diseqc.type)
	{
		case DiSEQC10:
			gxfrontend_diseqc_10(gs_frontend[para->tuner].frontend, &(para->diseqc.u.params_10));
			break;

		case DiSEQC11:
			gxfrontend_diseqc_11(gs_frontend[para->tuner].frontend, &(para->diseqc.u.params_11));
			break;

		case DiSEQC12:
			gxfrontend_diseqc_12(gs_frontend[para->tuner].frontend, &(para->diseqc.u.params_12));
			break;

		default:
			gxlogd("para->type = %d   err!!!!\n",para->diseqc.type);
			break;
	}
}

void GxFrontend_SendUnicableMonitor(int32_t tuner, GxFrontend_MonitorParams *params)
{
	if(NULL == params)
		return;

	if(tuner >= FRONTEND_MAX
			|| params->params[tuner].frontend < 0
			|| 0 == params->unicable[tuner].flag
			|| 0 == params->unicable[tuner].cmd.msg_len)
		return;

	ioctl(params->params[tuner].frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_13);//POL_VERTICAL
	GxCore_ThreadDelay(50);
	ioctl(params->params[tuner].frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_18);//POL_HORIZONTAL
	GxCore_ThreadDelay(50);
	ioctl(params->params[tuner].frontend, FE_DISEQC_SEND_MASTER_CMD, &(params->unicable[tuner].cmd));
	GxCore_ThreadDelay(200);
	ioctl(params->params[tuner].frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_13);//POL_VERTICAL
}

int32_t  GxFrontend_SameTp(GxFrontend *para)
{
	int32_t fre,symb,qam,tuner,sametp=1,bandwidth,type_1501,sat22k,polar;
	int32_t            dev;
	handle_t       demux;
	handle_t frontend;
	struct dvb_frontend_parameters params = { 0 };
	uint8_t    data_plp_id;
	uint8_t    common_plp_exist;
	uint8_t    common_plp_id;

	if(para==NULL || para->tuner>=FRONTEND_MAX)
		return 0;

	GxCore_MutexLock(gs_frontend_mutex[para->tuner]);

	fre       = para->fre;
	qam       = para->qam;
	symb      = para->symb;
	polar     = para->polar;
	tuner     = para->tuner;
	sat22k    = para->sat22k;
	bandwidth = para->bandwidth;
	data_plp_id = para->data_plp_id;
	common_plp_exist = para->common_plp_exist;
	common_plp_id = para->common_plp_id;
	dev = para->dev;
	demux = para->demux;
	type_1501 = para->type_1501;
	frontend = gs_frontend[tuner].frontend;
	switch(para->type){
		case FRONTEND_DVB_C:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbc_param.modulation = qam;
			params.u.dvbc_param.symbol_rate = symb;

			if(gs_frontend[tuner].qam != qam ||
					gs_frontend[tuner].symb != symb ||
					gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].frontend != frontend||
					gs_frontend[tuner].dev != dev)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DVB_S:
			params.frequency          = fre*1000;
			params.u.dvbs_param.symbol_rate = symb*1000;
			params.u.dvbs_param.fec_inner   = FEC_AUTO;

			if(gs_frontend[tuner].fre    != fre ||
					gs_frontend[tuner].symb  != symb ||
					gs_frontend[tuner].polar != polar||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].sat22k != sat22k)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DVB_T:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth||
					gs_frontend[tuner].frontend != frontend||
					gs_frontend[tuner].dev != dev)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DTMB:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;
			params.u.dvbt_param.constellation = qam;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth ||
					gs_frontend[tuner].type_1501 != type_1501||
					gs_frontend[tuner].dev != dev)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DVB_T2:
			params.frequency = fre;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;
			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].data_plp_id != data_plp_id||
					gs_frontend[tuner].common_plp_exist != common_plp_exist||
					gs_frontend[tuner].common_plp_id != common_plp_id ||
					gs_frontend[tuner].type_1501 != para->type_1501)
			{
				sametp = 0;
			}
			break;
	case FRONTEND_ATSC_T:
		/*不用乘1000*/
		params.frequency = fre;
		params.inversion = INVERSION_OFF;
		params.u.dvbt_param.bandwidth = bandwidth;

		if(gs_frontend[tuner].fre != fre||
				gs_frontend[tuner].bandwidth != bandwidth||
				gs_frontend[tuner].frontend != frontend||
				gs_frontend[tuner].dev != dev||
				gs_frontend[tuner].demux != demux)
		{
			sametp = 0;
		}
		break;
	case FRONTEND_ATSC_C:
		/*不用乘1000*/
		params.frequency = fre;
		params.inversion = INVERSION_OFF;
		params.u.dvbc_param.modulation = qam;
		params.u.dvbc_param.symbol_rate = symb;

		if(gs_frontend[tuner].qam != qam ||
				gs_frontend[tuner].symb != symb ||
				gs_frontend[tuner].fre != fre||
				gs_frontend[tuner].frontend != frontend||
				gs_frontend[tuner].dev != dev||
				gs_frontend[tuner].demux != demux)
		{
			sametp = 0;
		}
		break;
	default:
		break;
	}

	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return sametp;
}

#if 0
static uint32_t _bswap_32(uint32_t x)
{
	x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
	x = (x >> 16) | (x << 16);
	return x;
}
#endif


/*********************************** unicable start ******************************************/
/* ----------------------------------------------------------------------------
Name: scr_scrdrv_SetFrequency()

Description:
This routine will attempt to scan and find a QPSK signal based on the
passed in parameters.

Parameters:
InitialFrequency, IF value to commence scan (in kHz).
SymbolRate,       required symbol bit rate (in Hz).
MaxLNBOffset,     maximum allowed LNB offset (in Hz).
TunerStep,        Tuner's step value -- enables override of
TNR device's internal setting.
DerotatorStep,    derotator step (usually 6).
ScanSuccess,      boolean that indicates QPSK search success.
NewFrequency,     pointer to area to store locked frequency.

Return Value:
---------------------------------------------------------------------------- */
int GxFrontend_unicable2_odu_scd2_commands(uint32_t tuner, GxFrontend *unicable_param, GxFrontendUnicable2CMD cmd_type)
{
	handle_t frontend;
	unsigned char command_array [5] = {0x70, 0x00, 0x00, 0x00, 0x00};
	struct dvb_diseqc_master_cmd cmd;
	int Error = 0;
	uint32_t tuning_word;
	uint8_t hiband = 0;
	//uint8_t pol = 1;
	uint8_t pos = 0;
	uint8_t pin = 0xff;
	uint8_t i=0;


	if((0x80 != (unicable_param->lnb_index&0x80)) ||
			unicable_param->centre_fre < 100 ||
			unicable_param->if_fre==0 )
	{
		return (unicable_param->lnb_index);
	}

	frontend = gs_frontend[tuner].frontend;
	switch(cmd_type)
	{
		case UNICABLE2_ODU_CHANNEL_CHANGE:
			hiband = unicable_param->lnb_index&0x03;//bit0---unicable_param->lnb_index&0x01;
			//pol = (unicable_param->lnb_index&0x60)>>5;
			pos = (unicable_param->lnb_index&0x1C)>>2;//(unicable_param->lnb_index&0x0C)>>2;
			//unicable-2
			command_array[0] = 0x70;
			tuning_word = unicable_param->centre_fre - 100;
			cmd.msg_len = 4;
			command_array[1] = unicable_param->if_channel_index << 3;
			command_array[1] |= ((tuning_word >> 8) & 0x07);
			command_array[2] = (tuning_word & 0xff);
			//command_array[3] = ((pos & 0x3f) << 2) | (pol ? 2 : 0) | (hiband ? 1 : 0);
			command_array[3] = ((pos & 0x3f) << 2) | hiband;
			if (pin < 0xff)
			{
				cmd.msg_len = 5;
				command_array[0] = 0x71;
				command_array[4] = pin;
			}
			break;
		case UNICABLE2_ODU_CHANNEL_CHANGE_PIN:
			cmd.msg_len =5;
			command_array[0] = 0x71;
			command_array[1] = command_array[2]=0x70;
			command_array[3] = 0x70;
			command_array[4] = unicable_param->if_channel_index;
			break;
		case UNICABLE2_ODU_UB_AVAIL:
			cmd.msg_len = 1;
			cmd.msg[0] = 0x7a;
			break;
		case UNICABLE2_ODU_UB_AVAIL_REPLY:
			cmd.msg_len =5;
			cmd.msg[0] = 0x74;
			if(unicable_param->if_channel_index<8)
			{
				command_array[4] |= (unicable_param->if_channel_index<<1);
			}
			else if(unicable_param->if_channel_index<16)
			{
				command_array[3] |= (unicable_param->if_channel_index<<1);
			}
			else if(unicable_param->if_channel_index<24)
			{
				command_array[2] |= (unicable_param->if_channel_index<<1);
			}
			else
			{
				command_array[1] |= (unicable_param->if_channel_index<<1);
			}
			break;
		case UNICABLE2_ODU_UB_FREQU:
			cmd.msg_len = 2;
			cmd.msg[0]=0x7d;
			cmd.msg[1]=(unicable_param->if_channel_index<<3)&0xf8;
			break;
		case UNICABLE2_ODU_UB_FREQU_REPLY:
			cmd.msg_len = 3;
			cmd.msg[0]=0x74;
			tuning_word = unicable_param->if_fre - 100;
			command_array[2] = tuning_word & 0xFF;
			command_array[1] |= ((tuning_word & 0x700)>>8);
			command_array[1] |= (unicable_param->if_channel_index<<3);
			break;
		case UNICABLE2_ODU_UB_INUSE:
			cmd.msg_len = 1;
			cmd.msg[0]=0x7C;
			break;
		case UNICABLE2_ODU_UB_PIN:
			cmd.msg_len = 1;
			cmd.msg[0]=0x7B;
			break;
		case UNICABLE2_ODU_UB_SWITCHS:
			command_array[1] = (unicable_param->if_channel_index<<3)&0xf8;
			command_array[0] = 0x7e;
			cmd.msg_len = 2;
			break;
		case UNICABLE2_ODU_UB_SWITCHS_REPLY:
			cmd.msg_len = 3;
			command_array[0] = 0x74;
			command_array[1] = (unicable_param->if_channel_index<<3)&0xf8;
			command_array[2] = 0x0f;
			break;
		default:
			break;
	}

	//GxFrontend_Set22K(tuner, 1);//off

	for(i=0;i<cmd.msg_len;i++)
	{
		cmd.msg[i]=command_array[i];
	}

	GxFrontendSaveUnicableParams(tuner, &cmd);
	if(1 == gs_frontend_monitor[tuner])
	{
		return Error;
	}

	GxFrontend_SetVoltage(tuner, SEC_VOLTAGE_13);//POL_VERTICAL
	GxCore_ThreadDelay(50);
	GxFrontend_SetVoltage(tuner, SEC_VOLTAGE_18);//POL_HORIZONTAL
	GxCore_ThreadDelay(50);
	ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &cmd);
	GxCore_ThreadDelay(200);
	GxFrontend_SetVoltage(tuner, SEC_VOLTAGE_13);//POL_VERTICAL
	return(Error);
}


/* ----------------------------------------------------------------------------
Name: scr_scrdrv_SetFrequency()

Description:
	This routine will attempt to scan and find a QPSK signal based on the
	passed in parameters.

Parameters:
	InitialFrequency, IF value to commence scan (in kHz).
	SymbolRate,       required symbol bit rate (in Hz).
	MaxLNBOffset,     maximum allowed LNB offset (in Hz).
	TunerStep,        Tuner's step value -- enables override of
	TNR device's internal setting.
	DerotatorStep,    derotator step (usually 6).
	ScanSuccess,      boolean that indicates QPSK search success.
	NewFrequency,     pointer to area to store locked frequency.
	command_array[2] = tuning_word & 0xFF;
	temp = tuning_word & 0x700;
	temp = temp>>8;
	command_array[1] |= temp;
	temp = (params->lnb_index& 0x7f)<<2;//-Bank [2:0]

	Committed switches --3 7 b f
	Uncommitted switches--
Return Value:
---------------------------------------------------------------------------- */
int GxFrontend_SetUnicable(uint32_t tuner, GxFrontend *params)
{
	handle_t frontend;
	unsigned char command_array [5] = {0xE0, 0x10, 0x5A, 0x00, 0x00};
	struct dvb_diseqc_master_cmd cmd;
	int Error = 0;
	uint32_t tuning_word, temp;
	uint8_t i=0;

	frontend = gs_frontend[tuner].frontend;
	if(((params->lnb_index > 0x20)&&(0x80 != (params->lnb_index&0x80)))
			|| (params->lnb_index == 0x20) || (params->centre_fre ==0) || (params->if_fre==0))
	{
		GxFrontendClearUnicableParams(tuner);
		return (params->lnb_index);
	}

	if(0x80 == (params->lnb_index&0x80))//unicable2.0
	{
		command_array[0] = 0x70;
		tuning_word = params->centre_fre - 100;

		cmd.msg_len = 4;
		command_array[1] = params->if_channel_index << 3;
		command_array[1] |= ((tuning_word >> 8) & 0x07);
		command_array[2] = (tuning_word & 0xff);
		temp = params->lnb_index;
		command_array[3] = ((((temp&0x0C)>>2) & 0x3f) << 2) | (((temp&0x60)>>5) ? 2 : 0) | ((temp&0x01) ? 1 : 0);
		if (0)//(params->if_channel_index < 256)
		{
			cmd.msg_len = 5;
			command_array[0] = 0x71;
			command_array[4] = params->if_channel_index;
		}
		//GxFrontend_unicable2_odu_power_off(fe,params->if_channel_index);
		GxFrontend_unicable2_odu_scd2_commands(tuner, params, UNICABLE2_ODU_CHANNEL_CHANGE);
		return Error;
	}
	else//unicable1.0
	{
		params->centre_fre+= params->if_fre;
		/*
		if ((params->centre_fre % 4) < 2)
		params->centre_fre = params->centre_fre - params->centre_fre % 4;
		else
		params->centre_fre = params->centre_fre + (4 - params->centre_fre % 4);
		*/
		tuning_word = (params->centre_fre/4) - 350; /* Formula according to data sheet */

		command_array[4] = tuning_word & 0xFF;
		temp = tuning_word & 0x300;
		temp = temp>>8;
		command_array[3] |= temp;
		temp = params->if_channel_index<<5;
		command_array[3] |= temp;
		temp = (params->lnb_index & 0x7f)<<2;//-Bank [2:0]
		command_array[3] |= temp;
		cmd.msg_len =5;
	}
	//GxFrontend_Set22K(tuner, 1);//off

	for(i=0;i<cmd.msg_len;i++)
	{
		cmd.msg[i]=command_array[i];
	}

	GxFrontendSaveUnicableParams(tuner, &cmd);
	if(1 == gs_frontend_monitor[tuner])
	{
		return Error;
	}

	GxFrontend_SetVoltage(tuner, SEC_VOLTAGE_13);//POL_VERTICAL
	GxCore_ThreadDelay(50);
	GxFrontend_SetVoltage(tuner, SEC_VOLTAGE_18);//POL_HORIZONTAL
	GxCore_ThreadDelay(50);
	ioctl(frontend, FE_DISEQC_SEND_MASTER_CMD, &cmd);
	GxCore_ThreadDelay(200);
	GxFrontend_SetVoltage(tuner, SEC_VOLTAGE_13);//POL_VERTICAL
	return(Error);
}

#if 0
/* ----------------------------------------------------------------------------
Name: scr_scrdrv_Off()

Description:
It's to put  the SCR in off mode. Because during power on there are a lot of noise bands at the output.

Parameters:
OpenParams - To get the TopLevelHandle
DemodHandle -- Demod handle for DiSEqC operations.
LNBIndex -- LNB index number.
SCRBPF   -- Which SCR to switch off.

Return Value:Error
---------------------------------------------------------------------------- */
int GxFrontend_unicable2_odu_power_off(uint32_t tuner, uint8_t scrbpf)
{
	//TODO
	return 0;
}

/* ----------------------------------------------------------------------------
Name: scr_scrdrv_Off()

Description:
It's to put  the SCR in off mode. Because during power on there are a lot of noise bands at the output.

Parameters:
OpenParams - To get the TopLevelHandle
DemodHandle -- Demod handle for DiSEqC operations.
LNBIndex -- LNB index number.
SCRBPF   -- Which SCR to switch off.

Return Value:Error
---------------------------------------------------------------------------- */
int GxFrontend_unicable_odu_power_off(uint32_t tuner, uint8_t scrbpf)
{
	//TODO
	return 0;
}

int GxFrontend_unicable_satcr_tone_enable(uint32_t tuner)
{
	//TODO
	return 0;
}

int GxFrontend_unicable_odu_tone_enable_x(uint32_t tuner, uint32_t scr_index, uint32_t scr_nb)
{
	//TODO
	return 0;
}

static void GxFrontend_unicable_sat_cr_set_freq(uint32_t tuner, unsigned int mode_flags, int frequency_MHz)
{
	//TODO
	return;
}

static int GxFrontend_unicable_sat_cr_get_strength(uint32_t tuner)
{
	//TODO
	return 0;
}

static int GxFrontend_unicable_set_frq_get_agc_times(uint32_t tuner, unsigned int mode_flags, int freq)  //freq unit  MHZ
{
	//TODO
	return 0;
}

static unsigned int GxFrontend_unicable_calculate_agc_threshold(uint32_t tuner, unsigned int mode_flags, uint32_t StartFreq, uint32_t StopFreq)
{
	//TODO
	return 0;
}

static int GxFrontend_unicable_spectrum_analysis(uint32_t tuner, unsigned short *tone)
{
	//TODO
	return 0;
}
#endif
/*********************************** unicable end ******************************************/


int32_t  GxFrontend_SetTp(GxFrontend *para)
{
	int32_t fre,symb,qam,tuner,sametp=0,bandwidth,type_1501,sat22k,polar;
	int32_t            dev = -1;
	handle_t       demux = -1;
	handle_t frontend = -1;
	struct dvb_frontend_parameters params = { 0 };
	GxTime time;
	uint8_t    data_plp_id;
	uint8_t    common_plp_exist;
	uint8_t    common_plp_id;
	int32_t  re = 0;
	int32_t multistream_tag = 0;
	uint32_t ip = 0;
	uint32_t port = 0;
	socketParam_t socketParam = {0};
	int32_t pls_n = 0;
	uint8_t i = 0;
	GxFrontendType type;
	int32_t lnbf_tag = 0;
	uint32_t fre_min = 0, fre_max = 0;

	GxCore_GetTickTime(&time);
	//	gxlogd("start at %d:%d--------------------\n", time.seconds, time.microsecs);

	if(para==NULL || para->tuner>=FRONTEND_MAX)
		return -1;

	// temp 20120827
	//gxlogd("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>GxFrontend_SetTp-----\n");
	GxCore_MutexLock(gs_frontend_mutex[para->tuner]);
	fre       = para->fre;
	qam       = para->qam;
	symb      = para->symb;
	pls_n     = para->pls_n;
	polar     = para->polar;
	tuner     = para->tuner;
	sat22k    = para->sat22k;
	bandwidth = para->bandwidth;
	data_plp_id = para->data_plp_id;
	common_plp_exist = para->common_plp_exist;
	common_plp_id = para->common_plp_id;
	dev = para->dev;
	frontend = gs_frontend[tuner].frontend;

	if(para->type == FRONTEND_DVB_NET)
	{
		if(strcmp(gs_frontend[tuner].url, para->url) != 0){
			memcpy(gs_frontend[tuner].url, para->url, strlen(para->url)+1);
			if(net_ops.open){
				gs_tplock_flag[tuner] = TP_LOCK_START;
				g_frontend_set[tuner].set_tp = 1;//改变了频点，通知监控需要锁频
				g_frontend_set[tuner].set_unlock_ts = 0;//通知监控当前没有处于unlock ts 状态
				gs_strength[tuner] = 0;
				gs_frontend_quality[tuner].snr = 0;
				gs_frontend_quality[tuner].error_rate = 0;
				gs_frontend[tuner].demux = para->demux;
				if(net_ops.open(gs_frontend[tuner].url) < 0){
					re = -1;
					goto ret;
				}
			}else{
				gxloge("%s net frontend is not registered!\n", __func__);
				re = -1;
				goto ret;
			}
		}
		goto ret;
	}
	else
	{
		if(frontend < 0 )
		{
			gxlogd("open frontend err  frontend = %d!\n",frontend);
			re = -1;
			goto ret;
		}
		if(net_ops.close)//need close net frontend, while lock dvb
		{
			net_ops.close();
			for(i = 0; i < FRONTEND_MAX; i++)
			{
				type = GxFrontendGetType(i);
				if(FRONTEND_DVB_NET == type){
					memset(gs_frontend[i].url, 0, GXBUS_NETFRONTEND_MAX_URL_LENGTH);
					break;
				}
			}
		}
	}

	demux = para->demux;
	type_1501 = para->type_1501;
	switch(para->type){
		case FRONTEND_DVB_C:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbc_param.modulation = qam;
			params.u.dvbc_param.symbol_rate = symb;
			params.u.dvbc_param.timeout = 0;
			params.u.dvbc_param.tune_mode = para->tune_mode;

			if(gs_frontend[tuner].qam != qam ||
					gs_frontend[tuner].symb != symb ||
					gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].tune_mode != para->tune_mode||
					gs_frontend[tuner].frontend != frontend||
					gs_frontend[tuner].dev != dev)
			{
				gs_frontend[tuner].qam = qam;
				gs_frontend[tuner].symb = symb;
				gs_frontend[tuner].fre = fre;
				gs_frontend[tuner].tune_mode = para->tune_mode;
				gs_frontend[tuner].demux = demux;
				gs_frontend[tuner].frontend = frontend;
				gs_frontend[tuner].dev = dev;
				sametp = 0;
			}
			else
			{
				re = -2;
				sametp = 1;
			}
			//gxlogd("not support dvbc currently\n");
			//goto ret;
			break;
		case FRONTEND_DVB_S:
			params.frequency          = fre*1000;
			params.u.dvbs_param.symbol_rate = symb;
			params.u.dvbs_param.fec_inner   = FEC_AUTO;
			params.u.dvbs_param.pls_n       = pls_n;

			gs_frontend[tuner].centre_fre =  para->centre_fre;
			gs_frontend[tuner].if_fre = para->if_fre;
			gs_frontend[tuner].lnb_index =  para->lnb_index;
			gs_frontend[tuner].if_channel_index = para->if_channel_index;

			GxBus_ConfigGetInt(GXBUS_FRONTEND_MULTISTREAM,&multistream_tag,GXBUS_FRONTEND_MULTISTREAM_OFF);
			if(multistream_tag == GXBUS_FRONTEND_MULTISTREAM_ON)
			{
				params.u.dvbs_param.ts_code.ts_id= para->pls_ts_id;
				params.u.dvbs_param.ts_code.pls_code= para->pls_code;
				params.u.dvbs_param.ts_code.pls_change= para->pls_chanage;
				params.u.dvbs_param.ts_code.pls_status = para->pls_ts_status;

				gs_frontend[tuner].pls_ts_id = para->pls_ts_id;
				gs_frontend[tuner].pls_code = para->pls_code;
				gs_frontend[tuner].pls_chanage = para->pls_chanage;
				gs_frontend[tuner].pls_ts_status = para->pls_ts_status;
			}
			GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);//lnbf support #359333
			if(lnbf_tag == GXBUS_FRONTEND_LNBF_ON){
				fre_min = GXFRONTEND_LNBF_FRE_MIN;
				fre_max = GXFRONTEND_LNBF_FRE_MAX;
			}else{
				fre_min = GXFRONTEND_NORMAL_FRE_MIN;
				fre_max = GXFRONTEND_NORMAL_FRE_MAX;
			}
			if((params.u.dvbs_param.symbol_rate < 800)
					||(params.u.dvbs_param.symbol_rate > 66000)
					||(params.frequency < fre_min)
					||(params.frequency > fre_max))
			{
				gxlogd("DVB: out of range ");
				// add 20121016
				sg_TPParamError = 1;
				re = -1;
				//					ret = ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF);
				goto ret;
			}
			else
				sg_TPParamError = 0;

			ioctl(frontend, FE_SET_FRONTEND_TUNE_MODE, 0);
			if((gs_frontend[tuner].lnb_index<0x10) && (gs_frontend[tuner].centre_fre>0))
			{
				if(gs_frontend[tuner].centre_fre != para->centre_fre ||
						gs_frontend[tuner].fre != fre||
						gs_frontend[tuner].symb  != symb ||
						gs_frontend[tuner].polar != polar||
						gs_frontend[tuner].dev != dev||
						//gs_frontend[tuner].pls_num!= pls_n||
						gs_frontend[tuner].sat22k != sat22k)
				{
					gs_frontend[tuner].fre = fre;
					gs_frontend[tuner].symb = symb;
					gs_frontend[tuner].polar = polar;
					gs_frontend[tuner].dev = dev;
					gs_frontend[tuner].sat22k = sat22k;
					//gs_frontend[tuner].pls_num= pls_n;
					gs_frontend[tuner].demux = demux;

					gs_frontend[tuner].centre_fre =  para->centre_fre;
					gs_frontend[tuner].if_fre = para->if_fre;
					gs_frontend[tuner].lnb_index =  para->lnb_index;
					gs_frontend[tuner].if_channel_index = para->if_channel_index;
					sametp = 0;
				}
				else
				{
					re = -2;
					sametp = 1;
				}
			}
			else
			{
				if(gs_frontend[tuner].fre    != fre ||
						gs_frontend[tuner].symb  != symb ||
						gs_frontend[tuner].polar != polar||
						gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].pls_n != pls_n||
						gs_frontend[tuner].sat22k != sat22k)
				{
					gs_frontend[tuner].fre = fre;
					gs_frontend[tuner].symb = symb;
					gs_frontend[tuner].polar = polar;
					gs_frontend[tuner].dev = dev;
					gs_frontend[tuner].sat22k = sat22k;
				gs_frontend[tuner].pls_n = pls_n;

					//demux赋值放在判断里面复制会有tsdealy问题，但是放在判断外面会有更多的问题，因此选择放在判断里买呢
					//由应用处理ts dealy相关问题
					gs_frontend[tuner].demux = demux;
					sametp = 0;
				}
				else
				{
					re = -2;
					sametp = 1;
				}
			}
			break;
		case FRONTEND_DVB_T:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth||
					gs_frontend[tuner].frontend != frontend||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].demux != demux)
			{
				gs_frontend[tuner].fre = fre;
				gs_frontend[tuner].bandwidth = bandwidth;
				gs_frontend[tuner].demux = demux;
				gs_frontend[tuner].frontend = frontend;
				gs_frontend[tuner].dev = dev;
				sametp = 0;
			}
			else
			{
				re = -2;
				sametp = 1;
			}
			break;
		case FRONTEND_DTMB:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth ||
					gs_frontend[tuner].type_1501 != type_1501||
					gs_frontend[tuner].dev != dev)
			{
				gs_frontend[tuner].fre = fre;
				gs_frontend[tuner].bandwidth = bandwidth;
				if(gs_frontend[tuner].type_1501 != type_1501)
				{
					ioctl(frontend, FE_SET_FRONTEND_TUNE_MODE, para->type_1501);
					gs_frontend[tuner].type_1501 = type_1501;
				}
				gs_frontend[tuner].demux = demux;
				gs_frontend[tuner].dev = dev;
				sametp = 0;
			}
			else
			{
				re = -2;
				sametp = 1;
			}
			break;
		case FRONTEND_DVB_NET:
			if(gs_frontend[tuner].ip != ip||
				gs_frontend[tuner].port != port ||
				gs_frontend[tuner].dev != dev)
			{
				gs_frontend[tuner].ip = ip;
				gs_frontend[tuner].port = port;
				gs_frontend[tuner].demux = demux;
				gs_frontend[tuner].dev = dev;
				sametp = 0;
			}
			else
			{
				re = -2;
				sametp = 1;
			}
			gxfrontend_destroyFifo(demux);
			gxfrontend_createFifo(gs_frontend[tuner].hSocket, dev, demux);
			break;

		case FRONTEND_DVB_T2:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;
			params.u.dvbt_param.set_tp_mode = para->set_tp_mode;
			params.u.dvbt_param.work_mode = DVB_T_T2_AUTO;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].type_1501 != para->type_1501)
			{
				gs_frontend[tuner].fre = fre;
				gs_frontend[tuner].bandwidth = bandwidth;
				gs_frontend[tuner].demux = demux;
				gs_frontend[tuner].frontend = frontend;
				gs_frontend[tuner].dev = dev;
				gs_frontend[tuner].set_tp_mode = para->set_tp_mode;
				gs_frontend[tuner].data_plp_id = data_plp_id;
				gs_frontend[tuner].common_plp_exist = common_plp_exist;
				gs_frontend[tuner].common_plp_id = common_plp_id;
				/*add by zhangzhiyong for T2 frontend lock*/
				gs_frontend[tuner].type_1501 = para->type_1501;
				params.u.dvbt_param.code_rate_HP = data_plp_id;
				params.u.dvbt_param.code_rate_LP = common_plp_exist;
				params.u.dvbt_param.constellation = common_plp_id;
				sametp = 0;
			}
			else if((gs_frontend[tuner].data_plp_id != data_plp_id||
						gs_frontend[tuner].common_plp_exist != common_plp_exist||
						gs_frontend[tuner].common_plp_id != common_plp_id) &&
						(para->set_tp_mode == SET_MANUAL_PLP))
			{
				GxFrontend_Dvbt2SetPlp(tuner, data_plp_id, common_plp_exist, common_plp_id);
				re = -2;
				sametp = 1;
			}
			else
			{
				re = -2;
				sametp = 1;
			}
			break;
	case FRONTEND_ATSC_T:
		/*不用乘1000*/
		params.frequency = fre;
		params.inversion = INVERSION_OFF;
		params.u.dvbt_param.bandwidth = bandwidth;

		if(gs_frontend[tuner].fre != fre||
				gs_frontend[tuner].bandwidth != bandwidth||
				gs_frontend[tuner].frontend != frontend||
				gs_frontend[tuner].dev != dev||
				gs_frontend[tuner].demux != demux)
		{
			gs_frontend[tuner].fre = fre;
			gs_frontend[tuner].bandwidth = bandwidth;
			gs_frontend[tuner].demux = demux;
			gs_frontend[tuner].frontend = frontend;
			gs_frontend[tuner].dev = dev;
			sametp = 0;
		}
		else
		{
			re = -2;
			sametp = 1;
		}
		break;
	case FRONTEND_ATSC_C:
		/*不用乘1000*/
		params.frequency = fre;
		params.inversion = INVERSION_OFF;
		params.u.dvbc_param.modulation = qam;
		params.u.dvbc_param.symbol_rate = symb;

		if(gs_frontend[tuner].qam != qam ||
				gs_frontend[tuner].symb != symb ||
				gs_frontend[tuner].fre != fre||
				gs_frontend[tuner].frontend != frontend||
				gs_frontend[tuner].dev != dev||
				gs_frontend[tuner].demux != demux)
		{
			gs_frontend[tuner].qam = qam;
			gs_frontend[tuner].symb = symb;
			gs_frontend[tuner].fre = fre;
			gs_frontend[tuner].demux = demux;
			gs_frontend[tuner].frontend = frontend;
			gs_frontend[tuner].dev = dev;
			sametp = 0;
		}
		else
		{
			re = -2;
			sametp = 1;
		}

		break;
		default :
			goto ret;
	}

	gs_frontend[tuner].type      = para->type;

	if(!sametp){
//		GxFrontend_SetUnlock(tuner);
		gs_strength[tuner] = 0;
		gs_frontend_quality[tuner].snr = 0;
		gs_frontend_quality[tuner].error_rate = 0;
		if(gs_frontend_monitor[tuner] == 1)
		{
			g_frontend_set[tuner].set_tp = 1;//改变了频点，通知监控需要锁频
			g_frontend_set[tuner].set_unlock_ts = 0;//通知监控当前没有处于unlock ts 状态
			GxFrontendOccur(tuner);
			goto ret;
		}
		gs_tplock_flag[tuner] = TP_LOCK_START;
		if(para->type != FRONTEND_DVB_NET)
		{
			if (ioctl(frontend, FE_SET_FRONTEND, &params) == -1)
			{
				gxlogd("set frontend err\n");
				re = -1;
				goto ret;
			}
        }
	}
ret:
	g_check_count[tuner] = 0;
	gs_tplock_flag[tuner] = TP_LOCK_END;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	//GxCore_GetTickTime(&time);
	//gxlogd("end at %d:%d-------------------------\n", time.seconds, time.microsecs);
	return re;
}


int32_t  GxFrontend_Disconnect(GxFrontend *para, int force)
{
	int32_t fre,symb,qam,tuner,sametp=0,bandwidth,type_1501,sat22k,polar;
	int32_t            dev;
	handle_t       demux;
	handle_t frontend;
	struct dvb_frontend_parameters params = { 0 };
	uint8_t    data_plp_id;
	uint8_t    common_plp_exist;
	uint8_t    common_plp_id;
	int32_t  re = 0;
	int32_t tempfre = 0;

	tuner = para->tuner;
	if(tuner>=FRONTEND_MAX)
	{
		return -1;
	}

	frontend = gs_frontend[tuner].frontend;
	if(frontend < 0)
	{
		gxlogd("open frontend err  frontend = %d!\n",frontend);
		return -1;
	}

	if(force)
	{
		GxCore_MutexLock(gs_frontend_mutex[tuner]);
		g_frontend_set[tuner].set_unlock_ts = 1;//ts unlock状态，接下来需要改变频点了。这时候监控不能去锁频
		GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
		re = ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF+1);// OFF = 2, TS UNLOCK = 3,need match with demod driver
		if(re == -1)
		{
			gxlogd("set unlock err\n");
			return -1;
		}
		return 0;
	}

	if(para==NULL || para->tuner>=FRONTEND_MAX)
		return -1;

	GxCore_MutexLock(gs_frontend_mutex[para->tuner]);

	fre       = para->fre;
	qam       = para->qam;
	symb      = para->symb;
	polar     = para->polar;
	tuner     = para->tuner;
	sat22k    = para->sat22k;
	bandwidth = para->bandwidth;
	data_plp_id = para->data_plp_id;
	common_plp_exist = para->common_plp_exist;
	common_plp_id = para->common_plp_id;
	dev = para->dev;
	demux = para->demux;
	type_1501 = para->type_1501;
	sametp = 1;
	switch(para->type){
		case FRONTEND_DVB_C:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbc_param.modulation = qam;
			params.u.dvbc_param.symbol_rate = symb;

			if(gs_frontend[tuner].qam != qam ||
					gs_frontend[tuner].symb != symb ||
					gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].frontend != frontend||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].demux != demux)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DVB_S:
			params.frequency          = fre*1000;
			params.u.dvbs_param.symbol_rate = symb*1000;
			params.u.dvbs_param.fec_inner   = FEC_AUTO;
			if((gs_frontend[tuner].lnb_index<0x10) && (gs_frontend[tuner].centre_fre>0))
				tempfre = gs_frontend[tuner].centre_fre;
			else
				tempfre = gs_frontend[tuner].fre;

			if(/*gs_frontend[tuner].fre*/tempfre    != fre ||
					gs_frontend[tuner].symb  != symb ||
					gs_frontend[tuner].polar != polar||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].sat22k != sat22k)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DVB_T:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth||
					gs_frontend[tuner].frontend != frontend||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].demux != demux)
			{
				sametp = 0;
			}
			break;
		case FRONTEND_DTMB:
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbc_param.symbol_rate = symb;
			//params.u.dvbc_param.fec_inner = FEC_AUTO;
			params.u.dvbc_param.modulation = DVBC_32QAM;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth ||
					gs_frontend[tuner].type_1501 != type_1501||
					gs_frontend[tuner].dev != dev)
			{
				sametp = 0;
			}
			break;

		case FRONTEND_DVB_T2:
			//params.frequency = fre;
			params.frequency = fre*1000;
			params.inversion = INVERSION_OFF;
			params.u.dvbt_param.bandwidth = bandwidth;

			if(gs_frontend[tuner].fre != fre||
					gs_frontend[tuner].bandwidth != bandwidth||
					gs_frontend[tuner].dev != dev||
					gs_frontend[tuner].data_plp_id != data_plp_id||
					gs_frontend[tuner].common_plp_exist != common_plp_exist||
					gs_frontend[tuner].common_plp_id != common_plp_id)
			{
				sametp = 0;
			}
			break;
		default :
			goto ret;
	}

	if(!sametp){
		g_frontend_set[tuner].set_unlock_ts = 1;//ts unlock状态，接下来需要改变频点了。这时候监控不能去锁频
		re = ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF+1);//OFF = 2; TS UNLOCK = 3
		if(re == -1)
		{
			gxlogd("set unlock err\n");
		}
	}
ret:
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return re;
}

void GxFrontend_StopMonitor(int32_t tuner)
{
	if(tuner>=FRONTEND_MAX)
		return ;

	GxCore_MutexLock(gs_frontend_mutex[tuner]);

	gs_frontend_monitor[tuner] = 0;

	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	GxCore_ThreadDelay(20);

}

void GxFrontend_StartMonitor(int32_t tuner)
{
	if(tuner>=FRONTEND_MAX)
		return ;

	GxCore_MutexLock(gs_frontend_mutex[tuner]);

	gs_frontend_monitor[tuner] = 1;

	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
}

int32_t GxFrontend_QueryMonitor(int32_t tuner)
{
	if(tuner>=FRONTEND_MAX)
		return 0;
	return gs_frontend_monitor[tuner];
}

static int32_t gxfrontend_get_net_status(void)
{
	uint32_t status = 0;
	if(net_ops.read_status){
		if(net_ops.read_status(&status) < 0){
			gxloge("%s net frontend get status error!\n", __func__);
			return -1;
		}else{
			if(status == FE_HAS_LOCK)
				return 1;//lock
		}
	}else{
		gxloge("%s net frontend is not registered!\n", __func__);
		return -1;
	}
	return 0;//unlock
}

int32_t GxFrontend_QueryFrontendStatusMonitor(int32_t tuner)
{
	fe_status_t festatus;
	int32_t ret = -1;

	if(tuner>=FRONTEND_MAX||
			gs_frontend[tuner].dev<0||
			gs_frontend[tuner].demux<0||
			gs_frontend[tuner].frontend<0)
		return -1;

	if(gs_frontend[tuner].type == FRONTEND_DVB_NET)
		return gxfrontend_get_net_status();

	if((ret = ioctl(gs_frontend[tuner].frontend, FE_READ_STATUS, &festatus)) >= 0)
	{
		if(festatus == FE_HAS_LOCK)
		{
			//gxlogd("========tuner locked ===========\n");
			gs_frontend_status[tuner] = 1;
			return 1;

		}
		else if(festatus == FE_HAS_CARRIER)
		{
			gs_frontend_status[tuner] = 0;
			return 2;
		}
	}
	else
	{
		gs_frontend_status[tuner] = 0;
		return -1;
	}

	gs_frontend_status[tuner] = 0;
	return 0;
}

int32_t GxFrontend_QueryFrontendStatus(int32_t tuner)
{
	fe_status_t festatus;
	int32_t ret = -1;

	if(tuner>=FRONTEND_MAX||
			gs_frontend[tuner].dev<0||
			gs_frontend[tuner].demux<0||
			gs_frontend[tuner].frontend<0)
		return -1;

	if(gs_frontend[tuner].type == FRONTEND_DVB_NET)
		return gxfrontend_get_net_status();

	// add 20121016
	if(1 == sg_TPParamError)
	{
		return -1;
	}
	if(gs_frontend_monitor[tuner] == 1)
	{
		return gs_frontend_status[tuner];
	}

	if((ret = ioctl(gs_frontend[tuner].frontend, FE_READ_STATUS, &festatus)) >= 0)
	{
		if(festatus  == FE_HAS_LOCK)
		{
			//         gxlogd("========tuner locked ===========\n");
			return 1;

		}
		else if(festatus== FE_HAS_CARRIER)
		{
			return 2;
		}

	}
	else
	{
		return -1;
	}

	return 0;
}

int32_t GxFrontend_QueryStatus(int32_t tuner)
{
	GxDemuxProperty_TSLockQuery ts_lock_status = {TS_SYNC_UNLOCKED};
	int32_t ret = -1;

	if(tuner>=FRONTEND_MAX||
			gs_frontend[tuner].dev<0||
			gs_frontend[tuner].demux<0||
			gs_tplock_flag[tuner] == TP_LOCK_START||
			gs_frontend[tuner].frontend<0)
		return -1;

	if(gs_frontend[tuner].type == FRONTEND_DVB_NET)
		return gxfrontend_get_net_status();

	ret = GxAVGetProperty(gs_frontend[tuner].dev,
			gs_frontend[tuner].demux,
			GxDemuxPropertyID_TSLockQuery,
			&ts_lock_status,
			sizeof(GxDemuxProperty_TSLockQuery));
	if(ret < 0)
		return -1;

	return (ts_lock_status.ts_lock==TS_SYNC_LOCKED?1:0);
}

int32_t GxFrontend_SetTpInternal(int32_t tuner, GxFrontend_MonitorParams *params)
{
	int32_t re = 0;
	struct dvb_frontend_parameters params_set = { 0 };
	int32_t multistream_tag = 0;
	int32_t lnbf_tag = 0;
	uint32_t fre_min = 0, fre_max = 0;
	if(tuner>=FRONTEND_MAX||
			params->params[tuner].dev<0||
			params->params[tuner].frontend<0)
		return -1;

	if((params->set_status[tuner].set_tp == 0) || (params->set_status[tuner].set_unlock_ts == 1))//如果没有需要锁频或者是处于unlock ts状态，都不能去锁频
	{
		return -1;
	}
	else
	{
		params->set_status[tuner].set_tp = 0;
	}
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	/*	if((GxFrontend_QueryStatus(tuner) == 1)
		&& (GxFrontend_QueryFrontendStatus(tuner) == 1))
		{
	//gxlogd("\n+++++++++++++++++++++++++++++++++++\n");
	goto ret;
	}*/
	//gxlogd("\n--[GxFrontend_SetTpInternal][2]--\n");
	gs_tplock_flag[tuner] = TP_LOCK_START;

	switch(params->params[tuner].type){
		case FRONTEND_DVB_C:
			params_set.frequency                = params->params[tuner].fre*1000;
			params_set.inversion                = INVERSION_OFF;
			params_set.u.dvbc_param.modulation  = params->params[tuner].qam;
			params_set.u.dvbc_param.symbol_rate = params->params[tuner].symb;
			params_set.u.dvbc_param.timeout     = 0;
			params_set.u.dvbc_param.tune_mode   = params->params[tuner].tune_mode;
			//gxlogd("not support dvbc currently\n");
			//goto ret;
			break;

		case FRONTEND_DVB_S:
			params_set.frequency                = params->params[tuner].fre*1000;
			params_set.u.dvbs_param.symbol_rate = params->params[tuner].symb;
			params_set.u.dvbs_param.fec_inner   = FEC_AUTO;
			params_set.u.dvbs_param.pls_n       = params->params[tuner].pls_n;
			GxBus_ConfigGetInt(GXBUS_FRONTEND_MULTISTREAM,&multistream_tag,GXBUS_FRONTEND_MULTISTREAM_OFF);
			if(multistream_tag == GXBUS_FRONTEND_MULTISTREAM_ON)
			{
				params_set.u.dvbs_param.ts_code.ts_id      = params->params[tuner].pls_ts_id;
				params_set.u.dvbs_param.ts_code.pls_code   = params->params[tuner].pls_code;
				params_set.u.dvbs_param.ts_code.pls_change = params->params[tuner].pls_chanage;
				params_set.u.dvbs_param.ts_code.pls_status = params->params[tuner].pls_ts_status;
			}
			GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);//lnbf support #359333
			if(lnbf_tag == GXBUS_FRONTEND_LNBF_ON){
				fre_min = GXFRONTEND_LNBF_FRE_MIN;
				fre_max = GXFRONTEND_LNBF_FRE_MAX;
			}else{
				fre_min = GXFRONTEND_NORMAL_FRE_MIN;
				fre_max = GXFRONTEND_NORMAL_FRE_MAX;
			}
			if((params_set.u.dvbs_param.symbol_rate < 800)
					||(params_set.u.dvbs_param.symbol_rate > 66000)
					||(params_set.frequency < fre_min)
					||(params_set.frequency > fre_max))
			{
				gxlogd("DVB[1]: out of range ");

				//ret = ioctl (params->params[tuner].frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF);
				re = -1;
				goto ret;
			}
			else
				sg_TPParamError = 0;

			break;

		case FRONTEND_DVB_T:
			params_set.frequency              = params->params[tuner].fre*1000;
			params_set.inversion              = INVERSION_OFF;
			params_set.u.dvbt_param.bandwidth = params->params[tuner].bandwidth;
			break;
		case FRONTEND_DTMB:
			params_set.frequency                  = params->params[tuner].fre*1000;
			params_set.inversion                  = INVERSION_OFF;
			params_set.u.dvbt_param.constellation = params->params[tuner].qam;
			params_set.u.dvbt_param.bandwidth     = params->params[tuner].bandwidth;
			break;
		case FRONTEND_DVB_T2:
			params_set.frequency                  = params->params[tuner].fre*1000;
			params_set.inversion                  = INVERSION_OFF;
			params_set.u.dvbt_param.bandwidth     = params->params[tuner].bandwidth;
			params_set.u.dvbt_param.code_rate_HP  = params->params[tuner].data_plp_id;
			params_set.u.dvbt_param.code_rate_LP  = params->params[tuner].common_plp_exist;
			params_set.u.dvbt_param.constellation = params->params[tuner].common_plp_id;
			params_set.u.dvbt_param.set_tp_mode   = params->params[tuner].set_tp_mode;
			if(params_set.u.dvbt_param.code_rate_HP == 0xff
				&& params_set.u.dvbt_param.code_rate_LP == 0xff
				&& params_set.u.dvbt_param.constellation == 0xff)
			{
				if(gs_frontend[tuner].type_1501 == 1)
					params_set.u.dvbt_param.work_mode = DVB_T_ONLY;
				else
					params_set.u.dvbt_param.work_mode = DVB_T_T2_AUTO;
			}
			else
			{
				params_set.u.dvbt_param.work_mode = DVB_T2_ONLY;
			}
			break;
		case FRONTEND_DVB_NET:
			if(net_ops.open){
				if(net_ops.open(params->params[tuner].url) < 0){
					re = -1;
					goto ret;
				}
			}else{
				gxloge("%s net frontend is not registered!\n", __func__);
				re = -1;
				goto ret;
			}
			break;

		default :
			goto ret;
	}

	gs_tplock_flag[tuner] = TP_LOCK_START;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);

	if(params->params[tuner].type != FRONTEND_DVB_NET){
		if (ioctl(params->params[tuner].frontend, FE_SET_FRONTEND, &params_set) == -1)
		{
			gxlogd("set frontend err\n");
			GxCore_MutexLock(gs_frontend_mutex[tuner]);
			re = -1;
			goto ret;
		}
	}
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
#ifdef LINUX_OS
	GxCore_ThreadDelay(10);
#endif
ret:
	gs_tplock_flag[tuner] = TP_LOCK_END;
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	return re;
}

uint32_t GxFrontend_GetStrengthMonitor(int32_t tuner)
{
	int32_t s = 0;
	if(tuner>=FRONTEND_MAX||
			gs_frontend[tuner].dev<0||
			gs_frontend[tuner].frontend<0||
			gs_frontend_strenth_quality == 0)
		return 0;

	if(ioctl(gs_frontend[tuner].frontend,FE_READ_SIGNAL_STRENGTH,&s) >= 0)
	{
		//gs_strength[tuner] = (0xffff-s)/0xffff;
		//gs_strength[tuner] = s/16*100/4096;
		gs_strength[tuner] = s;
	}
	else
	{
		gxlogd("get strenth err !!!!!\n");
	}

	return gs_strength[tuner];
}

uint32_t GxFrontend_GetStrength(int32_t tuner)
{
	int32_t s = 0;
	if(tuner>=FRONTEND_MAX||
			gs_frontend[tuner].dev<0||
			gs_frontend[tuner].frontend<0)
		return 0;
	// add 20121016
	if(1 == sg_TPParamError)
	{
		gs_strength[tuner] = 0;
		return 0;
	}
	if(gs_frontend_monitor[tuner] == 1 ||
			gs_tplock_flag[tuner] == TP_LOCK_START)
	{
		return gs_strength[tuner];
	}

	if(ioctl(gs_frontend[tuner].frontend,FE_READ_SIGNAL_STRENGTH,&s) >= 0)
	{
		//gs_strength[tuner] = (0xffff-s)/0xffff;
		//gs_strength[tuner] = s/16*100/4096;
		gs_strength[tuner] = s;
	}
	else
	{
		gxlogd("get strenth err !!!!!\n");
	}
	return gs_strength[tuner];
}

int32_t GxFrontend_GetQualityMonitor(int32_t tuner,GxFrontendSignalQuality* sq)
{
	int32_t SNR = 0;
	int32_t BitError = 0;

	if(tuner >= FRONTEND_MAX ||
			gs_frontend[tuner].dev < 0 ||
			gs_frontend[tuner].frontend < 0 ||
			gs_frontend_strenth_quality == 0)
		goto err;

	// add 20121016
	if(1 == sg_TPParamError)
	{
		gs_frontend_quality[tuner].snr = 0;
		gs_frontend_quality[tuner].error_rate = 0;
		if(sq != NULL)
		{
			sq->snr = gs_frontend_quality[tuner].snr;
			sq->error_rate = gs_frontend_quality[tuner].error_rate;
		}
		return 0;
	}

	if(ioctl(gs_frontend[tuner].frontend,FE_READ_BER,&BitError) < 0)
	{
		gxlogd("get bit err rate err!!\n");
		goto err;
	}
	if(ioctl(gs_frontend[tuner].frontend,FE_READ_SNR,&SNR) < 0)
	{
		gxlogd("get SNR err!!\n");
		goto err;
	}
	if(sq != NULL)
	{
		//sq->snr = SNR/0xffff;
		//sq->error_rate = BitError/0xffffffff;
		sq->snr = SNR;
		sq->error_rate = BitError;
	}
	//gs_frontend_quality[tuner].snr = SNR/0xffff;
	//gs_frontend_quality[tuner].error_rate = BitError/0xffffffff;
	gs_frontend_quality[tuner].snr = SNR;
	gs_frontend_quality[tuner].error_rate = BitError;
	return 0;
err:
	return -1;
}

int32_t GxFrontend_GetQuality(int32_t tuner,GxFrontendSignalQuality* sq)
{
	int32_t SNR = 0;
	int32_t BitError = 0;

	if(tuner >= FRONTEND_MAX ||
			gs_frontend[tuner].dev < 0 ||
			gs_frontend[tuner].frontend < 0 )
		goto err;
	// add 20121016
	if(1 == sg_TPParamError)
	{
		gs_frontend_quality[tuner].snr = 0;
		gs_frontend_quality[tuner].error_rate = 0;
		if(sq != NULL)
		{
			sq->snr = gs_frontend_quality[tuner].snr;
			sq->error_rate = gs_frontend_quality[tuner].error_rate;
		}
		return 0;
	}
	if(gs_frontend_monitor[tuner] == 1 ||
			gs_tplock_flag[tuner] == TP_LOCK_START)
	{
		if(sq != NULL)
		{
			sq->snr = gs_frontend_quality[tuner].snr;
			sq->error_rate = gs_frontend_quality[tuner].error_rate;
			return 0;
		}
		else
		{
			goto err;
		}
	}
	if(ioctl(gs_frontend[tuner].frontend,FE_READ_BER,&BitError) < 0)
	{
		gxlogd("get bit err rate err!!\n");
		goto err;
	}
	if(ioctl(gs_frontend[tuner].frontend,FE_READ_SNR,&SNR) < 0)
	{
		gxlogd("get SNR err!!\n");
		goto err;
	}
	if(sq != NULL)
	{
		//sq->snr = SNR/0xffff;
		//sq->error_rate = BitError/0xffffffff;
		sq->snr = SNR;
		sq->error_rate = BitError;
	}
	//gs_frontend_quality[tuner].snr = SNR/0xffff;
	//gs_frontend_quality[tuner].error_rate = BitError/0xffffffff;
	gs_frontend_quality[tuner].snr = SNR;
	gs_frontend_quality[tuner].error_rate = BitError;
	return 0;
err:
	return -1;
}
int32_t GxFrontend_CleanRecord(int32_t tuner)
{
	handle_t fd = 0;
	if(tuner >= FRONTEND_MAX)
		return -1;
	fd = gs_frontend[tuner].frontend;
/*	if(gs_frontend[tuner].frontend != -1)
	{
		close(gs_frontend[tuner].frontend);//如果这里关闭前端，可能会多线程调用open，导致前端控制块丢失，长时间操作后前端再也无法打开。况且，确实没有必要关闭，还可以加快切台速度
	}*/
	memset(&gs_frontend[tuner],0,sizeof(GxFrontend));
	gs_frontend[tuner].dev = -1;
	gs_frontend[tuner].frontend = fd;
	gs_frontend[tuner].demux = -1;
	return 0;
}


void GxFrontend_InitNet(void)
{
	gxfrontend_initFifo();
	gxFrontendInitNet();
}

int32_t GxFrontend_CloseNet(int32_t tuner)
{
	if(gs_frontend[tuner].hSocket != 0)
	{
		if(gs_frontend[tuner].type == FRONTEND_DVB_NET)
		{
			gxFrontendDestroySocketFd((void*)(gs_frontend[tuner].hSocket));
			gs_frontend[tuner].hSocket = 0;
		}
	}
	return 0;
}

int32_t GxFrontend_NetStreamClose(int32_t tuner)
{
	if(tuner >= FRONTEND_MAX){
		gxloge("close net err tuner out of range!\n");
		return -1;
	}
	if(gs_frontend[tuner].type == FRONTEND_DVB_NET){
		if(net_ops.close){
			net_ops.close();
			return 0;
		}
	}
	return -1;
}

int32_t GxFrontend_NetStreamPause(int32_t tuner)
{
	if(tuner >= FRONTEND_MAX){
		gxloge("net pause err tuner out of range!\n");
		return -1;
	}
	if(gs_frontend[tuner].type == FRONTEND_DVB_NET){
		if(net_ops.pause_get_data){
			net_ops.pause_get_data();
			return 0;
		}
	}
	return -1;
}

int32_t GxFrontend_NetStreamContinue(int32_t tuner)
{
	if(tuner >= FRONTEND_MAX){
		gxloge("net continue err tuner out of range!\n");
		return -1;
	}
	if(gs_frontend[tuner].type == FRONTEND_DVB_NET){
		if(net_ops.continue_get_data){
			net_ops.continue_get_data();
			return 0;
		}
	}
	return -1;
}

int32_t GxFrontend_CleanRecordPara(int32_t tuner)
{
	if(tuner >= FRONTEND_MAX)
		return -1;

	gs_frontend[tuner].sat22k = -1;
	gs_frontend[tuner].fre = -1;
	gs_frontend[tuner].symb = -1;
	return 0;
}

int32_t GxFrontend_GetCurFre(int32_t tuner, GxFrontend* fre)
{
	if(tuner >= FRONTEND_MAX||
			fre == NULL)
		return -1;

	fre->dev       = gs_frontend[tuner].dev;
	fre->demux     = gs_frontend[tuner].demux;
	fre->tuner     = gs_frontend[tuner].tuner;
	fre->fre       = gs_frontend[tuner].fre;
	fre->symb      = gs_frontend[tuner].symb;
	fre->sat22k    = gs_frontend[tuner].sat22k;
	fre->qam       = gs_frontend[tuner].qam;
	fre->polar     = gs_frontend[tuner].polar;
	fre->bandwidth = gs_frontend[tuner].bandwidth;
	fre->type      = gs_frontend[tuner].type;
	fre->type_1501 = gs_frontend[tuner].type_1501;
	fre->common_plp_id    = gs_frontend[tuner].common_plp_id;
	fre->common_plp_exist = gs_frontend[tuner].common_plp_exist;
	fre->data_plp_id      = gs_frontend[tuner].data_plp_id;

	fre->centre_fre = gs_frontend[tuner].centre_fre;
	fre->if_fre = gs_frontend[tuner].if_fre;
	fre->lnb_index = gs_frontend[tuner].lnb_index;
	fre->if_channel_index  = gs_frontend[tuner].if_channel_index;

	fre->pls_ts_id = gs_frontend[tuner].pls_ts_id;
	fre->pls_code = gs_frontend[tuner].pls_code;
	fre->pls_chanage = gs_frontend[tuner].pls_chanage;
	fre->pls_ts_status = gs_frontend[tuner].pls_ts_status;
	return 0;
}

int32_t GxFrontend_GetInfo(int32_t frontend, GxFrontendInfo* info)
{
	struct dvb_frontend_parameters params = { 0 };
	struct dvb_frontend_info frontend_info = {{0}};

	if(ioctl(frontend,FE_GET_INFO, &frontend_info) >= 0)
	{
		if(frontend_info.type == FE_DVB_S)
		{
			if(ioctl(frontend,FE_GET_FRONTEND,&params)>=0)
			{
				info->type = params.u.dvbs_param.type;
				info->modulation = params.u.dvbs_param.modulation;
			}
		}
		else if(frontend_info.type == FE_DVB_T)
		{
			if(ioctl(frontend,FE_GET_FRONTEND,&params)>=0)
			{
				info->type = params.u.dvbt_param.type;
				info->modulation = params.u.dvbt_param.constellation;
			}
		}
		else if(frontend_info.type == FE_DVB_C)
		{
			if(ioctl(frontend,FE_GET_FRONTEND,&params)>=0)
			{
				info->modulation  = params.u.dvbc_param.modulation;
				info->symbol_rate = params.u.dvbc_param.symbol_rate;
			}
		}
		else if(frontend_info.type == FE_ISDB_T)
		{
			info->type = FE_DVB_T;
		}
	}
	else
		return -1;
	return 0;
}

bool GxFrontend_HasT2MIFeatures(int32_t tuner)
{
	struct dvb_frontend_info info = {{0},};

	if(tuner >= FRONTEND_MAX
        || gs_frontend[tuner].frontend < 0)
    {
        return false;
    }

    GxFrontend_GetDemodInfo(tuner, &info);
    if((info.caps & FE_CAN_T2MI) > 0)
    {
        return true;
    }

    return false;
}

handle_t GxFrontend_IdToHandle(int32_t tuner)
{
	if(tuner >= FRONTEND_MAX)
		return -1;
	return gs_frontend[tuner].frontend;
}

void GxFrontend_Set22KMonitor(int32_t tuner, GxFrontend_MonitorParams *params)
{
	handle_t frontend;
	int32_t ret = 0;
	GxFrontendType type = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set 22K err tuenr out range!\n");
		return ;
	}
	type = GxFrontendGetType(tuner);
	if((type != FRONTEND_DVB_S) && (type != FRONTEND_DVB_S2))
		return;
	if(params->tone[tuner].flag == 0)
	{
		return;
	}
	params->tone[tuner].flag = 0;
	frontend = params->params[tuner].frontend;
	if(params->tone[tuner].sat22k == 0)
	{
		ret = ioctl (frontend, FE_SET_TONE, SEC_TONE_ON);
	}
	else
	{
		ret = ioctl (frontend, FE_SET_TONE, SEC_TONE_OFF);
	}
	if(ret == -1)
	{
		gxlogd("set frontend 22K err\n");
	}

	return;
}

bool GxFrontend_HasSpecialFeatures(int32_t tuner)
{
	struct dvb_frontend_info info = {{0},};

	if(tuner >= FRONTEND_MAX
		|| gs_frontend[tuner].frontend < 0)
	{
		return false;
	}

	GxFrontend_GetDemodInfo(tuner, &info);
	if((info.caps & FE_CAN_PLS) > 0)
	{
		return true;
	}

	return false;
}

bool GxFrontend_HasAutoSpecialFeatures(int32_t tuner)
{
	struct dvb_frontend_info info = {{0},};

	if(tuner >= FRONTEND_MAX
		|| gs_frontend[tuner].frontend < 0)
	{
		return false;
	}

	GxFrontend_GetDemodInfo(tuner, &info);
	if((info.caps & FE_CAN_AUTO_PLS) > 0)
	{
		return true;
	}

	return false;
}

int32_t GxFrontend_GetTsid(uint32_t tuner, struct dvbs_tsid_info *params)
{
	int err = -1;

	if(tuner >= FRONTEND_MAX
		|| gs_frontend[tuner].frontend < 0
		|| params == NULL )
	{
		return -1;
	}

	if(!GxFrontend_HasSpecialFeatures(tuner))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend, FE_GET_MULTI_TSID, params);
	if(err < 0)
		gxlogd("%s error!", __FUNCTION__);

	return err;
}

void GxFrontend_SetTsid(uint32_t tuner, uint8_t ts_id, uint8_t code)
{
	handle_t frontend;
	struct dvbs_tsid_parameters params = {0};

	if(tuner>=FRONTEND_MAX)
	{
		return;
	}
	gs_frontend[tuner].pls_ts_id = ts_id;
	gs_frontend[tuner].pls_code = code;
	frontend = gs_frontend[tuner].frontend;
	params.ts_id = ts_id;
	params.pls_code = code;
	ioctl(frontend, FE_SET_MULTI_TSID, &params);

	return;
}

int32_t GxFrontend_GetAutoPlsNum(int32_t tuner, unsigned int *num)
{
	uint32_t err = 0;

	if((tuner >= FRONTEND_MAX)
		|| (gs_frontend[tuner].frontend < 0)
		|| (num == NULL))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend,FE_GET_PLS_N,num);
	if(err != 0)
	{
		*num = 0;
	}

	return err;
}

int32_t GxFrontend_SetAutoPlsNum(int32_t tuner, unsigned int num)
{
	int32_t err = 0;

	if(tuner >= FRONTEND_MAX
		|| gs_frontend[tuner].frontend < 0)
	{
		return -1;
	}

	if(!GxFrontend_HasSpecialFeatures(tuner))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend, FE_SET_PLS_N, num);

	return err;
}

int32_t GxFrontend_PlsNSourceSelect(int32_t tuner, fe_plsn_source_sel source)
{
	uint32_t err = 0;

	if((tuner >= FRONTEND_MAX)
		|| (gs_frontend[tuner].frontend < 0))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend, FE_PLSN_SOURCE_SELECT, source);

	return err;
}

int32_t GxFrontend_PlsNRootConvert(int32_t tuner, struct dvbs_plsn_convert_param *param)
{
	uint32_t err = 0;

	if((tuner >= FRONTEND_MAX)
		|| (gs_frontend[tuner].frontend < 0)
		|| (param == NULL))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend, FE_PLSN_ROOT_CONVERT, param);

	return err;
}

void GxFrontend_Set22K(int32_t tuner, int32_t sat22k)
{
	handle_t frontend;
	int32_t ret = 0;
	//return;
	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set 22K err tuenr out range!\n");
		return ;
	}

	GxFrontendSet22k(tuner,sat22k);
	if(gs_frontend_monitor[tuner] == 1)
	{
		return;
	}

	frontend = gs_frontend[tuner].frontend;
	if(sat22k == 0)
	{
		ret = ioctl (frontend, FE_SET_TONE, SEC_TONE_ON);
	}
	else
	{
		ret = ioctl (frontend, FE_SET_TONE, SEC_TONE_OFF);
	}
	if(ret == -1)
	{
		gxlogd("set frontend 22K err\n");
	}

	return;
}

void GxFrontend_SetVoltageMonitor(int32_t tuner, GxFrontend_MonitorParams *params)
{
	handle_t frontend;
	int32_t ret = 0;
	GxFrontendType type = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set voltage err tuenr out range!\n");
		return ;
	}
	type = GxFrontendGetType(tuner);
	if((type != FRONTEND_DVB_S) && (type != FRONTEND_DVB_S2))
		return;
	if(params->polar[tuner].flag == 0 || 1 == params->unicable[tuner].flag)
	{
		return;
	}
	params->polar[tuner].flag = 0;

	frontend = params->params[tuner].frontend;
	ret = ioctl (frontend, FE_SET_VOLTAGE, params->polar[tuner].polar);
	if(ret == -1)
	{
		gxlogd("set frontend voltage err\n");
	}

	return;
}

void GxFrontend_SetVoltage(int32_t tuner, fe_sec_voltage_t polar)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set voltage err tuenr out range!\n");
		return ;
	}

	GxFrontendSetPolar(tuner,polar);
	if(gs_frontend_monitor[tuner] == 1)
	{
		return;
	}

	frontend = gs_frontend[tuner].frontend;
	if(polar == SEC_VOLTAGE_13 )
	{
		ret = ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
	}
	else if(polar == SEC_VOLTAGE_18)
	{
		ret = ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
	}
	else if(polar == SEC_VOLTAGE_OFF)
	{
		ret = ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF);
	}
	if(ret == -1)
	{
		gxlogd("set frontend voltage err\n");
	}

	return;
}

void GxFrontendClearErrFreFlag(void)
{
	sg_TPParamError = 0;
}

void GxFrontend_SetUnlock(int32_t tuner)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set voltage err tuenr out range!\n");
		return ;
	}

	frontend = gs_frontend[tuner].frontend;
	GxCore_MutexLock(gs_frontend_mutex[tuner]);
	g_frontend_set[tuner].set_unlock_ts = 1;//ts unlock状态，接下来需要改变频点了。这时候监控不能去锁频
	GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	ret = ioctl (frontend, FE_TS_OUTPUT_CONTRL, 0);
	if(ret == -1)
	{
		gxlogd("set unlock err\n");
	}

	return;
}

void GxFrontend_TsOutCtrl(int32_t tuner, uint32_t enable)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set ts out ctrl err tuenr out range!\n");
		return ;
	}

	frontend = gs_frontend[tuner].frontend;
	enable = enable ? 1 : 0;
	ret = ioctl (frontend, FE_TS_OUTPUT_CONTRL, enable);
	if(ret == -1){
		gxlogd("set ts out ctrl err\n");
	}else{
		GxCore_MutexLock(gs_frontend_mutex[tuner]);
		g_frontend_set[tuner].set_unlock_ts = 1;//ts unlock状态
		GxCore_MutexUnlock(gs_frontend_mutex[tuner]);
	}

	return;
}

void GxFrontend_FememCtrl(int32_t tuner, uint32_t enable)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxloge("set ts out ctrl err tuenr out range!\n");
		return ;
	}

	frontend = gs_frontend[tuner].frontend;
	enable = enable ? 1 : 0;
	ret = ioctl (frontend, FE_FEMEM_CONTRL, enable);
	if(ret == -1)
		gxloge("femem contrl err\n");

	return;
}

int32_t GxFrontend_ReadStatus(int32_t tuner, uint32_t* status)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set voltage err tuenr out range!\n");
		return -1;
	}

	if(gs_frontend[tuner].type == FRONTEND_DVB_NET){
		if(net_ops.read_status){
			if(net_ops.read_status(status) < 0){
				gxloge("%s net frontend get status error!\n", __func__);
				ret = -1;
			}else{
				if(*status == FE_HAS_LOCK){
					if(net_ops.start_get_data)
						net_ops.start_get_data();
				}else{
					if(net_ops.stop_get_data)
						net_ops.stop_get_data();
				}
			}
		}else{
			gxloge("%s net frontend is not registered!\n", __func__);
			ret = -1;
		}
	}else{
		frontend = gs_frontend[tuner].frontend;
		ret = ioctl(frontend, FE_READ_STATUS, status);
		if(ret < 0)
		{
			gxlogd("read status err:%d\n", ret);
		}
	}

	return ret;
}

int32_t GxFrontend_Standby(int32_t tuner)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("stanby fail, tuner out of range!\n");
		return -1;
	}

	frontend = gs_frontend[tuner].frontend;
	ret = ioctl(frontend, FE_SET_STANDBY, NULL);
	if(ret < 0)
	{
		gxlogd("set standby err:%d\n", ret);
	}
	return ret;
}

int32_t GxFrontend_GetDemodInfo(int32_t tuner,struct dvb_frontend_info* info)
{
	handle_t frontend;
	int32_t ret = 0;

	if(tuner>=FRONTEND_MAX)
	{
		gxlogd("set voltage err tuenr out range!\n");
		return -1;
	}

	frontend = gs_frontend[tuner].frontend;
	if(frontend == 0xFFFF)
		return -1;
	ret = ioctl(frontend, FE_GET_INFO, info);

	return ret;
}
void GxFrontend_StartMonitorStrenthQuality(void)
{
	gs_frontend_strenth_quality = 1;
	return ;
}
void GxFrontend_StopMonitorStrenthQuality(void)
{
	gs_frontend_strenth_quality = 0;
	return ;
}

bool GxFrontend_UserSet(uint32_t tuner, const int32_t status, const GxFrontendSignalQuality sq)
{
	if (tuner >= FRONTEND_MAX)
		return false;
	if (gs_frontend_monitor[tuner]) {
		gs_frontend_status[tuner] = status;
		gs_frontend_quality[tuner].snr = sq.snr;
		gs_frontend_quality[tuner].error_rate = sq.error_rate;
		return true;
	} else
		return false;
}
int32_t GxFrontend_GetUnicableIffFreq(int32_t tuner,unsigned short *tone)
{
	uint32_t err = 0;

//	err = ioctl(gs_frontend[tuner].frontend,FE_GET_UNICALBE_IFF_FRE,tone);//驱动还没有实现

	if(err>= 0)
	{
		gxlogd("--------iFF=%d\n",err);
	}
	return 0;
}

int32_t GxFrontend_set_T2MI_contrl(int32_t tuner, uint32_t enable)
{
	int32_t err = 0;

	if(tuner >= FRONTEND_MAX
			|| gs_frontend[tuner].frontend < 0)
	{
		return -1;
	}

	if(!GxFrontend_HasSpecialFeatures(tuner))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend, FE_SET_T2MI_CONTRL, enable);

	return err;
}

int32_t GxFrontend_T2miSetPlp(int32_t tuner, uint16_t pid, uint8_t plp_id)
{
	int32_t err = 0;
	struct dvbt_plp_parameters param = { 0 };

	if(tuner >= FRONTEND_MAX
		|| gs_frontend[tuner].frontend < 0)
	{
		return -1;
	}

	if(!GxFrontend_HasSpecialFeatures(tuner))
	{
		return -1;
	}

	err = ioctl(gs_frontend[tuner].frontend, FE_SET_PID, pid);

	param.data_plp_id = plp_id;
	err = ioctl(gs_frontend[tuner].frontend, FE_SET_PLPID, &param);

	return err;
}

int32_t GxFrontend_Dvbt2SetPlp(int32_t tuner, uint8_t data_plp_id, uint8_t common_plp_exist, uint8_t common_plp_id)
{
	int32_t err = 0;
	struct dvbt_plp_parameters param = { 0 };

	if(tuner >= FRONTEND_MAX
		|| gs_frontend[tuner].frontend < 0)
	{
		return -1;
	}
	gs_frontend[tuner].data_plp_id = data_plp_id;
	gs_frontend[tuner].common_plp_exist = common_plp_exist;
	gs_frontend[tuner].common_plp_id = common_plp_id;

	param.data_plp_id = data_plp_id;
	param.common_plp_exist = common_plp_exist;
	param.common_plp_id = common_plp_id;
	err = ioctl(gs_frontend[tuner].frontend, FE_SET_PLPID, &param);

	return err;
}

