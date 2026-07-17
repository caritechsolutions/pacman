#include "gxcore.h"
#include "gxcas_psi.h"
#include "gxcas_dbg.h"
#include "gxcas_demux.h"


void GxCas_Nit_Init(void)
{
}

handle_t GxCas_Nit_Open(uint8_t version, uint8_t versionEQ)
{
	handle_t nit_handle = 0;
	sifilter_params_t filter = {0};

	filter.match[0] = NIT_TID;
	filter.mask[0] = 0xff;
	if (versionEQ == FALSE) {
		filter.match[5] = version;
		filter.mask[5] = 0x3E;
		filter.depth = 6;
	} else
		filter.depth = 1;


	filter.pid = NIT_PID;
	filter.flags |= CRC_FLAG;
	filter.flags |= REPEAT_FLAG;

	if(versionEQ == TRUE)
		filter.flags |= EQ_FLAG;

	nit_handle = GxCas_SiFilter_Start(filter);
	if (nit_handle == -1) {
		return 0;
	}
	return nit_handle;
}


int32_t GxCas_Nit_Close(handle_t handle)
{
	return GxCas_SiFilter_Stop(handle);
}

