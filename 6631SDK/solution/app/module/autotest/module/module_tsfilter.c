/*
 * =====================================================================================
 *
 *       Filename:  autotest_tsfilter.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  10/28/2014 04:55:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Z.Z.R
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "../include/common_autotest.h"
#include "module/app_log.h"

static handle_t	sg_TESTThreadHandle = -1;
static int sg_StartFlag = 0;


GxDemuxProperty_Slot sg_Slot = {0};
GxDemuxProperty_Filter sg_Filter = {0};

static TsFilterPara_t sg_TsPara;
static unsigned int sg_ReceivePackageCount = 0;
static unsigned int sg_ErrorPackageCount = 0;
static int dvr_handle;

int _parameter_init(TsFilterPara_t *pTsFilterPara)
{
//#define TEST_TS_SOURCE		1
//#define TEST_DEMUX_ID		1
//#define DATA_PID			0x0// TS PID
//#define TS_PACKET_LEN		188

	if(pTsFilterPara == NULL)
		return -1;
	memcpy(&sg_TsPara, pTsFilterPara, sizeof(TsFilterPara_t));
	sg_TsPara.DemuxPara.DemuxDev = -1;
	sg_TsPara.DemuxPara.DemuxModule = -1;
    sg_ReceivePackageCount = 0;
    sg_ErrorPackageCount = 0;
#if SECURE_FIREWALL_SUPPORT || SECURE_FULL_SUPPORT
    sg_TsPara.DemuxPara.DemuxId = 2;//  Demux ID
#endif
	return 0;
}

static int _set_demux(void)
{
	GxDemuxProperty_ConfigDemux demux;
	int32_t ret;

	if(0 >= sg_TsPara.DemuxPara.DemuxDev)
	{
	 	sg_TsPara.DemuxPara.DemuxDev = GxAvdev_CreateDevice(0);
		if(sg_TsPara.DemuxPara.DemuxDev <= 0)
		{
			app_log_error("\nTEST, create device failed, %s,%d\n",__FILE__,__LINE__);
			return -1;
		}
		if(sg_TsPara.DemuxPara.DemuxModule <= 0)
		{
	 		sg_TsPara.DemuxPara.DemuxModule = GxAvdev_OpenModule(sg_TsPara.DemuxPara.DemuxDev, GXAV_MOD_DEMUX, sg_TsPara.DemuxPara.DemuxId);
			if(sg_TsPara.DemuxPara.DemuxModule <= 0)
			{
				GxAvdev_DestroyDevice(sg_TsPara.DemuxPara.DemuxDev);
				sg_TsPara.DemuxPara.DemuxDev = -1;
				app_log_error("\nTEST, open module failed, %s,%d\n",__FILE__,__LINE__);
				return -1;
			}
		}
	}

    // demux config
    demux.source = sg_TsPara.DemuxPara.TsSource;
    demux.ts_select = FRONTEND;
    demux.stream_mode = sg_TsPara.DemuxPara.TsMode;
    demux.time_gate = 0xf;
    demux.byt_cnt_err_gate = 0x03;
    demux.sync_loss_gate = 0x03;
    demux.sync_lock_gate = 0x03;
    ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_Config, &demux, \
            sizeof(GxDemuxProperty_ConfigDemux));
	if(ret < 0)
	{
		app_log_error("\nTEST, config demux error, %x\n", ret);
		return -1;
	}
	return 0;
}

int _setup_slot(void)
{
#define TS_DEMUX_IN_SIZE  (sg_TsPara.TsPackageLen*1024)
#define TS_DEMUX_IN_GATE  (sg_TsPara.TsPackageLen*100)
    int ret = 0;
    //TODO dev
    //struct dvr_buffer buffer = {0};
    dvr_handle = GxAvdev_OpenModule(sg_TsPara.DemuxPara.DemuxDev, GXAV_MOD_DVR, sg_TsPara.DemuxPara.DemuxId);
    GxDvrProperty_Config config= {
	.src = DVR_INPUT_DMX,
	.dst = DVR_OUTPUT_MEM,
	.flags = DVR_FLAG_TS_CHECK_EN,
	.dst_buf = {
		.sw_buffer_size = TS_DEMUX_IN_SIZE,
		.almost_full_gate = TS_DEMUX_IN_GATE,
	},
    };

	memset(&sg_Slot, 0, sizeof(GxDemuxProperty_Slot));
	memset(&sg_Filter, 0, sizeof(GxDemuxProperty_Filter));

#if 0
    // sepical slot
    sg_MuxSlot.type = DEMUX_SLOT_MUXTS;
    sg_MuxSlot.flags = (DMX_TSOUT_EN | DMX_CRC_DISABLE|DMX_REPEAT_MODE);
    ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotAlloc,
					&sg_MuxSlot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		return -1;
	}
    // special filter
    sg_Filter.slot_id = sg_MuxSlot.slot_id;
    sg_Filter.sw_buffer_size = TS_DEMUX_IN_SIZE;
    sg_Filter.almost_full_gate = TS_DEMUX_IN_GATE;
    ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_FilterAlloc,
					&sg_Filter, sizeof(GxDemuxProperty_Filter));
	if (ret < 0)
	{
		return -1;
	}
#endif
    // normal slot
    sg_Slot.type = DEMUX_SLOT_PES;
	sg_Slot.flags = (DMX_REPEAT_MODE | DMX_TSOUT_EN | DMX_CRC_DISABLE);
	sg_Slot.ts_out_pin = dvr_handle;
	sg_Slot.pid = sg_TsPara.TsPID;

	ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotAlloc,
					&sg_Slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		return -1;
	}

	ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotConfig,
					&sg_Slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		return -1;
	}


    // TODO set dvr config
    ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, dvr_handle, GxDvrPropertyID_Config,
					&config, sizeof(GxDvrProperty_Config));
    if (ret < 0)
	{
		return -1;
	}

    // match with "247240"
    ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, dvr_handle, GxDvrPropertyID_Run, NULL, 0);
    if(0 > ret)
    {
        return -1;
    }

    ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev,sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotEnable,
					&sg_Slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		return -1;
	}

	sg_StartFlag = 1;
    return 0;
}

static int _ts_package_read(char *pBuffer, unsigned MaxBufferLen)
{
    int ret = 0;
    if(pBuffer == NULL)
    {
        return -1;
    }
    ret = GxAVModuleRead(sg_TsPara.DemuxPara.DemuxDev, dvr_handle, pBuffer, MaxBufferLen, 100000);
    if(ret < 0)
    {
		app_log_error("\nTEST, read, failed \n");
        return -1;
    }

    return ret;
}


#define __TEST_TEST__
int g_TestDebugFlag = 0;
int _deal_with_one_packet(char *pData)
{
	int i = 0;
	if(NULL == pData)
	{
		app_log_error("\nTEST, parameter error\n");
		return -1;
	}
	if((pData[0] != 0x47)
		|| (((pData[1]&0x1f)<<8)|(pData[2])) != sg_TsPara.TsPID)
	{
		app_log_debug("\nTEST, wrong data, pid = 0x%x\n", sg_TsPara.TsPID);
	    sg_ReceivePackageCount++;
		sg_ErrorPackageCount++;
#if 0
//#ifdef __TEST_TEST__
if(g_TestDebugFlag)
{
                char *p = NULL;
                int i = 0;
				app_log_debug("\n************************************************\n");
				p = pData;
				for(i = 0; i < sg_TsPara.TsPackageLen; i++)
				{
					app_log_debug("0x%02x ",p[i]);
					if(((i + 1)%16) == 0)
						app_log_debug("\n");
				}
				app_log_debug("\n************************************************\n");
}
#endif

		return -1;
	}

	// TODO: do yourself
if((pData[1]&0x40) == 0)
{// not start preload data
	for(i=12; i < sg_TsPara.TsPackageLen; i++)
	{
		if(pData[i] != (i - 4))
			break;
	}

	if(i != sg_TsPara.TsPackageLen)
	{
			sg_ErrorPackageCount++;
#if 0
	//#ifdef __TEST_TEST__
	if(g_TestDebugFlag)
	{
	                char *p = NULL;
	                int i = 0;
					app_log_debug("\n************************************************\n");
					p = pData;
					for(i = 0; i < sg_TsPara.TsPackageLen; i++)
					{
						app_log_debug("0x%02x ",p[i]);
						if(((i + 1)%16) == 0)
							app_log_debug("\n");
					}
					app_log_debug("\n************************************************\n");
	}
#endif

	}
	sg_ReceivePackageCount++;
}
else
{
#if 0
	if(g_TestDebugFlag)
	{
	                char *p = NULL;
	                int i = 0;
					app_log_debug("\n**************start preload*********************\n");
					p = pData;
					for(i = 0; i < sg_TsPara.TsPackageLen; i++)
					{
						app_log_debug("0x%02x ",p[i]);
						if(((i + 1)%16) == 0)
							app_log_debug("\n");
					}
					app_log_debug("\n************************************************\n");
	}
#endif

	sg_ReceivePackageCount++;
}

	return 0;
}


static void _test_process(void *arg)
{
#define READ_DATA_SIZE		sg_TsPara.TsPackageLen*100
	int ret = -1;
	int i = 0;
	char* pDataBuffer = calloc(1,READ_DATA_SIZE);
	char* p = NULL;
	if(NULL == pDataBuffer)
	{
		app_log_error("\nTEST, calloc failed, %s,%d\n",__FILE__,__LINE__);
		while(1);
	}
	while(1)
	{
		if(sg_StartFlag > 0)
		{
			memset(pDataBuffer,0,READ_DATA_SIZE);
            ret =  _ts_package_read(pDataBuffer,READ_DATA_SIZE);
			if((ret >= sg_TsPara.TsPackageLen) && (sg_StartFlag == 1))
			{// get the ts data
				//app_log_debug("\nTEST, read data\n");
#if 0
//#ifdef __TEST_TEST__
if(g_TestDebugFlag)
{
				app_log_debug("\n************************************************\n");
				p = pDataBuffer;
				for(i = 0; i < sg_TsPara.TsPackageLen; i++)
				{
					app_log_debug("0x%02x ",p[i]);
					if(((i + 1)%16) == 0)
						app_log_debug("\n");
				}
				app_log_debug("\n************************************************\n");
}
#endif
// do TS packet
				p = pDataBuffer;
				for(i = 0; i < (ret/sg_TsPara.TsPackageLen); i++)// check the data is right or not
				{
					_deal_with_one_packet(p);
					p += sg_TsPara.TsPackageLen;
				}
			}
			else if(ret < 0)
			{
				app_log_error("\nTEST, read data error\n");
			}

		}
		else
		{
	       // if(pDataBuffer)
           //     free(pDataBuffer);
            if(sg_StartFlag == 0)
        	{
				app_log_debug("\nTEST, EXIT THE TS THREAD\n");
				sg_StartFlag = -1;
        	}
		    GxCore_ThreadDelay(100);
		}
#ifdef ECOS_OS
		GxCore_ThreadDelay(60);
#endif

	}
}

static int _ts_start(void)
{
	int ret = 0;
	sg_Slot.slot_id = -1;

	ret = _set_demux();
	if(ret < 0)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	ret = _setup_slot();
	if(ret < 0)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if(sg_TESTThreadHandle <= 0)
		GxCore_ThreadCreate("test_console", &sg_TESTThreadHandle, (void*)_test_process, NULL,\
							100*1024,7);
    return 0;
}

int test_ts_destroy(void)
{
	if((sg_TsPara.DemuxPara.DemuxModule> 0) && (sg_TsPara.DemuxPara.DemuxDev > 0))
	{
		sg_StartFlag = 0;

		if(sg_Slot.slot_id > 0)
		{
            // disable slot
            GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotDisable,
								(void*)&sg_Slot, sizeof(GxDemuxProperty_Slot));
            // special filter
        	GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_FilterFree,
								(void*)&sg_Filter, sizeof(GxDemuxProperty_Filter));

			GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotFree,
								(void*)&sg_Slot, sizeof(GxDemuxProperty_Slot));

	        memset(&sg_Slot, 0, sizeof(GxDemuxProperty_Slot));
	        memset(&sg_Filter, 0, sizeof(GxDemuxProperty_Filter));
			sg_Slot.slot_id = -1;
		}
		//wait for thread exit
		while(sg_StartFlag != -1)GxCore_ThreadDelay(10);

		GxAvdev_CloseModule(sg_TsPara.DemuxPara.DemuxDev,sg_TsPara.DemuxPara.DemuxModule);
        // match with "247240"
        GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, dvr_handle, GxDvrPropertyID_Stop, NULL, 0);
        GxAvdev_CloseModule(sg_TsPara.DemuxPara.DemuxDev,dvr_handle);
		GxAvdev_DestroyDevice(sg_TsPara.DemuxPara.DemuxDev);
		sg_TsPara.DemuxPara.DemuxDev = -1;
		sg_TsPara.DemuxPara.DemuxModule = -1;
		dvr_handle = -1;

	}
	return 0;
}

int start_ts_compare(void *pTsFilterPara)
{
	int ret = 0;

	if(pTsFilterPara == NULL)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(sg_StartFlag > 0)
	{
		app_log_error("\nTEST, stop first\n");
		stop_ts_compare();
	}

	ret = _parameter_init((TsFilterPara_t*)pTsFilterPara);
	if(ret < 0)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	ret = _ts_start();
	return ret;
}
int stop_ts_compare(void)
{
	int ret = 0;
	ret = test_ts_destroy();
	memset(&sg_TsPara, 0, sizeof(TsFilterPara_t));
	return ret;
}

int get_ts_compare_result(int *ReceivePackageCount, int *ErrorPackageCount)
{
	if(sg_StartFlag > 0)
	{
		*ErrorPackageCount = sg_ErrorPackageCount;
		*ReceivePackageCount = sg_ReceivePackageCount;
	}
	else
	{
		*ErrorPackageCount = 0;
		*ReceivePackageCount = 0;
		return -1;
	}
    return 0;
}


