
#ifndef _ID3V2TAG_H
#define _ID3V2TAG_H

#include "id3_common.h"
#include "gx_stream.h"

#define TAGHSIZE sizeof(ID3V2H)
#define TAGFHSIZE sizeof(ID3V2FH)
#define TAGFLSIZE sizeof(ID3V2FL)

/* 媒体文件ID3V2标签头结构体 */
#pragma pack(1)
typedef struct ID3V2TagH
{
	char header[3]; 			/* 必须为"ID3"否则认为标签不存在,3字节 */
	char version[1]; 			/* 版本号ID3V2.3 就记录3，1字节 */
	char revision[1]; 			/* 副版本号此版本记录为0，1字节 */
	char flag[1]; 				/* 存放标志的字节，这个版本只定义了三位，1字节 */
	char size[4]; 				/* 标签大小，包括标签头的10个字节和所有的标签帧的大小，4字节 */
}ID3V2H;
#pragma pack()

/* 媒体文件ID3V2H标签帧帧头结构体 */
#pragma pack(1)
typedef struct ID3V2FrameH
{
	char frameID[4]; 			/* 用四个字符标识一个帧，说明其内容,4字节 */
	unsigned char size[4]; 				/* 帧内容的大小，不包括帧头，不得小于1，4字节 */
	char flags[2]; 				/* 存放标志，只定义了6 位，2字节 */
}ID3V2FH;
#pragma pack()

/* 媒体文件ID3V2标签帧链表结构体 */
#pragma pack(1)
typedef struct ID3V2TagFL
{
	char fName[4+1];			/* 多处的一个字节用来存放'\0' */
	char *pFBody;
	int  size;
	struct ID3V2TagFL *pNext;
}ID3V2FL;
#pragma pack()

/* 读取目标文件的标签信息并存储到ID3V2FL结构体链表中，成功返回0否则返回-1 */
int get_ID3V2Tag_info(GxStream* s, ID3V2FL *pID3V2FL, int *tagsize);

/* print_ID3v2Tag:打印出ID3v2标签信息 */
void print_ID3v2Tag(ID3V2FL *header);

/* free_frame_linkedtable:释放由结构体(ID3V2FL)组成的链表 */
void free_frame_linkedtable(ID3V2FL *header);

#endif

