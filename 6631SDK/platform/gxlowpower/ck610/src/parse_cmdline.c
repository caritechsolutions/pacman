
#include "gxcomm.h"

#define SramBaseAddr 0x100000
#if (GXCHIP_TYPE == GXCHIP_TYPE_GX3211 || GXCHIP_TYPE == GXCHIP_TYPE_GX6605S)
#define SramSize 0x4000
#else
#define SramSize 0x3000
#endif
#define FW_DATA_SIZE 0x80

/* Returns 0, or -errno.  arg is in kp->arg. */
typedef int (*param_get_fn)(char *val);

struct lowpower_param {
	const char *name;
	param_get_fn get;
};

struct gx_tag {
	unsigned int tag_header;
	char    cmdline[1];     /* this is the minimum size */
};

struct gx_fw_status *g_fw_status;
static struct gx_parse_cmdline *g_cmdline;
extern U32 s_cur_time_sec;

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
		struct lowpower_param *params,
		unsigned num_params)
{
	unsigned int i;

	/* Find parameter */
	for (i = 0; i < num_params; i++) {
		if (parameq(param, params[i].name)) {
			return params[i].get(val);
		}
	}
	return -1;
}


/* You can use " around spaces, but can't escape ". */
/* Hyphens and underscores equivalent in parameter names. */
static char *next_arg(char *args, char **param, char **val)
{
	unsigned int i, equals = 0;
	int in_quote = 0, quoted = 0;
	char *next;

	if (*args == '"') {
		args++;
		in_quote = 1;
		quoted = 1;
	}

	for (i = 0; args[i]; i++) {
		if (args[i] == ' ' && !in_quote)
			break;
		if (equals == 0) {
			if (args[i] == '=')
				equals = i;
		}
		if (args[i] == '"')
			in_quote = !in_quote;
	}

	*param = args;
	if (!equals)
		*val = NULL;
	else {
		args[equals] = '\0';
		*val = args + equals + 1;

		/* Don't include quotes in value. */
		if (**val == '"') {
			(*val)++;
			if (args[i-1] == '"')
				args[i-1] = '\0';
		}
		if (quoted && args[i-1] == '"')
			args[i-1] = '\0';
	}

	if (args[i]) {
		args[i] = '\0';
		next = args + i + 1;
	} else
		next = args + i;

	/* Chew up trailing spaces. */
	while (*next == ' ')
		next++;
	return next;
}

/* Args looks like "foo=bar,bar2 baz=fuz wiz". */
int parse_args(char *args,
		struct lowpower_param *params,
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

unsigned int htoi(const char *str)
{
	const char *cp;
	unsigned int data = 0, bdata = 0;

	for (cp = str, data = 0; *cp != 0; ++cp)
	{
		if (*cp >= '0' && *cp <= '9')
			bdata = *cp - '0';
		else if (*cp >= 'A' && *cp <= 'F')
			bdata = *cp - 'A' + 10;
		else if (*cp >= 'a' && *cp <= 'f')
			bdata = *cp - 'a' + 10;
		else
			break;
		data = (data << 4) | bdata;
	}

	return data;
}

unsigned int atoi(const char *str)
{
	const char *cp;
	unsigned int data = 0;

	if (str[0] == '0' && (str[1] == 'X' || str[1] == 'x'))
		return htoi(str + 2);

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			data = data * 10 + *cp - '0';
		else
			break;
	}

	return data;
}

int get_waketime(char *val)
{
	g_cmdline->waketime=atoi(val);
	return 0;
}

int get_gpionum(char *val)
{
	g_cmdline->gpionum=atoi(val);
	return 0;
}

int get_gpiolevel(char *val)
{
	g_cmdline->gpiolevel=atoi(val);
	return 0;
}

int get_keys(char *val)
{
	int i=0;
	g_cmdline->keybuf[i++]=atoi(val);
	for(;*val!=0;val++)
	{
		if(*val==',' && *(++val)!=0)
		{
			g_cmdline->keybuf[i++]=atoi(val);
		}
		if (i >= sizeof(g_cmdline->keybuf)/sizeof(unsigned int))
		{
			break;
		}
	}
	g_cmdline->keynum=i;
	return 0;
}

int get_powercut(char *val)
{
	int i=0;
	g_cmdline->powerio[i++]=atoi(val);
	for(;*val!=0;val++)
	{
		if(*val==',' && *(++val)!=0)
		{
			g_cmdline->powerio[i++]=atoi(val);
		}
		if (i >= sizeof(g_cmdline->powerio)/sizeof(unsigned int))
		{
			break;
		}
	}
	return 0;
}

int get_curtime(char *val)
{
	g_cmdline->curtime=atoi(val);
	return 0;
}

int get_timeshowflag(char *val)
{
	g_cmdline->timeshowflag=atoi(val);
	return 0;
}

int get_timezone(char *val)
{
	int integer_sign = 1;
	if (*val == '-') {
		integer_sign = -1;
		val++;
	}
	g_cmdline->timezone=atoi(val) * integer_sign;
	return 0;
}

int get_timesummer(char *val)
{
	g_cmdline->timesummer=atoi(val);
	return 0;
}

int get_panelio(char *val)
{
	int i=0;
	g_cmdline->panelio[i++]=atoi(val);
	for(;*val!=0;val++)
	{
		if(*val==',' && *(++val)!=0)
		{
			g_cmdline->panelio[i++]=atoi(val);
		}
		if (i > 1)
		{
			break;
		}
	}
	return 0;
}

int get_lowpower_clock_speed(char *val)
{
	g_cmdline->lowpower_clock_speed=atoi(val);
	return 0;
}

int get_panel_dot_ctrl(char *val)
{
	g_cmdline->panel_dot_ctrl=atoi(val);
	return 0;
}

int get_panel_brightness(char *val)
{
	g_cmdline->panel_brightness=atoi(val);
	return 0;
}

int get_reboot_flag(char *val)
{
	g_cmdline->reboot_flag=atoi(val);
	return 0;
}

static struct lowpower_param g_lowpower_param[]={
	{
		.name="waketime",
		.get = get_waketime,
	},
	{
		.name="gpiomask",
		.get = get_gpionum,
	},
	{
		.name="gpiodata",
		.get = get_gpiolevel,
	},
	{
		.name="keys",
		.get = get_keys,
	},
	{
		.name="powercut",
		.get = get_powercut,
	},
	{
		.name="curtime",
		.get = get_curtime,
	},
	{
		.name="timeshowflag",
		.get = get_timeshowflag,
	},
	{
		.name="timezone",
		.get = get_timezone,
	},
	{
		.name="timesummer",
		.get = get_timesummer,
	},
	{
		.name="panelio",
		.get = get_panelio,
	},
	{
		.name="lowpower_clock_speed",
		.get = get_lowpower_clock_speed,
	},
	{
		.name="panel_dot_ctrl",
		.get = get_panel_dot_ctrl,
	},
	{
		.name="panel_brightness",
		.get = get_panel_brightness,
	},
	{
		.name="rebootflag",
		.get = get_reboot_flag,
	},
};

void fw_status_init(void)
{
	g_fw_status = (struct gx_fw_status *)(SramBaseAddr + SramSize - FW_DATA_SIZE);
}

void time_write_back(void)
{
	g_fw_status->utctime = s_cur_time_sec - (g_cmdline->timezone + g_cmdline->timesummer * 3600);
	g_fw_status->curtime = s_cur_time_sec;
}

int parse_cmdline(struct gx_parse_cmdline *cmdline)
{
	struct gx_tag *tag=(struct gx_tag *)SramBaseAddr;
	g_cmdline=cmdline;
	if (tag->tag_header != TAG_MAGIC)
	{
		return 0;
	}
	return parse_args(tag->cmdline,g_lowpower_param,sizeof(g_lowpower_param)/sizeof(struct lowpower_param));
}

