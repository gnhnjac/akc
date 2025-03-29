#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "allocator.h"


static arena vect_arena;

void vect_init_arena()
{

	vect_arena = arena_create();

}

void vect_init(vector *v, size_t data_size)
{

	v->idx = 0;
	v->capacity = 1;
	v->data_size = data_size;

	v->data = (void *)arena_calloc(&vect_arena, data_size);

}

void vect_insert(vector *v, void *data)
{

	if (v->idx == v->capacity)
	{

		v->capacity *= 2;

		v->data = (void *)arena_realloc(&vect_arena, v->data, v->capacity * v->data_size);

	}

	memcpy(v->data + v->idx * v->data_size, data, v->data_size);

	v->idx++;

}

void vect_destroy()
{

	arena_destroy(&vect_arena);

}