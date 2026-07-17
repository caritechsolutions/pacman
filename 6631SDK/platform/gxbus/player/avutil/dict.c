/*
 * copyright (c) 2009 Michael Niedermayer
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>

#include "avstring.h"
#include "dict.h"
#include "gx_mem.h"
#include "avcodec.h"

int av_dict_count(const AVDictionary *m)
{
    return m ? m->count : 0;
}

AVDictionaryEntry *
av_dict_get(AVDictionary *m, const char *key, const AVDictionaryEntry *prev, int flags)
{
    unsigned int i, j;

    if(!m || m->count <= 0)
        return NULL;

    if(prev) i= prev - m->elems + 1;
    else     i= 0;

    for(; i<m->count; i++){
        const char *s= m->elems[i].key;
        if(flags & AV_DICT_MATCH_CASE) for(j=0;            s[j]  ==            key[j]  && key[j]; j++);
        else                           for(j=0; av_toupper(s[j]) == av_toupper(key[j]) && key[j]; j++);
        if(key[j])
            continue;
        if(s[j] && !(flags & AV_DICT_IGNORE_SUFFIX))
            continue;
        return &m->elems[i];
    }
    return NULL;
}

int av_dict_set(AVDictionary **pm, const char *key, const char *value, int flags)
{
    AVDictionary *m = *pm;
    AVDictionaryEntry *tag = NULL;
    char *oldval = NULL, *copy_key = NULL, *copy_value = NULL;

    if (!value)
        return 0;
    if (!(flags & AV_DICT_MULTIKEY)) {
        tag = av_dict_get(m, key, NULL, flags);
    }
    if (flags & AV_DICT_DONT_STRDUP_KEY)
        copy_key = (void *)key;
    else
        copy_key = av_strdup(key);
    if (flags & AV_DICT_DONT_STRDUP_VAL)
        copy_value = (void *)value;
    else if (copy_key)
        copy_value = av_strdup(value);
    if (!m)
        m = *pm = av_mallocz(sizeof(*m));
    if (!m || (key && !copy_key) || (value && !copy_value))
        goto err_out;

    if (tag) {
        if (flags & AV_DICT_DONT_OVERWRITE) {
            if (copy_key)
                av_free(copy_key);
            if (copy_value)
                av_free(copy_value);
            return 0;
        }
        if (flags & AV_DICT_APPEND) {
            oldval = tag->value;
        } else {
            if (tag->value)
                av_free(tag->value);
        }
        if (tag->key)
            av_free(tag->key);
        *tag = m->elems[--m->count];
    } else if (copy_value) {
        AVDictionaryEntry *tmp = av_realloc_array(m->elems,
                                                  m->count + 1, sizeof(*m->elems));
        if (!tmp)
            goto err_out;
        m->elems = tmp;
    }
    if (copy_value) {
        m->elems[m->count].key = copy_key;
        m->elems[m->count].value = copy_value;
        if (oldval && flags & AV_DICT_APPEND) {
            size_t len = strlen(oldval) + strlen(copy_value) + 1;
            char *newval = av_mallocz(len);
            if (!newval)
                goto err_out;
            av_strlcat(newval, oldval, len);
            if (oldval)
                av_freep(&oldval);
            av_strlcat(newval, copy_value, len);
            m->elems[m->count].value = newval;
            if (copy_value)
                av_freep(&copy_value);
        }
        m->count++;
    } else {
        if (copy_key)
            av_freep(&copy_key);
    }
    if (!m->count) {
        if (m->elems)
            av_freep(&m->elems);
        if (pm)
            av_freep(pm);
    }
    return 0;
err_out:
    if (m && !m->count) {
        if (m->elems)
            av_freep(&m->elems);
        if (pm)
            av_freep(pm);
    }
    if (copy_key)
        av_free(copy_key);
    if (copy_value)
        av_free(copy_value);
    return AVERROR(ENOMEM);
}

static int parse_key_value_pair(AVDictionary **pm, const char **buf,
                                const char *key_val_sep, const char *pairs_sep,
                                int flags)
{
    char *key = av_get_token(buf, key_val_sep);
    char *val = NULL;
    int ret;

    if (key && *key && strspn(*buf, key_val_sep)) {
        (*buf)++;
        val = av_get_token(buf, pairs_sep);
    }

    if (key && *key && val && *val)
        ret = av_dict_set(pm, key, val, flags);
    else
        ret = AVERROR(EINVAL);

    av_freep(&key);
    av_freep(&val);

    return ret;
}

int av_dict_parse_string(AVDictionary **pm, const char *str,
                         const char *key_val_sep, const char *pairs_sep,
                         int flags)
{
    int ret;

    if (!str)
        return 0;

    /* ignore STRDUP flags */
    flags &= ~(AV_DICT_DONT_STRDUP_KEY | AV_DICT_DONT_STRDUP_VAL);

    while (*str) {
        if ((ret = parse_key_value_pair(pm, &str, key_val_sep, pairs_sep, flags)) < 0)
            return ret;

        if (*str)
            str++;
    }

    return 0;
}

void av_dict_free(AVDictionary **pm)
{
    AVDictionary *m = *pm;

    if (m) {
        while(m->count--) {
            if (m->elems) {
                if (m->elems[m->count].key)
                    av_freep(&m->elems[m->count].key);
                if (m->elems[m->count].value)
                    av_freep(&m->elems[m->count].value);
            }
        }
        if (m->elems)
            av_freep(&m->elems);
    }
    if (*pm) {
        av_free(*pm);
        *pm = NULL;
    }
}

int av_dict_copy(AVDictionary **dst, AVDictionary *src, int flags)
{
    AVDictionaryEntry *t = NULL;
    if (!src)
        return 0;

    while ((t = av_dict_get(src, "", t, AV_DICT_IGNORE_SUFFIX))) {
        int ret = av_dict_set(dst, t->key, t->value, flags);
        if (ret < 0)
            return ret;
    }
    return 0;
}

int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags)
{
    char valuestr[22];
    snprintf(valuestr, sizeof(valuestr), "%lld", value);
    flags &= ~AV_DICT_DONT_STRDUP_VAL;
    return av_dict_set(pm, key, valuestr, flags);
}

int av_dict_metadata(AVDictionary **pm, const char *key, char *value, int flags)
{
    int ret = 0;
    if (value && *value) {
        ret = av_dict_set(pm, key, value, flags);
    }
    return ret;
}


