#pragma once
#include <stdbool.h>
#include "vector.h"
#include "lexer.h"

typedef enum
{

	EXPR_GLOBAL = 1,
	EXPR_SCOPE,
	EXPR_FUNC,
	EXPR_DECL,
	EXPR_ASSIGN,
	EXPR_VAR,
	EXPR_CONST,
	EXPR_BRANCH,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MULT,
	EXPR_DIV,
	EXPR_MOD,
	EXPR_NOT,
	EXPR_EQUAL,
	EXPR_NEQUAL,
	EXPR_BIGGER,
	EXPR_SMALLER,
	EXPR_OR,
	EXPR_AND,
	EXPR_EXIT,
	EXPR_CALL,
	EXPR_RET

} expr_type;

typedef struct
{

	char *name;

	int rbp_off;

} variable;

typedef struct
{

	char *name;

	size_t num_params;

} function;

typedef struct
{

	expr_type type;

} expr_node;

typedef struct _expr_scope
{

	expr_type type;

	vector body;

	vector variables;

	struct _expr_scope *parent;

} expr_scope;

typedef struct
{

	expr_type type;

	long value;

} expr_const;

typedef struct
{

	expr_type type;

	char *name;

	expr_node *value;

} expr_decl;

typedef struct
{

	expr_type type;

	char *name;

	expr_node *value; 

} expr_assign;

typedef struct
{

	expr_type type;

	char *name;

} expr_var;

typedef struct
{

	expr_type type;

	expr_node *exit_code;

} expr_exit;

typedef struct
{

	expr_type type;

	expr_node *lhs;

	expr_node *rhs;

} expr_binop;

typedef struct
{

	expr_type type;

	char *name;

	expr_scope *body;

	size_t num_params;

} expr_func;

typedef struct
{

	expr_type type;

	expr_node *condition;
	expr_scope *if_body;
	expr_scope *else_body;

} expr_branch;

typedef struct
{

	expr_type type;

	char *name;

	vector params;

	bool use_return;

} expr_call;

typedef struct
{

	expr_type type;

	expr_node *value;

} expr_ret;

void parser_finalize();
void parser_init(token *tkns, size_t token_count);
expr_scope *parser_gen_ast();
expr_node *parser_parse_expr(expr_node *parent);