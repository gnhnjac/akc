#include <stdlib.h>
#include <string.h>
#include "vector.h"

void vect_init(vector *v, size_t data_size)
{

	v->idx = 0;
	v->capacity = 1;
	v->data_size = data_size;

	v->data = (void *)malloc(data_size);

}

void vect_insert(vector *v, void *data)
{

	if (v->idx == v->capacity)
	{

		v->capacity *= 2;

		v->data = (void *)realloc(v->data, v->capacity * v->data_size);

	}

	memcpy(v->data + v->idx * v->data_size, data, v->data_size);

	v->idx++;

}

void vect_destroy(vector *v)
{

	free(v->data);

}