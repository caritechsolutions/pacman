#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <memory.h>


typedef int (*FunCallback)(int param_a, int param_b);


#define MEMCODE_SIZE   (4096)

#define FUNC_CODE_FILE	"funcode.bin"


static char static_mem_code[MEMCODE_SIZE];

static int inner_add(int a, int b);
static int inner_sub(int a, int b);
static unsigned long get_file_size(const char *filename);
int test_dynamic(void);


int main_dynamic(void)
{
	int i = 0;

	test_mem_static();

	for(i = 0; i < 10240; i++)
	{
		app_log_min("\nLoop: %d\n", i);
		test_mem_dynamic();
		//test_mem_static();
	}
}

int test_mem_static(void)
{

	FILE *pf = NULL;
	int sum = 0;
	int read_size = 0;
	unsigned long file_size = 0;
	int i = 0;

	FunCallback  mem_func = NULL;

	do
	{

		file_size = get_file_size(FUNC_CODE_FILE);
		if(file_size == 0)
		{
			fprintf(stderr, "file len = 0\n");
			break;
		}
		//file_size += 4;

		memset(static_mem_code, 0, MEMCODE_SIZE);

		pf=fopen(FUNC_CODE_FILE,"r");
		if(pf == NULL)
		{
			fprintf(stderr,"cannot open file.\n");
			break;
		}

		read_size = fread(static_mem_code, 1, MEMCODE_SIZE, pf);
		app_log_min("read size: %d\n", read_size);

		//mem_func=(FunCallback)inner_add;
	  	mem_func=(FunCallback)static_mem_code;
		sum=(mem_func)(14,13);
		app_log_min("Sum = %d\n", sum);

	}while(0);

	if( pf != NULL)
	{
		fclose(pf);
		pf = NULL;
	}

	return 0;
}

int test_mem_dynamic(void)
{
	char *alloc_mem_code = NULL;

	FILE *pf = NULL;
	int sum = 0;
	int read_size = 0;
	unsigned long file_size = 0;
	int i = 0;

	FunCallback  mem_func = NULL;

	do
	{

		file_size = get_file_size(FUNC_CODE_FILE);
		if(file_size == 0)
		{
			fprintf(stderr, "file len = 0\n");
			break;
		}
		//file_size += 4;

		alloc_mem_code=(char *)malloc(file_size);
		if(alloc_mem_code == NULL)
		{
			fprintf(stderr,"cannot alloc mem.\n");
			break;
		}
		memset(alloc_mem_code, 0, file_size);

		pf=fopen(FUNC_CODE_FILE,"r");
		if(pf == NULL)
		{
			fprintf(stderr,"cannot open file.\n");
			break;
		}

		read_size = fread(alloc_mem_code, 1, file_size, pf);
		app_log_min("read size: %d\n", read_size);

		for(i = 0; i < read_size; i++)
		{
			if(alloc_mem_code[i] != static_mem_code[i])
			{
				fprintf(stderr, "compiled function err!\n");
			}
		}
		//mem_func=(FunCallback)inner_add;
	  	mem_func=(FunCallback)alloc_mem_code;
		sum=(mem_func)(14,13);
		app_log_min("Sum = %d\n", sum);


	}while(0);

	if( alloc_mem_code != NULL)
	{
		free(alloc_mem_code);
		memset(alloc_mem_code, 0, file_size);
		alloc_mem_code = NULL;
	}

	if( pf != NULL)
	{
		fclose(pf);
		pf = NULL;
	}

	return 0;
}

static  int inner_add(int a, int b)
{
	return (a + b);
}

static  int inner_sub(int a, int b)
{
	return (a - b);
}

static unsigned long get_file_size(const char *filename)
{

	struct stat buf;

	if(stat(filename, &buf)<0)
	{
		return 0;
	}

	return (unsigned long)buf.st_size;
}
