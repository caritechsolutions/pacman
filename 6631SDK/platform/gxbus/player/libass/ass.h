// -*- c-basic-offset: 8; indent-tabs-mode: t -*-
// vim:ts=8:sw=8:noet:ai:
/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * libass is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libass is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libass; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBASS_ASS_H
#define LIBASS_ASS_H

#include <stdio.h>

typedef struct ass_event_s {
	long long Start; // ms
	long long Duration; // ms

	char Text[512];
} ass_event_t;

typedef enum {PST_UNKNOWN = 0, PST_INFO, PST_STYLES, PST_EVENTS, PST_FONTS} parser_state_t;

typedef struct ass_track_s {
	ass_event_t events; // the same as styles

	char *event_format;
	parser_state_t state;
	enum {TRACK_TYPE_UNKNOWN = 0, TRACK_TYPE_ASS, TRACK_TYPE_SSA} track_type;
	
	double Timer;
} ass_track_t;

ass_track_t* ass_new_track(void);
void ass_free_track(ass_track_t* track);

int ass_process_data(ass_track_t* track, char* data, int size);
void ass_process_codec_private(ass_track_t* track, char *data, int size);


#endif /* LIBASS_ASS_H */
