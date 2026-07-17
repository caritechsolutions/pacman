#include "gui_hash.h"
#ifndef NO_OS
#include "gui_private.h"
#include <stdio.h>
#include <stdlib.h>
#else
#include "mini_gui_private.h"
#endif /*NO_OS*/

#include <string.h>

hash_t *hash_new(int size, hash_free_func freefunc)
{
	hash_t *result = NULL;

	result = (hash_t *) GUI_MALLOC(sizeof(hash_t));
	if (NULL != result) {
		result->entries = (hashentry_t **)GUI_MALLOC(size * sizeof(hashentry_t *));
		if (NULL == result->entries) {
			GUI_FREE(result);
		}
		memset(result->entries, 0, size * sizeof(hashentry_t *));

		result->length = size;
		result->freefunc = freefunc;
		result->num_keys = 0;
		/* give the caller a reference */
		result->ref = 1;
	}

	return result;
}

/** obtain a new reference to an existing hash table */
hash_t *hash_clone(hash_t *table)
{
	table->ref++;
	return table;
}

/** release a hash table that is no longer needed */
int hash_release(hash_t * table)
{
	hashentry_t *entry, *next;
	int i;

	if (table->ref > 1)
		table->ref--;
	else {
		for (i = 0; i < table->length; i++) {
			entry = table->entries[i];
			while (entry != NULL) {
				next = entry->next;
				GUI_FREE(entry->key);
				if (table->freefunc)
					table->freefunc(entry->value);
				GUI_FREE(entry);
				entry = next;
			}
		}
		GUI_FREE(table->entries);
		GUI_FREE(table);
	}

	return 0;
}

/** hash a key for our table lookup */
static int _hash_key(hash_t *table, const char *key)
{
	unsigned int hash = 0; //change int to unsigned int, by zfz
	int shift = 0;
	const char *c = key;

	if (table->length == 1)
		return 0;
	while (*c != '\0') {
		/* assume 32 bit ints */
		hash ^= ((int)*c++ << shift);
		shift += 8;
		if (shift > 24)
			shift = 0;
	}

	return hash % table->length;
}

/** add a key, value pair to a hash table.
 *  each key can appear only once; the value of any
 *  identical key will be replaced
 */
int hash_add(hash_t *table, const char *key, void *data)
{
	hashentry_t *entry = NULL;
	int index = 0;

	if (key == NULL || data == NULL)
		return 1;

	index = _hash_key(table, key);
	entry = table->entries[index];
	while (entry != NULL) {
		/* traverse the linked list looking for the key */
		if (strcmp(key, entry->key) == 0) {
			/* match */
			if (table->freefunc && entry->value)
				table->freefunc(entry->value);
			entry->value = data;
			return 0;
		}
		entry = entry->next;
	}

	/* allocate and fill a new entry */
	entry = (hashentry_t*)GUI_MALLOC(sizeof(hashentry_t));
	if (entry == NULL)
		return 1;

	entry->key = GUI_STRDUP(key);
	if (entry->key == NULL) {
		GUI_FREE(entry);
		return 1;
	}
	entry->value = data;
	/* insert ourselves in the linked list */
	/* TODO: this leaks duplicate keys */
	entry->next = table->entries[index];
	table->entries[index] = entry;
	table->num_keys++;

	return 0;
}

/** look up a key in a hash table */
void *hash_get(hash_t * table, const char *key)
{
	hashentry_t *entry;
	int index;
	void *result = NULL;

	if (key == NULL || table->num_keys == 0)
		return (NULL);

	/* look up the hash entry */
	index = _hash_key(table, key);
	entry = table->entries[index];
	while (entry != NULL) {
		/* traverse the linked list looking for the key */
		if (!strcmp(key, entry->key)) {
			/* match */
			result = entry->value;
			return result;
		}

		entry = entry->next;
	}
	/* no match */
	return result;
}

/** delete a key from a hash table */
int hash_drop(hash_t * table, const char *key)
{
	hashentry_t *entry, *prev;
	int index;

	if (key == NULL)
		return 1;

	/* look up the hash entry */
	index = _hash_key(table, key);
	entry = table->entries[index];
	prev = NULL;
	while (entry != NULL) {
		/* traverse the linked list looking for the key */
		if (strcmp(key, entry->key) == 0) {
			GUI_FREE(entry->key);
			if (table->freefunc)
				table->freefunc(entry->value);
			if (prev == NULL)
				table->entries[index] = entry->next;
			else
				prev->next = entry->next;
			GUI_FREE(entry);
			table->num_keys--;
			return 0;
		}
		prev = entry;
		entry = entry->next;
	}
	/* no match */
	return 1;
}

int hash_num_keys(hash_t *table)
{
	return table->num_keys;
}

/** allocate and initialize a new iterator */
hash_iterator_t *hash_iter_new(hash_t *table)
{
	hash_iterator_t *iter = NULL;

	iter = (hash_iterator_t*)GUI_MALLOC(sizeof(hash_iterator_t));
	if (iter != NULL) {
		iter->ref = 1;
		iter->table = hash_clone(table);
		iter->entry = NULL;
		iter->index = 0;
	}

	return iter;
}

/** release an iterator that is no longer needed */
int hash_iter_release(hash_iterator_t *iter)
{
	if(NULL == iter)
		return (1);

	iter->ref--;
	if(iter->ref <= 0) {
		hash_release(iter->table);
		GUI_FREE(iter);
	}

	return (0);
}

/** return the next hash table key from the iterator.
    the returned key should not be freed */
const char *hash_iter_next(hash_iterator_t *iter)
{
	hash_t *table;
	hashentry_t *entry;

	if (iter == NULL)
		return NULL;

	table = iter->table;
	entry = iter->entry;

	/* advance until we find the next entry */
	if (entry != NULL)
		entry = entry->next;

	if (entry == NULL) {
		/* we're off the end of list, search for a new entry */
		while (iter->index < iter->table->length) {
			entry = table->entries[iter->index];
			if (entry != NULL) {
				iter->index++;
				break;
			}
			iter->index++;
		}
	}

	if ((entry == NULL) || (iter->index > table->length)) {
		/* no more keys! */
		return NULL;
	}

	/* remember our current match */
	iter->entry = entry;

	return entry->key;
}

void *hash_iter_get_data(hash_iterator_t *iter)
{
	if (iter == NULL)
		return NULL;
	if (iter->entry == NULL)
		return NULL;
	return iter->entry->value;
}

int hash_iter_set_data(hash_iterator_t *iter, void *data)
{
	if(NULL == iter)
		return 1;
	if(NULL == iter->entry)
		return 1;

	iter->entry->value = data;

	return (0);
}


