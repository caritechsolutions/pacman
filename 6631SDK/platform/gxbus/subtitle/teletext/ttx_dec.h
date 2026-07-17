#ifndef __TELTEXT_DEC_H__
#define __TELTEXT_DEC_H__

typedef enum {
	TTX_SUBTITLE,
	TTX_NO_SUBTITLE,
	TTX_STUFFING_UNIT
} TTX_DATA_TYPE;

typedef struct {
	unsigned char  reserve;
	unsigned char  field_parity;
	unsigned char  line_offset;
	unsigned char  framing_code;
	unsigned short magazine;
	unsigned short packet;
	unsigned char  data_block[42];
} TTX_DATA_FIELD;

typedef struct {
	TTX_DATA_TYPE   type;
	TTX_DATA_FIELD  field;
} TTX_EBU_UNIT;

typedef struct {
#define MAX_EBU_COUNT 256
	unsigned int  identifier;
	unsigned int  count;
	TTX_EBU_UNIT  unit[MAX_EBU_COUNT];
} TTX_EBU;

extern int ttx_dec(unsigned char *pes, unsigned int len, TTX_EBU *ebu);

#endif
