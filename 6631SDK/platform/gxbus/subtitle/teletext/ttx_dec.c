#include "gxcore.h"
#include "ttx_dec.h"

#define EBU_DATA_N    (46)

static void ebu_data_parse(unsigned char* ebuData, unsigned int size, TTX_EBU *ebu)
{
	int i = 0;
	unsigned char *pEbuData = ebuData, *PEbuData0 = NULL;
	unsigned char c = 0;

	memset(ebu, 0, sizeof(TTX_EBU));
	ebu->identifier = pEbuData[0];
	pEbuData++;
	size--;

	if ((ebu->identifier < 0x10 || ebu->identifier > 0x1F) &&
		   (ebu->identifier < 0x99 || ebu->identifier > 0x9B)) {
		gxlogd("%s %d ebu identifier 0x%02x\n", __FUNCTION__, __LINE__, ebu->identifier);
		return;
	}

	PEbuData0 = pEbuData;
	for (i = 0; (i < size/EBU_DATA_N) && (c < MAX_EBU_COUNT); i++) {
		unsigned char *pDataFeild = NULL;

		pEbuData = PEbuData0 + i * EBU_DATA_N;
		if (pEbuData[0] == 0x02)
			ebu->unit[c].type = TTX_NO_SUBTITLE;
		else if (pEbuData[0] == 0x03)
			ebu->unit[c].type = TTX_SUBTITLE;
		else
			continue;

		if (pEbuData[1] != 0x2c)
			continue;

		pDataFeild = pEbuData + 2;
		ebu->unit[c].field.field_parity = ((pDataFeild[0] >> 5) & 0x01);
		ebu->unit[c].field.line_offset  = ((pDataFeild[0]) & 0x1f);
		ebu->unit[c].field.framing_code = pDataFeild[1];
		ebu->unit[c].field.magazine     = ((pDataFeild[2] >> 5) & 0x07);
		ebu->unit[c].field.packet       = ((pDataFeild[2] & 0x1f) << 8) + pDataFeild[3];
		memcpy(&ebu->unit[c].field.data_block[0], &pDataFeild[2], 42);
		c++;
	}

	ebu->count = c;
	return;
}

int ttx_dec(unsigned char *pes, unsigned int len, TTX_EBU *ebu)
{
	int size = 0;
	unsigned int header_size = 0;

	if (len < 4) {
		gxlogd("[%s] [%d] pes length less than 4 bits\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if ((pes[0] != 0x00) || (pes[1] != 0x00) ||
			(pes[2] != 0x01) || (pes[3] != 0xbd)) {
		gxlogd("[%s] [%d] pes data error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	size = (pes[4] << 8) | pes[5];
	if ((size + 6) > len || size <= 39) {
		gxlogd("[%s] [%d] pes size error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	header_size = pes[8];
	if ((header_size + 3) >= size) {
		gxlogd("[%s] [%d] pes data error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ebu_data_parse(&pes[header_size + 9], (size - (3 + header_size)), ebu);

	return 0;
}
