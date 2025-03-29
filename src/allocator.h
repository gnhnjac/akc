#pragma once

#include <stddef.h>

typedef struct
{

	void *start;

	size_t head;

} arena;

#define ARENA_BLOCK_SZ 0x100000

arena arena_create();
void *arena_alloc(arena *a, size_t amt);
void *arena_calloc(arena *a, size_t amt);
void *arena_realloc(arena *a, void *src, size_t amt);
char *arena_strdup(arena *a, char *src);
void arena_destroy(arena *a);