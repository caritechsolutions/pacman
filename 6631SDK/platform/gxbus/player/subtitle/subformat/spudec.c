/* SPUdec.c
   Skeleton of function spudec_process_controll() is from xine sources.
   Further works:
   LGB,... (yeah, try to improve it and insert your name here! ;-)

   Kim Minh Kaplan
   implement fragments reassembly, RLE decoding.
   read brightness from the IFO.

   For information on SPU format see <URL:http://sam.zoy.org/doc/dvd/subtitles/>
   and <URL:http://members.aol.com/mpucoder/DVD/spu.html>

*/
#include "spudec.h"

/* Valid values for spu_aamode:
   0: none (fastest, most ugly)
   1: approximate
   2: full (slowest)
   3: bilinear (similiar to vobsub, fast and not too bad)
   4: uses swscaler gaussian (this is the only one that looks good)
*/

int spu_aamode = 3;
int spu_alignment = -1;
float spu_gaussvar = 1.0;
int sub_pos;


static void spudec_queue_packet(GxSpuDec* this, GxSpuPacket *packet)
{
  if (this->queue_head == NULL)
    this->queue_head = packet;
  else
    this->queue_tail->next = packet;
  this->queue_tail = packet;
}

static GxSpuPacket* spudec_dequeue_packet(GxSpuDec *this)
{
  GxSpuPacket* retval = this->queue_head;

  this->queue_head = retval->next;
  if (this->queue_head == NULL)
    this->queue_tail = NULL;

  return retval;
}

static void spudec_free_packet(GxSpuPacket* packet)
{
  if (packet->packet != NULL)
    av_free(packet->packet);
  av_free(packet);
}

static inline unsigned int get_be16(const unsigned char* p)
{
  return (p[0] << 8) + p[1];
}

static inline unsigned int get_be24(const unsigned char* p)
{
  return (get_be16(p) << 8) + p[2];
}

static void next_line(GxSpuPacket* packet)
{
  if (packet->current_nibble[packet->deinterlace_oddness] % 2)
    packet->current_nibble[packet->deinterlace_oddness]++;
  packet->deinterlace_oddness = (packet->deinterlace_oddness + 1) % 2;
}

static inline unsigned char get_nibble(GxSpuPacket* packet)
{
  unsigned char nib;
  unsigned int* nibblep = packet->current_nibble + packet->deinterlace_oddness;
  if (*nibblep / 2 >= packet->control_start) {
    gx_msg(MSGT_SPUDEC,MSGL_WARN, "SPUdec: ERROR: get_nibble past end of packet\n");
    return 0;
  }
  nib = packet->packet[*nibblep / 2];
  if (*nibblep % 2)
    nib &= 0xf;
  else
    nib >>= 4;
  ++*nibblep;
  return nib;
}

static inline int mkalpha(int i)
{
  /* In mplayer's alpha planes, 0 is transparent, then 1 is nearly
     opaque upto 255 which is transparent*/
  switch (i) {
  case 0xf:
    return 1;
  case 0:
    return 0;
  default:
    return (0xf - i) << 4;
  }
}

unsigned int spudec_palette_to_yuv(unsigned int pal)
{
    int r, g, b, y, u, v;
    // Palette in idx file is not rgb value, it was calculated by wrong formula.
    // Here's reversed formula of the one used to generate palette in idx file.
    r = pal >> 16 & 0xff;
    g = pal >> 8 & 0xff;
    b = pal & 0xff;
    y = av_clip_uint8( 0.1494 *  r + 0.6061 * g + 0.2445 * b);
    u = av_clip_uint8( 0.6066 *  r - 0.4322 * g - 0.1744 * b + 128);
    v = av_clip_uint8(-0.08435*  r - 0.3422 * g + 0.4266 * b + 128);
    y = y*  219 / 255 + 16;
    return y << 16 | u << 8 | v;
}

unsigned int spudec_rgb_to_yuv(unsigned int rgb)
{
    int r, g, b, y, u, v;
    r = rgb >> 16 & 0xff;
    g = rgb >> 8 & 0xff;
    b = rgb & 0xff;
    y = ( 0.299  *  r + 0.587   * g + 0.114   * b) * 219 / 255 + 16.5;
    u = (-0.16874*  r - 0.33126 * g + 0.5     * b) * 224 / 255 + 128.5;
    v = ( 0.5    *  r - 0.41869 * g - 0.08131 * b) * 224 / 255 + 128.5;
    return y << 16 | u << 8 | v;
}


/* Cut the sub to visible part*/
static inline void spudec_cut_image(GxSpuDec* this)
{
  unsigned int fy, ly;
  unsigned int first_y, last_y;
  unsigned char* image;
  unsigned char* aimage;

  if (this->stride == 0 || this->height == 0) {
    return;
  }

  for (fy = 0; fy < this->image_size && !this->aimage[fy]; fy++);
  for (ly = this->stride*  this->height-1; ly && !this->aimage[ly]; ly--);
  first_y = fy / this->stride;
  last_y = ly / this->stride;
  //gxlogd("first_y: %d, last_y: %d\n", first_y, last_y);
  this->start_row += first_y;

  // Some subtitles trigger this condition
  if (last_y + 1 > first_y ) {
	  this->height = last_y - first_y +1;
  } else {
	  this->height = 0;
	  this->image_size = 0;
	  return;
  }

//  gxlogd("new h %d new start %d (sz %d st %d)---\n\n", this->height, this->start_row, this->image_size, this->stride);

  image = av_malloc(2*  this->stride * this->height);
  if(image){
    this->image_size = this->stride*  this->height;
    aimage = image + this->image_size;
    memcpy(image, this->image + this->stride*  first_y, this->image_size);
    memcpy(aimage, this->aimage + this->stride*  first_y, this->image_size);
    av_free(this->image);
    this->image = image;
    this->aimage = aimage;
  } else {
    gx_msg(MSGT_SPUDEC, MSGL_FATAL, "Fatal: update_spu: av_malloc requested %d bytes\n", 2*  this->stride * this->height);
  }
}

static void guess_palette(GxSpuDec* this,
		unsigned int *cmap, unsigned int *alpha, unsigned int *colormap, unsigned int subtitle_color)
{
	static const uint8_t level_map[4][4] = {
		{0xff},
		{0x00, 0xff},
		{0x00, 0x80, 0xff},
		{0x00, 0x55, 0xaa, 0xff},
	};
	uint8_t color_used[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0};
	int nb_opaque_colors, i, level, j, r, g, b;

	for(i = 0; i < 4; i++) {
		cmap[i] = 0;
	}

	nb_opaque_colors = 0;
	for (i = 0; i < 4; i++) {
		if (alpha[i] != 0 && !color_used[colormap[i]]) {
			color_used[colormap[i]] = 1;
			nb_opaque_colors++;
		}
	}

	if (nb_opaque_colors == 0)
		return;

	memset(this->global_palette, 0, sizeof(this->global_palette));
	j = 0;
	memset(color_used, 0, 16);
	for(i = 0; i < 4; i++) {
		if (alpha[i] != 0) {
			if (!color_used[colormap[i]])  {
				level = level_map[nb_opaque_colors - 1][j];
				r = (((subtitle_color >> 16) & 0xff) * level) >> 8;
				g = (((subtitle_color >> 8) & 0xff) * level) >> 8;
				b = (((subtitle_color >> 0) & 0xff) * level) >> 8;
				cmap[i] = b | (g << 8) | (r << 16);
				this->global_palette[colormap[i]] = cmap[i];
				color_used[colormap[i]] = (i + 1);
				j++;
			} else {
				cmap[i] = cmap[color_used[colormap[i]] - 1] & 0xffffff;
				this->global_palette[colormap[i]] = cmap[i];
			}
		}
	}
	for (i = 0; i < 4; i++) {
		cmap[i] = (cmap[i] >> 16) & 0xff;
	}
}

static void spudec_process_data(GxSpuDec* this, GxSpuPacket *packet)
{
  unsigned int cmap[4], alpha[4];
  unsigned int i, x, y;

  this->scaled_frame_width = 0;
  this->scaled_frame_height = 0;
  this->start_col = packet->start_col;
  this->end_col = packet->end_col;
  this->start_row = packet->start_row;
  this->end_row = packet->end_row;
  this->height = packet->height;
  this->width = packet->width;
  this->stride = packet->stride;

  for (i = 0; i < 4; ++i) {
    alpha[i] = mkalpha(packet->alpha[i]);
    if (this->custom && (this->cuspal[i] >> 31) != 0)
      alpha[i] = 0;
    if (alpha[i] == 0)
      cmap[i] = 0;
    else if (this->custom){
      cmap[i] = ((this->cuspal[i] >> 16) & 0xff);
      if (cmap[i] + alpha[i] > 255)
	cmap[i] = 256 - alpha[i];
    }
    else {
      cmap[i] = ((this->global_palette[packet->palette[i]] >> 16) & 0xff);
      if (cmap[i] + alpha[i] > 255)
	cmap[i] = 256 - alpha[i];
    }
  }
	if (this->has_palette == 0)
	 guess_palette(this, cmap, alpha, packet->palette, 0xffffff00);

  if (this->image_size < this->stride*  this->height) {
    if (this->image != NULL) {
      av_free(this->image);
      this->image_size = 0;
    }
    this->image = av_malloc(2*  this->stride * this->height);
    if (this->image) {
      this->image_size = this->stride*  this->height;
      this->aimage = this->image + this->image_size;
    }
  }
  if (this->image == NULL)
    return;

  /* Kludge: draw_alpha needs width multiple of 8.*/
  if (this->width < this->stride)
    for (y = 0; y < this->height; ++y) {
      memset(this->aimage + y*  this->stride + this->width, 0, this->stride - this->width);
      /* FIXME: Why is this one needed?*/
      memset(this->image + y*  this->stride + this->width, 0, this->stride - this->width);
    }

  i = packet->current_nibble[1];
  x = 0;
  y = 0;
  while (packet->current_nibble[0] < i
	 && packet->current_nibble[1] / 2 < packet->control_start
	 && y < this->height) {
    unsigned int len, color;
    unsigned int rle = 0;
    rle = get_nibble(packet);
    if (rle < 0x04) {
      rle = (rle << 4) | get_nibble(packet);
      if (rle < 0x10) {
	rle = (rle << 4) | get_nibble(packet);
	if (rle < 0x040) {
	  rle = (rle << 4) | get_nibble(packet);
	  if (rle < 0x0004)
	    rle |= ((this->width - x) << 2);
	}
      }
    }
    color = 3 - (rle & 0x3);
    len = rle >> 2;
    if (len > this->width - x || len == 0)
      len = this->width - x;
    /* FIXME have to use palette and alpha map*/
    memset(this->image + y*  this->stride + x, packet->palette[color], len);
    memset(this->aimage + y*  this->stride + x, alpha[color], len);
    x += len;
    if (x >= this->width) {
      next_line(packet);
      x = 0;
      ++y;
    }
  }
  spudec_cut_image(this);
}


/*
  This function tries to create a usable palette.
  It determines how many non-transparent colors are used, and assigns different
gray scale values to each color.
  I tested it with four streams and even got something readable. Half of the
times I got black characters with white around and half the reverse.
*/
static void compute_palette(GxSpuDec* this, GxSpuPacket *packet)
{
  int used[16],i,cused,start,step,color;

  memset(used, 0, sizeof(used));
  for (i=0; i<4; i++)
    if (packet->alpha[i]) /* !Transparent?*/
       used[packet->palette[i]] = 1;
  for (cused=0, i=0; i<16; i++)
    if (used[i]) cused++;
  if (!cused) return;
  if (cused == 1) {
    start = 0x80;
    step = 0;
  } else {
    start = this->font_start_level;
    step = (0xF0-this->font_start_level)/(cused-1);
  }
  memset(used, 0, sizeof(used));
  for (i=0; i<4; i++) {
    color = packet->palette[i];
    if (packet->alpha[i] && !used[color]) { /* not assigned?*/
       used[color] = 1;
       this->global_palette[color] = start<<16;
       start += step;
    }
  }
}

static void spudec_process_control(GxSpuDec* this, int pts100)
{
  int a,b; /* Temporary vars*/
  unsigned int date, type;
  unsigned int off;
  unsigned int start_off = 0;
  unsigned int next_off;
  unsigned int start_pts = 0;
  unsigned int end_pts = 0;
  unsigned int current_nibble[2] = {0, 0};
  unsigned int control_start;
  unsigned int display = 0;
  unsigned int start_col = 0;
  unsigned int end_col = 0;
  unsigned int start_row = 0;
  unsigned int end_row = 0;
  unsigned int width = 0;
  unsigned int height = 0;
  unsigned int stride = 0;

  control_start = get_be16(this->packet + 2);
  next_off = control_start;
  while (start_off != next_off) {
    start_off = next_off;
    date = get_be16(this->packet + start_off)*  1024;
    next_off = get_be16(this->packet + start_off + 2);
    off = start_off + 4;
    for (type = this->packet[off++]; type != 0xff; type = this->packet[off++]) {
      //gxlogf( "cmd=%d  ",type);
		switch(type) {
			case 0x00:
				/* Menu ID, 1 byte*/
				//gxlogf("Menu ID\n");
				/* shouldn't a Menu ID type force display start?*/
				start_pts = pts100 < 0 && -pts100 >= date ? 0 : pts100 + date;
				display = 1;
				this->is_forced_sub=~0; // current subtitle is forced
				break;
			case 0x01:
				/* Start display*/
				//gxlogf("Start display!\n");
				start_pts = pts100 < 0 && -pts100 >= date ? 0 : pts100 + date;
				display = 1;
				this->is_forced_sub=0;
				break;
			case 0x02:
				/* Stop display*/
				//gxlogf("Stop display!\n");
				end_pts = pts100 < 0 && -pts100 >= date ? 0 : pts100 + date;
				break;
			case 0x03:
				/* Palette*/
				this->palette[0] = this->packet[off] >> 4;
				this->palette[1] = this->packet[off] & 0xf;
				this->palette[2] = this->packet[off + 1] >> 4;
				this->palette[3] = this->packet[off + 1] & 0xf;
				//gxlogi("Palette %d, %d, %d, %d\n",this->palette[0], this->palette[1], this->palette[2], this->palette[3]);
				off+=2;
			break;
			case 0x04:
				/* Alpha*/
				this->alpha[0] = this->packet[off] >> 4;
				this->alpha[1] = this->packet[off] & 0xf;
				this->alpha[2] = this->packet[off + 1] >> 4;
				this->alpha[3] = this->packet[off + 1] & 0xf;
				//gxlogi("Alpha %d, %d, %d, %d\n", this->alpha[0], this->alpha[1], this->alpha[2], this->alpha[3]);
				off+=2;
				break;
			case 0x05:
				/* Co-ords*/
				a = get_be24(this->packet + off);
				b = get_be24(this->packet + off + 3);
				start_col = a >> 12;
				end_col = a & 0xfff;
				width = (end_col < start_col) ? 0 : end_col - start_col + 1;
				stride = (width + 7) & ~7; /* Kludge: draw_alpha needs width multiple of 8*/
				start_row = b >> 12;
				end_row = b & 0xfff;
				height = (end_row < start_row) ? 0 : end_row - start_row /* + 1*/;
				//gxlogf("Coords  col: %d - %d  row: %d - %d  (%dx%d)\n",start_col, end_col, start_row, end_row,width, height);
				off+=6;
				break;
			case 0x06:
				/* Graphic lines*/
				current_nibble[0] = 2*  get_be16(this->packet + off);
				current_nibble[1] = 2*  get_be16(this->packet + off + 2);
				//gxlogf("Graphic offset 1: %d  offset 2: %d\n", current_nibble[0] / 2, current_nibble[1] / 2);
				off+=4;
				break;
			case 0xff:
				/* All done, bye-bye*/
				gxlogf("Done!\n");
				return;
			default:
				gxloge("spudec: Error determining control type 0x%02x.  Skipping %d bytes.\n",type, next_off - off);
				goto next_control;
		}
    }
  next_control:
    if (!display)
      continue;
    if (end_pts == UINT_MAX && start_off != next_off) {
      end_pts = get_be16(this->packet + next_off)*  1024;
      end_pts = 1 - pts100 >= end_pts ? 0 : pts100 + end_pts - 1;
    }
    if (end_pts > 0) {
      GxSpuPacket* packet = av_calloc(1, sizeof(GxSpuPacket));
      int i;
      packet->start_pts = start_pts;
      packet->end_pts = end_pts;
      packet->current_nibble[0] = current_nibble[0];
      packet->current_nibble[1] = current_nibble[1];
      packet->start_row = start_row;
      packet->end_row = end_row;
      packet->start_col = start_col;
      packet->end_col = end_col;
      packet->width = width;
      packet->height = height;
      packet->stride = stride;
      packet->control_start = control_start;
      for (i=0; i<4; i++) {
			packet->alpha[i] = this->alpha[i];
			packet->palette[i] = this->palette[i];
      }
      packet->packet = av_malloc(this->packet_size);
      memcpy(packet->packet, this->packet, this->packet_size);
      spudec_queue_packet(this, packet);
    }
  }
}

static void spudec_decode(GxSpuDec* this, int pts100)
{
#if 0
  if (!this->hw_spu)
    spudec_process_control(this, pts100);
  else if (pts100 >= 0) {
    static vo_mpegpes_t packet = { NULL, 0, 0x20, 0 };
    static vo_mpegpes_t* pkg=&packet;
    packet.data = this->packet;
    packet.size = this->packet_size;
    packet.timestamp = pts100;
    this->hw_spu->draw_frame((uint8_t**)&pkg);
  }
 #else
 spudec_process_control(this, pts100);
 #endif
}

int spudec_changed(void*  this)
{
    GxSpuDec*  spu = (GxSpuDec*)this;
    return spu->spu_changed || spu->now_pts > spu->end_pts;
}

void spudec_assemble(void* this, unsigned char *packet, unsigned int len, int pts100)
{
  GxSpuDec* spu = (GxSpuDec*)this;
//  spudec_heartbeat(this, pts100);
  if (len < 2) {
      gx_msg(MSGT_SPUDEC,MSGL_WARN,"SPUasm: packet too short len =%d\n",len);
      return;
  }
#if 0
  if ((spu->packet_pts + 10000) < pts100) {
    // [cb] too long since last fragment: force new packet
    spu->packet_offset = 0;
  }
#endif
 // fgxlogi(stderr,"%s %d offset= %d\n",__func__,__LINE__,spu->packet_offset);
  //spu->packet_pts = pts100;
  if (spu->packet_offset == 0) {
    unsigned int len2 = get_be16(packet);
//fgxlogi(stderr,"%s %d len=%d\n",__func__,__LINE__,len2);
    // Start new fragment
    if (spu->packet_reserve < len2) {
		if (spu->packet != NULL)
			av_free(spu->packet);
      spu->packet = av_malloc(len2);
      spu->packet_reserve = spu->packet != NULL ? len2 : 0;
    }
    if (spu->packet != NULL) {
      spu->packet_size = len2;
      if (len > len2) {
	gx_msg(MSGT_SPUDEC,MSGL_WARN,"SPUasm: invalid frag len / len2: %d / %d \n", len, len2);
	return;
      }
      memcpy(spu->packet, packet, len);
      spu->packet_offset = len;
      spu->packet_pts = pts100;
    }
  } else {
    // Continue current fragment
    if (spu->packet_size < spu->packet_offset + len){
      gx_msg(MSGT_SPUDEC,MSGL_WARN,"SPUasm: invalid fragment\n");
      spu->packet_size = spu->packet_offset = 0;
      return;
    } else {
      memcpy(spu->packet + spu->packet_offset, packet, len);
      spu->packet_offset += len;
    }
  }
#if 1
  // check if we have a complete packet (unfortunatelly packet_size is bad
  // for some disks)
  // [cb] packet_size is padded to be even -> may be one byte too long
  if ((spu->packet_offset == spu->packet_size) ||
      ((spu->packet_offset + 1) == spu->packet_size)){
    unsigned int x=0,y;
    while(x+4<=spu->packet_offset){
      y=get_be16(spu->packet+x+2); // next control pointer
//fgxlogi(stderr,"SPUtest: x=%d y=%d off=%d size=%d\n",x,y,spu->packet_offset,spu->packet_size);
      if(x>=4 && x==y){		// if it points to self - we're done!
		//gxlogf("SPUgot: off=%d  size=%d \n",spu->packet_offset,spu->packet_size);
		spudec_decode(spu, spu->packet_pts);
		spu->packet_offset = 0;
		break;
      }
      if(y<=x || y>=spu->packet_size){ // invalid?
//fgxlogi(stderr,"SPUtest: broken packet!!!!! y=%d < x=%d\n",y,x);
        spu->packet_size = spu->packet_offset = 0;
        break;
      }
      x=y;
    }
    // [cb] packet is done; start new packet
    spu->packet_offset = 0;
  }
#else
  if (spu->packet_offset == spu->packet_size) {
    spudec_decode(spu, pts100);
    spu->packet_offset = 0;
  }
#endif
}

void spudec_reset(void* this)	// called after seek
{
  GxSpuDec* spu = (GxSpuDec*)this;
  while (spu->queue_head)
    spudec_free_packet(spudec_dequeue_packet(spu));
  spu->now_pts = 0;
  spu->end_pts = 0;
  spu->packet_size = spu->packet_offset = 0;
}

void spudec_heartbeat(void* this, unsigned int pts100)
{
  GxSpuDec* spu = (GxSpuDec*) this;
  spu->now_pts = pts100;

  while (spu->queue_head != NULL && pts100 >= spu->queue_head->start_pts) {
    GxSpuPacket* packet = spudec_dequeue_packet(spu);
    spu->start_pts = packet->start_pts;
    spu->end_pts = packet->end_pts;
    if (spu->auto_palette)
      compute_palette(spu, packet);
    spudec_process_data(spu, packet);
    spudec_free_packet(packet);
    spu->spu_changed = 1;
  }
}

int spudec_visible(void* this){
    GxSpuDec* spu = (GxSpuDec *)this;
    int ret=(spu->start_pts <= spu->now_pts &&
	     spu->now_pts < spu->end_pts &&
	     spu->height > 0);
//    gxlogd("spu visible: %d  \n",ret);
    return ret;
}

void spudec_set_forced_subs_only(void*  const this, const unsigned int flag)
{
  if(this){
      ((GxSpuDec* )this)->forced_subs_only=flag;
      gxlogf("SPU: Display only forced subs now %s\n", flag ? "enabled": "disabled");
  }
}


void spudec_draw(void* this, void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride,void* spu))
{
    GxSpuDec* spu = (GxSpuDec *)this;
    if (spu->start_pts <= spu->now_pts && spu->now_pts < spu->end_pts && spu->image)
    {
	draw_alpha(spu->start_col, spu->start_row, spu->width, spu->height,
		   spu->image, spu->aimage, spu->stride,spu);
	spu->spu_changed = 0;
    }
}


/* calc the bbox for spudec subs*/
void spudec_calc_bbox(void* me, unsigned int dxs, unsigned int dys, unsigned int* bbox)
{
  GxSpuDec* spu;
  spu = (GxSpuDec* )me;
  if (spu->orig_frame_width == 0 || spu->orig_frame_height == 0
  || (spu->orig_frame_width == dxs && spu->orig_frame_height == dys)) {
    bbox[0] = spu->start_col;
    bbox[1] = spu->start_col + spu->width;
    bbox[2] = spu->start_row;
    bbox[3] = spu->start_row + spu->height;
  }
  else if (spu->scaled_frame_width != dxs || spu->scaled_frame_height != dys) {
    unsigned int scalex = 0x100*  dxs / spu->orig_frame_width;
    unsigned int scaley = 0x100*  dys / spu->orig_frame_height;
    bbox[0] = spu->start_col*  scalex / 0x100;
    bbox[1] = spu->start_col*  scalex / 0x100 + spu->width * scalex / 0x100;
    switch (spu_alignment) {
    case 0:
      bbox[3] = dys*sub_pos/100 + spu->height*  scaley / 0x100;
      if (bbox[3] > dys) bbox[3] = dys;
      bbox[2] = bbox[3] - spu->height*  scaley / 0x100;
      break;
    case 1:
      if (sub_pos < 50) {
        bbox[2] = dys*sub_pos/100 - spu->height*  scaley / 0x200;
        bbox[3] = bbox[2] + spu->height;
      } else {
        bbox[3] = dys*sub_pos/100 + spu->height*  scaley / 0x200;
        if (bbox[3] > dys) bbox[3] = dys;
        bbox[2] = bbox[3] - spu->height*  scaley / 0x100;
      }
      break;
    case 2:
      bbox[2] = dys*sub_pos/100 - spu->height*  scaley / 0x100;
      bbox[3] = bbox[2] + spu->height;
      break;
    default: /* -1*/
      bbox[2] = spu->start_row*  scaley / 0x100;
      bbox[3] = spu->start_row*  scaley / 0x100 + spu->height * scaley / 0x100;
      break;
    }
  }
}
/* transform mplayer's alpha value into an opacity value that is linear*/
static inline int canon_alpha(int alpha)
{
  return alpha ? 256 - alpha : 0;
}

typedef struct {
  unsigned position;
  unsigned left_up;
  unsigned right_down;
}scale_pixel;

static void spudec_parse_extradata(GxSpuDec* this,
                                   uint8_t* extradata, int extradata_len)
{
  uint8_t* buffer, *ptr;
  unsigned int* pal = this->global_palette, *cuspal = this->cuspal;
  unsigned int tridx;
  int i;

  if (extradata_len == 16*4) {
    for (i=0; i<16; i++)
      pal[i] = AV_RB32(extradata + i*4);
    this->auto_palette = 0;
    return;
  }

  if (!(ptr = buffer = av_malloc(extradata_len+1)))
    return;
  memcpy(buffer, extradata, extradata_len);
  buffer[extradata_len] = 0;

  do {
    sscanf((const char*)ptr, "size: %dx%d", &this->orig_frame_width, &this->orig_frame_height);
    if (sscanf((const char*)ptr, "palette: %x, %x, %x, %x, %x, %x, %x, %x,"
                            " %x, %x, %x, %x, %x, %x, %x, %x",
               &pal[ 0], &pal[ 1], &pal[ 2], &pal[ 3],
               &pal[ 4], &pal[ 5], &pal[ 6], &pal[ 7],
               &pal[ 8], &pal[ 9], &pal[10], &pal[11],
               &pal[12], &pal[13], &pal[14], &pal[15]) == 16) {
      this->has_palette = 1;
      this->auto_palette = 0;
    }
    if (!strncasecmp((const char*)ptr, "forced subs: on", 15))
      this->forced_subs_only = 1;
    if (sscanf((const char*)ptr, "custom colors: ON, tridx: %x, colors: %x, %x, %x, %x",
               &tridx, cuspal+0, cuspal+1, cuspal+2, cuspal+3) == 5) {
      for (i=0; i<4; i++) {
        cuspal[i] = spudec_rgb_to_yuv(cuspal[i]);
        if (tridx & (1 << (12-4*i)))
          cuspal[i] |= 1 << 31;
      }
      this->custom = 1;
    }
  } while ((ptr=(uint8_t*)strchr((const char*)ptr,'\n')) &&* ++ptr);

  av_free(buffer);
}

void* spudec_new_scaled(unsigned int *palette, unsigned int frame_width, unsigned int frame_height, uint8_t *extradata, int extradata_len)
{
  GxSpuDec* this = av_calloc(1, sizeof(GxSpuDec));
  if (this){
    this->orig_frame_height = frame_height;
    // set up palette:
    if (palette)
      memcpy(this->global_palette, palette, sizeof(this->global_palette));
    else
      this->auto_palette = 1;
    this->has_palette = 0;
	if (extradata) {
		this->orig_frame_width = 0;
		this->orig_frame_height = 0;
		spudec_parse_extradata(this, extradata, extradata_len);
	}
    /* XXX Although the video frame is some size, the SPU frame is
       always maximum size i.e. 720 wide and 576 or 480 high*/
	if ((this->orig_frame_width == 0) || (this->orig_frame_height == 0)) {
		this->orig_frame_width = 720;
		if (this->orig_frame_height == 480 || this->orig_frame_height == 240)
			this->orig_frame_height = 480;
		else
			this->orig_frame_height = 576;
	}
  }
  else
    gx_msg(MSGT_SPUDEC,MSGL_FATAL, "FATAL: spudec_init: av_calloc");
  return this;
}

void* spudec_new(unsigned int *palette)
{
    return spudec_new_scaled(palette, 0, 0, NULL, 0);
}

void spudec_free(void* this)
{
  GxSpuDec* spu = (GxSpuDec*)this;
  if (spu) {
    while (spu->queue_head)
      spudec_free_packet(spudec_dequeue_packet(spu));
    if (spu->packet)
		av_free(spu->packet);
    if (spu->scaled_image)
		av_free(spu->scaled_image);
    if (spu->image)
		av_free(spu->image);
    av_free(spu);
  }
}

/*
void spudec_set_hw_spu(void* this, const vo_functions_t *hw_spu)
{
  GxSpuDec* spu = (GxSpuDec*)this;
  if (!spu)
    return;
  spu->hw_spu = hw_spu;
  hw_spu->control(VOCTRL_SET_SPU_PALETTE,spu->global_palette);
}*/

