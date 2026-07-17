#include"gx_common.h"
int main(int argc, char* *argv)
{
	uint8_t buffer[1024];

	GxStream* stream;
	stream = GxStream_Open("rtsp://192.168.120.189/audio_test.mp3", 0);
	if (!stream) {
		gxlogd("open fail return NULL\n");
		return 0;
	}
	GxStream_Read(stream, buffer, 1023);
	gxlogd("%s\n", buffer);
}
