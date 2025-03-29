#include "allocator.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

arena arena_create()
{

	arena a = { .start = (void *)malloc(ARENA_BLOCK_SZ), .head = 0 };

	return a;

}

void *arena_alloc(arena *a, size_t amt)
{

	void *ptr = a->start + a->head;

	*((size_t *)ptr) = amt;

	ptr += 8;

	a->head += amt + 8;

	if (a->head >= ARENA_BLOCK_SZ)
	{

		fprintf(stderr, "arena ran out of memory, aborting");
		exit(1);

	}

	return ptr;

}

void *arena_calloc(arena *a, size_t amt)
{

	void *ptr = arena_alloc(a, amt);

	memset(ptr, 0, amt);

	return ptr;

}

void *arena_realloc(arena *a, void *src, size_t amt)
{
	
	size_t sz = *((size_t *)(src - 8));

	if (sz >= amt)
		return src;

	void *ptr = arena_alloc(a, amt);

	memcpy(ptr, src, sz);

	return ptr;

}

char *arena_strdup(arena *a, char *src)
{
	
	void *ptr = arena_alloc(a, strlen(src) + 1);

	strcpy(ptr, src);

	return ptr;

}

void arena_destroy(arena *a)
{

	free(a->start);

}