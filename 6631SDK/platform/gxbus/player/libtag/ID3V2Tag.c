#include <stdio.h>
#include "ID3V2Tag.h"

/* 读取目标文件的标签信息并存储到ID3V2FL结构体链表中，成功返回0否则返回-1*/
int get_ID3V2Tag_info(GxStream* s, ID3V2FL *pID3V2FL, int *tagsize_p)
{
	int counter = 0;
	int counter1 = 0;
	int tag_size = 0;				/* ID3V2标签的大小,包括标签头的10个字节和所有的标签帧的大小*/
	int read_bytes = 0;
	long fbody_size = 0;			/* 标签帧帧体的大小*/
	char* buffer = NULL;
	ID3V2H tagHeader;			/* ID3V2标签头结构体*/
	ID3V2FH frameHeader;			/* ID3V2标签帧帧头结构体*/
	ID3V2FL* p1 = NULL;
	ID3V2FL* p2 = NULL;

	if (s == NULL){
		gxlogd("File stream's pointer is NULL!\n");
		return -1;
	}

	if (pID3V2FL == NULL){
		gxlogd("pID3V2FL  is NULL!\n");
		return -1;
	}

	GxStream_Seek(s, 0);
	GxStream_Read(s, (uint8_t*)&tagHeader, TAGHSIZE); /* 读取标签头*/

	if(strncmp(tagHeader.header, "ID3", 3) != 0){
		gxlogd("This medium has not ID3v2Tag!\n");
		return -1;
	}

	/* 计算ID3V2标签的大小，包括标签头的10字节大小*/
	*tagsize_p = tag_size = (((tagHeader.size)[0] & 0x7F) << 21) |
		(((tagHeader.size)[1] & 0x7F) << 14) |
		(((tagHeader.size)[2] & 0x7F) << 7) |
		((tagHeader.size)[3] & 0x7F);

	//gxlogd("tag_size:%d\n", tag_size);
	if(tagHeader.flag[0]&0x40){
		gxlogd("extened flags, skip 10 bytes.\n");
		GxStream_Skip(s, 10);
		tag_size -= 10;
	}

	p2 = pID3V2FL;									/* 链表头结点不存储信息*/

	while (read_bytes < (tag_size - 10))
	{
		GxStream_Read(s, (uint8_t*)&frameHeader, TAGFHSIZE);

		fbody_size=((frameHeader.size)[0] << 24) |
			((frameHeader.size)[1] << 16) |
			((frameHeader.size)[2] << 8) |
			(frameHeader.size)[3];

		read_bytes += (10 + fbody_size);

		/*		if(strcmp(frameHeader.frameID,"APIC")==0){
				char* p = av_malloc();
				read_bytes +=(10+(int)(frameHeader.size));
				fread(&frameHeader, read_bytes, 1, fp);
				continue;
				}
				*/
		if (fbody_size == 0){
			++counter;
			if (counter >= 5){					//当程序获取标签帧的帧体连续5次都五内容
				//gxlogd("No more frame!\n");   //程序就判断后续已没有信息标签帧，随即中循环
				break;
			}
			continue;
		}else{
			counter = 0;
		}

		//gxlogd("fbody_size:\t%d\t", fbody_size);

		if (!(p1 = (ID3V2FL* )av_malloc(TAGFLSIZE))){
			gxlogd("Memory allocate for ID3V2FL error\n");
			return -1;
		}else{
			memset(p1,0,TAGFLSIZE);
		}

		if (!(buffer = (char* )av_malloc(fbody_size + 1)) ){
			gxlogd("Memory allocate for buffer error: body_size=%ld\n",fbody_size);
			return -1;
		}else{
			memset(buffer, 0, fbody_size + 1);
		}

		strncpy(p1->fName, frameHeader.frameID, sizeof(p1->fName)-1);

		p1->pFBody = buffer;

		if(strcmp(p1->fName,"APIC")!=0){
			while (counter1 < fbody_size){
				p1->pFBody[counter1] = GxStream_ReadChar(s);
				++counter1;
			}
			p1->size = counter1;
		}
		else
		{
			GxStream_Read(s, (uint8_t*)p1->pFBody, fbody_size);
			p1->size = fbody_size;
		}

		counter1 = 0;							/* 计数器清零，开始下一标签帧帧体的计数*/

		p2->pNext = p1;
		p1->pNext = NULL;
		p2 = p1;

	}/* end of while*/

	buffer = NULL;

	return 0;
}

/* print_ID3v2Tag:打印出ID3v2标签信息*/
void print_ID3v2Tag(ID3V2FL* header)
{
	ID3V2FL* p = header;
	char pic[13] = {'\0'};

	if (NULL == p)
		return;

	gxlogd("--------------------->ID3v2<---------------------\n");
	while (p->pNext != NULL)
	{
		p = p->pNext;							/* 由于头结点不存信息，所以跳过头结点*/
		if(strcmp(p->fName,"APIC")!=0)
			gxlogd("%s\t%s\n", p->fName, p->pFBody);
		else
		{
			memcpy(pic,p->pFBody+1,11);
			gxlogd("%s\t%s\n", p->fName, pic);
#if 0
			char* file= NULL;
			if(strstr(p->pFBody+1,"jpg") || strstr(p->pFBody+1,"JPG"))
				file="a.jpg";
			else if(strstr(p->pFBody+1,"png") || strstr(p->pFBody+1,"PNG"))
				file="a.jpg";
			else
				file="a.bin";
			FILE* fp=fopen(file,"wb+");
			fwrite((p->pFBody+13),p->size,1,fp);
			fclose(fp);
#endif
		}
	}
	gxlogd("--------------------->ID3v2<---------------------\n");
}

/* free_frame_linkedtable:释放由结构体(ID3V2FL)组成的链表*/
void free_frame_linkedtable(ID3V2FL* header)
{
	//int counter = 0;
	ID3V2FL* p1 = NULL, *p2 = NULL;

	if (NULL == header)
		return;

	p1 = header;

	do
	{
		p2 = p1->pNext;

		if (p1->pFBody != NULL)
			av_free(p1->pFBody);
		p1->pFBody = NULL;

		av_free(p1);
		//gxlogd("ID3v2-Node[%2d] is free!\n", counter);

		//++counter;

	}while ( (p1 = p2) != NULL );

	p2 = p1 = NULL;
}

