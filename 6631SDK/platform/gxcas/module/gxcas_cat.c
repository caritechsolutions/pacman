#include "gxcore.h"
#include "gxcas_misc.h"
#include "gxcas_psi.h"
#include "gxcas_dbg.h"
#include "gxcas_demux.h"

static uint16_t emm_pid_record = PSI_INVALID_PID;
static CheckCAId cat_check_ca = NULL;

void GxCas_Cat_Init(CheckCAId docheck)
{
	if(docheck)
		cat_check_ca = docheck;
	else
		CAS_ERR(SI,"cat init err, pls check param!");

}

int32_t GxCas_Cat_Parse(uint8_t *section, uint32_t size, GxCas_EmmInfo *emminfo)
{
	uint8_t  descriptor_length;
	uint8_t* data = section;
	int32_t  len = size;
	uint32_t CA_system_id;
	uint16_t emm_pid = PSI_INVALID_PID;
	int32_t count=0;

	len  -= 12;
	data += 8;
	while (len > 0) {
		descriptor_length = data[1];
		if(data[0] == 0x09) {
			CA_system_id = data[2] << 8 | data[3];
			emm_pid = (data[4] & 0x1F) << 8 | data[5];;
			if (TRUE == cat_check_ca(CA_system_id, PSI_INVALID_PID)) {
				if (PSI_INVALID_PID != emm_pid && emm_pid != emm_pid_record) {
					emminfo->emmpid[count] = emm_pid;
					emm_pid_record = emm_pid;
					count++;
					emminfo->emm_count = count;
				}
			}
		}

		data += 2;
		len -= 2;
		if(descriptor_length == 0)
			continue;

		len -= descriptor_length;
		data += descriptor_length;
	}
	return 0;
}

handle_t GxCas_Cat_Open(uint8_t version, uint8_t versionEQ)
{
	handle_t cat_handle = 0;
	sifilter_params_t filter = {0};

	filter.match[0] = CAT_TID;
	filter.mask[0] = 0xff;
	if (versionEQ == FALSE) {
		filter.match[5] = version;
		filter.mask[5] = 0x3E;
		filter.depth = 6;
	} else
		filter.depth = 1;

	filter.pid = CAT_PID;
	filter.flags |= CRC_FLAG;
	filter.flags |= REPEAT_FLAG;

	if(versionEQ == TRUE)
		filter.flags |= EQ_FLAG;
	

	cat_handle = GxCas_SiFilter_Start(filter);
	if (cat_handle == -1)
		return 0;

	return cat_handle;
}

int32_t GxCas_Cat_Close(handle_t handle)
{
	emm_pid_record = PSI_INVALID_PID;
	return GxCas_SiFilter_Stop(handle);
}
