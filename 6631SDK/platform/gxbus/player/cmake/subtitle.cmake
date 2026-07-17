IF (CONFIG_SUBTITLE_MEDIA)
	SET (SRC_SUBTITLE ${SRC_SUBTITLE}
		subtitle/subformat/vobsub.c
		subtitle/subformat/spudec.c
		subtitle/subformat/subrender.c
		subtitle/subformat/subrender_std.c
		subtitle/subformat/subrender_yuv.c
		subtitle/subformat/subrender_rgb.c
		subtitle/subformat/subreader.c
		subtitle/subformat/subreader_chr.c
		subtitle/subformat/subreader_spu.c
		subtitle/subformat/xsubdec.c
		subtitle/sub_sync.c
		subtitle/sub_pes.c
		subtitle/player_sub_dvb.c
		subtitle/player_sub_inside.c
		subtitle/player_sub_outside.c
		)
ENDIF()

IF (CONFIG_SUBTITLE_ARIBCC)
	SET (SRC_SUBTITLE ${SRC_SUBTITLE}
		subtitle/player_sub_aribcc.c
		)
ENDIF (CONFIG_SUBTITLE_ARIBCC)

IF (CONFIG_SUBTITLE_ATSCCC)
	SET (SRC_SUBTITLE ${SRC_SUBTITLE}
		subtitle/player_sub_atsccc.c
		)
ENDIF (CONFIG_SUBTITLE_ATSCCC)

IF (CONFIG_SUBTITLE_SCTE)
	SET (SRC_SUBTITLE ${SRC_SUBTITLE}
		subtitle/player_sub_scte.c
		)
ENDIF (CONFIG_SUBTITLE_SCTE)

IF (CONFIG_SUBTITLE_TELETEXT)
	SET (SRC_SUBTITLE ${SRC_SUBTITLE}
		subtitle/player_sub_teletext.c
		subtitle/player_sub_magazine.c
		)
ENDIF (CONFIG_SUBTITLE_TELETEXT)

