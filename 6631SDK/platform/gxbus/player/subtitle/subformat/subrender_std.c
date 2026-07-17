#include "gx_subrender.h"
#include "spudec.h"

static void output_std(FILE* f, int w, int h, unsigned char *src, unsigned char *srca, int stride,void* data)
{
	int x, y,i;
	GxSpuDec* spu = (GxSpuDec*)data;

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			char res;
			if (src[x]&&srca[x])
			{
				if(spu->custom){
					res = '4';
					for(i=0;i<4;i++){
						if(src[x]==(spu->cuspal[i]>>16)){
							if(spu->alpha[i]!=0)
								res = i+48;
							else
								res = ' ';
							break;
						}
					}
				}
				else{
					res = '4';
					for(i=0;i<16;i++){
						if(src[x]==(spu->global_palette[i]>>16)){
							res = i+48;
							break;
						}
					}
				}
			}
			else
				res = ' ';
			putc(res, f);
		}
		putc('\n', f);
		src += stride;
		srca += stride;
	}
}

static void draw_std(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride,void* data)
{
	FILE* f;
	char buf[128];
	GxSpuDec* spu = (GxSpuDec*)data;

	snprintf(buf, sizeof(buf), "subtitle-%d-%d.pgm", spu->start_pts / 90, spu->end_pts / 90);
	f = fopen(buf, "w");
	output_std(f, w, h, src, srca, stride,data);
	fclose(f);
}

GxSubRender sub_render_std = {
	.draw  = draw_std,
	.clean = NULL,
};

