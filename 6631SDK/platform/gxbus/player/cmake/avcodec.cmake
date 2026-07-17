if (CONFIG_FFMPEG_ENABLE)

	list(APPEND SRC_AVCODEC
		avcodec/mpeg4audio.c
		avcodec/mpegaudio.c
		avcodec/mpegaudiodata.c
		avcodec/mpegaudiodecheader.c
		avcodec/lossless_audiodsp.c
		avcodec/bswapdsp.c
		)

    if (CONFIG_AD_SW_APE)
        list(APPEND SRC_AVCODEC
			avcodec/apedec.c
        )
    endif()

    if (CONFIG_DEMUX_AC3)
        list(APPEND SRC_AVCODEC
			avcodec/ac3_parser.c
			avcodec/ac3tab.c
        )
    endif()

    if (CONFIG_DEMUX_DTS)
        list(APPEND SRC_AVCODEC
            avcodec/dca.c
        )
    endif()

endif()