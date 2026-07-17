#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define BUF_SIZE  (4096)
static unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
					? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

int help()
{
	printf("./read_file <inputfile> <outputfile> <start_addr> <file_end> [length]\n");
	printf("file_end indicates whether the end of the file is read, 1 is end!\n");
	return 0;
}

int main(int argc, char* argv[])
{
	int ret = 0;
	int src_fd = 0;
	int dst_fd = 0;
	int src_length = 0;
	int start_addr = 0;
	int length = 0, read_size = 0;
	int file_end = 0;
	char *buffer = NULL;


	if (argc < 5) {
		help();
		return -1;
	}

	src_fd = open(argv[1], O_RDWR, 0666);
	if (src_fd < 0) {
		printf("open %s error!\n", argv[1]);
		ret = -1;
		goto exit;
	}

	dst_fd = open(argv[2], O_RDWR | O_CREAT, 0666);
	if (dst_fd < 0) {
		printf("open %s error!\n", argv[2]);
		ret = -1;
		goto exit;
	}

	start_addr = simple_strtoul((const char *)argv[3], NULL, 10);
	file_end = simple_strtoul((const char *)argv[4], NULL, 10);

	src_length = lseek(src_fd, 0, SEEK_END);
	if (file_end)
		length = src_length - start_addr;
	else
		length = simple_strtoul((const char *)argv[5], NULL, 10);

	lseek(src_fd, start_addr, SEEK_SET);

	buffer = malloc(BUF_SIZE);
	if (!buffer) {
		printf("malloc error!\n");
		ret = -1;
		goto exit;
	}

	while(length > 0) {
		if (length > BUF_SIZE)
			read_size = BUF_SIZE;
		else
			read_size = length;

		read(src_fd, buffer, read_size);
		write(dst_fd, buffer, read_size);
		length -= BUF_SIZE;
	}

exit:
	if (src_fd > 0) {
		close(src_fd);
		src_fd = 0;
	}
	if (dst_fd > 0) {
		close(dst_fd);
		dst_fd = 0;
	}
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}

	return ret;
}
