	list(APPEND SRC_AVUTIL
		avutil/msg.c
		avutil/avmalloc.c
		avutil/avtick.c
		avutil/avtimer.c
		avutil/avdebug.c
		avutil/pvr_enc.c
		avutil/audio_resample.c
		avutil/audio_resample_441_table.c
		avutil/audio_resample_all_table.c
	)

if (CONFIG_FFMPEG_ENABLE)
	list(APPEND SRC_AVUTIL
		avutil/crc.c
		avutil/opt.c
		avutil/eval.c
		avutil/md5.c
		avutil/fifo.c
		avutil/rc4.c
		avutil/des.c
		avutil/aes.c
		avutil/aes_ctr.c
		avutil/samplefmt.c
		avutil/dict.c
		avutil/string.c
		avutil/mem.c
		avutil/lfg.c
		avutil/dovi_meta.c
		avutil/error.c
		avutil/bprintf.c
		#avutil/hmac.c
		#avutil/sha.c
		#avutil/sha512.c
	)

    if (CONFIG_DEMUX_MPEG_AUDIO)
        list(APPEND SRC_AVUTIL
			avutil/intfloat_readwrite.c
			avutil/lzo.c
			avutil/mathematics.c
			avutil/rational.c
			avutil/base64.c
        )
    endif()

    if (CONFIG_DEMUX_MOV)
        list(APPEND SRC_AVUTIL
            avutil/encryption_info.c
        )
    endif()
endif()
