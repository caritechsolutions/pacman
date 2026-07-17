#include"gx_common.h"

int main(int argc, char* *argv)
{
	GxStream* stream, *stream2;

	int len = 0;
	FILE* fd;
	int i = 0, j = 0, k;

	char buffer[1024] = { 0, };

	GxMediaFilter_Init();

	GxClassPrintTree(&gxRootClass, 0);

	stream = GxStream_Open("stream_ftp.c", 0);

	stream2 = GxStream_Open("file:///home/software/jiangzq/gxcore/player/access/write_file", GX_STREAM_WRITE);

	//gxlogd("stream->pos:%d\n",stream->pos);   
	//GxStream_Seek(stream,30000);
	//gxlogd("stream->buf_pos:%d\n",stream->buf_pos);
	//gxlogd("stream->pos:%d\n",stream->pos);
	gxlogd("stream->eof:%d\n", stream->eof);
	while (!stream->eof) {
		len = GxStream_Read(stream, buffer, 1024);
		GxStream_Write(stream2, buffer, len);
		memset(buffer, 0, 1024);
		GxStream_ReadLine(stream, buffer, 1024);
		gxlogd("%s\n", buffer);
	}
	gxlogd("after read:stream->eof is:%d\n", stream->eof);
	GxStream_Close(stream2);
	GxStream_Close(stream);

	GxStreamClass_Destroy();
	return 0;
}
