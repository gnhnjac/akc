#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "generator.h"
#include "vector.h"
#include "parser.h"

char *assembly_code;

size_t assembly_code_len = 0;

size_t clause_counter = 0;

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

void add_to_code_fmt(char *fmt, ...)
{

	va_list valist;

	va_start(valist, fmt);

	char *text;

	vasprintf(&text, fmt, valist);

	add_to_code(text);

	free(text);

	va_end(valist);

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

		add_to_code(variables[i].name);
		add_to_code(": dq 0\n");

	}

	variables = (variable *)scope->subscope_variables.data;

	for(int i = 0; i < scope->subscope_variables.idx; i++)
	{

		add_to_code(variables[i].name);
		add_to_code(": dq 0\n");

	}

}

static inline void gen_insert_variable(expr_scope *scope_node, variable v)
{

	vect_insert(&scope_node->variables,&v);

	if (scope_node->type == EXPR_GLOBAL || scope_node->type == EXPR_FUNCSCOPE)
		return;

	while(scope_node->type != EXPR_GLOBAL && scope_node->type != EXPR_FUNCSCOPE)
		scope_node = scope_node->parent;

	vect_insert(&scope_node->subscope_variables,&v);

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

	if (!node)
	{

		fprintf(stderr, "received null node, aborting\n");
		exit(1);

	}

	switch(node->type)
	{

	case EXPR_BRANCH:

		gen_assemble(scope, ((expr_branch *)node)->condition);

		size_t clause_number = clause_counter;

		clause_counter++;

		add_to_code_fmt("pop rax\ncmp rax, 0\njz _else%d\n", clause_number);

		gen_assemble(scope, (expr_node *)((expr_branch *)node)->if_body);

		add_to_code_fmt("jmp _end%d\n_else%d:\n", clause_number, clause_number);

		gen_assemble(scope, (expr_node *)((expr_branch *)node)->else_body);

		add_to_code_fmt("_end%d:\n", clause_number);

		break;

	case EXPR_SUBSCOPE:

		expr_node **items = ((expr_scope *)node)->body.data;

		size_t items_sz = ((expr_scope *)node)->body.idx;

		for(int i = 0; i < items_sz; i++)
		{

			gen_assemble((expr_scope *)node, items[i]);

		}

		break;

	case EXPR_GLOBAL:

		items = ((expr_scope *)node)->body.data;

		items_sz = ((expr_scope *)node)->body.idx;

		for(int i = 0; i < items_sz; i++)
		{

			gen_assemble((expr_scope *)node, items[i]);

		}

		gen_assemble_global_data((expr_scope *)node);

		break;

	case EXPR_DECL:

		if (!gen_get_variable(scope, ((expr_decl *)node)->name).name)
		{

			variable v;

			v.name = ((expr_decl *)node)->name; // already malloced...

			gen_insert_variable(scope, v);

		}
		else
		{

			fprintf(stderr, "variable %s already declared, aborting\n",((expr_decl *)node)->name);
			exit(1);

		}

		break;

	case EXPR_ASSIGN:

		if (!gen_get_variable(scope, ((expr_assign *)node)->name).name)
		{

			fprintf(stderr, "variable %s is not declared, aborting\n",((expr_assign *)node)->name);
			exit(1);

		}

		gen_assemble(scope, ((expr_assign *)node)->value);

		add_to_code_fmt("pop QWORD [%s]\n", ((expr_assign *)node)->name);

		break;

	case EXPR_CONST:

		add_to_code_fmt("push %ld\n", ((expr_const *)node)->value);

		break;

	case EXPR_VAR:

		if (!gen_get_variable(scope, ((expr_var *)node)->name).name)
		{

			fprintf(stderr, "variable %s is not declared, aborting\n",((expr_var *)node)->name);
			exit(1);

		}

		add_to_code_fmt("push QWORD [%s]\n", ((expr_var *)node)->name);

		break;

	case EXPR_ADD:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rax\npop rdx\nadd rax, rdx\npush rax\n");

		break;

	case EXPR_BIGGER:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\njg _bigger%d\npush 0\njmp _end%d\n_bigger%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		clause_counter++;

		break;

	case EXPR_SMALLER:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\njl _smaller%d\npush 0\njmp _end%d\n_smaller%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		clause_counter++;

		break;

	case EXPR_EQUAL:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\nje _equal%d\npush 0\njmp _end%d\n_equal%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		clause_counter++;

		break;

	case EXPR_NEQUAL:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\njne _nequal%d\npush 0\njmp _end%d\n_nequal%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		clause_counter++;

		break;

	case EXPR_NOT:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\nje _zero%d\npush 0\njmp _end%d\n_zero%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		clause_counter++;

		break;

	case EXPR_AND:

		clause_number = clause_counter;

		clause_counter++;

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _firstequal%d\npush 0\njmp _end%d\n_firstequal%d:\n", clause_number, clause_number, clause_number);

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _secondequal%d\npush 0\njmp _end%d\n_secondequal%d:\npush 1\n_end%d:\n", clause_number, clause_number, clause_number, clause_number);

		break;

	case EXPR_OR:

		clause_number = clause_counter;

		clause_counter++;

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _equal%d\n", clause_number);

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _equal%d\npush 0\njmp _end%d\n_equal%d:\npush 1\n_end%d:\n", clause_number, clause_number, clause_number, clause_number);

		break;

	case EXPR_SUB:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rdx\npop rax\nsub rax, rdx\npush rax\n");

		break;

	case EXPR_MULT:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rbx\npop rax\nmul rbx\npush rax\n");

		break;

	case EXPR_DIV:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rbx\npop rax\ndiv rbx\npush rax\n");

		break;

	case EXPR_MOD:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rbx\npop rax\ndiv rbx\npush rdx\n");

		break;

	case EXPR_EXIT:

		gen_assemble(scope, ((expr_exit *)node)->exit_code);

		add_to_code("pop rdi\n");

		add_to_code("mov rax, 60\nsyscall\n");

		break;

	default:
		fprintf(stderr, "unknown expression %d, aborting\n", node->type);
		exit(1);

	}

}