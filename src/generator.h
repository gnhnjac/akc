#pragma once

#include "parser.h"

typedef enum
{

	OP_MOV_REG = 1,
	OP_MOV_IMM,
	OP_ADD_REG

} op_type;

void gen_init();
void add_to_code(char *text);
//void gen_assemble_op(expr_node *node, char *dst, size_t sz, op_type op);
char *gen_generate_assembly(expr_scope *global_node);
void gen_assemble(expr_scope *scope, expr_node *node);