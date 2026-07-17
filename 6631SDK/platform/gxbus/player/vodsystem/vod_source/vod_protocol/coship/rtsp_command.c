#include "../../include/vod_in_common_def.h"
#include "../../include/vod_porting_all.h"
#include "rtsp_util.h"
#include "gx_common.h"

#define USER_AGENT "HangZhouGouXin"

#define ADD_FIELD(fmt, value) \
	ret = sprintf(buffer + *at, (fmt), (value)); \
*at += ret;

static void build_cmd(char *buffer, int *at, rtsp_client_t *client, rtsp_cmd_t *cmd)
{
	int ret;

	ADD_FIELD("CSeq: %u\r\n", client->seq);

	if (cmd && client->session)
	{
		ADD_FIELD("Session: %s\r\n", client->session);
	}
	if (cmd && cmd->range)
	{
		ADD_FIELD("Range: %s\r\n", cmd->range);
	}
	if (cmd && cmd->scale)
	{
		ADD_FIELD("Scale: %d\r\n", cmd->scale);
	}
	if (cmd && cmd->transport)
	{
		ADD_FIELD("Transport: %s\r\n", cmd->transport);
	}
	if (cmd && cmd->content_type)
	{
		ADD_FIELD("Content_Type: %s\r\n", cmd->content_type);
	}
	if (cmd && cmd->content)
	{
		ADD_FIELD("Content_Length: %d\r\n", strlen(cmd->content) + 6);
	}
	if (cmd && cmd->rebuild)
	{
		ADD_FIELD("Rebuild: position=%d\r\n", cmd->rebuild);
	}


//	ADD_FIELD("User-Agent: %s\r\n", USER_AGENT);

	if (cmd && cmd->content)
	{
		ADD_FIELD("\r\n%s\r\n", cmd->content);
	}

	if (cmd && cmd->keys)
	{
		ADD_FIELD("\r\n%s\r\n", cmd->keys);
	}

	ADD_FIELD("%s", "\r\n");

}

int coship_rtsp_send_command(rtsp_client_t *client, rtsp_cmd_t *cmd)
{
	int len;

	if(client->sock <= 0)
		return -1;

	len = sprintf(client->sendbuf, "%s %s RTSP/1.0\r\n", cmd->title, client->url);
	build_cmd(client->sendbuf, &len, client, cmd);

	GxVod_debug(">>>>> %s", client->sendbuf);

	if (vod_porting_socket_send(client->sock, client->sendbuf, len) <= 0)
	{
		GxVod_debug("rtsp send fail, len=%d\n", len);
		return -1;
	}
	return 0;
}

