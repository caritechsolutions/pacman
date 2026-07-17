#include "../../include/vod_in_common_def.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_common_func.h"
#include "rtsp_util.h"
#include "gx_common.h"

int coship_rtsp_recv_response_comm(rtsp_client_t *client, rtsp_resp_comm_t* resp, rtsp_time_info_t* time_info)
{
	char word[64];
	int ret;
	char* p;
	int body_len = 0;

	if(client->sock < 0)
		return -1;

	while (1)
	{
		ret = socket_recv_by_linebreak(client->sock, client->recvbuf, RTSP_RECV_BUFFER_SIZE, 200);//client->timeout);
		if (ret != 0)
			return ret;

		GxVod_debug("<<<<< %s\n", client->recvbuf);
		p = str_get_field(client->recvbuf, "CSeq", word);
		if (p)
			resp->seq = atoi(word);

		p = str_get_field(client->recvbuf, "RTSP/1.0", word);
		if (p)
			resp->retcode = atoi(word);

		p = strstr(client->recvbuf, "SET_PARAMETER");
		if (p)
		{
			p = str_get_field(client->recvbuf, "Locate", word);
			if (p)
			{
				if (strcmp(word, "EOS") == 0)
					resp->close = RESP_EOS;
				else
					resp->close = RESP_BOS;
				return 0;
			}
			p = str_get_field(client->recvbuf, "CLOSE", word);
			if (p)
			{
				resp->close = RESP_CLOSE;
				return 0;
			}

			body_len = 0;
			p = str_get_field(client->recvbuf, "Content-Length", word);
			if(p)
				body_len = atoi(word);
			if(body_len > 0)
			{
				memset(client->recvbuf, 0, RTSP_RECV_BUFFER_SIZE);
				ret = socket_recv_by_linebreak(client->sock, client->recvbuf, body_len, client->timeout);
				GxVod_debug("%s\n", client->recvbuf);
				if (ret == 0)
				{
					p = str_get_field(client->recvbuf, "Locate", word);
					if (p)
					{
						if (strcmp(word, "EOS") == 0)
							resp->close = RESP_EOS;
						else
							resp->close = RESP_BOS;
					}
					return 0;
				}
				else
				{
					return ret;
				}
			}
			return 0;
		}

		body_len = 0;
		p = str_get_field(client->recvbuf, "Content-Length", word);
		if (p)
		{
			if(p)
				body_len = atoi(word);
			if(body_len > 0)
			{
				memset(client->recvbuf, 0, RTSP_RECV_BUFFER_SIZE);
				ret = socket_recv_by_linebreak(client->sock, client->recvbuf, body_len, client->timeout);
				if (ret == 0)
				{
					GxVod_debug("%s\n", client->recvbuf);
					p = str_get_field(client->recvbuf, "position", word);
					if (p){
						if(client->shift_vod == 0){
							resp->position = atoi(word);
							time_info->really_time = (int64_t)(atoi(word)*1000);
							time_info->read_position_time = get_tick_time();
						}else if(client->shift_vod == 1){
							resp->position = client->shift_pos;//单位：s
							time_info->really_time = (int64_t)(client->shift_pos*1000);//单位：ms
							time_info->read_position_time = get_tick_time();
						}
					}
				}
				else
				{
					return ret;
				}
			}
		}

		if (resp->seq < client->seq)
		{
			GxVod_debug("rtsp recv old response, %d %d\n", resp->seq, client->seq);
			return RESP_ERR_SEQUENCE;
		}
		else if(resp->seq > client->seq)
		{
			GxVod_debug("rtsp recv response seq > cur_seq, impossible, %d, %d\n", resp->seq, client->seq);
			return RESP_ERR_PARSE;
		}
		else
		{
			break;
		}
	}

	if (resp->retcode != 200)
	{
		GxVod_debug("resp response code %d, not 200\n", resp->retcode);
		return RESP_ERR_CODE;
	}

	client->seq++;

	return 0;
}

int coship_rtsp_recv_response_describe(rtsp_client_t * client, rtsp_resp_describe_t * resp)
{
	char word[64];
	int ret;
	int body_len = 0;
	char* p;


	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, RTSP_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	GxVod_debug("<<<<< %s\n", client->recvbuf);
	p = str_get_field(client->recvbuf, "RTSP/1.0", word);
	if (p)
	{
		resp->retcode = atoi(word);
		if (resp->retcode != 200)
			return RESP_ERR_CODE;
	}
	else
		return RESP_ERR_PARSE;

	p = str_get_field(client->recvbuf, "CSeq", word);
	if (p)
	{
		resp->seq = atoi(word);
		if (resp->seq != client->seq)
			return RESP_ERR_PARSE;
	}
	else
		return RESP_ERR_PARSE;

	p = str_get_field(client->recvbuf, "Session", word);
	if (p)
		resp->session = str_save(word);
	else
		return RESP_ERR_PARSE;

	p = str_get_field(client->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if (socket_recv_by_length(client->sock, client->recvbuf, body_len, 2000) < 0)
	{
		str_free(resp->session);
		resp->session = 0;
		return RESP_ERR_SOCK;
	}
	GxVod_debug("%s\n", client->recvbuf);

	p = str_get_field(client->recvbuf, "ProgramNo", word);
	if (p)
		resp->program_no= atoi(word);	

	p = str_get_field(client->recvbuf, "Frequency", word);
	if (p)
		resp->freq = atoi(word);	

	p = str_get_field(client->recvbuf, "Modulation", word);
	if (p)
		resp->qamsize = atoi(word);			

	p = str_get_field(client->recvbuf, "SymbolRate", word);
	if (p)
		resp->symbrate = atoi(word);	

	p = str_get_field(client->recvbuf, "StartTime", word);
	if (p)
		resp->start_time = str_save(word);

	p = str_get_field(client->recvbuf, "Duration", word);
	if (p)
		resp->duration = atoi(word);

	p = str_get_field(client->recvbuf, "TrickMode", word);
	if (p)
		resp->trickmode = atoi(word);

	if (resp->trickmode)
	{
		p = str_get_field(client->recvbuf, "TrickSpeeds", word);
		if (p)
		{
			if (strstr(word, "32"))
				resp->trickmode = 32;
			else if (strstr(word, "16"))
				resp->trickmode = 16;
			else if (strstr(word, "8"))
				resp->trickmode = 8;
			else if (strstr(word, "4"))
				resp->trickmode = 4;
			else if (strstr(word, "2"))
				resp->trickmode = 2;
			else
				resp->trickmode = 0;
		}
	}

	p = str_get_field(client->recvbuf, "VideoPID", word);
	if (p)
		resp->videopid = atoi(word);

	p = str_get_field(client->recvbuf, "AudioPID", word);
	if (p)
		resp->audiopid = atoi(word);

	p = str_get_field(client->recvbuf, "PcrPID", word);
	if (p)
		resp->pcrpid = atoi(word);

	client->seq++;

	return 0;
}

int coship_rtsp_recv_response_setup(rtsp_client_t * client, rtsp_resp_setup_t * resp, rtsp_resp_describe_t * describe_resp)
{
	char word[64];
	int ret;
	char* p;

	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, RTSP_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	GxVod_debug("<<<<< %s\n", client->recvbuf);
	p = str_get_field(client->recvbuf, "RTSP/1.0", word);
	if (p)
	{
		resp->retcode = atoi(word);
		if (resp->retcode != 200)
			return RESP_ERR_CODE;
	}
	else
		return RESP_ERR_PARSE;


	p = str_get_field(client->recvbuf, "CSeq", word);
	if (p)
	{
		resp->seq = atoi(word);
		if (resp->seq != client->seq)
			return RESP_ERR_PARSE;
	}
	else
		return RESP_ERR_PARSE;

	p = str_get_field(client->recvbuf, "destination", word);
	if (p)
		resp->dst_ip= str_save(word);

	p = str_get_field(client->recvbuf, "client_port", word);
	if (p)
		resp->dst_port = atoi(word);	

	p = str_get_field(client->recvbuf, "keystring", word);
	if (p)
		resp->keystring = str_save(word);

	p = str_get_field(client->recvbuf, "ProgramNo", word);
	if (p)
		describe_resp->program_no= atoi(word);	
	p = str_get_field(client->recvbuf, "Frequency", word);
	if (p)
		describe_resp->freq = atoi(word);
	p = str_get_field(client->recvbuf, "Modulation", word);
	if (p)
		describe_resp->qamsize = atoi(word);
	p = str_get_field(client->recvbuf, "SymbolRate", word);
	if (p)
		describe_resp->symbrate = atoi(word);

	client->seq++;

	return 0;
}

