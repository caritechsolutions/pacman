/*
 *  Copyright (C) 2006 Benjamin Zores
 *   heavily base on the Freebox patch for xine by Vincent Mussard
 *   but with many enhancements for better RTSP RFC compliance.
 *
 *   This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HAVE_RTSP_RTP_H
#define HAVE_RTSP_RTP_H

#include <unistd.h>
#include "rtsp.h"
#include <fcntl.h>

#define MAX_PREVIEW_SIZE 4096

struct rtp_rtsp_session_t {
	int      rtp_socket;
	int      rtcp_socket;
	char     *control_url;
	uint64_t last_time;
	int64_t  duration;
};

struct rtp_rtsp_session_t *rtp_setup_and_play (rtsp_t* rtsp_session);
off_t rtp_read (struct rtp_rtsp_session_t* st, char *buf, off_t length);
void rtp_session_free (struct rtp_rtsp_session_t *st);
void rtcp_send_rr (rtsp_t *s, struct rtp_rtsp_session_t *st);
int64_t rtp_session_get_duration(struct rtp_rtsp_session_t* st);
int rtp_session_resume(rtsp_t* s, struct rtp_rtsp_session_t* st);
int rtp_session_pause(rtsp_t* s, struct rtp_rtsp_session_t* st);
int rtp_session_seek(rtsp_t* s, struct rtp_rtsp_session_t* st, int64_t start_ms);

#endif /* HAVE_RTSP_RTP_H */

