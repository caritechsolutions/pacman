#ifndef __pack_h__
#define __pack_h__

typedef unsigned char u8;
typedef unsigned int  u32;

enum {
	CMD_DECODE    ,
	CMD_DECODE_ACK,
	CMD_PUSHDATA  ,
	CMD_FRAMEHASH ,
	CMD_STOP      ,
	CMD_STOP_ACK  ,
	CMD_FRAMEHASH_OVER,
};

enum {
	MEDIA_VIDEO,
	MEDIA_AUDIO,
};

enum {
	VSAME_CODEC_MPEG2,
	VSAME_CODEC_H264 ,
	VSAME_CODEC_H265 ,
	VSAME_CODEC_MPEG4,
	VSAME_CODEC_AVS  ,
	VSAME_CODEC_DIV3 ,
};

enum {
	VSAME_STREAM_ES,
	VSAME_STREAM_TS,
};

struct cmd_decode_body {
	unsigned char media_type; //audio ? video
	unsigned char codec;
} __attribute__((packed));

struct cmd_pushdata_body {
	unsigned char stream_type; //es ? ts
	unsigned char eos;
} __attribute__((packed));

struct cmd_framehash_body {
	unsigned int  frame_id;
	unsigned char finish;
	unsigned char hash[32];
} __attribute__((packed));

struct cmd_framedata_body {
	unsigned int  frame_id;
	unsigned char frame_finish;
	unsigned char stream_finish;
} __attribute__((packed));

struct cmd_header {
	unsigned int  magic;
	unsigned char cmd_id;
	unsigned char client_id;
	unsigned int  body_len;
} __attribute__((packed));

typedef struct cmd {
	struct cmd_header header;
	unsigned char *body;
} Cmd;


#define VSAME_MAGIC (0x55667788)
#define MAGIC_LEN   (4)
#define CRC_LEN     (4)

Cmd* new_cmd(void);
Cmd* new_cmd_decode(u8 client_id, u8 media_type, u8 codec);
Cmd* new_cmd_decode_ack(u8 client_id);
Cmd* new_cmd_pushdata(u8 client_id, u8 stream_type, u8 eos, u8* data, u32 data_len);
Cmd* new_cmd_framehash     (u8 client_id, u32 frame_id, unsigned char hash[], unsigned char finish);
Cmd* new_cmd_framehash_over(u8 client_id);
Cmd* new_cmd_stop(u8 client_id);
Cmd* new_cmd_stop_ack(u8 client_id);

void delete_cmd(struct cmd *cmd);
void cmd_print(Cmd *cmd);

#endif

