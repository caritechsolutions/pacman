#ifndef __SUB_PES_H__
#define __SUB_PES_H__

extern int sub_parse_pes(uint8_t* src,
		int32_t src_len, uint8_t** dst, int32_t* dst_len, int64_t* pts);

#endif
