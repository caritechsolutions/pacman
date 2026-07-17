#include <gxmsg.h>
#include <gxcore.h>
#include <service/net_service.h>
#include <module/player/gxmedia_api.h>

#include <network.h>
#include <arpa/inet.h>

#include "cmd.h"
#include "fifo.h"


#define MAX_VIDEO_INSTANCE (2)


extern int net_is_init;
extern int sock_handle[2];

static struct vsame_board_service {
	int id;
	handle_t cmd_parse_thread;
	handle_t video;
	vsFIFO*  fifo;
	int      enable;
	int      es_cnt;
	int      data_pull_thread;
	int      data_parse_thread;
	int      last_frame_id;
	int      eos;
} vsame_param[MAX_VIDEO_INSTANCE];

static int netcodec2boadcodec(int net_codec)
{
	int i, board_codec = 0;

	struct {
		int net_codec;
		int board_codec;
	} dict[] = {
		{.net_codec = VSAME_CODEC_MPEG2, .board_codec = VIDEO_CODEC_MPEG12},
		{.net_codec = VSAME_CODEC_H264 , .board_codec = VIDEO_CODEC_H264  },
		{.net_codec = VSAME_CODEC_H265 , .board_codec = VIDEO_CODEC_H265  },
		{.net_codec = VSAME_CODEC_MPEG4, .board_codec = VIDEO_CODEC_MPEG4 },
		{.net_codec = VSAME_CODEC_AVS  , .board_codec = VIDEO_CODEC_AVS   },
		{.net_codec = VSAME_CODEC_DIV3 , .board_codec = VIDEO_CODEC_DIV3  },
	};

	for (i = 0; i < sizeof(dict)/sizeof(dict[0]); i++) {
		if (net_codec == dict[i].net_codec) {
			board_codec = dict[i].board_codec;
			break;
		}
	}
	if (i == sizeof(dict)/sizeof(dict[0]))
		printf("\n[Vseam-servise] Bad net-codec id!\n");

	return board_codec;
}

void NET_SEND(int id, void *data, int len){
	GxMessage *new_msg = NULL;
	GxDataNode *net_data = NULL;

	new_msg = GxBus_MessageNew(GXMSG_NET_SEND);
	net_data = GxBus_GetMsgPropertyPtr(new_msg, GxDataNode);
	net_data->data = data;
	net_data->len = len;
	net_data->id  = id;
	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);
}

static int stream_read(int sock, unsigned char *buf, int len)
{
	int l = 0;
	int get = 0;

	while (get < len) {
		l = vsFIFO_Get((vsFIFO *)sock, buf+get, len-get);
		//printf("l = %d\n", l);
		if (l > 0)
			get += l;
		else
			GxCore_ThreadDelay(100);
	}

	return get;
}

static int get_magic(int sock)
{
	int i;
	unsigned char ch;
	unsigned char magic[] = {0x55, 0x66, 0x77, 0x88};

	for (i = 0; i < 4; i++) {
		stream_read(sock, &ch, 1);
		//printf("ch = 0x%x\n", ch);
		if (ch == magic[i])
			continue;
		else
			break;
	}

	return (i == 4);
}

static Cmd* get_cmd(int sock)
{
	unsigned char ch;
    ssize_t i, len = 0;
	Cmd *cmd = new_cmd();

	while (1) {
		if (get_magic(sock)) {
			cmd->header.magic = VSAME_MAGIC;
			stream_read(sock, &cmd->header.cmd_id, sizeof(cmd->header)-MAGIC_LEN);
			if (cmd->header.body_len) {
				cmd->header.body_len = ntohl(cmd->header.body_len);
				cmd->body = malloc(cmd->header.body_len);
				stream_read(sock, cmd->body, cmd->header.body_len);
			} else {
				cmd->body = NULL;
			}
			break;
		}
	}

	return cmd;
}

int video_finish_callback(int id)
{
	int finish = 0;
	Cmd *cmd = NULL;
	int client_id = id;
	GxMessage *new_msg = NULL;
	GxMediaModuleDecState state;

	GxMediaApi_VideoGetDecState(vsame_param[id].video, &state);
	finish = (state.state == GXMEDIA_STATE_STOPPED);

	cmd = new_cmd_framehash_over(client_id);
	NET_SEND(id, &(cmd->header), sizeof(struct cmd_header));
	delete_cmd(cmd);
	if (finish)
		printf("[APP-%d] Hash end\n", id);

	return 0;
}

int video_eachframe_callback(int id)
{
	int ret = -1;
	int client_id = id;
	Cmd *cmd = NULL;
	GxMediaVideoFrameHash hash = {0};
	GxMessage *new_msg = NULL;

	ret = GxMediaApi_VideoGetHash(vsame_param[id].video, &hash);
	if (ret == 0) {
		int i;
		printf("\n[APP-%d] frame_ID: %u\t", id, hash.frame_id);
		for (i = 0; i < 32; i ++) {
			printf("%02x", hash.hash[i]);
		}
		printf("\n");
		GxMediaApi_VideoResume(vsame_param[id].video);

		cmd = new_cmd_framehash(client_id, hash.frame_id, hash.hash, 0);
		NET_SEND(id, &(cmd->header), sizeof(struct cmd_header));
		NET_SEND(id, cmd->body, cmd->header.body_len);
		delete_cmd(cmd);
	} else {
		printf("\n[APP-%d] get_hash failed!\n", id);
	}

	return ret;
}

static int do_cmd_decode(struct vsame_board_service *vsame, Cmd *cmd)
{
	int ret = -1;
	Cmd *ret_cmd = NULL;
	GxMediaVideoPara param = {0};
	GxMessage *new_msg = NULL;
	GxDataNode *net_data = NULL;
	struct cmd_decode_body *body = (struct cmd_decode_body *)cmd->body;

	vsame->eos = 0;
	vsame->es_cnt = 0;
	if (vsame->video != -1) {
		param.codectype   = netcodec2boadcodec(body->codec);
		param.width       = 720;
		param.height      = 576;
		param.mosaic_gate = 100;
		param.frame_rate  = 25;
		param.speed_mode  = GXMEDIA_VIDEO_STEP_SPEED;
		param.decode_mode = GXMEDIA_VIDEO_DECODE_ALL;
		param.each_frame_cb   = video_eachframe_callback;
		param.stream_finish_cb = video_finish_callback;
		ret = GxMediaApi_VideoConfig(vsame->video, param);
		if (ret < 0) {
			printf("[APP] video config failed");
			goto out;
		}

		ret = GxMediaApi_VideoStart(vsame->video);
		if (ret < 0) {
			printf("[APP] video start failed");
			goto out;
		}
	} else {
		printf("[APP] %s:%d, open Video failed!", __func__, __LINE__);
		goto out;
	}

	ret_cmd = new_cmd_decode_ack(1);
	NET_SEND(vsame->id, &ret_cmd->header, sizeof(struct cmd_header));
	printf("client-%d [send]: decode ack\n\n", vsame->id);
	delete_cmd(ret_cmd);

out:
	return ret;
}

int dbg_inject_data = 0;
int do_cmd_pushdata(struct vsame_board_service *vsame, Cmd *cmd)
{
	int ret = -1;
	struct cmd_pushdata_body *body = (struct cmd_pushdata_body *)cmd->body;

	do {
		ret = GxMediaApi_VideoInjectData(vsame->video, GXMEDIA_SOURCE_ES, cmd->body+2, cmd->header.body_len-2, 0);
		GxCore_ThreadDelay(100);
	} while(ret != cmd->header.body_len-2);
	vsame->es_cnt += cmd->header.body_len-2;

	if (dbg_inject_data)
		printf("\n[APP-%d] inject %d, total %d\n", vsame->id, cmd->header.body_len, vsame->es_cnt);

	if (body->eos) {
		vsame->eos = 1;
		GxMediaApi_VideoEos(vsame->video);
		printf("\n[APP-%d] Eos & Flushed!!!\n", vsame->id);
	}

	return GXCORE_SUCCESS;
}

int do_cmd_stop(struct vsame_board_service *vsame, Cmd *cmd)
{
	Cmd *ret_cmd = NULL;

	GxMediaApi_VideoStop(vsame->video, 0);
	vsFIFO_Reset(vsame->fifo);
	printf("\n#### vsame[%d] fifo reset!####\n", vsame->id);

	ret_cmd = new_cmd_stop_ack(1);
	NET_SEND(vsame->id, &ret_cmd->header, sizeof(struct cmd_header));
	printf("client-%d [send]: stop ack\n\n", vsame->id);
	delete_cmd(ret_cmd);
	vsame->enable = 0;

	return GXCORE_SUCCESS;
}

static void thread_data_pull(void* arg)
{
	int   ret, put_len;
	int   ibuf_size = 65*1024;
	char *ibuf;
	struct vsame_board_service *vsame = (struct vsame_board_service*)arg;

	ibuf = malloc(ibuf_size);
	if (ibuf == NULL)
		printf("[Vsame] ibuf malloc failed\n");

	while(1) {
		if (net_is_init == 0) {
			GxCore_ThreadDelay(1000);
			continue;
		}

		ssize_t len = read(sock_handle[vsame->id], ibuf, ibuf_size);
		if (len > 0) {
			if (vsame->enable == 0)
				vsame->enable = 1;
			put_len = 0;
			while(1) {
				ret = vsFIFO_Put(vsame->fifo, (void *)&ibuf[put_len], len-put_len);
				put_len += ret;
				if (put_len == len)
					break;
			}
		}
	}
}

static void thread_data_parse(void* arg)
{
	struct vsame_board_service *vsame = (struct vsame_board_service*)arg;

	while(1) {
		if (vsFIFO_Len(vsame->fifo) == 0) {
			GxCore_ThreadDelay(1000);
			continue;
		}

		vsFIFO *sock =  vsame->fifo;
		Cmd *cmd = get_cmd((int)sock);

		if (cmd->header.cmd_id == CMD_DECODE) {
			//cmd_print(cmd);
			printf("\n[CMD%d] recv: decode\n\n", vsame->id);
			do_cmd_decode(vsame, cmd);
			delete_cmd(cmd);
		}

		if (cmd->header.cmd_id == CMD_PUSHDATA) {
			//cmd_print(cmd);
			printf("\n[CMD%d] recv: pushdata\n\n", vsame->id);
			do_cmd_pushdata(vsame, cmd);
			delete_cmd(cmd);
		}

		if (cmd->header.cmd_id == CMD_STOP) {
			//cmd_print(cmd);
			printf("\n[CMD%d] recv: stop\n\n", vsame->id);
			do_cmd_stop(vsame, cmd);
			delete_cmd(cmd);
		}
	}
}

status_t GxVsameServiceInit(handle_t self,int priority_offset)
{
	int i;
	handle_t sch;
	GxMediaVideoMod mod_sel = {0};

	GxBus_MessageListen(self, GXMSG_NET_RECEIVE);

	GxMediaApi_Init();
	for (i = 0; i < MAX_VIDEO_INSTANCE; i++) {
		vsame_param[i].id = i;
		vsame_param[i].fifo  = vsFIFO_New(3*1024*1024);
		mod_sel.vdec_modid = i;
		mod_sel.vout_modid = 0;
		vsame_param[i].video = GxMediaApi_VideoOpen3(mod_sel);
	}

	sch = GxBus_SchedulerCreate("VsameMsgScheduler", GXBUS_SCHED_MSG, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	sch = GxBus_SchedulerCreate("VsameConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 32, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	GxCore_ThreadCreate("vsame_data_pull_0", &vsame_param[0].data_pull_thread, thread_data_pull, &vsame_param[0], 1024*64, GXOS_DEFAULT_PRIORITY);
	GxCore_ThreadCreate("vsame_data_pull_1", &vsame_param[1].data_pull_thread, thread_data_pull, &vsame_param[1], 1024*64, GXOS_DEFAULT_PRIORITY);

	GxCore_ThreadCreate("vsame_data_parse_0", &vsame_param[0].data_parse_thread, thread_data_parse, &vsame_param[0], 1024*64, GXOS_DEFAULT_PRIORITY);
	GxCore_ThreadCreate("vsame_data_parse_1", &vsame_param[1].data_parse_thread, thread_data_parse, &vsame_param[1], 1024*64, GXOS_DEFAULT_PRIORITY);

	return GXCORE_SUCCESS;
}

void GxVsameServiceDestroy(handle_t self)
{
}

GxMsgStatus GxVsameServiceRecvMsg(handle_t self, GxMessage* Msg)
{
	int ret = 0;
	//gxlogi("[Vsame]: Recv Msg[%-2d]\n",Msg->msg_id);
	switch(Msg->msg_id)
	{
		case GXMSG_NET_RECEIVE:
			{
#if 0
				int read_len = 0;
				GxDataNode* net_data = NULL;
				net_data = GxBus_GetMsgPropertyPtr(Msg, GxDataNode);
				if (net_data == NULL)
					return GXCORE_ERROR;
				while(1) {
					if (vsame_param[net_data->id].enable == 0)
						vsame_param[net_data->id].enable = 1;
					ret = vsFIFO_Put(vsame_param[net_data->id].fifo, (void *)&net_data->data[read_len], net_data->len - read_len);
					read_len += ret;
					if (read_len == net_data->len) {
						return GXCORE_SUCCESS;
					}
				}
#endif
				break;
			}
		default:
			break;
	}
	return GXCORE_ERROR;
}

void GxVsameServiceConsole(handle_t self)
{
#if 0
	int i;
	for (i = 0; i < MAX_VIDEO_INSTANCE; i++) {
		if (vsame_param[i].enable == 0)
			continue;

		vsFIFO *sock =  vsame_param[i].fifo;
		Cmd *cmd = get_cmd((int)sock);

		if (cmd->header.cmd_id == CMD_DECODE) {
			//cmd_print(cmd);
			printf("board [receive]: decode\n\n");
			do_cmd_decode(&vsame_param[i], cmd);
			delete_cmd(cmd);
		}

		if (cmd->header.cmd_id == CMD_PUSHDATA) {
			//cmd_print(cmd);
			printf("board [receive]: pushdata\n\n");
			do_cmd_pushdata(&vsame_param[i], cmd);
			delete_cmd(cmd);
		}

		if (cmd->header.cmd_id == CMD_STOP) {
			//cmd_print(cmd);
			printf("board [receive]: stop\n\n");
			do_cmd_stop(&vsame_param[i], cmd);
			delete_cmd(cmd);
		}
	}
#endif
}

GxServiceClass vsame_service = {
	"vsame service",
	GxVsameServiceInit,
	GxVsameServiceDestroy,
	GxVsameServiceRecvMsg,
	GxVsameServiceConsole,
	.priority_offset = 0,
};



