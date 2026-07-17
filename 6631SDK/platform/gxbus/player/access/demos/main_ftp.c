#include "gx_mediaplayer"

int main()
{
	GxStream* stream;
	stream = GxStream_Open("smb://192.168.120.253", 0);
	GxStream_Close(stream);
	return 0;
}
