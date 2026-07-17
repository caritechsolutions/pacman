#include"gx_common.h"
#include "tcp.h"

static struct stream_priv_s {
	char* user;
	char* pass;
	char* host;
	int port;
	char* filename;

	char* cput, *cget;
	int handle;
	int cavail, cleft;
	char* buf;
} stream_priv_dflts = {
	"anonymous", "no@spam", NULL, 21, NULL, NULL, NULL, 0, 0, 0, NULL
};

#define BUFSIZE 2048

#define ST_OFF(f) M_ST_OFF(struct stream_priv_s,f)
/// URL definition

#define TELNET_IAC      255	/* interpret as command:*/
#define TELNET_IP       244	/* interrupt process--permanently*/
#define TELNET_SYNCH    242	/* for telfunc calls*/

unsigned int ftp_seekable = 0;
// Check if there is something to read on a fd. This avoid hanging
// forever if the network stop responding.
static int fd_can_read(int fd, int timeout)
{
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	return (select(fd + 1, &fds, NULL, NULL, &tv) > 0);
}

/*
 *  read a line of text
 *
 *  return -1 on error or bytecount
 */
static int readline(char *buf, int max, struct stream_priv_s *ctl)
{
	int x, retval = 0;
	char* end, *bp = buf;
	int eof = 0;

	do {
		if (ctl->cavail > 0) {
			x = (max >= ctl->cavail) ? ctl->cavail : max - 1;
			end = memccpy(bp, ctl->cget, '\n', x);
			if (end != NULL)
				x = end - bp;
			retval += x;
			bp += x;
			*bp = '\0';
			max -= x;
			ctl->cget += x;
			ctl->cavail -= x;
			if (end != NULL) {
				bp -= 2;
				if (strcmp(bp, "\r\n") == 0) {
					*bp++ = '\n';
					*bp++ = '\0';
					--retval;
				}
				break;
			}
		}
		if (max == 1) {
			*buf = '\0';
			break;
		}
		if (ctl->cput == ctl->cget) {
			ctl->cput = ctl->cget = ctl->buf;
			ctl->cavail = 0;
			ctl->cleft = BUFSIZE;
		}
		if (eof) {
			if (retval == 0)
				retval = -1;
			break;
		}

		if (!fd_can_read(ctl->handle, 15)) {
			gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] read timed out\n");
			retval = -1;
			break;
		}

		if ((x = recv(ctl->handle, ctl->cput, ctl->cleft, 0)) == -1) {
			gx_msg(MSGT_STREAM, MSGL_ERR, "[ftp] read error: %s\n", strerror(errno));
			retval = -1;
			break;
		}
		if (x == 0)
			eof = 1;
		ctl->cleft -= x;
		ctl->cavail += x;
		ctl->cput += x;
	} while (1);

	return retval;
}

/*
 *  read a response from the server
 *
 *  return 0 if first char doesn't match
 *  return 1 if first char matches
 */
static int readresp(struct stream_priv_s *ctl, char *rsp)
{
	static char response[256];
	char match[5];
	int r;

	if (readline(response, 256, ctl) == -1)
		return 0;

	r = atoi(response) / 100;
	if (rsp)
		strcpy(rsp, response);

	gx_msg(MSGT_STREAM, MSGL_V, "[ftp] < %s", response);

	if (response[3] == '-') {
		strncpy(match, response, 3);
		match[3] = ' ';
		match[4] = '\0';
		do {
			if (readline(response, 256, ctl) == -1) {
				gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] Control socket read failed\n");
				return 0;
			}
			gx_msg(MSGT_OPEN, MSGL_V, "[ftp] < %s", response);
		} while (strncmp(response, match, 4));
	}
	return r;
}

static int FtpSendCmd(const char *cmd, struct stream_priv_s *nControl, char *rsp)
{
	int l = strlen(cmd);
	int hascrlf = cmd[l - 2] == '\r' && cmd[l - 1] == '\n';

	if (hascrlf && l == 2)
		gx_msg(MSGT_STREAM, MSGL_V, "\n");
	else
		gx_msg(MSGT_STREAM, MSGL_V, "[ftp] > %s", cmd);

	while (l > 0) {
		int s = send(nControl->handle, cmd, l, 0);

		if (s <= 0) {
			gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] write error: %s\n", strerror(errno));
			return 0;
		}

		cmd += s;
		l -= s;
	}

	if (hascrlf)
		return readresp(nControl, rsp);
	else
		return FtpSendCmd("\r\n", nControl, rsp);
}

static int FtpOpenPort(struct stream_priv_s *p, PLAYER_INTERRUPT_CBK interrupt_cbk)
{
	int resp, fd;
	char rsp_txt[256];
	char* par, str[128];
	int num[6];

	resp = FtpSendCmd("PASV", p, rsp_txt);
	if (resp != 2) {
		gx_msg(MSGT_OPEN, MSGL_WARN, "[ftp] command 'PASV' failed: %s\n", rsp_txt);
		return 0;
	}

	par = strchr(rsp_txt, '(');

	if (!par || !par[0] || !par[1]) {
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] invalid server response: %s ??\n", rsp_txt);
		return 0;
	}

	sscanf(par + 1, "%u,%u,%u,%u,%u,%u", &num[0], &num[1], &num[2], &num[3], &num[4], &num[5]);
	sngxlogf(str, 127, "%d.%d.%d.%d", num[0], num[1], num[2], num[3]);
	fd = connect_2_server(str, (num[4] << 8) + num[5], 0, interrupt_cbk);

	if (fd < 0)
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] failed to create data connection\n");

	return fd;
}

static int FtpOpenData(GxStream *s, size_t newpos)
{
	struct stream_priv_s* p = s->priv;
	int resp;
	char str[256], rsp_txt[256];

	// Open a new connection
	s->fd = FtpOpenPort(p, s->interrupt_cbk);

	if (s->fd < 0)
		return 0;

	if (newpos > 0) {
		sngxlogf(str, 255, "REST %lld", (int64_t) newpos);

		resp = FtpSendCmd(str, p, rsp_txt);
		if (resp != 3) {
			gx_msg(MSGT_OPEN, MSGL_WARN, "[ftp] command '%s' failed: %s\n", str, rsp_txt);
			newpos = 0;
		}
	}
	// Get the file
	sngxlogf(str, 255, "RETR %s", p->filename);
	resp = FtpSendCmd(str, p, rsp_txt);

	if (resp != 1) {
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] command '%s' failed: %s\n", str, rsp_txt);
		return 0;
	}

	s->pos = newpos;
	return 1;
}

static int fill_buffer(GxStream *s, uint8_t *buffer, size_t max_len)
{
	int r;

	if (s->fd < 0 && !FtpOpenData(s, s->pos))
		return -1;

	if (!fd_can_read(s->fd, 15)) {
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] read timed out\n");
		return -1;
	}

	r = recv(s->fd, buffer, max_len, 0);
	return (r <= 0) ? -1 : r;
}

static int seek(GxStream *s, off_t newpos)
{
	struct stream_priv_s* p = s->priv;
	int resp;
	char rsp_txt[256];

	if (s->pos > s->end_pos) {
		s->eof = 1;
		return 0;
	}
	// Check to see if the server did not already terminate the transfer
	if (fd_can_read(p->handle, 0)) {
		if (readresp(p, rsp_txt) != 2)
			gx_msg(MSGT_OPEN, MSGL_WARN,
					"[ftp] Warning the server didn't finished the transfer correctly: %s\n", rsp_txt);
		closesocket(s->fd);
		s->fd = -1;
	}
	// Close current download
	if (s->fd >= 0) {
		static const char pre_cmd[] = { TELNET_IAC, TELNET_IP, TELNET_IAC, TELNET_SYNCH };
		//int fl;

		// First close the fd
		closesocket(s->fd);
		s->fd = 0;

		// Send send the telnet sequence needed to make the server react

		// Dunno if this is really needed, lftp have it. I let
		// it here in case it turn out to be needed on some other OS
		//fl=fcntl(p->handle,F_GETFL);
		//fcntl(p->handle,F_SETFL,fl&~O_NONBLOCK);

		// send only first byte as OOB due to OOB braindamage in many unices
		send(p->handle, pre_cmd, 1, MSG_OOB);
		send(p->handle, pre_cmd + 1, sizeof(pre_cmd) - 1, 0);

		//fcntl(p->handle,F_SETFL,fl);

		// Get the 426 Transfer aborted
		// Or the 226 Transfer complete
		resp = readresp(p, rsp_txt);
		if (resp != 4 && resp != 2) {
			gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] Server didn't abort correctly: %s\n", rsp_txt);
			s->eof = 1;
			return 0;
		}
		// Send the ABOR command
		// Ignore the return code as sometimes it fail with "nothing to abort"
		FtpSendCmd("ABOR", p, rsp_txt);
	}
	return FtpOpenData(s, newpos);
}

static int ftp_seek(GxStream *s, off_t newpos)
{
	if (ftp_seekable) {
		return seek(s, newpos);
	}

	else {
		return -1;
	}

}

static void close_f(GxStream *s)
{
	struct stream_priv_s* p = s->priv;

	if (!p)
		return;

	if (s->fd > 0) {
		closesocket(s->fd);
		s->fd = 0;
	}

	FtpSendCmd("QUIT", p, NULL);

	if (p->handle)
		closesocket(p->handle);
	if (p->buf)
		av_free(p->buf);
	if (p->user)
		av_free(p->user);
	if (p->pass)
		av_free(p->pass);
	if (p->host)
		av_free(p->host);
	if (p->filename)
		av_free(p->filename);

	//m_struct_free(&stream_opts,p);
	ftp_seekable = 0;
	av_free(p);
}

static int parse_url(char* url, struct stream_priv_s *ftp_header)
{
	//struct stream_priv_s* ftp_header =* ftp_head;
	char* user = NULL, *pass = NULL, *host = NULL, *filename = NULL;
	int port;
	char* ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL, *ptr4 = NULL;
	int ptr1_len, ptr2_len, ptr3_len;
	//ftp_header = av_malloc(sizeof(struct stream_priv_s));

	ptr1 = strstr(url, "://");

	if (ptr1 == NULL) {
		gxlogf("url error\n");
		return 0;
	}

	ptr1 += 3;

	ptr2 = strstr(ptr1, "@");
	ptr3 = strstr(ptr1, "/");

	if (ptr3 != NULL && ptr3 < ptr2) {
		ptr2 = NULL;
		gxlogf("not a real username\n");
	}

	if (ptr2 != NULL) {
		ptr3 = strstr(ptr1, ":");
		ptr1_len = strlen(ptr1);
		ptr2_len = strlen(ptr2);
		ptr3_len = strlen(ptr3);

		gxlogf("ptr1_len:%d\nptr2_len:%d\nptr3_len:%d\n", ptr1_len, ptr2_len, ptr3_len);

		//get the username and password
		//      memcpy(ftp_header->pass,ptr3+1,sizeof(ptr3)-sizeof(ptr2));
		//user = av_malloc(ptr1_len - ptr3_len + 1);
		user = (char* )av_malloc(5);
		//gxlogf("ptr1_len-ptr3_len:%d\n",ptr1_len-ptr3_len);
		pass = (char* )av_malloc(ptr3_len - ptr2_len);
		memcpy(user, ptr1, ptr1_len - ptr3_len);
		user[ptr1_len - ptr3_len] = '\0';
		//gxlogf("av_malloc size:%d\n",strlen(user));
		//ptr4 = av_malloc(4);
		//ftp_header->user = memcpy(ptr4, "list",4);
		memcpy(pass, ptr3 + 1, ptr3_len - ptr2_len - 1);
		pass[ptr3_len - ptr2_len - 1] = '\0';
	} else {
		user = av_malloc(sizeof("anonymous"));
		pass = av_malloc(sizeof("no@spam"));
		user = "anonymous";
		pass = "no@spam";
	}

	if (ptr2 == NULL) {
		ptr3 = strstr(ptr1, ":");
		ptr2 = ptr1 - 1;
	} else {
		ptr3 = strstr(ptr2, ":");
	}

	if (ptr3 == NULL) {
		ptr2_len = strlen(ptr2);
		gxlogf("ptr2_len:%d\n", ptr2_len);
		host = (char* )av_malloc(ptr2_len);
		host = ptr2;
		port = 21;
		filename = NULL;
		return 1;
	} else {
		ptr2_len = strlen(ptr2);
		ptr3_len = strlen(ptr3);
		gxlogf("ptr2_Len:%d\nptr3_len:%d\n", ptr2_len, ptr3_len);
		host = (char* )av_malloc(ptr2_len - ptr3_len);
		memcpy(host, ptr2 + 1, ptr2_len - ptr3_len - 1);
		host[ptr2_len - ptr3_len - 1] = '\0';
	}

	ptr2 = strstr(ptr3, "/");
	if (ptr2 == NULL) {
		port = atoi(ptr3 + 1);
		filename = NULL;
	} else {
		ptr2_len = strlen(ptr2);
		ptr3_len = strlen(ptr3);
		ptr4 = av_malloc(ptr3_len - ptr2_len);
		memcpy(ptr4, ptr3 + 1, ptr3_len - ptr2_len);
		ptr4[ptr3_len - ptr2_len] = '\0';
		port = atoi(ptr4);
		av_free(ptr4);
		filename = av_malloc(ptr2_len);
		strcpy(filename, ptr2 + 1);
	}

	ftp_header->user = user;
	ftp_header->pass = pass;
	ftp_header->host = host;
	ftp_header->port = port;
	ftp_header->filename = filename;

	return 1;
}

static int open_f(GxStream *stream, int mode)
{
	int len = 0, resp;
	struct stream_priv_s* p;
	char str[256], rsp_txt[256];

	p = av_malloc(sizeof(struct stream_priv_s));
	stream->priv = p;

	parse_url(stream->url, p);

	gxlogf("user:%s\n", p->user);
	//gxlogf("user lenth:%d\n",strlen(p->user));
	gxlogf("pass:%s\n", p->pass);
	gxlogf("host:%s\n", p->host);
	gxlogf("port:%d\n", p->port);
	gxlogf("filename:%s\n", p->filename);

	if (mode != GX_STREAM_READ) {
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] Unknown open mode %d\n", mode);
		close_f(stream);
		return GX_PLAYER_ERROR;
	}

	if (!p->filename || !p->host) {
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] Bad url\n");
		close_f(stream);
		return GX_PLAYER_ERROR;
	}
	// Open the control connection
	p->handle = connect_2_server(p->host, p->port, 1, stream->interrupt_cbk);

	if (p->handle < 0) {
		close_f(stream);
		return GX_PLAYER_ERROR;
	}
	// We got a connection, let's start serious things
	stream->fd = -1;
	stream->priv = p;
	p->buf = av_malloc(BUFSIZE);

	if (readresp(p, NULL) == 0) {
		close_f(stream);
		//m_struct_free(&stream_opts,opts);
		av_free(p);
		return GX_PLAYER_ERROR;
	}
	// Login
	sngxlogf(str, 255, "USER %s", p->user);
	resp = FtpSendCmd(str, p, rsp_txt);

	// password needed
	if (resp == 3) {
		sngxlogf(str, 255, "PASS %s", p->pass);
		resp = FtpSendCmd(str, p, rsp_txt);
		if (resp != 2) {
			gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] command '%s' failed: %s\n", str, rsp_txt);
			close_f(stream);
			return GX_PLAYER_ERROR;
		}
	} else if (resp != 2) {
		gx_msg(MSGT_OPEN, MSGL_ERR, "[ftp] command '%s' failed: %s\n", str, rsp_txt);
		close_f(stream);
		return GX_PLAYER_ERROR;
	}
	// Set the transfer type
	resp = FtpSendCmd("TYPE I", p, rsp_txt);
	if (resp != 2) {
		gx_msg(MSGT_OPEN, MSGL_WARN, "[ftp] command 'TYPE I' failed: %s\n", rsp_txt);
		close_f(stream);
		return GX_PLAYER_ERROR;
	}
	// Get the filesize
	sngxlogf(str, 255, "SIZE %s", p->filename);
	resp = FtpSendCmd(str, p, rsp_txt);
	if (resp != 2) {
		gx_msg(MSGT_OPEN, MSGL_WARN, "[ftp] command '%s' failed: %s\n", str, rsp_txt);
	} else {
		int dummy;
		sscanf(rsp_txt, "%d %d", &dummy, &len);
	}

	if (len > 0) {
		ftp_seekable = 1;
		stream->end_pos = len;
	}
	// The data connection is really opened only at the first
	// read/seek. This must be done when the cache is used
	// because the connection would stay open in the main process,
	// preventing correct abort with many servers.
	stream->fd = -1;
	stream->priv = p;
	//stream->fill_buffer = fill_buffer;
	//stream->close = close_f;

	return GX_PLAYER_OK;
}

extern int streambase_transmission(GxMediaFilter*  filter, void *data, size_t size);

GxStreamClass gx_stream_ftp = {
	._inherit = {
		._inherit = {
			.name = "StreamFtp",
			.parent = &gx_streambase,
			.size = sizeof(GxStream),
			.version = {7, 2},
		},
	},
	.protocols = {"ftp", NULL},
	DEF_AUTHOR("Stream","ftp","No description","L.F","No comment"),

	.flags   = GX_STREAM_NEED_PROBE,
	.open    = open_f,
	.read    = fill_buffer,
	.write   = NULL,
	.seek    = ftp_seek,
	.control = NULL,
	.close   = close_f,
};

#ifdef FTP_TEST

int main()
{

}

#endif
