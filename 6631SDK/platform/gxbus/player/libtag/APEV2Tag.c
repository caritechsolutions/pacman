#include <stdio.h>

#include "APEV2Tag.h"

/* get_APEv2Tag_info:读取目标文件的APEv2标签信息，成功返回0否从返回-1*/
int get_APEv2Tag_info(GxStream* s, APEv2Item *header, int tagflag, int ID3v2Size)
{
	int c;
	int i;
	int item_num;
	int item_size;
	long tag_size;
	APEv2HF HF_st;
	APEv2Item * p1 = NULL, *p2 = NULL;
	off_t file_size = 0;

	if (NULL == s){
		gxlogd("File stream's pointer is NULL!\n");
		return -1;
	}

	if (NULL == header){
		gxlogd("APEv2HF's pointeris NULL!\n");
		return -1;
	}

	GxStream_Control(s, GX_STREAM_CTRL_GET_SIZE, &file_size);
gxlogd("------- file_size %lld\n", file_size);
	/* 从文件尾部开始搜寻APEv2标签标签尾*/
	if (tagflag == 1 || tagflag == 3){
		GxStream_Seek(s, file_size-(128+32));
	}else{
		GxStream_Seek(s, file_size-32);
	}

	GxStream_Read(s, (uint8_t*)&HF_st, sizeof(APEv2HF));

	if (0 == strncmp(HF_st.preamble, "APETAGEX", 8))
	{
		gxlogd("This medium has APEv2Tag footer!\n");

		tag_size =	((HF_st.tagsize)[3] << 24) |
			((HF_st.tagsize)[2] << 16) |
			((HF_st.tagsize)[1] << 8) |
			(HF_st.tagsize)[0];

		item_num = 	((HF_st.itemcount)[3] << 24) |
			((HF_st.itemcount)[2] << 16) |
			((HF_st.itemcount)[1] << 8) |
			(HF_st.itemcount)[0];

		if (tagflag == 1 || tagflag == 3){
			if(file_size < (128L + tag_size)){
				gxlogd("file size [%lld] error\n", file_size);
				return -1;
			}
			GxStream_Seek(s, file_size-(128+tag_size));
		}else{
			if(file_size < (tag_size)){
				gxlogd("file size [%lld] error\n", file_size);
				return -1;
			}
			GxStream_Seek(s, file_size-tag_size);
		}

		//gxlogd("TagSize:%ld\t", tag_size);
		//gxlogd("Itemcount:%d\n", item_num);
	}
	else
	{
		/* 从文件头部开始搜寻APEv2标签标签头*/
		if (tagflag == 2 || tagflag == 3)				/* 判断是否有ID3v2标签*/
			GxStream_Seek(s, ID3v2Size);
		else
			GxStream_Seek(s, 0);

		GxStream_Read(s, (uint8_t*)&HF_st, sizeof(APEv2HF));
		if (0 == strncmp(HF_st.preamble, "APETAGEX", 8))
		{
			gxlogd("This medium has APEv2Tag header!\n");

			tagflag += HAS_APEv2;

			tag_size =	((HF_st.tagsize)[3] << 24) |
				((HF_st.tagsize)[2] << 16) |
				((HF_st.tagsize)[1] << 8) |
				(HF_st.tagsize)[0];

			item_num = 	((HF_st.itemcount)[3] << 24) |
				((HF_st.itemcount)[2] << 16) |
				((HF_st.itemcount)[1] << 8) |
				(HF_st.itemcount)[0];

			if (tagflag == 2 || tagflag == 3)				/* 判断是否有ID3v2标签*/
				GxStream_Seek(s, ID3v2Size + 32);
			else
				GxStream_Seek(s, 32);

			//gxlogd("TagSize:%ld\t", tag_size);
			//gxlogd("Itemcount:%d\n", item_num);
		}
		else
		{
			gxlogd("This medium has not APEv2Tag!\n");
			return -1;
		}
	}/* end of else*/

	/* 头结点不存储信息*/
	p2 = header;

	/* 搜寻标签元素中的关键字和其对应的值*/
	for (i=0; i<item_num; i++)
	{

		if ( NULL == (p1 = (APEv2Item* )av_malloc(sizeof(APEv2Item))) )
		{
			gxloge ("Memory allocate for item error\n");
			return -1;
		}

		GxStream_Read(s, (uint8_t*)p1->itemvaluesize, 4);
		GxStream_Read(s, (uint8_t*)p1->itemflags, 4);

		item_size = ((p1->itemvaluesize)[3] << 24) |
			((p1->itemvaluesize)[2] << 16) |
			((p1->itemvaluesize)[1] << 8) |
			(p1->itemvaluesize)[0];

		/* 当标签元素的大小为负的时候，表示提取APEv2标签元素有问题，终止提取*/
		if (item_size < 0)
		{
			break;
		}

		//gxlogd("(%2d)ItemFlags: ", i);
		//gxlogd("ItemSize:%d\t", item_size);

		while ( (c = GxStream_ReadChar(s) ) != 0x00)
		{
			(p1->itemkey)[strlen(p1->itemkey)] = c;
		}
		//gxlogd("itemkey:%s\t", p1->itemkey);

		if ( NULL == (p1->itemvalue_p = (char* )av_malloc(item_size + 1)) )
		{
			gxloge ("Memory allocate for itemvalue error\n");
			return -1;
		}
		else
		{
			memset(p1->itemvalue_p, 0, item_size + 1);
		}

		GxStream_Read(s, (uint8_t*)p1->itemvalue_p, item_size);
		//gxlogd("itemvalue:->%s<-\n", p1->itemvalue_p);


		p2->nextp = p1;
		p2 = p1;
		p1->nextp = NULL;
	}/* end of for*/

	return 0;
}

/* print_APEv2Tag: 打印APEv2标签信息*/
void print_APEv2Tag(APEv2Item* header)
{
	APEv2Item* p = header;

	if (NULL == p)
		return;

	gxlogd("--------------------->APEv2<---------------------\n");
	while (p->nextp != NULL)
	{
		p = p->nextp;					/* 由于头结点不存信息，所以跳过头结点*/
		gxlogd("%s\t%s\n", p->itemkey, p->itemvalue_p);
	}
	gxlogd("--------------------->APEv2<---------------------\n");
}

/* free_item_linkedtable:释放有标签元素结构体(APEv2Item)结构体组成的链表*/
void free_item_linkedtable(APEv2Item* item_stp)
{
	//int counter = 0;
	APEv2Item* p1 = NULL, *p2 = NULL;

	p1 = item_stp;

	if (NULL == p1)
		return;

	do
	{
		p2 = p1->nextp;

		av_free(p1->itemvalue_p);
		p1->itemvalue_p = NULL;

		av_free(p1);
		//gxlogd("APEv2-Node[%2d] is free!\n", counter);
		//counter++;

	}while ( (p1 = p2) != NULL);

	p2 = p1 = item_stp = NULL;

}

