#pragma once

typedef struct
{

	size_t idx;
	size_t capacity;
	size_t data_size;

	void *data;

} vector;

void vect_init(vector *v, size_t data_size);
void vect_insert(vector *v, void *data);