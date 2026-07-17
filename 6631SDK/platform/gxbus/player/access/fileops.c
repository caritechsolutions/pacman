#include "gx_fileops.h"

typedef struct {
	const char *name;
	GxFileOps  *ops;
} GxFileOpsClass;

extern GxFileOps gx_fileops_volume ;
extern GxFileOps gx_fileops_posix;
#define MAX_FILE_OPS_CLASS (2)
static GxFileOpsClass fileops[MAX_FILE_OPS_CLASS] = {
	{.name = ".dvr", .ops = &gx_fileops_volume},
	{.name = ".pvr", .ops = NULL},
};

GxFileOps* GxFileGetOps(char* filename)
{
	unsigned int i = 0;
	GxFileOps* ops = NULL;

	if(filename) {
		for (i = 0; i < MAX_FILE_OPS_CLASS; i++) {
			if (NULL == fileops[i].name)
				break;
			if (strstr(fileops[i].name, ".pvr")) {
				if (strstr(filename, ".pvr") ||
						strstr(filename, ".evt") ||
						strstr(filename, ".lst") ||
						strstr(filename, ".seg")) {
					ops = fileops[i].ops;
					break;
				}
			} else {
				if (strstr(filename, fileops[i].name)) {
					ops = fileops[i].ops;
					break;
				}
			}
		}
		if (ops == NULL)
			ops = &gx_fileops_posix;
	}

	return ops;
}

void GxFileRegisterPVR(void)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_FILE_OPS_CLASS; i++) {
		if (strstr(".pvr", fileops[i].name)) {
			extern GxFileOps gx_fileops_pvr;
			fileops[i].ops = &gx_fileops_pvr;
			break;
		}
	}

	return;
}
