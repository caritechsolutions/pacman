#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "common/gxlog.h"

enum {
	REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
};

struct _getopt_data {
	int optind;
	int opterr;
	char *optarg;
	unsigned int optopt;
	unsigned int __initialized;
	unsigned int __ordering;
	unsigned int __posixly_correct;
	char *__nextchar;
	int __first_nonopt;
	int __last_nonopt;
};

char *optarg;
int optind = 1;
int opterr = 1;
int optopt = '?';
static struct _getopt_data getopt_data;

# define SWAP_FLAGS(ch1, ch2)

static void exchange (char **argv, struct _getopt_data *d)
{
	int bottom = d->__first_nonopt;
	int middle = d->__last_nonopt;
	int top = d->optind;
	char *tem;

	while (top > middle && middle > bottom) {
		if (top - middle > middle - bottom) {
			int len = middle - bottom;
			register int i;

			for (i = 0; i < len; i++) {
				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
				SWAP_FLAGS (bottom + i, top - (middle - bottom) + i);
			}
			top -= len;
		} else {
			int len = top - middle;
			register int i;

			for (i = 0; i < len; i++) {
				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
				SWAP_FLAGS (bottom + i, middle + i);
			}
			bottom += len;
		}
	}

	d->__first_nonopt += (d->optind - d->__last_nonopt);
	d->__last_nonopt = d->optind;
}


static const char *_getopt_initialize(int argc, char *const *argv,
		const char *optstring, struct _getopt_data *d)
{
	d->__first_nonopt = d->__last_nonopt = d->optind;

	d->__nextchar = NULL;

	d->__posixly_correct = !!getenv ("POSIXLY_CORRECT");

	if (optstring[0] == '-') {
		d->__ordering = RETURN_IN_ORDER;
		++optstring;
	} else if (optstring[0] == '+') {
		d->__ordering = REQUIRE_ORDER;
		++optstring;
	} else if (d->__posixly_correct)
		d->__ordering = REQUIRE_ORDER;
	else
		d->__ordering = PERMUTE;

	return optstring;
}

static int _getopt_internal_r (int argc, char *const *argv,
		const char *optstring,
		const struct Option *longopts,
		int *longind,
		int long_only,
		struct _getopt_data *d)
{
	int print_errors = d->opterr;
	if (optstring[0] == ':')
		print_errors = 0;

	if (argc < 1)
		return -1;

	d->optarg = NULL;

	if (d->optind == 0 || !d->__initialized) {
		if (d->optind == 0)
			d->optind = 1;	/* Don't scan ARGV[0], the program name.  */
		optstring = _getopt_initialize (argc, argv, optstring, d);
		d->__initialized = 1;
	}

# define NONOPTION_P (argv[d->optind][0] != '-' || argv[d->optind][1] == '\0')

	if (d->__nextchar == NULL || *d->__nextchar == '\0') {
		if (d->__last_nonopt > d->optind)
			d->__last_nonopt = d->optind;
		if (d->__first_nonopt > d->optind)
			d->__first_nonopt = d->optind;

		if (d->__ordering == PERMUTE) {
			if (d->__first_nonopt != d->__last_nonopt
					&& d->__last_nonopt != d->optind)
				exchange ((char **) argv, d);
			else if (d->__last_nonopt != d->optind)
				d->__first_nonopt = d->optind;

			while (d->optind < argc && NONOPTION_P)
				d->optind++;
			d->__last_nonopt = d->optind;
		}

		if (d->optind != argc && !strcmp (argv[d->optind], "--")) {
			d->optind++;

			if (d->__first_nonopt != d->__last_nonopt
					&& d->__last_nonopt != d->optind)
				exchange ((char **) argv, d);
			else if (d->__first_nonopt == d->__last_nonopt)
				d->__first_nonopt = d->optind;
			d->__last_nonopt = argc;

			d->optind = argc;
		}

		if (d->optind == argc) {
			if (d->__first_nonopt != d->__last_nonopt)
				d->optind = d->__first_nonopt;
			return -1;
		}

		if (NONOPTION_P) {
			if (d->__ordering == REQUIRE_ORDER)
				return -1;
			d->optarg = argv[d->optind++];
			return 1;
		}

		d->__nextchar = (argv[d->optind] + 1
				+ (longopts != NULL && argv[d->optind][1] == '-'));
	}

	if (longopts != NULL
			&& (argv[d->optind][1] == '-'
				|| (long_only && (argv[d->optind][2]
						|| !strchr (optstring, argv[d->optind][1])))))
	{
		char *nameend;
		const struct Option *p;
		const struct Option *pfound = NULL;
		int exact = 0;
		int ambig = 0;
		int indfound = -1;
		int option_index;

		for (nameend = d->__nextchar; *nameend && *nameend != '='; nameend++)
			/* Do nothing.  */ ;

		for (p = longopts, option_index = 0; p->name; p++, option_index++)
			if (!strncmp (p->name, d->__nextchar, nameend - d->__nextchar)) {
				if ((unsigned int) (nameend - d->__nextchar)
						== (unsigned int) strlen (p->name))
				{
					/* Exact match found.  */
					pfound = p;
					indfound = option_index;
					exact = 1;
					break;
				}
				else if (pfound == NULL)
				{
					/* First nonexact match found.  */
					pfound = p;
					indfound = option_index;
				}
				else if (long_only
						|| pfound->has_arg != p->has_arg
						|| pfound->flag != p->flag
						|| pfound->val != p->val)
					/* Second or later nonexact match found.  */
					ambig = 1;
			}

		if (ambig && !exact) {
			if (print_errors) {
				gxloge ( "%s: option `%s' is ambiguous\n",
						argv[0], argv[d->optind]);
			}
			d->__nextchar += strlen (d->__nextchar);
			d->optind++;
			d->optopt = 0;
			return '?';
		}

		if (pfound != NULL) {
			option_index = indfound;
			d->optind++;
			if (*nameend) {
				/* Don't test has_arg with >, because some C compilers don't
				   allow it to be used on enums.  */
				if (pfound->has_arg)
					d->optarg = nameend + 1;
				else {
					if (print_errors) {
						if (argv[d->optind - 1][1] == '-') {
							gxloge ( "\
									%s: option `--%s' doesn't allow an argument\n",
									argv[0], pfound->name);
						} else {
							gxloge ( "\
									%s: option `%c%s' doesn't allow an argument\n",
									argv[0], argv[d->optind - 1][0],
									pfound->name);
						}
					}

					d->__nextchar += strlen (d->__nextchar);

					d->optopt = pfound->val;
					return '?';
				}
			} else if (pfound->has_arg == 1) {
				if (d->optind < argc)
					d->optarg = argv[d->optind++];
				else {
					if (print_errors) {
						gxloge (
								"%s: option `%s' requires an argument\n",
								argv[0], argv[d->optind - 1]);
					}
					d->__nextchar += strlen (d->__nextchar);
					d->optopt = pfound->val;
					return optstring[0] == ':' ? ':' : '?';
				}
			}
			d->__nextchar += strlen (d->__nextchar);
			if (longind != NULL)
				*longind = option_index;
			if (pfound->flag) {
				*(pfound->flag) = pfound->val;
				return 0;
			}
			return pfound->val;
		}

		if (!long_only || argv[d->optind][1] == '-'
				|| strchr (optstring, *d->__nextchar) == NULL) {
			if (print_errors) {
				if (argv[d->optind][1] == '-') {
					/* --option */
					gxloge ( "%s: unrecognized option `--%s'\n",
							argv[0], d->__nextchar);
				} else {
					/* +option or -option */
					gxloge ( "%s: unrecognized option `%c%s'\n",
							argv[0], argv[d->optind][0], d->__nextchar);
				}

			}
			d->__nextchar = (char *) "";
			d->optind++;
			d->optopt = 0;
			return '?';
		}
	}

	{
		char c = *d->__nextchar++;
		char *temp = strchr (optstring, c);

		/* Increment `optind' when we start to process its last character.  */
		if (*d->__nextchar == '\0')
			++d->optind;

		if (temp == NULL || c == ':')
		{
			if (print_errors)
			{
				if (d->__posixly_correct)
				{
					/* 1003.2 specifies the format of this message.  */
					gxloge ( "%s: illegal option -- %c\n", argv[0], c);
				}
				else
				{
					gxloge ( "%s: invalid option -- %c\n", argv[0], c);
				}

			}
			d->optopt = c;
			return '?';
		}
		if (temp[1] == ':')
		{
			if (temp[2] == ':')
			{
				/* This is an option that accepts an argument optionally.  */
				if (*d->__nextchar != '\0')
				{
					d->optarg = d->__nextchar;
					d->optind++;
				}
				else
					d->optarg = NULL;
				d->__nextchar = NULL;
			}
			else
			{
				/* This is an option that requires an argument.  */
				if (*d->__nextchar != '\0')
				{
					d->optarg = d->__nextchar;
					/* If we end this ARGV-element by taking the rest as an arg,
					   we must advance to the next element now.  */
					d->optind++;
				}
				else if (d->optind == argc)
				{
					if (print_errors)
					{
						/* 1003.2 specifies the format of this message.  */
						gxloge (
								"%s: option requires an argument -- %c\n",
								argv[0], c);
					}
					d->optopt = c;
					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				}
				else
					/* We already incremented `optind' once;
					   increment it again when taking next ARGV-elt as argument.  */
					d->optarg = argv[d->optind++];
				d->__nextchar = NULL;
			}
		}
		return c;
	}
}

static int
_getopt_internal (int argc, char *const *argv, const char *optstring,
		const struct Option *longopts, int *longind, int long_only)
{
	int result;

	getopt_data.optind = optind;
	getopt_data.opterr = opterr;

	result = _getopt_internal_r (argc, argv, optstring, longopts,
			longind, long_only, &getopt_data);

	optind = getopt_data.optind;
	optarg = getopt_data.optarg;
	optopt = getopt_data.optopt;

	return result;
}

int GetOpt (int argc, char *const *argv, const char *optstring)
{
	return _getopt_internal (argc, argv, optstring,
			(const struct Option *) 0,
			(int *) 0,
			0);
}

int GetOpt_Long (int argc, char *const *argv, const char *options,
		const struct Option *long_options, int *opt_index)
{
	return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}


int GetOpt_Long_Only (int argc, char *const *argv, const char *options,
		const struct Option *long_options, int *opt_index)
{
	return _getopt_internal (argc, argv, options, long_options, opt_index, 1);
}
