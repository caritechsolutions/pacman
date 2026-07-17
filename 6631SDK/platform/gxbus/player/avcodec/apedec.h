#ifndef __APEDEC_H__
#define __APEDEC_H__

int ape_decode_close(AVCodecContext *avctx);
int ape_decode_init (AVCodecContext *avctx);
int ape_decode_frame(AVCodecContext *avctx,
		unsigned char *outdata,
		unsigned int *outdata_size,
		int *got_frame_ptr,
		AVPacket *avpkt);

#endif
