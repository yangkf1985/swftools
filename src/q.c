/* q.c

   Part of the swftools package.
   
   Copyright (c) 2001 Matthias Kramm <kramm@quiss.org>

   This file is distributed under the GPL, see file COPYING for details */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "q.h"

// ------------------------------- malloc, alloc routines ---------------------

#ifndef STRNDUP
char* strndup(const char*str, int size)
{
    char*m = (char*)malloc(size+1);
    memcpy(m, str, size);
    m[size] = 0;
    return m;
}
#endif
void* qmalloc_internal(int len)
{
    void*val = malloc(len);
    if(!val) {
	printf("memory error! Couldn't reserve %d bytes\n", len);
	fprintf(stderr, "memory error! Couldn't reserve %d bytes\n", len);
	exit(1);
    }
    return val;
}
void* qrealloc_internal(void*old, int len)
{
    void*val = realloc(old, len);
    if(!val) {
	printf("memory error! Couldn't reserve %d bytes\n", len);
	fprintf(stderr, "memory error! Couldn't reserve %d bytes\n", len);
	exit(1);
    }
    return val;
}
void qfree_internal(void*old)
{
    free(old);
}
char*qstrdup(const char*string)
{
    return strdup(string);
}
char*qstrndup(const char*string, int len)
{
    return strndup(string, len);
}

// ------------------------------- mem_t --------------------------------------

void mem_init(mem_t*mem)
{
    memset(mem, 0, sizeof(mem_t));
}
void mem_clear(mem_t*mem)
{
    free(mem->buffer);
}
void mem_destroy(mem_t*mem)
{
    mem_clear(mem);
    free(mem);
}
static int mem_put_(mem_t*m,void*data, int length, int null)
{
    int n = m->pos;
    m->pos += length + (null?1:0);
    if(m->pos > m->len)
    { 
	//m->len += 1024>length?1024:(null?length*2:length);

	m->len *= 2;
	while(m->len < m->pos)
	    m->len += 64;

	m->buffer = m->buffer?realloc(m->buffer,m->len):malloc(m->len);
    }
    memcpy(&m->buffer[n], data, length);
    if(null)
	m->buffer[n + length] = 0;
    return n;
}
int mem_put(mem_t*m,void*data, int length)
{
    return mem_put_(m, data, length, 0);
}
int mem_putstring(mem_t*m,string_t str)
{
    return mem_put_(m, str.str, str.len, 1);
}

// ------------------------------- string_t ------------------------------------

void string_set2(string_t*str, char*text, int len)
{
    str->len = len;
    str->str = text;
}
void string_set(string_t*str, char*text)
{
    str->len = strlen(text);
    str->str = text;
}
void string_dup2(string_t*str, const char*text, int len)
{
    str->len = len;
    str->str = strndup(text, len);
}
void string_dup(string_t*str, const char*text)
{
    str->len = strlen(text);
    str->str = strdup(text);
}
int string_equals(string_t*str, const char*text)
{
    int l = strlen(text);
    if(str->len == l && !strncmp(str->str, text, l))
	return 1;
    return 0;
}
int string_equals2(string_t*str, string_t*str2)
{
    if(str->len == str2->len && !strncmp(str->str, str2->str, str->len))
	return 1;
    return 0;
}
char* string_cstr(string_t*str)
{
    return strndup(str->str, str->len);
}

// ------------------------------- stringarray_t ------------------------------

typedef struct _stringarray_internal_t
{
    mem_t data;
    mem_t pos;
    int num;
} stringarray_internal_t;
void stringarray_init(stringarray_t*sa)
{
    stringarray_internal_t*s;
    sa->internal = (stringarray_internal_t*)malloc(sizeof(stringarray_internal_t)); 
    memset(sa->internal, 0, sizeof(stringarray_internal_t));
    s = (stringarray_internal_t*)sa->internal;
    mem_init(&s->data);
    mem_init(&s->pos);
}
void stringarray_put(stringarray_t*sa, string_t str)
{
    stringarray_internal_t*s = (stringarray_internal_t*)sa->internal;
    int pos;
    pos = mem_putstring(&s->data, str);
    mem_put(&s->pos, &pos, sizeof(int));
    s->num++;
}
char* stringarray_at(stringarray_t*sa, int pos)
{
    stringarray_internal_t*s = (stringarray_internal_t*)sa->internal;
    int p;
    if(pos<0 || pos>=s->num)
	return 0;
    p = *(int*)&s->pos.buffer[pos*sizeof(int)];
    if(p<0)
	return 0;
    return &s->data.buffer[p];
}
string_t stringarray_at2(stringarray_t*sa, int pos)
{
    string_t s;
    s.str = stringarray_at(sa, pos);
    s.len = s.str?strlen(s.str):0;
    return s;
}
void stringarray_del(stringarray_t*sa, int pos)
{
    stringarray_internal_t*s = (stringarray_internal_t*)sa->internal;
    *(int*)&s->pos.buffer[pos*sizeof(int)] = -1;
}
int stringarray_find(stringarray_t*sa, string_t* str)
{
    stringarray_internal_t*s = (stringarray_internal_t*)sa->internal;
    int t;
    for(t=0;t<s->num;t++) {
	string_t s = stringarray_at2(sa, t);
	if(s.str && string_equals2(&s, str)) {
	    return t;
	}
    }
    return -1;
}
void stringarray_clear(stringarray_t*sa)
{
    stringarray_internal_t*s = (stringarray_internal_t*)sa->internal;
    mem_clear(&s->data);
    mem_clear(&s->pos);
    free(s);
}
void stringarray_destroy(stringarray_t*sa)
{
    stringarray_clear(sa);
    free(sa);
}


// ------------------------------- map_t --------------------------------------

typedef struct _map_internal_t
{
    stringarray_t keys;
    stringarray_t values;
    int num;
} map_internal_t;

void map_init(map_t*map)
{
    map_internal_t*m;
    map->internal = (map_internal_t*)malloc(sizeof(map_internal_t));
    memset(map->internal, 0, sizeof(map_internal_t));
    m = (map_internal_t*)map->internal;
    stringarray_init(&m->keys);
    stringarray_init(&m->values);
}
void map_put(map_t*map, string_t t1, string_t t2)
{
    map_internal_t*m = (map_internal_t*)map->internal;
    stringarray_put(&m->keys, t1);
    stringarray_put(&m->values, t2);
    m->num++;
}
char* map_lookup(map_t*map, const char*name)
{
    int s;
    map_internal_t*m = (map_internal_t*)map->internal;
    string_t str;
    string_set(&str, (char*)name);
    s = stringarray_find(&m->keys, &str);
    if(s>=0) {
	string_t s2 = stringarray_at2(&m->values, s);
	return s2.str;
    }
    return 0;
}
void map_dump(map_t*map, FILE*fi, const char*prefix)
{
    int t;
    map_internal_t*m = (map_internal_t*)map->internal;
    for(t=0;t<m->num;t++) {
	string_t s1 = stringarray_at2(&m->keys, t);
	string_t s2 = stringarray_at2(&m->values, t);
	fprintf(fi, "%s%s=%s\n", prefix, s1.str, s2.str);
    }
}
void map_clear(map_t*map)
{
    map_internal_t*m = (map_internal_t*)map->internal;
    stringarray_clear(&m->keys);
    stringarray_clear(&m->values);
    free(m);
}
void map_destroy(map_t*map)
{
    map_clear(map);
    free(map);
}

// ------------------------------- dictionary_t --------------------------------------

typedef struct _dictionary_internal_t
{
    stringarray_t keys;
    mem_t values;
    int num;
} dictionary_internal_t;

void dictionary_init(dictionary_t*dict)
{
    dictionary_internal_t*d;
    dict->internal = (dictionary_internal_t*)malloc(sizeof(dictionary_internal_t));
    memset(dict->internal, 0, sizeof(dictionary_internal_t));
    d = (dictionary_internal_t*)dict->internal;
    stringarray_init(&d->keys);
    mem_init(&d->values);
}
void dictionary_put(dictionary_t*dict, string_t t1, void* t2)
{
    dictionary_internal_t*d = (dictionary_internal_t*)dict->internal;
    int s=0;
    s = stringarray_find(&d->keys, &t1);
    if(s>=0) {
	/* replace */
	*(void**)(&d->values.buffer[s*sizeof(void*)]) = t2;
    } else {
	stringarray_put(&d->keys, t1);
	mem_put(&d->values, &t2, sizeof(void*));
	d->num++;
    }
}
void dictionary_put2(dictionary_t*dict, const char*t1, void* t2)
{
    string_t s;
    string_set(&s, (char*)t1);
    dictionary_put(dict, s, t2);
}
void* dictionary_lookup(dictionary_t*dict, const char*name)
{
    int s;
    dictionary_internal_t*d = (dictionary_internal_t*)dict->internal;
    string_t str;
    string_set(&str, (char*)name);
    s = stringarray_find(&d->keys, &str);
    if(s>=0) {
	return *(void**)&d->values.buffer[sizeof(void*)*s];
    }
    return 0;
}
void dictionary_dump(dictionary_t*dict, FILE*fi, const char*prefix)
{
    dictionary_internal_t*d = (dictionary_internal_t*)dict->internal;
    int t;
    for(t=0;t<d->num;t++) {
	string_t s1 = stringarray_at2(&d->keys, t);
	fprintf(fi, "%s%s=%08x\n", prefix, s1.str, *(void**)&d->values.buffer[sizeof(void*)*t]);
    }
}
void dictionary_del(dictionary_t*dict, const char* name)
{
    dictionary_internal_t*d = (dictionary_internal_t*)dict->internal;
    int s;
    string_t str;
    string_set(&str, (char*)name);
    s = stringarray_find(&d->keys, &str);
    if(s>=0) {
	*(void**)(&d->values.buffer[s*sizeof(void*)]) = 0;
	stringarray_del(&d->keys, s);
    }
}
void dictionary_clear(dictionary_t*dict)
{
    dictionary_internal_t*d = (dictionary_internal_t*)dict->internal;
    stringarray_clear(&d->keys);
    mem_clear(&d->values);
    free(d);
}
void dictionary_destroy(dictionary_t*dict)
{
    dictionary_clear(dict);
    free(dict);
}
