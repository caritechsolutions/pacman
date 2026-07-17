	#align to 4096 byte
	.align 12
	.globl	embed_kernel_img
embed_kernel_img:
	.incbin	CONFIG_EMBED_KERNEL_IMG_FILE
	.globl	embed_kernel_img_end
embed_kernel_img_end:
