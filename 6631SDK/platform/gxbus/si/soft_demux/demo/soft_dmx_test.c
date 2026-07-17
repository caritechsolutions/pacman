#include <stdio.h>
#include <soft_demux.h>
#include <stdlib.h>
#include <ts_file.h>


int main(void)
{
	uint32_t len, i;
	uint8_t *p_section_data;
	uint8_t match[8];
	uint8_t mask[8]={0};

	//p_ucDfAddr = (uint8_t*)GxCore_Malloc(FILTER_SIZE);
	
	if (TsFileCtrl.open(&g_TsFile, STREAM_READ) != 0)
	{
		gxlogd("open file failed!\n");
		return 0;
	}

	len = CheckTSLock();
	if (len != 0) {
		gxlogd("ts package len %d\n", len);
	}
	else {
		gxlogd("ts unlock!\n");
	}

	TsFileCtrl.close(&g_TsFile);

	SetPidValue(0x11);
	ConfigFilter(match, mask, 1);

	p_section_data = GetSectionData();

	if (p_section_data == NULL)
	{
		gxlogd("can't find the right section!\n");
	}

	for (i=0; i<1024; i++)
	{
		gxlogd("0x%x ",p_section_data[i]);
		if (i%12 == 0)
			gxlogd("\n");
	}

	//GxCore_Free(p_ucDfAddr);
	return 0;
}


