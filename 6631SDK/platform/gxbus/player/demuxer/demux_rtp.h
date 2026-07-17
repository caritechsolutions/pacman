/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _DEMUX_RTP_H
#define _DEMUX_RTP_H

#include <stdlib.h>
#include <stdio.h>
#include "gx_demux.h"

// Open a RTP demuxer (which was initiated either from a SDP file,
// or from a RTSP URL):
GxDemuxer* demux_open_rtp(GxDemuxer* demuxer);

// Test whether a RTP demuxer is for a MPEG stream:
int demux_is_mpeg_rtp_stream(GxDemuxer* demuxer);

// Test whether a RTP demuxer contains combined (multiplexed)
// audio+video (and so needs to be demuxed by higher-level code):
int demux_is_multiplexed_rtp_stream(GxDemuxer* demuxer);

// Read from a RTP demuxer:
int demux_rtp_fill_buffer(GxDemuxer *demux, GxDemuxStream* ds);

// Close a RTP demuxer
void demux_close_rtp(GxDemuxer* demuxer);

int demux_rtp_control(GxDemuxer *demuxer, int cmd, void *arg);
#endif /* MPLAYER_DEMUX_RTP_H */
