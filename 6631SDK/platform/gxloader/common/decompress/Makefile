all:
	gcc -g decompress.c main.c \
		decompress_inflate.c \
		decompress_unlzo.c \
		decompress_unlzma.c \
		-o lzo -I../../include -DCONFIG_DECOMPRESS_LZO -DCONFIG_DECOMPRESS_GZIP -DCONFIG_DECOMPRESS_LZMA
clean:
	rm -f *.o lzo

test:
	lzop -9 a -c >|a.lzo
	gzip -9 a -c >|a.gz
	lzma -z a -c >|a.lzma
