#include"gx_common.h"

int main(int argc, char* *argv)
{
	GxStream* stream, *stream2;
	int len = 0;
	int wlen = 0;
	FILE* fd;
	char* filename;
	char buffer[1025];

	//fd = fopen("write_file","w");

	//filename = "config.txt"

	GxMediaFilter_Init();
	GxClassPrintTree(&gxRootClass, 0);

	if (argc > 1) {
		stream = GxStream_Open(argv[1], 0);
		if (!stream) {
			gxlogd("open smb failure \n");
			return -1;
		}

		GxStream_Control(stream, GX_STREAM_CTRL_GET_SIZE, NULL);

		GxStream_Seek(stream, 1000);

		stream2 =
		    GxStream_Open("smb://software:123456@192.168.120.253/software/jiangzq/config.txt", GX_STREAM_WRITE);
		//gxlogd("open ok\n");
		//gxlogd("stream->eof:%d\n",stream->eof);
		//gxlogd("fd : %d\n",fd);
		while (!stream->eof) {
			//      gxlogd("reading\n");
			len = GxStream_Read(stream, buffer, 1024);
			//fwrite(buffer,1,1024,fd);
			// gxlogd("buf=%s\n", buffer);
			if (!stream2) {
				gxlogd("open config.txt failed\n");
				return -1;
			}

			wlen = GxStream_Write(stream2, buffer, len);
			if (wlen < 0) {
				gxlogd("smb writefile failed\n");
			}
			memset(buffer, 0, 1024);
		}
		//      gxlogd("stream->fd:%d\n",stream->fd);
		//GxStream_Read(stream, buffer, 1024);
		//gxlogd("buf=%s\n", buffer);

		GxStream_Close(stream2);
		GxStream_Close(stream);

		GxStreamClass_Destroy();
		return 0;
	}

	return -1;
}
