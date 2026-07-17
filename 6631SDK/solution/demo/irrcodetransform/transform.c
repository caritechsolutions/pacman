#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

/*
 * 此遥控器与国芯键值转换工具只针对NEC协议
 * 使用需要自己编译可执行文件，使用方法看help
 */

static unsigned char _bit_order_reverse(unsigned char code)
{
    int i = 0;
    unsigned char ret = 0, tmp = 0, tmp2 = 0;
    int buf[4] = {7, 5, 3, 1};

    tmp = code & 0x0f;
    tmp2 = code & 0xf0;

    for(i = 0; i < 4; i++)
    {
        ret |= (tmp << (2 * i + 1)) & (0x10 << i);
        ret |= (tmp2 >> (7 - 2 * i)) & (0x1 << i);
    }
    return ret;
}

static unsigned int _transform_to_factory_code(unsigned int code)
{
	unsigned char syscode = 0;
	unsigned char syscode_reverse = 0;
	unsigned char usrcode = 0;
	unsigned char usrcode_reverse = 0;

    syscode = ~((code >> 24) & 0xff);
    syscode_reverse = ~((code >> 16) & 0xff);
    usrcode = ~((code >> 8) & 0xff);
    usrcode_reverse = ~((code)  & 0xff);

    return _bit_order_reverse(syscode) << 16 | _bit_order_reverse(syscode_reverse) << 8 | _bit_order_reverse(usrcode);

}

static unsigned int _transform_to_gx_code(unsigned int code)
{
	unsigned char syscode = 0;
	unsigned char syscode_reverse = 0;
	unsigned char usrcode = 0;
	unsigned char usrcode_reverse = 0;

    syscode = ~_bit_order_reverse((code >> 16) & 0xff);
    syscode_reverse = ~_bit_order_reverse((code >> 8) & 0xff);
    usrcode = ~_bit_order_reverse(code & 0xff);
    usrcode_reverse = ~((usrcode) & 0xff);

    if(syscode + syscode_reverse != 0xff)
        return 0;

    return syscode << 24 | syscode_reverse << 16 | usrcode << 8 | usrcode_reverse;
}

static const char short_options[] = "f:g:h";
static const struct option long_options[] =
{
	{ "factory",              	required_argument, 	NULL, 'f' },
	{ "guotech",              	required_argument, 	NULL, 'g' },
	{ 0, 0, 0, 0 }
};

static int usage(void)
{
	printf(" Usage: transform [-g] [-f] [-h]\n");
	printf(" -f, --factory           | transform factory irr code to gx code, just support hex\n");
	printf(" eg: transform -f 0x22dd41\n");
	printf(" -g, --guotech           | transform gx irr code to factory  code, just support hex\n");
	printf(" eg: transform -g 0xbb4457a8\n");
	printf(" -h, --help              | print help message\n");
	printf("\n");

	exit(0);
}

int main(int argc, char **argv)
{
    char *endptr = NULL, *hexStr = NULL; // 用于存储转换结束的位置
    unsigned int ret = 0;
    long int code = 0;
    int c = 0;

    if(1 == argc)
    {
        usage();
        return -1;
    }

	while((c = getopt_long(argc, argv, short_options, long_options, NULL)) != EOF)
    {
		if(c == '?')
        {
			usage();
			return -1;
		}
        switch (c)
        {
            case 'f':
                code = strtol(optarg, &endptr, 16); // 基数为16，表示16进制
                if(*endptr != '\0')
                {
                    printf("Conversion error: %s\n", optarg);
                    return -1;
                }
                ret = _transform_to_gx_code(code);
                break;
            case 'g':
                code = strtol(optarg, &endptr, 16); // 基数为16，表示16进制
                if(*endptr != '\0')
                {
                    printf("Conversion error: %s\n", optarg);
                    return -1;
                }
                ret = _transform_to_factory_code(code);
                break;
            case 'h':
                usage();
                return 0;
            default:
                break;
        }
    }

    if(0 == ret)
    {
        printf("irr code verify error\n");
        return -1;
    }
    printf("\033[32moriginal: 0x%lx\033[0m\n", code);
    printf("\033[32mtrans: 0x%x\033[0m\n", ret);
    return 0;
}
