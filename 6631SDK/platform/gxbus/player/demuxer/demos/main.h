#ifndef _GX_MEDIA_H_
#define _GX_MEDIA_H_

#include <stdio.h>
#include <stdlib.h>

#include "../../include/gx_stream.h"
#include "../../include/gx_demux.h"
#include "../../include/stheader.h"
#include "../../include/gx_dumpfilter.h"
#include "../../avutil/common.h"

#define HAS_PIN 			1




GxMedia *GxMedia_Create   (GxMedia *media, const char *filename);
void GxMedia_Release   (GxMedia *media);
int GxMedia_DecodeAudio(GxMedia *media);
int GxMedia_DecodeVideo(GxMedia *media);
int GxMedia_DecodeSub  (GxMedia *media);

#endif
