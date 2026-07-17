#include "gx_media.h"
#include "sub_pes.h"

int sub_parse_pes(uint8_t* src, int32_t src_len, uint8_t** dst, int32_t* dst_len, int64_t* pts)
{
	uint16_t pes_len = 0;

	if (src[0] != 0x0 || src[1] != 0x0 || src[2] != 0x1) {
		gxlogf("%s %d, PES Stream Flags error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	pes_len = (src[4]<<8) + src[5];
	if (pes_len == 0) {
		*dst     = src;
		*dst_len = src_len;
		return 0;
	} else if ((pes_len+6) > src_len) {
		gxlogf("%s %d, PES Len error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (src[7] & 0x80) {
		*pts  = (int64_t) (src[9] & 0x0E) << 29;
		*pts |= (int64_t) (src[10] << 22);
		*pts |= (int64_t)((src[11] & 0xFE) << 14);
		*pts |= (int64_t) (src[12] << 7);
		*pts |= (int64_t)((src[13] & 0xFE) >> 1);
	} else
		*pts = -1;

	*dst     = src;
	*dst_len = pes_len+6;
	return 0;
}
