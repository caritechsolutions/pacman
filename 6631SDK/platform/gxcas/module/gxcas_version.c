#include "stdio.h"
#include "gxcas_dbg.h"

void GxCas_VersionInfo()
{
	printf("GxCas Branch:%s\nGxCas Version:%s\nGxCas Promulgator:%s\n", GXCAS_BRANCH, GXCAS_VERSION, GXCAS_GENERATOR);
}

