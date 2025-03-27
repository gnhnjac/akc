#pragma once
#include "vector.h"
#include "lexer.h"

typedef enum
{

	EXPR_GLOBAL = 1,
	EXPR_SCOPE,
	EXPR_DECL,
	EXPR_ASSIGN,
	EXPR_VAR,
	EXPR_CONST,
	EXPR_BRANCH,
	EXPR_BINOP,
	EXPR_EQUAL,
	EXPR_NEQUAL,
	EXPR_BIGGER,
	EXPR_SMALLER,
	EXPR_EXIT

} expr_type;

typedef enum
{

	DATA_INT = 1,
	DATA_CHAR,
	DATA_WORD,
	DATA_LONG,
	DATA_PTR

} data_type;

typedef struct
{

	data_type type;

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

	data_type var_type;

	char *name;

} expr_decl;

typedef struct
{

	expr_type type;

	data_type var_type;

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

void parser_init(token *tkns, size_t token_count);
expr_scope *parser_gen_ast();
expr_node *parser_parse_expr(expr_node *parent);