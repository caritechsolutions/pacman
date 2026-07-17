
#include "gx_subrender.h"
#include "convert_utf.h"
#include "spudec.h"

#if 0
#include <iconv.h>

int code_convert(char* from , char*  to,char* in_buf,size_t in_len,char* outbuf,size_t out_len){
	iconv_t cd;
	char* *pin = &in_buf;
	char** pout = &outbuf;

	cd = iconv_open(to,from);
	if(cd == 0)
		return -1;
	memset(outbuf,0,out_len);
	if(iconv(cd,pin,&in_len,pout,&out_len)== -1 )
		return -1;
	iconv_close(cd);
	return 0;
}

int u2g(char* in_buf,int in_len,char* outbuf,int out_len){

	return (code_convert("UTF-8", "gb18030", in_buf, in_len,  outbuf, out_len));
}
#else
int u2g(char* in_buf,int in_len,char* outbuf,int out_len){

	return 0;
}
#endif

#ifndef I386_LINUX
extern GxSubRender sub_render_rgb;
extern GxSubRender sub_render_yuv;
#endif
extern GxSubRender sub_render_std;


static GxSubRender*  sub_render[] = {
#ifndef I386_LINUX
 	&sub_render_yuv,
    &sub_render_rgb,
   #endif
	&sub_render_std
};

static GxSubRender* sub_render_class=NULL;
static int          sub_render_inited=0;

int GxSubRender_Init(int type,void* args)
{
	if(sub_render_inited == 0)
	{
		if((type>SUB_RENDER_STD) ||(type<SUB_RENDER_SPP))
			return GX_PLAYER_ERROR;

		sub_render_class = sub_render[type];
		if(sub_render_class->init)
		{
			if(sub_render_class->init(args)==GX_PLAYER_OK)
			{
				sub_render_inited=1;
				return GX_PLAYER_OK;
			}
			else
				return GX_PLAYER_ERROR;
		}
		sub_render_inited=1;
	}
	return GX_PLAYER_OK;
}

void GxSubRender_Destory(void)
{
	if(sub_render_class && sub_render_class->destroy)
		sub_render_class->destroy();
	sub_render_inited = 0;
	sub_render_class = NULL;
}

void GxSubRender_Draw(GxSubData* sub)
{
	if(!sub)
		return ;

	if(sub_render_inited==0)
		return ;

	switch(sub->encode){
		case SUB_ENC_SPU:
			spudec_draw(sub->spu, sub_render_class->draw);
			break;
		default:
			break;
	}
	return ;
}

void GxSubRender_Clean(GxSubData* sub)
{
	if(sub_render_class && sub_render_class->clean)
		sub_render_class->clean();
}


void GxSubRender_Hide(GxSubData* sub)
{
	if(sub_render_class && sub_render_class->hide)
		sub_render_class->hide();
}

void GxSubRender_Show(GxSubData* sub)
{
	if(sub_render_class && sub_render_class->show)
		sub_render_class->show();
}


