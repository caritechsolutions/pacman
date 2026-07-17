#include "types.h"
#define TAG_MAGIC    0x54410003

struct item {
	const char *name;
	int (*get)(char *val);
};

static char dash2underscore(char c)
{
	if (c == '-')
		return '_';
	return c;
}

static int parameq(const char *input, const char *paramname)
{
	unsigned int i;
	for (i = 0; dash2underscore(input[i]) == paramname[i]; i++)
		if (input[i] == '\0')
			return 1;
	return 0;
}

static int parse_one(char *param,
		char *val,
		struct item *params,
		unsigned num_params)
{
	unsigned int i;

	/* Find parameter */
	for (i = 0; i < num_params; i++) {
		if (parameq(param, params[i].name) && (val != NULL)) {
			return params[i].get(val);
		}
	}
	return -1;
}

/* Args looks like "foo=bar,bar2 baz=fuz wiz". */
int parse_args(char *args,
		struct item *params,
		unsigned num)
{
	char *param, *val;

	/* Chew leading spaces */
	while (*args == ' ')
		args++;

	while (*args) {
		int ret;

		args = next_arg(args, &param, &val);
		ret = parse_one(param, val, params, num);
		switch (ret) {
			case -1:
				break;
			case 0:
				break;
			default:
				return ret;
		}
	}
	/* All parsed OK. */
	return 0;
}

static unsigned long memparse(char *ptr, char **retptr)
{
	char *endptr;   /* local pointer to end of parsed string */

	unsigned long ret=0;
	ret = simple_strtoull(ptr, &endptr, 0);

	switch (*endptr) {
		case 'G':
		case 'g':
			ret <<= 10;
		case 'M':
		case 'm':
			ret <<= 10;
		case 'K':
		case 'k':
			ret <<= 10;
			endptr++;
		default:
			break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

static unsigned int vm;
static unsigned int fm;
static unsigned int cm;
static int videomem_fun(char *val)
{
	vm = memparse((char *)val, (void *)&val);
	return 0;
}

static int mem_fun(char *val)
{
	cm = memparse((char *)val, (void *)&val);
	return 0;
}

static int fbmem_fun(char *val)
{
	fm = memparse((char *)val, (void *)&val);
	return 0;
}

static struct item ota_men[]={
	{
		.name = "videomem",
		.get  =  videomem_fun,
	},
	{
		.name = "mem",
		.get  =  mem_fun,
	},
	{
		.name = "fbmem",
		.get  =  fbmem_fun,
	},
};


int parse_cmdline(char *cmdline)
{
	unsigned int total_size = 0;
	if (cmdline[4] != 0x3 || cmdline[5] != 0x0 ||
		cmdline[6] != 0x41 || cmdline[7] != 0x54)
		return 0;

	parse_args((cmdline+8),ota_men,sizeof(ota_men)/sizeof(struct item));

	total_size = cm + vm + fm ;
	return total_size;
}

