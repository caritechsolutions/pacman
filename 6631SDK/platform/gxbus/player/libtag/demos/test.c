
#include "gx_id3.h"

int main(int argc, char* *argv)
{
	int tagflag = 0;						/* 标签存在标志*/
	int ID3v2Size = 0;						/* ID3v2标签大小*/
	FILE* fp = NULL;
	ID3V1* pID3V1 = NULL;				/* ID3V1标签结构体指针 */
	ID3V2FL* pID3V2FL = NULL;
	APEv2Item* item_hp = NULL;			/* APEv2标签结构体指针 */
	
	if (argc != 2)
	{
		gxlogd("Usage:%s	<filename>\n", argv[0]);
		return -1;
	}

//	argv[1] = "nng.mp3";
	
	if ( NULL == (fp = fopen(argv[1], "rb")) )
	{
		gxloge ( "Open file '%s' failure!\n", argv[1]);
		return -1;
	}
	else
	{
		gxlogd("Open file '%s' success!\n", argv[1]);
	}
	
	if ( NULL == (pID3V1 = (ID3V1* )av_malloc(TAGSIZE)) )
	{
		perror("Memory allocate for ID3V1 error");
		return -1;
	}
	
	if ( NULL == (pID3V2FL = (ID3V2FL* )av_malloc(TAGFLSIZE)) )
	{
		av_free(pID3V1);
		pID3V1 = NULL;
		
		perror("Memory allocate for ID3V2FL error");
		return -1;
	}
	
	if ( NULL == (item_hp = (APEv2Item* )av_malloc(sizeof(APEv2Item))) )
	{
		av_free(pID3V1);
		pID3V1 = NULL;
		
		av_free(pID3V2FL);
		pID3V2FL = NULL;
		
		perror("Memory allocate for APEv2Item error");
		return -1;
	}
	
	/* 读取ID3v1标签*/
	if (0 == get_ID3V1Tag_info(fp, pID3V1))
	{
		tagflag += HAS_ID3v1;
	}
	
	/* 读取ID3v2标签*/
	if (0 == get_ID3V2Tag_info(fp, pID3V2FL, &ID3v2Size))
	{
		tagflag += HAS_ID3v2;
	}
	
	/* 读取APEv2标签*/
	if (0 == get_APEv2Tag_info(fp, item_hp, tagflag, ID3v2Size))
	{
		tagflag += HAS_APEv2;
	}
	
	fclose(fp);
	
	print_ID3v1Tag(pID3V1);
 	print_ID3v2Tag(pID3V2FL);
 	print_APEv2Tag(item_hp);
 	
	/* 释放ID3v1标签结构体*/
	if (pID3V1 != NULL)
	{
		av_free(pID3V1);
		pID3V1 = NULL;
	}
	
	/* 释放由ID3v2结构体组成的链表*/
	free_frame_linkedtable(pID3V2FL);
	pID3V2FL = NULL;
	
	/* 释放由APEv2Item结构体组成的链表*/
	free_item_linkedtable(item_hp);
	item_hp = NULL;
	
	return 0;
}
