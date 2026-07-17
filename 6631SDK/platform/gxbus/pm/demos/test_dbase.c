#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "module/berkeley/db.h"
#include "module/pm/gxpm_manage.h"
#include "gxcore.h"





int GxCore_Startup(int argc, char **argv)
{
	int32_t i ;
	uint32_t count = 0;
	GxBusPmDataSat sat_input = {0};
	GxBusPmDataTP tp_input = {0};
	GxBusPmDataProg prog_input = {0};
	GxBusPmDataProg prog_arry[105];
	GxBusPmDataTP tp_arry[20];

	DBT data, key, keydata;
	DB *dbp = NULL;
	int input_key=1;
	uint32_t id= 0;
	uint32_t  id_arry[5];
	GxBusPmDataFavItem  fav_arry[2];

	GxBus_PmDbaseInit();

	//GxBus_PmFavGroupGetById(32, &fav_arry);
	GxBus_PmFavGroupGetByPos(0,2,fav_arry);
	
	sat_input.type = GXBUS_PM_SAT_S;
	sat_input.tuner = 1;
	sat_input.sat_s.diseqc10 = 1;
	sat_input.sat_s.diseqc11 = 2;
	sat_input.sat_s.diseqc12_pos = 3;
	sat_input.sat_s.diseqc_version = 4;
	sat_input.sat_s.lnb1 =5150;
	sat_input.sat_s.lnb2 = 5750;
	sat_input.sat_s.lnb_power = 0;
	sat_input.sat_s.lnb_type = GXBUS_PM_SAT_LNB_UNIVERSAL_1;
	sat_input.sat_s.longitude = 180;
	sat_input.sat_s.longitude_direct = 0;
	memcpy(sat_input.sat_s.sat_name,"sat111",strlen("sat111"));
	sat_input.sat_s.switch_12V = 1;
	sat_input.sat_s.switch_22K =0;
	for(i = 0; i<100; i++)
	{
		sat_input.sat_s.lnb1 =(i+1)*1000;
		GxBus_PmSatAdd( &sat_input);
		//gxlogd("id = %d\n",sat_input.id);

	}
	tp_input.sat_id = 1;
	tp_input.tp_s.polar = 1;
	tp_input.tp_s.symbol_rate = 1000;
	for(i=0; i<1000; i++)
	{
		tp_input.frequency = i*100;
		GxBus_PmTpAdd( &tp_input);
		//gxlogd("id = %d\n",tp_input.id);
	}

	prog_input.ac3_flag = 1;
	prog_input.audio_count =4;
	prog_input.audio_level = 0;
	prog_input.audio_mode = 1;
	prog_input.audio_volume = 20;
	prog_input.bouquet_id = 899;
	prog_input.cas_id = 900;
	prog_input.cc_flag = 0;
	prog_input.count = 0;
	prog_input.ecm_pid_v = 600;
	prog_input.favorite_flag = 0;
	prog_input.lock_flag = 0;
	prog_input.pcr_pid = 700;
	prog_input.pmt_pid =100;
	prog_input.pmt_version = 5;
	memcpy(prog_input.prog_name,"cctv1",strlen("cctv1"));
	prog_input.sat_id = 1;
	prog_input.scramble_flag = 0;
	prog_input.service_id = 400;
	prog_input.service_type = 0;
	prog_input.skip_flag = 0;
	prog_input.subt_flag = 1;
	prog_input.tp_id = 1;
	prog_input.ts_id = 200;
	prog_input.ttx_flag = 1;
	prog_input.video_pid = 300;
	prog_input.video_type = 0;
	for(i=0; i<101; i++)
	{
		prog_input.video_pid = i*100;
		GxBus_PmProgAdd(&prog_input);
		gxlogd("id = %d\n",prog_input.id);
	}
	count = GxBus_PmProgNumGet();
	gxlogd("count = %d\n",count);
	gxlogd("!!!!!!!!!!!!!!!!!!!!\n");
	count = GxBus_PmProgGetByPos(0,100,prog_arry);
	gxlogd("count = %d\n",count);
	for(i=0; i<count; i++)
	{
		gxlogd("id = %d pos = %d\n",prog_arry[i].id,prog_arry[i].pos);
	}
	
	
	remove("./sat.db");
	remove("./tp.db");
	remove("./prog.db");
	remove("./fav.db");
	remove("./stream.db");
	GxCore_Loop();

	return 0;
}

