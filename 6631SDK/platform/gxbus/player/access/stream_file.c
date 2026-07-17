#include <string.h>

#include "gx_stream.h"
#include "gx_fileops.h"

// /1/2/3/4/5/6/7
#ifndef strsep
extern char *strsep(char **stringp, const char *delim);
#endif

static int stream_mkdir(const char* url)
{
	char* path = av_strdup(url);
	char* now = path+1;

	while(1) {
		now = strchr(now,'/');
		if (now) {
			*now = '\0';
			if(GxCore_FileExists(path) == GXCORE_FILE_EXIST) {
				*now = '/';
				now++;
			}
			else {
				if(GxCore_Mkdir(path)!= GXCORE_SUCCESS) {
					av_free(path);
					return -1;
				}
				*now = '/';
				now++;
			}
		}
		else {
			av_free(path);
			return 0;
		}
	}
}

static int file_open(GxStream *s, int mode)
{
	mode_t m = 0;
	char* filename;

	if (s == NULL)
		return GX_PLAYER_ERROR;

	s->fd = -1;
	if (s->url == NULL)
		return GX_PLAYER_ERROR;

	switch(mode) {
		case GX_STREAM_READ:
			m = O_RDONLY;
			break;

		case GX_STREAM_WRITE:
			m = O_WRONLY | O_CREAT;
			break;

		case GX_STREAM_TRUNC:
			m =  O_RDWR | O_CREAT | O_TRUNC;
			break;

		case GX_STREAM_APPEND:
			m = O_RDWR | O_CREAT | O_APPEND;
			break;
		default:
			gxlogf("[File] Unknown open mode %d\n", mode);
			return GX_PLAYER_ERROR;
	}
	if(strstr(s->url, "?") && strstr(s->url, "=")){
		char *query, *q, *name, *value;
		int pos = strcspn(s->url, "?");
		s->url[pos]= '\0';
		query = s->url + pos + 1;
		q = query;
		int len = strlen(query);
		while(strsep(&q, "&"));
		for(q = query; q < (query + len);) {
			value = name = q;
			/*   Skip to next assignment */
			for(q += strlen(q); q < (query + len) && !*q; q++);
			/*   Assign variable */
			name = strsep(&value,"=");
			if(name && value && !strcmp(name, "quiet") && !strcmp(value, "1")){
				s->isquiet = 1;
				break;
			}
		}
	}
	filename = strstr(s->url, "://");

	if (filename != NULL)
		filename += 3;
	else
		filename = s->url;

	s->fops = GxFileGetOps(filename);
	s->isdvr = strstr(filename, ".dvr") ? 1 : 0;
	s->ispvr  = strstr(filename, ".pvr") ? 1 : 0;
	s->ispvr |= strstr(filename, ".seg") ? 1 : 0;
	s->ispvr |= strstr(filename, ".evt") ? 1 : 0;
	s->ispvr |= strstr(filename, ".lst") ? 1 : 0;

	if(s->fops) {
		s->fd = s->fops->open(filename, m);
		if (s->fd == -1) {
			if (m & O_WRONLY || m & O_RDWR) {
				if (stream_mkdir(s->url) == 0) {
					s->fd = s->fops->open(filename, m);
					if(s->fd == -1) {
						gxlogf("[File] Can't Open File ; %s!\n", filename);
						return GX_PLAYER_ERROR;
					}
				}
				else {
					gxlogf("[File] Mkdir Failed ; %s!\n", filename);
					return GX_PLAYER_ERROR;
				}
			}
			else
				return GX_PLAYER_ERROR;
		}
		s->end_pos     = s->fops->getsize(s->fd);
		s->file_format = GX_STREAMTYPE_FILE;
		s->fops->seek(s->fd,0,SEEK_SET);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int file_read(GxStream *s, uint8_t *buffer, size_t max_len)
{
	int r = 0;
	off_t outpos = 0;
	if(s->fops->read_ext){
		r = s->fops->read_ext(s->fd, buffer, max_len, s->read_timeoutms, &outpos);
		if(r > 0)
			s->pos = outpos;
	}else{
		r = s->fops->read(s->fd, buffer, max_len, s->read_timeoutms);
	}

	return (r <= 0) ? -1 : r;
}

static int file_write(GxStream *s, uint8_t *buffer, size_t len)
{
	int r = s->fops->write(s->fd, buffer, len);

	return (r <= 0) ? -1 : r;
}

static int file_seek(GxStream *s, off_t newpos)
{
	off_t outpos = 0;
	s->pos = newpos;
	if(s->fops->seek_ext){
		if (s->fops->seek_ext(s->fd, s->pos, SEEK_SET, &outpos) < 0){
			return GX_PLAYER_ERROR;
		}else{
			s->pos = outpos;
		}
	}else{
		if (s->fops->seek(s->fd, s->pos, SEEK_SET) < 0)
			return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

static int file_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
		case GX_STREAM_CTRL_GET_SIZE:
		{
			off_t* size = (off_t*)arg;
			*size = s->end_pos = s->fops->getsize(s->fd);
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_RES_SIZE:
		{
			off_t* size = (off_t*)arg;
			*size = (s->fops->getsize(s->fd) - s->pos);
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_VOL_INFO:
		{
			if(s->fops->getvolinfo){
				struct vol_info *info = (struct vol_info*)arg;
				*info = s->fops->getvolinfo(s->fd);
			}else{
				struct vol_info *info = (struct vol_info*)arg;
				info->size = s->fops->getsize(s->fd);
				info->truesize = info->size;
			}
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_CAP_SIZE:
		{
			int64_t *size = (int64_t*)arg;
			GxDiskInfo Info;

			if(size) {
				*size = -1;
				if (GxCore_DiskInfo(s->url, &Info) == GXCORE_SUCCESS) {
					*size = (int64_t)(Info.freed_size);
					return GX_PLAYER_OK;
				}
			}
			return GX_PLAYER_ERROR;
		}
		case GX_STREAM_CTRL_GET_ERROR_CODE:
		{
			int* error = (int*)arg;

			if (s->fops->geterrorcode)
				*error = s->fops->geterrorcode(s->fd);
			else
				*error = 0;

			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_SET_TIMESTAMP:
		{
			if(s->fops->settimestamp){
				s->fops->settimestamp(s->fd, *(unsigned int*)arg);
			}
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_NEWVOLUME:
		{
			if (s->fops->getnewvolume) {
				s->fops->getnewvolume(s->fd, (struct vol_new *)arg);
				return GX_PLAYER_OK;
			}
			return GX_PLAYER_ERROR;
		}
		case GX_STREAM_CTRL_RESET:
		{
			if (s->fops)
				file_seek(s, 0);

			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_WRITE_HDR:
		{
			int ret = 0;
			struct vol_hdr *hdr = (struct vol_hdr *)arg;

			if (s->fops->write_hdr)
				ret = s->fops->write_hdr(s->fd, hdr->buffer, hdr->size);

			return (ret >= 0) ? GX_PLAYER_OK : GX_PLAYER_ERROR;
		}
		case GX_STREAM_CTRL_READ_HDR:
		{
			int ret = 0;
			struct vol_hdr *hdr = (struct vol_hdr *)arg;

			if (s->fops->read_hdr)
				ret = s->fops->read_hdr(s->fd, hdr->buffer, hdr->size, 0);

			return (ret >= 0) ? GX_PLAYER_OK : GX_PLAYER_ERROR;
		}
		case GX_STREAM_CTRL_PROBE_HDR:
		{
			int ret = 0;

			if (s->fops->probe_hdr)
				ret = s->fops->probe_hdr(s->fd);
			*(int *)arg = ret;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_SYNC_DATA:
		{
			int ret = 0;

			if (s->fops->sync_data)
				ret = s->fops->sync_data(s->fd);

			return (ret >= 0) ? GX_PLAYER_OK : GX_PLAYER_ERROR;
		}
		case GX_STREAM_CTRL_PVR_SET_CONTROL:
		{
			int ret = -1;
			if(s->fops->set_pvrctrl)
				ret = s->fops->set_pvrctrl(s->fd, (GxRecordPVRControl *)arg);
			return ret;
		}
		case GX_STREAM_CTRL_PVR_GET_CONTROL:
		{
			int ret = -1;
			if (s->fops->get_pvrctrl)
				ret = s->fops->get_pvrctrl(s->fd, (GxRecordPVRControl *)arg);
			return ret;
		}
		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

static void file_close(GxStream *s)
{
	if(s && s->fd != -1){
		s->fops->close(s->fd);
		s->fd = -1;
	}
}

static int file_stop(GxMediaFilter *filter){
	GxStream*  s = GXSTREAM(filter);

	if(s && s->fd != -1){
		s->fops->close(s->fd);
		s->fd = -1;
	}
	return GX_PLAYER_OK;
}

extern int GxVolumeFile_Init(void);
static int file_init(void)
{
	return GxVolumeFile_Init();
}

GxStreamClass gx_stream_file = {
	._inherit = {
		._inherit = {
			.name   = "StreamFile",
			.parent = &gx_streambase,
			.size   = sizeof(GxStream),
			.init   = file_init,
		},
		.run     = NULL,
		.stop    = file_stop,
		.pause   = NULL,
		.resume  = NULL,
		.config  = NULL,
	},
	.protocols = {"file", "", NULL},
	DEF_AUTHOR("Stream","file","No description","L.F","No comment"),

	.flags   = GX_STREAM_NEED_PROBE,
	.open    = file_open,
	.read    = file_read,
	.write   = file_write,
	.seek    = file_seek,
	.control = file_control,
	.close   = file_close,
};
