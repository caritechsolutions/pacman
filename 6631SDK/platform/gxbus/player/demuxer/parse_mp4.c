

#include <stdio.h>
#include "parse_mp4.h"
#include "gx_stream.h"
#include "gx_common.h"


#define MP4_DL MSGL_V
#define freereturn(a,b) av_free(a); return b

int mp4_read_descr_len(GxStream* s) {
  uint8_t b;
  uint8_t numBytes = 0;
  uint32_t length = 0;

  do {
    b = GxStream_ReadChar(s);
    numBytes++;
    length = (length << 7) | (b & 0x7F);
  } while ((b & 0x80) && numBytes < 4);

  //gxlogd("MP4 read desc len: %d\n", length);
  return length;
}

/* parse the data part of MP4 esds atoms*/
int mp4_parse_esds(unsigned char* data, int datalen, esds_t *esds) {
  /* create memory stream from data*/
  GxStream* s = GxStream_NewMemory(data, datalen);
  uint16_t len;

  memset(esds, 0, sizeof(esds_t));

  esds->version = GxStream_ReadChar(s);
  esds->flags = GxStream_Read_Int24(s);
  gx_msg(MSGT_DEMUX, MP4_DL,
      "ESDS MPEG4 version: %d  flags: 0x%06X\n",
      esds->version, esds->flags);

  /* get and verify ES_DescrTag*/
  if (GxStream_ReadChar(s) == MP4ESDescrTag) {
    /* read length*/
    len = mp4_read_descr_len(s);

    esds->ESId = GxStream_ReadWord(s);
    esds->streamPriority = GxStream_ReadChar(s);
    gx_msg(MSGT_DEMUX, MP4_DL,
      	"ESDS MPEG4 ES Descriptor (%dBytes):\n"
	" -> ESId: %d\n"
	" -> streamPriority: %d\n",
	len, esds->ESId, esds->streamPriority);

    if (len < (5 + 15)) {
      freereturn(s,1);
    }
  } else {
    esds->ESId = GxStream_ReadWord(s);
    gx_msg(MSGT_DEMUX, MP4_DL,
      	"ESDS MPEG4 ES Descriptor (%dBytes):\n"
	" -> ESId: %d\n", 2, esds->ESId);
  }

  /* get and verify DecoderConfigDescrTab*/
  if (GxStream_ReadChar(s) != MP4DecConfigDescrTag) {
    freereturn(s,1);
  }

  /* read length*/
  len = mp4_read_descr_len(s);

  esds->objectTypeId = GxStream_ReadChar(s);
  esds->streamType = GxStream_ReadChar(s);
  esds->bufferSizeDB = GxStream_Read_Int24(s);
  esds->maxBitrate = GxStream_ReadDword(s);
  esds->avgBitrate = GxStream_ReadDword(s);
  gx_msg(MSGT_DEMUX, MP4_DL,
      "ESDS MPEG4 Decoder Config Descriptor (%dBytes):\n"
      " -> objectTypeId: %d\n"
      " -> streamType: 0x%02X\n"
      " -> bufferSizeDB: 0x%06X\n"
      " -> maxBitrate: %.3fkbit/s\n"
      " -> avgBitrate: %.3fkbit/s\n",
      len, esds->objectTypeId, esds->streamType,
      esds->bufferSizeDB, esds->maxBitrate/1000.0,
      esds->avgBitrate/1000.0);

  esds->decoderConfigLen=0;

  if (len < 15) {
    freereturn(s,0);
  }

  /* get and verify DecSpecificInfoTag*/
  if (GxStream_ReadChar(s) != MP4DecSpecificDescrTag) {
    freereturn(s,0);
  }

  /* read length*/
  esds->decoderConfigLen = len = mp4_read_descr_len(s);

  esds->decoderConfig = av_malloc(esds->decoderConfigLen);
  if (esds->decoderConfig) {
    GxStream_Read(s, esds->decoderConfig, esds->decoderConfigLen);
  } else {
    esds->decoderConfigLen = 0;
  }
  gx_msg(MSGT_DEMUX, MP4_DL,
      "ESDS MPEG4 Decoder Specific Descriptor (%dBytes)\n", len);

  /* get and verify SLConfigDescrTag*/
  if(GxStream_ReadChar(s) != MP4SLConfigDescrTag) {
    freereturn(s,0);
  }

  /* Note: SLConfig is usually constant value 2, size 1Byte*/
  esds->SLConfigLen = len = mp4_read_descr_len(s);
  esds->SLConfig = av_malloc(esds->SLConfigLen);
  if (esds->SLConfig) {
    GxStream_Read(s, esds->SLConfig, esds->SLConfigLen);
  } else {
    esds->SLConfigLen = 0;
  }
  gx_msg(MSGT_DEMUX, MP4_DL,
      "ESDS MPEG4 Sync Layer Config Descriptor (%dBytes)\n"
      " -> predefined: %d\n", len, esds->SLConfig[0]);

  /* will skip the remainder of the atom*/
  freereturn(s,0);

}

/* cleanup all mem occupied by mp4_parse_esds*/
void mp4_free_esds(esds_t* esds) {
  if(esds->decoderConfigLen)
    av_free(esds->decoderConfig);
  if(esds->SLConfigLen)
    av_free(esds->SLConfig);
}



