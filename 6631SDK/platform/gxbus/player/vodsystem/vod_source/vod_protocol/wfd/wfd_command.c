#include "../../include/vod_in_common_def.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_common_func.h"
#include "wfd_util.h"
#include "gx_common.h"

#define WFD_ADD_FIELD(fmt, value) \
	ret = sprintf(buffer + *at, (fmt), (value)); \
*at += ret;

static void wfd_build_servercmd(char* buffer,
		int *at, wfd_client_t *client, wfd_servercmd_t *cmd)
{
	int ret;

	if(cmd && cmd->date)
	{
		WFD_ADD_FIELD("Date: %s\r\n", cmd->date);
	}
	if(cmd && cmd->server)
	{
		WFD_ADD_FIELD("Server: %s\r\n", cmd->server);
	}

	WFD_ADD_FIELD("CSeq: %u\r\n", client->server_seq);

	if(cmd && cmd->public)
	{
		WFD_ADD_FIELD("Public: %s\r\n", cmd->public);
	}
	if (cmd && cmd->content)
	{
		WFD_ADD_FIELD("Content-Length: %d\r\n", strlen(cmd->content) + 2);
	}
	if (cmd && cmd->content_type)
	{
		WFD_ADD_FIELD("Content-Type: %s\r\n", cmd->content_type);
	}

	if (cmd && cmd->content)
	{
		WFD_ADD_FIELD("\r\n%s\r\n", cmd->content);
	}
	if (cmd && cmd->keys)
	{
		WFD_ADD_FIELD("\r\n%s\r\n", cmd->keys);
	}

	//WFD_ADD_FIELD("%s", "\r\n");
}

int wfd_send_server_cmd(wfd_client_t *client, wfd_servercmd_t *cmd)
{
	int len;

	if(client->sock <= 0)
		return -1;

	len = sprintf(client->sendbuf, "RTSP/1.0 200 OK\r\n");
	wfd_build_servercmd(client->sendbuf, &len, client, cmd);

	if (vod_porting_socket_send(client->sock, client->sendbuf, len) <= 0)
	{
		gxlogd("[%s-%d]: send server command error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	client->server_seq++;
	gxlogd(">>>>> %s", client->sendbuf);
	return 0;
}

static void wfd_build_clientcmd(char *buffer,
		int *at, wfd_client_t *client, wfd_clientcmd_t *cmd)
{
	int ret;

	if(cmd && cmd->date)
	{
		WFD_ADD_FIELD("Date: %s\r\n", cmd->date);
	}
	if(cmd && cmd->server)
	{
		WFD_ADD_FIELD("Server: %s\r\n", cmd->server);
	}

	WFD_ADD_FIELD("CSeq: %u\r\n", client->client_seq);

	if (cmd && cmd->require)
	{
		WFD_ADD_FIELD("Require: %s\r\n", cmd->require);
	}
	if (cmd && client->session)
	{
		WFD_ADD_FIELD("Session: %s\r\n", client->session);
	}
	if (cmd && cmd->transport)
	{
		WFD_ADD_FIELD("Transport: %s\r\n", cmd->transport);
	}
	if (cmd && cmd->content)
	{
		WFD_ADD_FIELD("Content-Length: %d\r\n", strlen(cmd->content) + 2);
	}
	if (cmd && cmd->content_type)
	{
		WFD_ADD_FIELD("Content-Type: %s\r\n", cmd->content_type);
	}

	if (cmd && cmd->content)
	{
		WFD_ADD_FIELD("\r\n%s\r\n", cmd->content);
	}

	if (cmd && cmd->keys)
	{
		WFD_ADD_FIELD("\r\n%s\r\n", cmd->keys);
	}

	//WFD_ADD_FIELD("%s", "\r\n");
}

int wfd_send_client_cmd(wfd_client_t *client, wfd_clientcmd_t *cmd)
{
	int len;

	if(client->sock <= 0)
		return -1;

	len = sprintf(client->sendbuf, "%s %s RTSP/1.0\r\n", cmd->title, cmd->url);
	wfd_build_clientcmd(client->sendbuf, &len, client, cmd);

	if (vod_porting_socket_send(client->sock, client->sendbuf, len) <= 0)
	{
		gxlogd("[%s-%d]: send client command error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	gxlogd(">>>>> %s", client->sendbuf);
	return 0;

}
