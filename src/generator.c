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

vector functions;

function cur_func;

int rsp_off = 0;

void gen_init()
{

	assembly_code_len = strlen("global _start\n\nsection .text\n_start:\ncall main\n");

	assembly_code = (char *)malloc(assembly_code_len + 1);

	strcpy(assembly_code, "global _start\n\nsection .text\n_start:\ncall main\n");

	add_to_code("mov rdi, 0\nmov rax, 60\nsyscall\n"); // exit

	vect_init(&functions, sizeof(function));

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

}

void gen_exit_scope(expr_scope *scope)
{

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

	if (!node)
	{

		fprintf(stderr, "received null node, aborting\n");
		exit(1);

	}

	switch(node->type)
	{

	case EXPR_RET:

		gen_assemble(scope, ((expr_ret *)node)->value);
		add_to_code_fmt("pop rax\nmov rsp, rbp\npop rbp\nret %d\n", cur_func.num_params * 8);

		break;

	case EXPR_CALL:

		int found = 0;

		for (int i = 0; i < functions.idx; i++)
		{
			if (!strcmp(((function *)functions.data)[i].name, ((expr_call *)node)->name))
			{

				if (((expr_call *)node)->params.idx != ((function *)functions.data)[i].num_params)
				{

					fprintf(stderr, "expected %ld parameters, instead got %ld, aborting", ((function *)functions.data)[i].num_params, ((expr_call *)node)->params.idx);
					exit(1);

				}

				found = 1;

			}

		}

		if (!found)
		{

			fprintf(stderr, "function %s not defined, aborting\n", ((expr_call *)node)->name);
			exit(1);

		}

		vector params = ((expr_call *)node)->params;

		for (int i = params.idx - 1; i >=0; i--)
		{

			gen_assemble(scope, ((expr_node **)params.data)[i]);

		}

		add_to_code_fmt("call %s\n",((expr_call *)node)->name);

		if (((expr_call *)node)->use_return)
			add_to_code("push rax\n");

		break;


	case EXPR_FUNC:

		function f;

		f.name = ((expr_func *)node)->name; // already malloced
		f.num_params = ((expr_func *)node)->num_params;

		vect_insert(&functions, &f);

		add_to_code_fmt("%s:\npush rbp\nmov rbp, rsp\n", ((expr_func *)node)->name);

		rsp_off = 0;

		cur_func = f;

		gen_assemble(scope, (expr_node *)((expr_func *)node)->body);

		add_to_code_fmt("mov rsp, rbp\npop rbp\nret %d\n", f.num_params * 8);

		break;

	case EXPR_BRANCH:

		gen_assemble(scope, ((expr_branch *)node)->condition);

		size_t clause_number = clause_counter;

		clause_counter++;

		add_to_code_fmt("pop rax\ncmp rax, 0\njz _else%d\n", clause_number);

		rsp_off--; // pop

		gen_assemble(scope, (expr_node *)((expr_branch *)node)->if_body);

		add_to_code_fmt("jmp _end%d\n_else%d:\n", clause_number, clause_number);

		gen_assemble(scope, (expr_node *)((expr_branch *)node)->else_body);

		add_to_code_fmt("_end%d:\n", clause_number);

		break;

	case EXPR_SCOPE:

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

			if (scope->type == EXPR_SCOPE)
			{

				add_to_code("sub rsp, 8\n");

				rsp_off += 8;

				v.rbp_off = rsp_off;

			}
			else
			{
				v.rbp_off = 0;
			}

			if (((expr_decl *)node)->value)
			{

				gen_assemble(scope, ((expr_assign *)node)->value);

				if(v.rbp_off)
				{
					if (v.rbp_off > 0)
						add_to_code_fmt("pop QWORD [rbp-%d]\n", v.rbp_off);
					else
						add_to_code_fmt("pop QWORD [rbp+%d]\n", -v.rbp_off);
				}
				else
					add_to_code_fmt("pop QWORD [%s]\n", v.name);

				rsp_off--; // pop
				
			}

			gen_insert_variable(scope, v);

		}
		else
		{

			fprintf(stderr, "variable %s already declared, aborting\n",((expr_decl *)node)->name);
			exit(1);

		}

		break;

	case EXPR_ASSIGN:

		variable v = gen_get_variable(scope, ((expr_assign *)node)->name);

		if (!v.name)
		{

			fprintf(stderr, "variable %s is not declared, aborting\n",((expr_assign *)node)->name);
			exit(1);

		}

		gen_assemble(scope, ((expr_assign *)node)->value);

		if(v.rbp_off)
		{
			if (v.rbp_off > 0)
				add_to_code_fmt("pop QWORD [rbp-%d]\n", v.rbp_off);
			else
				add_to_code_fmt("pop QWORD [rbp+%d]\n", -v.rbp_off);
		}
		else
			add_to_code_fmt("pop QWORD [%s]\n", v.name);

		rsp_off--; // pop

		break;

	case EXPR_CONST:

		add_to_code_fmt("push %ld\n", ((expr_const *)node)->value);

		rsp_off++; // push

		break;

	case EXPR_VAR:

		v = gen_get_variable(scope, ((expr_var *)node)->name);

		if (!v.name)
		{

			fprintf(stderr, "variable %s is not declared, aborting\n",((expr_var *)node)->name);
			exit(1);

		}

		if(v.rbp_off)
		{
			if (v.rbp_off > 0)
				add_to_code_fmt("push QWORD [rbp-%d]\n", v.rbp_off);
			else
				add_to_code_fmt("push QWORD [rbp+%d]\n", -v.rbp_off);
		}
		else
			add_to_code_fmt("push QWORD [%s]\n", v.name);

		rsp_off++; // push

		break;

	case EXPR_ADD:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rax\npop rdx\nadd rax, rdx\npush rax\n");

		rsp_off--; // pop

		break;

	case EXPR_BIGGER:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\njg _bigger%d\npush 0\njmp _end%d\n_bigger%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		rsp_off--; // pop, pop, push

		clause_counter++;

		break;

	case EXPR_SMALLER:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\njl _smaller%d\npush 0\njmp _end%d\n_smaller%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		rsp_off--; // pop, pop, push

		clause_counter++;

		break;

	case EXPR_EQUAL:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\nje _equal%d\npush 0\njmp _end%d\n_equal%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		rsp_off--; // pop, pop, push

		clause_counter++;

		break;

	case EXPR_NEQUAL:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rdx\npop rax\ncmp rax, rdx\njne _nequal%d\npush 0\njmp _end%d\n_nequal%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		rsp_off--; // pop, pop, push

		clause_counter++;

		break;

	case EXPR_NOT:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\nje _zero%d\npush 0\njmp _end%d\n_zero%d:\npush 1\n_end%d:\n", clause_counter, clause_counter, clause_counter, clause_counter);

		// pop, push

		clause_counter++;

		break;

	case EXPR_AND:

		clause_number = clause_counter;

		clause_counter++;

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _firstequal%d\npush 0\njmp _end%d\n_firstequal%d:\n", clause_number, clause_number, clause_number);

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _secondequal%d\npush 0\njmp _end%d\n_secondequal%d:\npush 1\n_end%d:\n", clause_number, clause_number, clause_number, clause_number);

		// pop, push, pop, push

		break;

	case EXPR_OR:

		clause_number = clause_counter;

		clause_counter++;

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _equal%d\n", clause_number);

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code_fmt("pop rax\ncmp rax, 0\njne _equal%d\npush 0\njmp _end%d\n_equal%d:\npush 1\n_end%d:\n", clause_number, clause_number, clause_number, clause_number);

		// pop, push, pop, push

		break;

	case EXPR_SUB:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("pop rdx\npop rax\nsub rax, rdx\npush rax\n");

		rsp_off--; // pop, pop, push

		break;

	case EXPR_MULT:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("xor rdx, rdx\npop rbx\npop rax\nmul rbx\npush rax\n");

		rsp_off--; // pop, pop, push

		break;

	case EXPR_DIV:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("xor rdx, rdx\npop rbx\npop rax\ndiv rbx\npush rax\n");

		rsp_off--; // pop, pop, push

		break;

	case EXPR_MOD:

		gen_assemble(scope, (expr_node *)((expr_binop *)node)->lhs);
		gen_assemble(scope, (expr_node *)((expr_binop *)node)->rhs);

		add_to_code("xor rdx, rdx\npop rbx\npop rax\ndiv rbx\npush rdx\n");

		rsp_off--; // pop, pop, push

		break;

	case EXPR_EXIT:

		gen_assemble(scope, ((expr_exit *)node)->exit_code);

		add_to_code("pop rdi\n");

		rsp_off--; // pop

		add_to_code("mov rax, 60\nsyscall\n");

		break;

	default:
		fprintf(stderr, "unknown expression %d, aborting\n", node->type);
		exit(1);

	}

}