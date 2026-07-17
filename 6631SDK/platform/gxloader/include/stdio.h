#ifndef __STDIO_H__
#define __STDIO_H__
#include <sys/types.h>

// stdarg
typedef void *va_list;

struct heap_info {
	unsigned long total_size;
	unsigned long free;
	unsigned long used;
	unsigned long real_used; /*used + overhead*/
	unsigned long max_used;
	unsigned long max_malloc_size; /* Indicates that you can apply for the largest space at a time using malloc */
};

// addr must be multiple of sizeof(TYPE) : 4 bytes(int), 8 bytes(long long)
#define __va_addr_algin(AP, TYPE) \
	(((int)AP & (sizeof(TYPE)-1)) ? (((int)(AP) + sizeof(TYPE)-1) & ~(sizeof(int)-1)) : (int)(AP))

// size of memory storing the data with specified type
// data size must be multiple of sizeof(int) : 4 bytes
#define __va_rounded_size(TYPE)  \
	( (sizeof(TYPE)+sizeof(int)-1) & ~(sizeof(int)-1) )

// stdarg standard macros
#define va_start(AP, LASTARG)   (AP = (va_list) __builtin_next_arg(LASTARG))
#define va_end(AP)
#define va_arg(AP, TYPE)        (AP = (va_list) __va_addr_algin(AP, TYPE), AP = (va_list) ((char *) (AP) + __va_rounded_size (TYPE)), *((TYPE *) (void *) ((char *) (AP) - __va_rounded_size(TYPE))))

#ifndef EOF
#define EOF (-1)
#endif

struct _iobuf {
	char *_ptr;
	int _cnt;
	char *_base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char *_tmpfname;
};

typedef struct _iobuf FILE;

#define SEEK_SET (0)
#define SEEK_CUR (1)
#define SEEK_END (2)

/* This is a macro so it can be used as initializer */
#define __create_file(__fd) ((FILE *)(size_t)((__fd)))

#define stdin  __create_file(0)
#define stdout __create_file(1)
#define stderr __create_file(2)

/* function prototypes */

//int sprintf(char *buf, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
int sprintf(char * buf, const char *fmt, ...)
		__attribute__ ((format (__printf__, 2, 3)));
int vsprintf(char *buf, const char *fmt, va_list args);

int snprintf(char * buf, unsigned int length, const char *fmt, ...)
		__attribute__ ((format (__printf__, 3, 4)));
int vsnprintf(char *buf, unsigned int length, const char *fmt, va_list args);

int fprintf(FILE * file, const char *fmt, ...)
	    __attribute__ ((format (__printf__, 2, 3)));
int vfprintf(FILE * file, const char *fmt, va_list args);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
int putc(int ch);
int putchar(int ch);
int getchar(void);
int getc(void);
int fputc(int c, FILE * f);
char *readline(const char *prompt);
void perror(const char *msg);

void puts(const char *t);
void putnstr(const char *t, int size);
char *gets(char *str, int maxlen, int flag);

int heap_info(struct heap_info *info);
void heap_init(void *startptr, unsigned int size);
void *malloc(unsigned int size);
void free(void *ptr);
void *realloc(void *p, unsigned int size);
void *page_malloc(unsigned int size);
void page_free(void *p);

int tmp_heap_info(struct heap_info *info);
void tmp_heap_init(void *startptr, unsigned int size);
void *tmp_malloc(unsigned int size);
void tmp_free(void *ptr);
void *tmp_realloc(void *ptr, unsigned int size);

FILE    *fopen(const char *path, const char *mode);
size_t   fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t   fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int      fseek(FILE *stream, long offset, int whence);
long     ftell(FILE *stream);
int      fclose(FILE *stream);

size_t _fwrite(const void *buf, size_t count, FILE * f);

static __inline__ int fileno(FILE * __f)
{
	/* This should really be intptr_t, but size_t should be the same size */
	return (int)(size_t) __f;
}

#endif

