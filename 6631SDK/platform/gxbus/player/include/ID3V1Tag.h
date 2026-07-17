
#ifndef _ID3V1TAG_H
#define _ID3V1TAG_H

#include "id3_common.h"
#include "gx_stream.h"
#define TAGSIZE sizeof(ID3V1)		/* 标签结构体的大小 */

/* 此结构体中的成员字符数组的长度必须多定义一个长度，转换字符串时用 */
/* 多处的一个字节用来存放'\0' */
typedef struct tagID3V1
{
	char Header[4+1]; 				/* 标签头必须是"TAG"否则认为没有标签,3字节 */
	char Title[30+1]; 				/* 标题,30字节 */
	char Artist[30+1]; 				/* 作者,30字节 */
	char Album[30+1]; 			/* 专集,30字节 */
	char Year[4+1]; 				/* 出品年代,4字节 */
	char Comment[28+1]; 			/* 备注,28字节 */
	char Reserve[1]; 				/* 保留,1字节 */
	char Track[1+1]; 				/* 音轨,1字节 */
	char Genre[1+1]; 				/* 类型,1字节 */
}ID3V1;

/* 读取目标文件的标签信息并存储到ID3V1结构体中，成功返回0否则返回-1 */
int get_ID3V1Tag_info(GxStream *s, ID3V1 *pID3V1);

/* print_ID3v1Tag:打印出标记结构体中的成员信息 */
void print_ID3v1Tag(ID3V1 *pID3V1);

#endif
