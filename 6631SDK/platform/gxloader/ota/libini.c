#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/list.h>
#include "libini.h"

#define assert(a)
#define INI_LINE_LEN    256

typedef struct {
	struct list_head  entry;
	char*               param;
	char*               value;
}INI_PARAM;

typedef struct {
	struct list_head  entry;
	char*               section;
	struct list_head  param_list;
}INI_SECTION;

typedef struct {
	char*               fname;
	struct list_head  section_list;
	handle_t            lock_mutex;
}INI;

#define INI_LOCK(ini)
#define INI_UNLOCK(ini)

#define FILE_MODE "c"

static char* skip_head_blank(char* str)
{
	char* tmp;

	assert(str != NULL);

	for(tmp = str; *tmp==' '||*tmp=='\t'; tmp++) ;

	return tmp;
}

static char* skip_tail_blank(char* str)
{
	int     len;

	assert(str != NULL);

	for(len = strlen(str) - 1; (len > 0) && (str[len] == ' ' || str[len] == '\t'); len--) {
		;
	}

	str[++len] = '\0';

	return str;
}

static char* skip_comment(char* str)
{
	char*   tmp = str;

	assert(str != NULL);

	for(tmp = str; *tmp != '\0' && *tmp != ';'; tmp++) {
		;
	}

	*tmp = '\0';

	return str;
}

static INI_SECTION* ini_append_section(INI* pini, const char* section)
{
	INI_SECTION* sec;

	assert(pini != NULL);
	assert(section != NULL);

	sec = malloc(sizeof(INI_SECTION));
	if (sec == NULL) {
		return NULL;
	}
	sec->section = strdup(section);
	if (sec->section == NULL) {
		free(sec);
		return NULL;
	}

	INIT_LIST_HEAD(&sec->param_list);
	list_add_tail( &sec->entry, &pini->section_list);

	return sec;
}

static INI_PARAM* ini_append_param(INI_SECTION* section, const char* param, const char* value)
{
	INI_PARAM* pa;

	assert(section != NULL);
	assert(param != NULL);
	assert(value != NULL);

	pa = malloc(sizeof(INI_PARAM));
	if (pa == NULL) {
		return NULL;
	}
	pa->param = strdup(param);
	pa->value = strdup(value);

	if(pa->param == NULL || pa->value == NULL) {
		if (pa->param != NULL)
			free(pa->param);
		if (pa->value != NULL)
			free(pa->value);
		free(pa);
		return NULL;
	}
	list_add_tail(&pa->entry, &section->param_list);

	return pa;
}

INI_SECTION *ini_addline(handle_t ini, INI_SECTION* sec, char *line)
{
	INI*            pini = (INI*) ini;
	char*           p;
	char*           s;  
	char*           param = NULL;
	char*           value = NULL;

	p = skip_head_blank(line);
	if ( *p == '#' || *p == '\r' || *p == '\n') {
		return NULL;
	}

	if (*p == '[') {
		p = skip_head_blank(++p);
		for (s = p; *p != ']' && *p != '\r' && *p != '\n'&&*p !='\0'; p++) {
			;
		}
		*p = '\0';
		skip_tail_blank(s);
		sec = ini_append_section(pini, s);
	} else if (sec != NULL) {
		for (s = p; *p != '='&&*p != '\r' && *p != '\n'&&*p !='\0'; p++) {
			;
		}
		*p++ = '\0';

		param = s;
		p = skip_head_blank(p);
		for (s = p; *p!='\0'&&*p!='='&&*p!='\r'&&*p!='\n'; p++){
			;
		}
		*p = '\0';
		value = s;

		param = skip_comment(skip_tail_blank(param));
		value = skip_comment(skip_tail_blank(value));
		ini_append_param(sec, param , value);
	}

	return sec;
}

handle_t ini_loadmemory(const char *memory, int size)
{
	INI*    pini;
	int     DataLen = 0, cur_index = 0;
	char    line[INI_LINE_LEN];
	char    *pStr;
	INI_SECTION*    sec = NULL;

	pini = (INI *)malloc(sizeof(INI));
	assert(pini!=NULL);
	if (pini == NULL) {
		return EINVALID_HANDLE;
	}

	memset(pini, 0, sizeof(INI));
	//GxCore_MutexCreate(&pini->lock_mutex);
	INIT_LIST_HEAD(&pini->section_list);

	while (DataLen < size) {
		memset(line, 0, INI_LINE_LEN);
		cur_index = 0;

		while (cur_index < INI_LINE_LEN - 1) {
			if (DataLen >= size)
				break;
			if (memory[DataLen] == 0xFF) {
				DataLen++;
				continue;
			}
			if (memory[DataLen] == 0xA) {
				line[cur_index++] = memory[DataLen++];
				break;
			}
			line[cur_index++] = memory[DataLen++];
		}
		line[cur_index] = '\0';
		if (strlen(line) == 0) {
			continue;
		}

		pStr = strchr(line, '\n');

		if (pStr != NULL) *pStr = 0;

		sec = ini_addline((handle_t)pini, sec, line);
	}

	return (handle_t) pini;
}

size_t ini_savememory(handle_t ini, char* mem, size_t size)
{
	char line[INI_LINE_LEN];
	struct list_head*     pos;
	struct list_head*     n1;
	struct list_head*     n2;
	INI_SECTION*            sec;
	INI_PARAM*              pa;
	INI*                    pini = (INI*)ini;
	size_t                  w_size = 0;

	if (pini == NULL)
		return 0;

	memset(mem, 0, size);
	list_for_each_safe(pos, n1, &pini->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		snprintf(line, INI_LINE_LEN, "[%s]\n", sec->section);

		strcat(mem, line);
		w_size += strlen(line);
		if (w_size > size)
			return 0;

		list_for_each_safe(pos, n2, &sec->param_list) {
			pa = list_entry(pos, INI_PARAM, entry);
			snprintf(line, INI_LINE_LEN, "\t%s = %s\n", pa->param, pa->value);
			strcat(mem, line);

			w_size += strlen(line);
			if (w_size > size)
				return 0;
		}
	}

	return w_size;
}

void ini_close(handle_t ini)
{
	struct list_head*     pos;
	struct list_head*     n1;
	struct list_head*     n2;
	INI_SECTION*            sec;
	INI_PARAM*              pa;
	INI*                    pini = (INI*)ini;

	if (pini == NULL) {
		return;
	}

	INI_LOCK(pini);
	list_for_each_safe(pos, n1, &pini->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		list_for_each_safe(pos, n2, &sec->param_list) {
			pa = list_entry(pos, INI_PARAM, entry);
			free(pa->param);
			free(pa->value);
			free(pa);
		}
		free(sec->section);
		free(sec);
	}
	if (pini->fname != NULL) {
		free(pini->fname);
	}
	INI_UNLOCK(pini);
	//GxCore_MutexDelete(pini->lock_mutex);

	free(pini);
	return;
}


char *ini_get(handle_t ini, const char* section, const char* param)
{
	struct list_head*     pos;
	struct list_head*     n1;
	struct list_head*     n2;
	INI_SECTION*            sec;
	INI_PARAM*              pa;
	INI*                    pini = (INI*)ini;
	char *ret = NULL;

	if (pini == NULL || section == NULL || param == NULL) {
		return NULL;
	}

	INI_LOCK(pini);
	list_for_each_safe(pos, n1, &pini->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		if (strcmp(sec->section, section) == 0) {
			list_for_each_safe(pos, n2, &sec->param_list) {
				pa = list_entry(pos, INI_PARAM, entry);
				if (strcmp(pa->param, param) == 0) {
					ret = pa->value;
					goto end;
				}
			}
		}
	}
end:
	INI_UNLOCK(pini);

	return ret;
}

int ini_set(handle_t ini, const char* section, const char* param, const char* value)
{
	struct list_head*     pos;
	struct list_head*     n1;
	struct list_head*     n2;
	INI_SECTION*            sec;
	INI_PARAM*              pa;
	INI*                    pini = (INI*)ini;
	int ret = -1;

	if (pini == NULL || section == NULL || param == NULL || value == NULL) {
		return -1;
	}

	INI_LOCK(pini);
	list_for_each_safe(pos, n1, &pini->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		if (strcmp(sec->section, section) == 0) {
			list_for_each_safe(pos, n2, &sec->param_list) {
				pa = list_entry(pos, INI_PARAM, entry);
				if (strcmp(pa->param, param) == 0) {
					if (strcmp(pa->value, value) == 0) {
						ret = 0;
					} else {
						if (strlen(pa->value) >= strlen(value)) {
							strcpy(pa->value, value);
						} else {
							free(pa->value);
							pa->value = strdup(value);
						}
						ret = 1;
					}
					goto end;
				}
			}
		}
	}
end:
	INI_UNLOCK(pini);

	return ret;
}

int ini_append(handle_t ini, const char* section, const char* param, const char* value)
{
	struct list_head*     pos;
	INI_SECTION*            sec;
	INI*                    pini = (INI*)ini;
	int ret = 0;

	if (pini == NULL || param == NULL || value == NULL) {
		return -1;
	}

	INI_LOCK(pini);
	list_for_each(pos, &pini->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		if (strcmp(sec->section, section) == 0) {
			if( ini_append_param(sec, param ,value) == NULL) {
				ret = -1;
				goto end;
			}
			goto end;
		}
	}

	sec = ini_append_section(pini, section);
	if( ini_append_param(sec, param, value) == NULL) {
		ret = -1;
	}

end:
	INI_UNLOCK(pini);

	return ret;
}

int ini_remove(handle_t ini, const char* section, const char* param)
{
	struct list_head*     pos;
	struct list_head*     n1;
	struct list_head*     n2;
	INI_SECTION*            sec;
	INI_PARAM*              pa;
	INI*                    pini = (INI*)ini;
	int ret = -1;

	if (pini == NULL || section == NULL) {
		return -1;
	}

	INI_LOCK(pini);
	list_for_each_safe(pos, n1, &pini->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		if (strcmp(sec->section, section) == 0) {
			list_for_each_safe(pos, n2, &sec->param_list) {
				pa = list_entry(pos, INI_PARAM, entry);
				if (param != NULL) {
					if (strcmp(pa->param, param) != 0 ) {
						continue;
					}
				}
				list_del(&pa->entry);
				free(pa->param);
				free(pa->value);
				free(pa);

			}
			list_del(&sec->entry);
			free(sec->section);
			free(sec);
			ret = 0;
			break;
		}
	}
	INI_UNLOCK(pini);

	return ret;
}

handle_t ini_merge(handle_t  old_ini, handle_t new_ini)
{
	struct list_head*     pos;
	struct list_head*     n1;
	struct list_head*     n2;
	INI_SECTION*            sec;
	INI_PARAM*              pa;
	INI*                    src_old = (INI*)old_ini;
	INI*                    src_new = (INI*)new_ini;

	if (src_old == NULL || src_new == NULL) {
		return EINVALID_HANDLE;
	}

	//    INI_LOCK(src_old);
	INI_LOCK(src_new);
	list_for_each_safe(pos, n1, &src_new->section_list) {
		sec = list_entry(pos, INI_SECTION, entry);
		list_for_each_safe(pos, n2, &sec->param_list) {
			pa = list_entry(pos, INI_PARAM, entry);
			if (ini_set(old_ini, sec->section, pa->param, pa->value) < 0) {
				ini_append(old_ini, sec->section, pa->param, pa->value);
			}
		}

	}
	INI_UNLOCK(src_new);
	//    INI_UNLOCK(src_old);

	return old_ini;
}

