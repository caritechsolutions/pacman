
#include "demux_rmvb.h"

rm_parser* pParser = HXNULL;
rm_stream_header* pHdr = HXNULL;
rm_packet* pPacket = HXNULL;
rv_depack* pRVDepack = HXNULL;
ra_depack* pRADepack = HXNULL;

extern unsigned char is_aac;
extern unsigned char* AUDIO_USED_SDRAM_PARA_ADDRESS;

#define REAL_RAW_DEMUX 0
#define REAL_LUNA_DEMUX 0

#define CM_SYNC_WORD_LEN 16
#define CM_SYNC_WORD 0x00000155

static inline uint32_t bswap32(uint32_t x)
{
	x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
	x = (x >> 16) | (x << 16);

	return x;
}

void ra_depack_test_error(void* pError, HX_RESULT result, const char *pszMsg)
{
	gxlogf("%s\n",pszMsg);
}

void rv_depack_test_error(void* pError, HX_RESULT result, const char *pszMsg)
{
	gxlogf("%s\n",pszMsg);
}

void PackRAFormatInfo(ra_format_info*  pInfo, BYTE * pBuf, UINT32 ulLen)
{
	if (pInfo && pBuf && ulLen >= 22) {
		rm_pack32(pInfo->ulSampleRate, &pBuf, &ulLen);
		rm_pack16(pInfo->usBitsPerSample, &pBuf, &ulLen);
		rm_pack16(pInfo->usNumChannels, &pBuf, &ulLen);
		rm_pack16(pInfo->usAudioQuality, &pBuf, &ulLen);
		rm_pack32(pInfo->ulBitsPerFrame, &pBuf, &ulLen);
		rm_pack32(pInfo->ulGranularity, &pBuf, &ulLen);
		rm_pack32(pInfo->ulOpaqueDataSize, &pBuf, &ulLen);
		/* Sanity Check*/
		if (pInfo->ulOpaqueDataSize && ulLen >= pInfo->ulOpaqueDataSize) {
			memcpy(pBuf, pInfo->pOpaqueData, pInfo->ulOpaqueDataSize);
		}
	}
}

void PackRVFormatInfo(rv_format_info* pInfo, BYTE* pBuf, UINT32 ulLen)
{
	if (pInfo && pBuf && ulLen >= pInfo->ulLength)
	{
		rm_pack32(pInfo->ulLength,          &pBuf, &ulLen);
		rm_pack32(pInfo->ulMOFTag,          &pBuf, &ulLen);
		rm_pack32(pInfo->ulSubMOFTag,       &pBuf, &ulLen);
		rm_pack16(pInfo->usWidth,           &pBuf, &ulLen);
		rm_pack16(pInfo->usHeight,          &pBuf, &ulLen);
		rm_pack16(pInfo->usBitCount,        &pBuf, &ulLen);
		rm_pack16(pInfo->usPadWidth,        &pBuf, &ulLen);
		rm_pack16(pInfo->usPadHeight,       &pBuf, &ulLen);
		rm_pack32(pInfo->ufFramesPerSecond, &pBuf, &ulLen);
		/* Sanity Check*/
		if (ulLen >= pInfo->ulOpaqueDataSize)
		{
			memcpy(pBuf, pInfo->pOpaqueData, pInfo->ulOpaqueDataSize);
		}
	}
}

void PackFrame(rv_frame*  pFrame, BYTE * pBuf, UINT32 ulLen)
{
	if (pFrame && pBuf) {
		UINT32 ulPackLen = 20 + pFrame->ulNumSegments*  8 + pFrame->ulDataLen + CM_SYNC_WORD_LEN;
		UINT32 i = 0;
		if (ulLen >= ulPackLen) {
			rm_pack32(CM_SYNC_WORD, &pBuf, &ulLen);
			rm_pack32(CM_SYNC_WORD, &pBuf, &ulLen);
			rm_pack32(CM_SYNC_WORD, &pBuf, &ulLen);
			rm_pack32(CM_SYNC_WORD, &pBuf, &ulLen);
			rm_pack32(pFrame->ulDataLen, &pBuf, &ulLen);
			rm_pack32(pFrame->ulTimestamp, &pBuf, &ulLen);
			rm_pack16(pFrame->usSequenceNum, &pBuf, &ulLen);
			rm_pack16(pFrame->usFlags, &pBuf, &ulLen);
			rm_pack32(pFrame->bLastPacket, &pBuf, &ulLen);
			rm_pack32(pFrame->ulNumSegments, &pBuf, &ulLen);
			for (i = 0; i < pFrame->ulNumSegments; i++) {
				rm_pack32((UINT32) pFrame->pSegment[i].bIsValid, &pBuf, &ulLen);
				rm_pack32((UINT32) pFrame->pSegment[i].ulOffset, &pBuf, &ulLen);
			}
			/* Sanity check*/
			if (ulLen >= pFrame->ulDataLen) {
				memcpy((void* )pBuf, pFrame->pData, pFrame->ulDataLen);
			}
		}
	}
}

#if REAL_RAW_DEMUX
void PackBlock(ra_block*  pBlock, BYTE * pBuf, UINT32 ulLen)
{
	if (pBlock && pBuf) {
#if (REAL_LUNA_DEMUX ==0)
		UINT32 ulPackLen = 12 + pBlock->ulDataLen;
#else
		UINT32 ulPackLen = pBlock->ulDataLen;
#endif
		if (ulLen >= ulPackLen) {
#if (REAL_LUNA_DEMUX ==0)
			rm_pack32(pBlock->ulDataLen, &pBuf, &ulLen);
			rm_pack32(pBlock->ulTimestamp, &pBuf, &ulLen);
			rm_pack32(pBlock->ulDataFlags, &pBuf, &ulLen);
#endif
			/* Sanity check*/
			if (ulLen >= pBlock->ulDataLen) {
				memcpy((void* )pBuf, pBlock->pData, pBlock->ulDataLen);
			}
		}
	}
}

HX_RESULT ra_block_available(void* pAvail, UINT32 ulSubStreamNum, ra_block * pBlock)
{
	HX_RESULT retVal = HXR_FAIL;
	GxDemuxer* demuxer = (GxDemuxer *) pAvail;
	GxDemuxPacket* packet = NULL;

	if (demuxer && pBlock) {
		/* Compute the size of the packed buffer*/
#if (REAL_LUNA_DEMUX ==0)
		UINT32 ulPackLen = 12 + pBlock->ulDataLen;
#else
		UINT32 ulPackLen =  pBlock->ulDataLen;
#endif
		// TODO liufei
		packet = GxDemuxPacket_Create(demuxer, NULL, ulPackLen);
		if (packet) {
			/* Pack the block*/
			PackBlock(pBlock, (BYTE* ) packet->buffer, ulPackLen);
			GxDemuxStream_AddPacket(demuxer->audio, packet);
			packet->pts = ulPackLen;
			/* Clean up the frame*/
			ra_depack_destroy_block(pRADepack, &pBlock);
			/* Clear the return value*/
			retVal = HXR_OK;
		}
	}

	return retVal;
}

#else

void PackBlock(ra_block*  pBlock, BYTE * pBuf, UINT32 ulLen)
{
	if ( pBuf) {
		rm_pack16(0, &pBuf, &ulLen);
		rm_pack16(pPacket->usDataLen+12, &pBuf, &ulLen);
		rm_pack16(pPacket->usStream, &pBuf, &ulLen);
		rm_pack32(pPacket->ulTime, &pBuf, &ulLen);
		pPacket->flag  = ((pPacket->flag == 2)?0:1);
		rm_pack16(pPacket->flag, &pBuf, &ulLen);
		memcpy((void* )pBuf, pPacket->pData, pPacket->usDataLen);
	}
}

HX_RESULT ra_block_available(void* pAvail, UINT32 ulSubStreamNum, ra_block * pBlock)
{
	ra_depack_destroy_block(pRADepack, &pBlock);

	return HXR_OK;
}


#endif

HX_RESULT rv_frame_available(void* pAvail, UINT32 ulSubStreamNum, rv_frame * pFrame)
{
	HX_RESULT retVal = HXR_FAIL;
	GxDemuxer* demuxer = (GxDemuxer *) pAvail;
	GxDemuxPacket* packet;

	if (demuxer && pFrame) {
		/* Compute the size of the packed buffer*/
		UINT32 ulPackLen = 20 + pFrame->ulNumSegments*  8 + pFrame->ulDataLen + CM_SYNC_WORD_LEN;
		/* Allocate a buffer*/

		// TODO liufei
		packet = GxDemuxPacket_Create(demuxer, NULL, ulPackLen);
		if (packet) {
			/* Pack the frame*/
			PackFrame(pFrame, (BYTE* ) packet->buffer, ulPackLen);
			packet->pts = pFrame->ulTimestamp;
			GxDemuxStream_AddPacket(demuxer->video, packet);
			/* Clean up the frame*/
			rv_depack_destroy_frame(pRVDepack, &pFrame);
			retVal = HXR_OK;
		}
	}
	return retVal;
}


UINT32 rm_read_func(void* pUserRead, BYTE * pBuf,	/* Must be at least ulBytesToRead long */
		UINT32 ulBytesToRead)
{
	UINT32 ulRet = 0;
	GxDemuxer* demuxer;

	if (pUserRead && pBuf && ulBytesToRead) {
		demuxer = (GxDemuxer* ) pUserRead;
		ulRet = GxStream_Read(demuxer->stream, (uint8_t* ) pBuf, ulBytesToRead);
	}
	return ulRet;
}

void rm_seek_func(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
	GxDemuxer* demuxer;
	if (pUserRead) {
		demuxer = (GxDemuxer* ) pUserRead;
		if(ulOrigin == HX_SEEK_ORIGIN_SET)
			GxStream_Seek(demuxer->stream, ulOffset);
		else if(ulOrigin == HX_SEEK_ORIGIN_CUR)
			GxStream_Seek(demuxer->stream, ulOffset+GxStream_Tell(demuxer->stream));
		else
			return;
	}
}

static int demux_rmvb_check_file(GxDemuxer*  demuxer)
{
	DemuxRmvbPriv* priv;
	int ucBuf;

	ucBuf = GxStream_ReadDword_Le(demuxer->stream);

	if (!rm_parser_is_rm_file((BYTE* ) & ucBuf, 16)) {
		gxlogf("not an .rm file.\n");
		return GX_DEMUXER_TYPE_UNKNOWN;
	}

	GxStream_Seek(demuxer->stream, 0);

	priv = av_malloc(sizeof(DemuxRmvbPriv));
	memset(priv, 0, sizeof(DemuxRmvbPriv));
	priv->ra_stream_id = 0xffff;
	priv->rv_stream_id = 0xfffe;
	priv->ra_pts = -1;
	priv->rv_pts = -1;
	demuxer->priv = priv;

	return GX_DEMUXER_TYPE_RMVB;

}

static int demux_rmvb_fill_buffer(GxDemuxer*  demuxer,GxDemuxStream* ds)
{
	int retVal = GX_PLAYER_ERROR,getvideo=0,getaudio=0;
	DemuxRmvbPriv* priv = demuxer->priv;

	if (!GxStream_Eof(demuxer->stream))
	{
		retVal = HXR_OK;
		while (retVal == HXR_OK) {
			retVal = rm_parser_get_packet(pParser, &pPacket);

			if(retVal == HXR_OK) {
				if (pPacket->usStream == priv->rv_stream_id) {
					retVal = rv_depack_add_packet(pRVDepack, pPacket);
					getvideo = 1;
				}
				else if (pPacket->usStream == priv->ra_stream_id) {
#if (REAL_RAW_DEMUX ==0)
					GxDemuxPacket* packet = NULL;
					/* Compute the size of the packed buffer*/
					UINT32 ulPackLen = 12 + pPacket->usDataLen;
					// TODO liufei
					packet = GxDemuxPacket_Create(demuxer, NULL, ulPackLen);
					if (packet) {
						/* Pack the block*/
						PackBlock(NULL,(BYTE* ) packet->buffer, ulPackLen);
						packet->pts = pPacket->ulTime;
						if (packet->pts == -1 || packet->pts == 0)
							packet->pts = 1;

						if (packet->pts == priv->ra_pts)
							packet->pts++;

						priv->ra_pts = packet->pts;
						GxDemuxStream_AddPacket(demuxer->audio, packet);
					}
#else
					retVal = ra_depack_add_packet(pRADepack, pPacket);
#endif
					getaudio = 1;
				}

				rm_parser_destroy_packet(pParser, &pPacket);
			}
			else {
				gxlogd("[Player] File Demux Finished . ret = 0x%x\n",retVal);
				demuxer->stream->eof = 1;
				return GX_PLAYER_ERROR;
			}

			if(ds == NULL || ds->sh == NULL)
				break;

			if(ds == demuxer->video && getvideo)
				break;

			if(ds == demuxer->audio && getaudio)
				break;
		}
	}
	else{
		demuxer->stream->eof = 1;
		gxlogd("[Player] File Demux Finished .............\n");
	}

	return retVal;
}

/* please upload RV10 samples WITH INDEX CHUNK*/
static int demux_rmvb_seek(GxDemuxer*  demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
	demux_rmvb_fill_buffer(demuxer,demuxer->video);
	demux_rmvb_fill_buffer(demuxer,demuxer->audio);

	rm_parser_seek(pParser, rel_seek_ms);

	GxDemuxStream_FreePacks(demuxer->video);
	GxDemuxStream_FreePacks(demuxer->audio);

	return ret;
}

static GxDemuxer* demux_rmvb_open(GxDemuxer * demuxer)
{

	HX_RESULT retVal = HXR_OK;
	UINT32 ulNumStreams = 0;
	UINT32 i = 0;
	BYTE*  pTmp ;
	int* pCodec = NULL;
	DemuxRmvbPriv* priv = demuxer->priv;;
	rv_format_info* pRvInfo = &priv->rv_format;
	ra_format_info* pRaInfo = &priv->ra_format;

	/* Create the parser struct*/
	pParser = rm_parser_create(NULL, rv_depack_test_error);
	if (!pParser) {
		goto cleanup;
	}

	/* Set the FILE* into the parser*/
	retVal = rm_parser_init_io(pParser, (void* )demuxer, rm_read_func, rm_seek_func);
	if (retVal != HXR_OK) {
		goto cleanup;
	}

	/* Read all the headers at the beginning of the .rm file*/
	retVal = rm_parser_read_headers(pParser);
	if (retVal != HXR_OK) {
		goto cleanup;
	}

	/* Get the number of streams*/
	ulNumStreams = rm_parser_get_num_streams(pParser);
	if (ulNumStreams == 0) {
		gxlogf("Error: rm_parser_get_num_streams() returns 0\n");
		goto cleanup;
	}

	/* Now loop through the stream headers and print out their properties*/
	for (i = 0; i < ulNumStreams && retVal == HXR_OK; i++) {
		retVal = rm_parser_get_stream_header(pParser, i, &pHdr);
		if (retVal == HXR_OK) {
			if (rm_stream_is_realvideo(pHdr)) {
				GxStreamVideoHeader* sh =
					GxStreamHeader_VideoNew(demuxer, pHdr->ulStreamNumber, pHdr->ulStreamNumber);
				sh->ds = demuxer->video;
				demuxer->video->sh = sh;
				priv->rv_stream_id = (UINT16) i;
				/* Create the RealVideo depacketizer*/
				pRVDepack = rv_depack_create((void* )demuxer,
						rv_frame_available, NULL, rv_depack_test_error);
				if (!pRVDepack) {
					goto cleanup;
				}

				/* Initialize the RV depacketizer with the stream header*/
				retVal = rv_depack_init(pRVDepack, pHdr);
				if (retVal != HXR_OK) {
					goto cleanup;
				}

				/* Get the bitstream header information*/
				retVal = rv_depack_get_codec_init_info(pRVDepack, &pRvInfo);
				if (retVal != HXR_OK) {
					goto cleanup;
				}
				sh->disp_w = pRvInfo->usWidth;
				sh->disp_h = pRvInfo->usHeight;
				sh->format = pRvInfo->ulMOFTag;
				sh->format = vcodec_fourcc2std(sh->format);
				sh->fps    = (float)(pRvInfo->ufFramesPerSecond/60000);
				sh->aspect = (float)sh->disp_w/sh->disp_h;
				sh->i_bps  = rm_parser_get_avg_bit_rate(pParser)/8;
				memset(sh->priv.codec,0,5);
				pCodec = (int*)(sh->priv.codec);
				*pCodec = bswap32(pRvInfo->ulSubMOFTag);

				pTmp = (BYTE*) av_malloc(pRvInfo->ulLength);
				if (!pTmp)
				{
					gxlogd("Cannot allocate temporary buffer of size %lu\n", pRvInfo->ulLength);
					goto cleanup;
				}
				/* Pack the rv_format_info struct*/
				PackRVFormatInfo(pRvInfo, pTmp, pRvInfo->ulLength);

				sh->priv.header.data = (void*)pTmp;
				sh->priv.header.len  = pRvInfo->ulLength;
				/* Destroy the codec init info*/
				rv_depack_destroy_codec_init_info(pRVDepack, &pRvInfo);

			} else if (rm_stream_is_realaudio(pHdr)) {
				GxStreamAudioHeader* sh =
					GxStreamHeader_AudioNew(demuxer, pHdr->ulStreamNumber, pHdr->ulStreamNumber);
				sh->ds = demuxer->audio;
				demuxer->audio->sh = sh;
				priv->ra_stream_id = (UINT16) i;

				/* Create the RealAudio depacketizer*/
				pRADepack = ra_depack_create((void* )demuxer,
						ra_block_available, NULL, ra_depack_test_error);
				if (!pRADepack) {
					goto cleanup;
				}

				AUDIO_USED_SDRAM_PARA_ADDRESS = av_malloc(sizeof(input_info_header));
				if(!AUDIO_USED_SDRAM_PARA_ADDRESS){
					goto cleanup;
				}

				/* Initialize the RA depacketizer with the stream header*/
				retVal = ra_depack_init(pRADepack, pHdr);
				if (retVal != HXR_OK) {
					goto cleanup;
				}

				/* Get the bitstream header information*/
				retVal = ra_depack_get_codec_init_info(pRADepack, 0, &pRaInfo);
				if (retVal != HXR_OK) {
					goto cleanup;
				}
				sh->wf = av_malloc(sizeof(WAVEFORMATEX));
				sh->wf->nChannels = pRaInfo->usNumChannels;
				sh->wf->nSamplesPerSec =pRaInfo->ulSampleRate;
				sh->wf->nAvgBytesPerSec = pRaInfo->ulActualRate;
				sh->wf->wBitsPerSample = pRaInfo->usBitsPerSample;

				sh->samplerate = pRaInfo->ulSampleRate;
				sh->channels = pRaInfo->usNumChannels;
				sh->samplesize = pRaInfo->usBitsPerSample;

				memset(sh->priv.codec,0,5);
				sh->priv.para.data =  (void*)AUDIO_USED_SDRAM_PARA_ADDRESS;
				sh->priv.para.len =  sizeof(input_info_header);
#ifdef I386_LINUX
				{
					char Name[128]={0};
					FILE *fp ;
					char* dot = demuxer->stream->url;
					char* postfixptr = NULL;

					if(strstr(dot,"/"))
					{
						while(dot)
						{
							postfixptr = ++dot;
							dot = strchr(dot,'/');
						}
					}
					else
						postfixptr = dot;

					snprintf(Name, sizeof(Name), "%s", postfixptr);

					dot = strstr(Name,".");
					*dot = '\0';

					strncat(Name,".m2p", sizeof(Name)-1);

					fp = fopen(Name,"wb+");
					fwrite(sh->priv.para.data,sh->priv.para.len,1,fp);
					fclose(fp);
				}
#endif
				if(is_aac){
					sh->format = AUDIO_CODEC_RA_AAC;
					strncpy(sh->priv.codec ,"AAC", sizeof(sh->priv.codec)-1);
				}
				else
				{
					sh->format = AUDIO_CODEC_RA_RA8LBR;
					strncpy(sh->priv.codec ,"cook", sizeof(sh->priv.codec)-1);
				}

				/* Destroy the codec init info*/
				ra_depack_destroy_codec_init_info(pRADepack, &pRaInfo);

			}
			rm_parser_destroy_stream_header(pParser, &pHdr);
		}
	}

	/* Do we have a RealVideo stream in this .rm file?*/
	if (!ulNumStreams) {
		gxlogf("There is no AV stream in this file.\n");
		goto cleanup;
	}

	rm_parser_seek(pParser, 1000);
	demux_rmvb_fill_buffer(demuxer,NULL);

	demuxer->seekable = 1;
	demuxer->duration = rm_parser_get_duration(pParser);

	return demuxer;

cleanup:

	/* Destroy the depacketizer*/
	if (pRVDepack) {
		rv_depack_destroy(&pRVDepack);
	}
	if (pRADepack) {
		ra_depack_destroy(&pRADepack);
	}
	/* If we have a packet, destroy it*/
	if (pPacket) {
		rm_parser_destroy_packet(pParser, &pPacket);
	}
	/* Destroy the stream header*/
	if (pHdr) {
		rm_parser_destroy_stream_header(pParser, &pHdr);
	}
	/* Destroy the parser*/
	if (pParser) {
		rm_parser_destroy(&pParser);
	}

	return NULL;
}

static void demux_rmvb_close(GxDemuxer*  demuxer)
{
	if (demuxer->priv)
		av_free(demuxer->priv);

	if (pRVDepack) {
		rv_depack_destroy(&pRVDepack);
	}
	if (pRADepack) {
		ra_depack_destroy(&pRADepack);
	}
	/* If we have a packet, destroy it*/
	if (pPacket) {
		rm_parser_destroy_packet(pParser, &pPacket);
	}
	/* Destroy the stream header*/
	if (pHdr) {
		rm_parser_destroy_stream_header(pParser, &pHdr);
	}
	/* Destroy the parser*/
	if (pParser) {
		rm_parser_destroy(&pParser);
	}
}

static int demux_rmvb_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	if(demuxer && demuxer->priv)
	{
		switch(cmd)
		{
			case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
				{
					*(int64_t*)arg=(int64_t)rm_parser_get_duration(pParser);
					return GX_DEMUXER_CTRL_OK;
				}
				break;
			default:
				return GX_DEMUXER_CTRL_NOTIMPL;
		}
	}
	return GX_DEMUXER_CTRL_ERROR;
}

GxDemuxerClass gx_demux_rmvb = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer RMVB",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.stop  = NULL,
	},
	DEF_AUTHOR("demuxer","rmvb","No description","L.F","No comment"),

	.name        = "Demux RMVB",
	.type        = GX_DEMUXER_TYPE_RMVB,
	.check_file  = demux_rmvb_check_file,
	.open        = demux_rmvb_open,
	.fill_buffer = demux_rmvb_fill_buffer,
	.close       = demux_rmvb_close,
	.seek        = demux_rmvb_seek,
	.control     = demux_rmvb_control,
};
