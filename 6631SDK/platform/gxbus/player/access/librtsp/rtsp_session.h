/*
 * This file was ported to MPlayer from xine CVS rtsp_session.h,v 1.4 2003/01/31 14:06:18
 */

/*
 * Copyright (C) 2000-2002 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 *
 * high level interface to rtsp servers.
 *
 *    2006, Benjamin Zores and Vincent Mussard
 *      Support for MPEG-TS streaming through RFC compliant RTSP servers
 */

#ifndef HAVE_RTSP_SESSION_H
#define HAVE_RTSP_SESSION_H

#include"gx_network.h"
#include"gx_stream.h"

typedef struct gx_stream_rtsp{
        GxStream parent;
        GxStreamingCtrl *streaming_ctrl;
}GxStreamRtsp;


typedef struct rtsp_session_s rtsp_session_t;

rtsp_session_t *rtsp_session_start(int fd, char **mrl, char *path, char *host,
  int port, int *redir, uint32_t bandwidth, char *user, char *pass);

int rtsp_session_read(rtsp_session_t *session, char *data, int len, int time_over);

void rtsp_session_end(rtsp_session_t *session);

int rtsp_session_resume(rtsp_session_t* session);

int rtsp_session_pause(rtsp_session_t* session);

int rtsp_session_send_alive(rtsp_session_t* this);

int rtsp_session_seek(rtsp_session_t* this, uint64_t start_time);

int rtsp_session_get_answer(rtsp_session_t* this);

int64_t rtsp_get_duration(rtsp_session_t* session);
#endif
