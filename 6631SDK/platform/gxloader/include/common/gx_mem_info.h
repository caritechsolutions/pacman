#ifndef __GX_MEM_INFO_H__
#define __GX_MEM_INFO_H__

#define GXMEM_FLAG_DEMUX_ES                 (1 << 0)
#define GXMEM_FLAG_DEMUX_TSW                (1 << 1)
#define GXMEM_FLAG_DEMUX_TSR                (1 << 2)
#define GXMEM_FLAG_GP_ES                    (1 << 3)
#define GXMEM_FLAG_AUDIO_FIRMWARE           (1 << 4)
#define GXMEM_FLAG_AUDIO_FRAME              (1 << 5)
#define GXMEM_FLAG_VIDEO_FIRMWARE           (1 << 6)
#define GXMEM_FLAG_VIDEO_FRAME              (1 << 7)
#define GXMEM_FLAG_VPU_CAP                  (1 << 8)
#define GXMEM_FLAG_SCPU                     (1 << 9)
#define GXMEM_FLAG_SOFT                     (1 << 15) // 表示内容保护软件模式

/*
 * gx_mem_info - memory info
 *
 * @start: the start addr of memory
 * @size : the size of memory
 *
 * */
struct gx_mem_info {
        unsigned int start;
        unsigned int size;
};

/*
 * gx_mem_info_get()
 *
 *  DESCRIPTION
 *      Get the memory info in cmdline
 *
 *  PARAMETER
 *      @name
 *           The name of the memory in cmdline, such as "mem"/"videomem"
 *      @info
 *           the struct data to store memory info
 *
 *  RETURN VALUE
 *      On sucess, zero is returned and memory info is stroaged in info struct
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 *  EXAMPLE
 *      struct gx_mem_info video_info;
 *      int ret = gx_mem_info_get("videomem", &video_info);
 *      if (ret) {
 *              printf("start = 0x%x\n", videv_info.start);
 *              printf("size  = 0x%x\n", videv_info.size);
 *      }
 *
 * */
int gx_mem_info_get(char *name, struct gx_mem_info *info);


/*
 * gx_mem_protect_type_get()
 *
 *  DESCRIPTION
 *      Get the memory protect type in cmdline
 *
 *  PARAMETER
 *      @type
 *           obtained memory protect type information
 *
 *  RETURN VALUE
 *      On sucess, zero is returned and memory info is stroaged in info struct
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int gx_mem_protect_type_get(int *type);
int gx_mem_secure_align(void);
int parse_tag_cmdline(void);

int gx_mem_get_int32_value(char *name, int *value);

#endif
