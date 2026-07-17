#include"gx_common.h"

int main(int argc, char* *argv)
{
	int i = 0;
	GxStream* stream;
	char buffer[2048] = { 'f', 'f', 'd', 'a', 'd', 'f', 'd', 'a', 'd' };
	FILE* fw;
	//stream = av_malloc(sizeof(GxStream));
	//stream -> url ="http://www.soft.guoxintech.com/";

	GxMediaFilter_Init();
//	GxClassPrintTree(&gxRootClass, 0);
	//GxStream_Reset(stream);
	fw = fopen("write_file", "w");
	stream = GxStream_Open(argv[1], GX_STREAM_READ);
	//stream_reset(stream); 
	// i = GxStream_Write(stream,buffer,4);
	// gxlogd("==========write %d,%s==========\n",i,buffer);
	memset(buffer, 0, 2048);

	// GxStream_Seek(stream,30000);
	// gxlogd("stream->pos: %d\n",stream->pos);
	while (!GxStream_Eof(stream)) {
		//      gxlogd("stream->eof:%d\n", GxStream_Eof(stream));
		//      i = GxStream_Read(stream, buffer, 2048);
		buffer[0] = GxStream_ReadChar(stream);
		//      gxlogd("read bytes:%d\n", i);
	//	fwrite(&buffer[0], 1, 1, fw);
		//      gxlogd("after read stream->eof:%d\n", GxStream_Eof(stream));
	}
	gxlogd("%s\n", buffer);
	//gxlogd("===================Seek===============\n");

	//gxlogd("stream->pos: %d\n",stream->pos); 
	//GxStream_Seek(stream,3000);

	//gxlogd("stream->pos: %d\n",stream->pos);
	GxStream_Close(stream);

	GxStreamClass_Destroy();
	return 0;
}
