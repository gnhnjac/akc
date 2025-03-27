#pragma once
#include "vector.h"
#include "lexer.h"

typedef enum
{

	EXPR_GLOBAL = 1,
	EXPR_FUNCSCOPE,
	EXPR_SUBSCOPE,
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
	EXPR_EXIT

} expr_type;

typedef struct
{

	char *name;

} variable;

typedef struct
{

	expr_type type;

} expr_node;

typedef struct _expr_scope
{

	expr_type type;

	vector body;

	vector variables;

	vector subscope_variables;

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

	expr_node *lhs;

	expr_node *rhs;

} expr_compare;

typedef struct
{

	expr_type type;

	expr_node *condition;
	expr_scope *if_body;
	expr_scope *else_body;

} expr_branch;

void parser_init(token *tkns, size_t token_count);
expr_scope *parser_gen_ast();
expr_node *parser_parse_expr(expr_node *parent);