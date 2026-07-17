#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"

Cmd* new_cmd(void)
{
	struct cmd *cmd = (struct cmd*)malloc(sizeof(struct cmd));
	if (cmd)
		memset(cmd, 0, sizeof(struct cmd));

	return cmd;
}

static void cmd_fill_header(struct cmd_header *header, u8 cmd_id, u8 client_id, u32 body_len)
{
	if (header) {
		header->magic     = VSAME_MAGIC;
		header->cmd_id    = cmd_id;
		header->client_id = client_id;
		header->body_len  = body_len;
	}
}

Cmd* new_cmd_decode(u8 client_id, u8 media_type, u8 codec)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		body_len = sizeof(struct cmd_decode_body);
		cmd_fill_header(&cmd->header, CMD_DECODE, client_id, body_len);
		cmd->body = (u8*)malloc(body_len);
		if (cmd->body) {
			cmd->body[0] = media_type;
			cmd->body[1] = codec;
		}
	}

	return cmd;
}

Cmd* new_cmd_decode_ack(u8 client_id)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		body_len = 0;
		cmd_fill_header(&cmd->header, CMD_DECODE_ACK, client_id, body_len);
	}

	return cmd;
}

Cmd* new_cmd_pushdata(u8 client_id, u8 stream_type, u8 eos, u8* data, u32 data_len)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		body_len = sizeof(struct cmd_pushdata_body) + data_len;
		cmd_fill_header(&cmd->header, CMD_PUSHDATA, client_id, body_len);
		cmd->body = (u8*)malloc(body_len);
		if (cmd->body) {
			cmd->body[0] = stream_type;
			cmd->body[1] = eos;
			memcpy(&(cmd->body[2]), data, data_len);
		}
	}

	return cmd;
}

Cmd* new_cmd_framehash(u8 client_id, u32 frame_id, unsigned char hash[], unsigned char finish)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		body_len = sizeof(struct cmd_framehash_body);
		cmd_fill_header(&cmd->header, CMD_FRAMEHASH, client_id, body_len);
		cmd->body = malloc(body_len);
		if (cmd->body) {
			*(u32*)(&cmd->body[0]) = frame_id;
			*(u32*)(&cmd->body[4]) = finish;
			memcpy(&(cmd->body[5]), hash, 32);
		}
	}

	return cmd;
}

Cmd* new_cmd_framehash_over(u8 client_id)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		cmd_fill_header(&cmd->header, CMD_FRAMEHASH_OVER, client_id, body_len);
	}

	return cmd;
}

Cmd* new_cmd_stop(u8 client_id)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		body_len = 0;
		cmd_fill_header(&cmd->header, CMD_STOP, client_id, body_len);
	}

	return cmd;
}

Cmd* new_cmd_stop_ack(u8 client_id)
{
	int body_len = 0;
	struct cmd *cmd = new_cmd();

	if (cmd) {
		body_len = 0;
		cmd_fill_header(&cmd->header, CMD_STOP_ACK, client_id, body_len);
	}

	return cmd;
}

void delete_cmd(struct cmd *cmd)
{
	if (cmd) {
		if (cmd->body)
			free(cmd->body);
		free(cmd);
	}
}

void cmd_print(Cmd *cmd)
{
	int i;

	if (cmd) {
		printf("======== cmd start ==============\n");
		printf("\tmagic     = 0x%x\n", cmd->header.magic);
		printf("\tcmd_id    = %d\n", cmd->header.cmd_id);
		printf("\tclient_id = %d\n", cmd->header.client_id);
		printf("\tbody_len  = %d\n\n", cmd->header.body_len);

		printf("\tbody:\n");
		for (i = 0; i < cmd->header.body_len && i < 10; i++)
			printf("\t[%d] 0x%x\n", i, cmd->body[i]);
		printf("======== cmd start ==============\n");
	}
}
