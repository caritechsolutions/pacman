#ifndef __GETOPT_H__
#define __GETOPT_H__

extern char *gOptArg;
extern int   gOptInd;
extern int   gOptErr;
extern int   gOptOpt;

struct Option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};


extern int GetOpt (int ___argc, char *const *___argv, const char *__shortopts);

extern int GetOpt_Long (int ___argc, char *const *___argv,
		const char *__shortopts,
		const struct Option *__longopts, int *__longind);

extern int GetOpt_Long_Only (int ___argc, char *const *___argv,
		const char *__shortopts,
		const struct Option *__longopts, int *__longind);

#endif /* getopt.h */
