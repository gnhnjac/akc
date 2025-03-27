#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "generator.h"
#include "vector.h"
#include "parser.h"

char *assembly_code;

size_t assembly_code_len = 0;

void gen_init()
{

	assembly_code_len = strlen("global _start\n\nsection .text\n_start:\n");

	assembly_code = (char *)malloc(assembly_code_len + 1);

	strcpy(assembly_code, "global _start\n\nsection .text\n_start:\n");

}

void add_to_code(char *text)
{

	char *tmp = assembly_code;

	assembly_code_len += strlen(text);

	assembly_code = realloc(assembly_code, assembly_code_len + 1);

	strcat(assembly_code,text);

}

void gen_assemble_move(expr_node *node, char *dst, op_type op)
{

	char *text;

	switch (node->type)
	{

	case EXPR_CONST:

		switch(op)
		{

		case OP_MOV_REG:
			asprintf(&text, "mov %s, %ld\n", dst, ((expr_const *)node)->value);
			break;
		case OP_MOV_IMM:
			asprintf(&text, "mov rbx, %ld\nmov [%s], rbx\n", ((expr_const *)node)->value, dst);
			break;
		default:
			fprintf(stderr, "unknown op type %d, aborting\n",op);

		}

		add_to_code(text);

		free(text);

		break;

	case EXPR_VAR:


		switch(op)
		{

		case OP_MOV_REG:
			asprintf(&text, "mov rbx, [%s]\nmov %s, rbx\n", ((expr_var *)node)->name, dst);
			break;
		case OP_MOV_IMM:
			asprintf(&text, "mov rbx, [%s]\nmov [%s], rbx\n", ((expr_var *)node)->name, dst);
			break;
		default:
			fprintf(stderr, "unknown op type %d, aborting\n",op);

		}

		add_to_code(text);

		free(text);

		break;

	default:
		fprintf(stderr, "unknown expression %d, aborting\n", node->type);
		exit(1);

	}

}

size_t type_to_size(data_type type)
{

	switch(type)
	{

	case DATA_CHAR:
		return 1;

	case DATA_WORD:
		return 2;

	case DATA_INT:
		return 4;

	case DATA_PTR: case DATA_LONG:
		return 8;

	}

	return 0;

}

void gen_assemble_global_data(expr_scope *scope)
{

	if (scope->type != EXPR_GLOBAL)
	{

		fprintf(stderr, "expected global scope expression, got %d instead\n",scope->type);
		exit(1);

	}

	variable *variables = (variable *)scope->variables.data;

	add_to_code("section .data\n");

	for(int i = 0; i < scope->variables.idx; i++)
	{

		char *data_size;

		switch(type_to_size(variables[i].type))
		{

		case 1:
			data_size = ": db 0\n";
			break;
		case 2:
			data_size = ": dw 0\n";
			break;
		case 4:
			data_size = ": dd 0\n";
			break;
		case 8:
			data_size = ": dq 0\n";
			break;
		default:
			fprintf(stderr, "unknown data type %d, aborting\n",variables[i].type);
			exit(1);

		}

		add_to_code(variables[i].name);
		add_to_code(data_size);

	}

}

static inline void gen_insert_variable(expr_scope *scope_node, variable v)
{

	vect_insert(&scope_node->variables,&v);

}

variable gen_get_variable(expr_scope *scope_node, char *name)
{

	while(scope_node)
	{
		for(int i = 0; i < scope_node->variables.idx; i++)
		{

			variable v = ((variable *)scope_node->variables.data)[i];

			if (!strcmp(v.name, name))
				return v;

		}

		scope_node = scope_node->parent;

	}

	variable v = { 0 };

	return v;
	

}

char *gen_generate_assembly(expr_scope *global_node)
{

	gen_assemble(0, (expr_node *)global_node);

	return assembly_code;

}

void gen_assemble(expr_scope *scope, expr_node *node)
{

	switch(node->type)
	{

	case EXPR_GLOBAL:

		expr_node **items = ((expr_scope *)node)->body.data;

		size_t items_sz = ((expr_scope *)node)->body.idx;

		for(int i = 0; i < items_sz; i++)
		{

			gen_assemble((expr_scope *)node, items[i]);

		}

		gen_assemble_global_data((expr_scope *)node);

		break;

	case EXPR_ASSIGN:

		if (!gen_get_variable(scope, ((expr_assign *)node)->name).type)
		{

			if (!((expr_assign *)node)->var_type)
			{

				fprintf(stderr, "variable %s is not declared, aborting\n",((expr_assign *)node)->name);
				exit(1);

			}

			variable v;

			v.type = ((expr_assign *)node)->var_type;
			v.name = ((expr_assign *)node)->name; // already malloced...

			gen_insert_variable(scope, v);

		}
		else
		{

			if (((expr_assign *)node)->var_type)
			{

				fprintf(stderr, "variable %s already declared, aborting\n",((expr_assign *)node)->name);
				exit(1);

			}

		}

		gen_assemble_move(((expr_assign *)node)->value, ((expr_assign *)node)->name, OP_MOV_IMM);

		break;

	case EXPR_EXIT:

		gen_assemble_move(((expr_exit *)node)->exit_code, "rdi", OP_MOV_REG);

		add_to_code("mov rax, 60\nsyscall\n");

		break;

	default:
		fprintf(stderr, "unknown expression %d, aborting\n", node->type);
		exit(1);

	}

}