
#include"gx_common.h"

#include <libsmbclient.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "url.h"

int smbseekable = 0;
static char smb_password[15];
static char smb_username[15];

static void smb_auth_fn(const char* server, const char *share,
			char* workgroup, int wgmaxlen, char *username, int unmaxlen, char *password, int pwmaxlen)
{
/*
	char temp[128];

	strcpy(temp, "LAN");
	if (temp[strlen(temp) - 1] == 0x0a)
		temp[strlen(temp) - 1] = 0x00;

	if (temp[0])
		strncpy(workgroup, temp, wgmaxlen - 1);

	strcpy(temp, smb_username);
	if (temp[strlen(temp) - 1] == 0x0a)
		temp[strlen(temp) - 1] = 0x00;

	if (temp[0])
		strncpy(username, temp, unmaxlen - 1);

	strcpy(temp, smb_password);
	if (temp[strlen(temp) - 1] == 0x0a)
		temp[strlen(temp) - 1] = 0x00;

	if (temp[0])
		strncpy(password, temp, pwmaxlen - 1);
*/
	strncpy(password, smb_password, pwmaxlen - 1);
	strncpy(username, smb_username, unmaxlen - 1);
	gxlogf("%s:(%S:%s)\n", __FUNCTION__, username, password);
}

static int control(GxStream*  s, int cmd, va_list arg)
{
	switch (cmd) {
	case GX_STREAM_CTRL_GET_SIZE:{
			off_t size = smbc_lseek(s->fd, 0, SEEK_END);
			smbc_lseek(s->fd, s->pos, SEEK_SET);
			if (size != (off_t) - 1) {
				*((off_t* ) arg) = size;
				gxlogf("file_size : %d\n", size);
				//&arg = size;
				return 1;
			}
		}
	}
	return GX_PLAYER_ERROR;
}

static int seek(GxStream*  s, off_t newpos)
{
	s->pos = newpos;
	if (smbseekable) {
		if (smbc_lseek(s->fd, s->pos, SEEK_SET) < 0) {
			s->eof = 1;
			return 0;
		}

		return 1;
	} else
		return -1;
}

static int fill_buffer(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	int r = smbc_read(s->fd, buffer, max_len);
	return (r <= 0) ? -1 : r;
}

static int write_buffer(GxStream*  s, uint8_t * buffer, size_t len)
{
//      struct stat* buf;
//      int error;
	//int smbchmod;
//      buf = av_malloc(sizeof(struct stat));
//      error = smbc_fstat(s->fd,buf);
//      gxlogf("the file mode:%o\n",buf->st_mode);
	//smbchmod = smbc_chmod(s->url,1023);
	//gxlogf("smbchmod:%d\n",smbchmod);
	//error = smbc_fstat(s->fd,buf);
	//gxlogf("after chmod the file mode:%o\n",buf->st_mode);

	int r = smbc_write(s->fd, buffer, len);
	if (r < 0) {
		gxlogf("error status:%d\n", errno);
	}
	return (r <= 0) ? -1 : r;
}

static void close_f(GxStream*  s)
{
	smbc_close(s->fd);
}

static int open_f(GxStream*  stream, int mode)
{
	// struct stream_priv_s* p = (struct stream_priv_s*)opts;
	mode_t m = 0;
	off_t len;
	int fd, err;
	GxURL* url;
	char newurl[512];
	int smbchmod;
	if (mode == GX_STREAM_READ)
		m = O_RDONLY;
	else if (mode == GX_STREAM_WRITE)	//who's gonna do that ?
	{
		m = O_RDWR | O_CREAT | O_TRUNC;
		gxlogf("open with writeable\n");
	} else {
		gxlogf("[smb] Unknown open mode %d\n", mode);
		//m_struct_free (&stream_opts, opts);
		return GX_PLAYER_ERROR;
	}

	if (!stream->url) {
		gxlogf("[smb] Bad url\n");
		//m_struct_free(&stream_opts, opts);
		return GX_PLAYER_ERROR;
	}

	url = url_new(stream->url);
	strncpy(smb_username, url->username, 14);
	strncpy(smb_password, url->password, 14);

	snprintf(newurl, sizeof(newurl), "%s://%s%s", url->protocol, url->hostname, url->file);

	url_free(url);

	err = smbc_init(smb_auth_fn, 1);
	if (err < 0) {
		//m_struct_free(&stream_opts, opts);
		return GX_PLAYER_ERROR;
	}

	gxlogf("filename=%s\n", newurl);

	fd = smbc_open(newurl, m, 0755);
	if (fd < 0) {
		//m_struct_free(&stream_opts, opts);
		return GX_PLAYER_ERROR;
	}

	stream->flags = mode;
	len = 0;
	if (mode == GX_STREAM_READ) {
		len = smbc_lseek(fd, 0, SEEK_END);
		smbc_lseek(fd, 0, SEEK_SET);
	}
	if (len > 0 || mode == GX_STREAM_WRITE) {
		stream->flags |= GX_STREAM_SEEK;
		smbseekable = 1;
		if (mode == GX_STREAM_READ)
			stream->end_pos = len;
	}
	stream->file_format = GX_STREAMTYPE_SMB;
	stream->fd = fd;
	//stream->fill_buffer = fill_buffer;
	//stream->write_buffer = write_buffer;
	//stream->close = close_f;
	//stream->control = control;

	//m_struct_free(&stream_opts, opts);
	return GX_PLAYER_OK;
}

extern int streambase_transmission(GxMediaFilter*  fileter, void *data, size_t size);

GxStreamClass gx_stream_smb = {
	._inherit = {
		._inherit = {
			.name    = "streamSmb",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStream),
			.version = {7, 2},
		},
	},
	.protocols = {"smb", NULL},
	DEF_AUTHOR("Stream","smb","No description","L.F","No comment"),

	.flags   = 0,
	.open    = open_f,
	.read    = fill_buffer,
	.write   = write_buffer,
	.seek    = seek,
	.control = control,
	.close   = close_f,
};
