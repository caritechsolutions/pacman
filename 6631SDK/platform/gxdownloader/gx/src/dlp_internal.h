#ifndef __DLP_INTERNAL_H__
#define __DLP_INTERNAL_H__

struct demod {
	GxDLP_Demod_Type type;
	char *name;
#ifdef LINUX_OS
	unsigned int attach;
#else
	demod_attach_func attach;
#endif
	unsigned char i2c_chipaddr;
};

struct tuner {
	GxDLP_Tuner_Type type;
	char *name;
#ifdef LINUX_OS
	unsigned int attach;
#else
	tuner_attach_func attach;
#endif
	unsigned char i2c_chipaddr;
};

struct demod *demod_type_to_demod(GxDLP_Demod_Type type);
struct demod *demod_name_to_demod(char *name);
struct tuner *tuner_type_to_tuner(GxDLP_Tuner_Type type);
struct tuner *tuner_name_to_tuner(char *name);

#endif
