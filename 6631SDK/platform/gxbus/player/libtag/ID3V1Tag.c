#include <stdio.h>
#include "ID3V1Tag.h"

/* 读取目标文件的标签信息并存储到ID3V1结构体中，成功返回0否则返回-1*/
int get_ID3V1Tag_info(GxStream* s, ID3V1 *pID3V1)
{
	off_t file_size = 0;
	if (s == NULL){
		gxlogd("File stream's pointer is NULL!\n");
		return -1;
	}else if(pID3V1 == NULL){
		gxlogd("Tag struct's pointer is NULL!\n");
		return -1;
	}

	GxStream_Control(s, GX_STREAM_CTRL_GET_SIZE, &file_size);
	if(file_size < 128){
		gxlogd("file size [%lld] error\n", file_size);
		return -1;
	}

	GxStream_Seek(s, file_size-128);
	GxStream_Read(s, (uint8_t*)pID3V1->Header, 3);

	/* 判断标签头中的内容是否为“TAG”，否则认为此媒体文件没有ID3V1标签*/
	if (memcmp(pID3V1->Header, "TAG", 3) == 0)
	{
		GxStream_Read(s, (uint8_t*)pID3V1->Title, 30);
		GxStream_Read(s, (uint8_t*)pID3V1->Artist, 30);
		GxStream_Read(s, (uint8_t*)pID3V1->Album, 30);
		GxStream_Read(s, (uint8_t*)pID3V1->Year, 4);
		GxStream_Read(s, (uint8_t*)pID3V1->Comment, 28);
		GxStream_Read(s, (uint8_t*)pID3V1->Reserve, 1);
		GxStream_Read(s, (uint8_t*)pID3V1->Track, 1);
		GxStream_Read(s, (uint8_t*)pID3V1->Genre, 1);
	}else{
		gxlogd("This medium has not ID3v1Tag!\n");
		return -1;
	}

	return 0;
}

/* print_ID3v1Tag:打印出标记结构体中的成员信息*/
void print_ID3v1Tag(ID3V1* pID3V1)
{
	if(pID3V1)
	{
		gxlogd("--------------------->ID3v1<---------------------\n");
		gxlogd("Header:\t%s\n", pID3V1->Header);
		gxlogd("Title:\t%s\n", pID3V1->Title);
		gxlogd("Artist:\t%s\n", pID3V1->Artist);
		gxlogd("Album:\t%s\n", pID3V1->Album);
		gxlogd("Year:\t%s\n", pID3V1->Year);
		gxlogd("Comment:\t%s\n", pID3V1->Comment);
		gxlogd("Reserve:\t%d\n", pID3V1->Reserve[0]);
		gxlogd("Track:\t%d\n", pID3V1->Track[0]);
		gxlogd("Genre:\t%d\n", pID3V1->Genre[0]);
		gxlogd("--------------------->ID3V1<---------------------\n");
	}
}
