/*
 * Will get cpu info and mem info from stb running Linux
 *
 * Hangzhou Nationalchip Science&Technology Co.Ltd.
 * Copyright (C) 2013, by LiuJianhua
 *
 */

#include <gxos/gxcore_os.h>

#define SYSINFO_THREAD_STACK_SIZE		(100*1024)
#define SYSINFO_THREAD_NAME			    "sysinfo"

typedef struct _MemOccupy
{
	char name[20];
	char name2[20];
	unsigned long total;
	unsigned long free;
	unsigned long cache;
	unsigned long buffer;
}MemOccupy;


typedef struct _CpuOccupy
{
	char name[20];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
}CpuOccupy;


void mem_occupy_get(MemOccupy *mem)
{
    FILE *fd;
    char buff[256];
    MemOccupy *m;
    m=mem;

    fd = fopen ("/proc/meminfo", "r");

    fgets (buff, sizeof(buff), fd);
    sscanf (buff, "%s %lu %s", m->name, &m->total, m->name2);

    fgets (buff, sizeof(buff), fd);
    sscanf (buff, "%s %lu %s", m->name2, &m->free, m->name2);

    fgets (buff, sizeof(buff), fd);
    sscanf (buff, "%s %lu %s", m->name2, &m->cache, m->name2);

    fgets (buff, sizeof(buff), fd);
    sscanf (buff, "%s %lu %s", m->name2, &m->buffer, m->name2);

    fclose(fd);
}

float cpu_occupy_cal(CpuOccupy *old, CpuOccupy *new)
{
    unsigned long od, nd;
    unsigned long total, idle;
    float cpu_use = 0;

    od = (unsigned long) (old->user + old->nice + old->system +old->idle);
    nd = (unsigned long) (new->user + new->nice + new->system +new->idle);

    total = (unsigned long) (nd - od);
    idle  = (unsigned long) (new->idle - old->idle);
//	app_log_debug("old: %lu,  new: %lu, total: %lu, idle: %lu \n", od, nd, total, idle);
    if((nd-od) != 0)
	{
    	cpu_use = ((float)total - idle) * 100/total;
	}
    else
		cpu_use = 0;

	return cpu_use;
}

void cpu_occupy_get(CpuOccupy *cpust)
{
    FILE *fd;
    char buff[256];
    CpuOccupy *cpu_occupy;


    fd = fopen ("/proc/stat", "r");
    fgets (buff, sizeof(buff), fd);

    cpu_occupy=cpust;
    sscanf (buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle);

    fclose(fd);
}


static handle_t sysinfo_thread_id = 0;

static void sysinfo_thread(void *arg)
{
    CpuOccupy cpu_stat1;
    CpuOccupy cpu_stat2;
    MemOccupy mem_stat;
	float cpu_ratio;
    unsigned long mem_usable;

    cpu_occupy_get((CpuOccupy *)&cpu_stat1);

	while(1)
	{
	    cpu_occupy_get((CpuOccupy *)&cpu_stat2);
	    cpu_ratio = cpu_occupy_cal((CpuOccupy *)&cpu_stat1, (CpuOccupy *)&cpu_stat2);

	    mem_occupy_get((MemOccupy *)&mem_stat);
		mem_usable = mem_stat.free + mem_stat.cache + mem_stat.buffer;

		app_log_min("[Sysinfo] CPU load: %.2f%% , Mem (usable/total): %luKB  %luKB\n", cpu_ratio, mem_usable, mem_stat.total);

	    cpu_occupy_get((CpuOccupy *)&cpu_stat1);

		GxCore_ThreadDelay(10000);

	}
}

void sysinfo_thread_create(void)
{
	app_log_min("[Sysinfo] thread was created.\n");
	if( 0 == sysinfo_thread_id )
	{
		GxCore_ThreadCreate(SYSINFO_THREAD_NAME,&sysinfo_thread_id,sysinfo_thread,NULL,SYSINFO_THREAD_STACK_SIZE,GXOS_DEFAULT_PRIORITY);
	}
}

void sysinfo_thread_destroy(void)
{
	if( sysinfo_thread_id > 0 )
	{
		GxCore_ThreadJoin(sysinfo_thread_id);
		sysinfo_thread_id = 0;
	}
}

