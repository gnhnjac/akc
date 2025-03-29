#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "parser.h"
#include "allocator.h"

expr_scope glob_node;

int tkn_idx;

static arena parser_arena;

static token *tokens;

size_t tkn_count;

static inline token parser_peek()
{

	if (tkn_idx >= tkn_count)
	{

		token t = { 0 };

		return t;

	}

	return tokens[tkn_idx];

}

static inline token parser_peek_ahead(int ahead)
{

	if (tkn_idx + ahead >= tkn_count)
	{

		token t = { 0 };

		return t;

	}

	return tokens[tkn_idx + ahead];

}

static inline token parser_consume()
{

	return tokens[tkn_idx++];

}

static inline void parser_expect_scope(expr_node *parent)
{

	if (parent->type != EXPR_SCOPE)
	{

		fprintf(stderr, "expected scope as parent, instead got %d, aborting\n", parent->type);
		exit(1);

	}

}

static inline void parser_expect_global(expr_node *parent)
{

	if (parent->type != EXPR_GLOBAL && parent->type != EXPR_SCOPE)
	{

		fprintf(stderr, "expected scope or global as parent, instead got %d, aborting\n", parent->type);
		exit(1);

	}

}

static inline void parser_expect_global_exclusive(expr_node *parent)
{

	if (parent->type != EXPR_GLOBAL)
	{

		fprintf(stderr, "expected scope or global as parent, instead got %d, aborting\n", parent->type);
		exit(1);

	}

}

void parser_init(token *tkns, size_t token_count)
{

	parser_arena = arena_create();

	tkn_idx = 0;

	glob_node.type = EXPR_GLOBAL;

	vect_init(&glob_node.body, sizeof(expr_node *));
	vect_init(&glob_node.variables, sizeof(variable));

	tokens = tkns;
	tkn_count = token_count;

}

void parser_finalize()
{

	arena_destroy(&parser_arena);

}

void parser_peek_expect(tkn_type type)
{

	if (parser_peek().type != type)
	{

		fprintf(stderr, "expected token %d, got %d instead\n", type, parser_peek().type);
		exit(1);

	}

}

void parser_consume_expect(tkn_type type)
{

	parser_peek_expect(type);

	parser_consume();

}

expr_scope *parser_gen_ast()
{

	parser_parse_expr((expr_node *)&glob_node);

	return &glob_node;

}

expr_type binop_tok_to_expr(tkn_type tok)
{

	switch(tok)
	{

	case TKN_PLUS:
		return EXPR_ADD;
	case TKN_MINUS:
		return EXPR_SUB;
	case TKN_ASTERISK:
		return EXPR_MULT;
	case TKN_FORSLASH:
		return EXPR_DIV;
	case TKN_PERCENT:
		return EXPR_MOD;
	case TKN_BIGGER:
		return EXPR_BIGGER;
	case TKN_SMALLER:
		return EXPR_SMALLER;
	case TKN_EQUAL:
		return EXPR_EQUAL;
	case TKN_NEQUAL:
		return EXPR_NEQUAL;
	case TKN_EXCL:
		return EXPR_NOT;
	case TKN_OR:
		return EXPR_OR;
	case TKN_AND:
		return EXPR_AND;
	default:
		fprintf(stderr, "unknown binop token %d, aborting\n", tok);
		exit(1);

	}

}

bool is_binop_tok(expr_type expr)
{

	switch(expr)
	{
	case TKN_PLUS: case TKN_MINUS: case TKN_BIGGER: case TKN_SMALLER: case TKN_EQUAL: case TKN_NEQUAL: case TKN_OR: case TKN_AND: case TKN_ASTERISK: case TKN_FORSLASH: case TKN_PERCENT: case TKN_EXCL:
		return true;
	default:
		return false;
	}

}

bool is_binop_expr(expr_type expr)
{

	switch(expr)
	{
	case EXPR_BIGGER: case EXPR_SMALLER: case EXPR_EQUAL: case EXPR_NEQUAL: case EXPR_OR: case EXPR_AND: case EXPR_ADD: case EXPR_SUB: case EXPR_MULT: case EXPR_DIV: case EXPR_MOD:
		return true;
	default:
		return false;
	}

}

int binop_expr_to_prec(expr_type expr)
{

	switch(expr)
	{

	case EXPR_OR: case EXPR_AND:
		return 0;
	case EXPR_BIGGER: case EXPR_SMALLER: case EXPR_EQUAL: case EXPR_NEQUAL:
		return 1;
	case EXPR_ADD: case EXPR_SUB:
		return 2;
	case EXPR_MULT: case EXPR_DIV: case EXPR_MOD:
		return 3;
	default:
		fprintf(stderr, "unknown binop expr %d, aborting\n", expr);
		exit(1);

	}

}

expr_node *parser_parse_binop(expr_node *node)
{

	switch (parser_peek().type)
	{

	case TKN_PLUS: case TKN_MINUS: case TKN_BIGGER: case TKN_SMALLER: case TKN_EQUAL: case TKN_NEQUAL: case TKN_OR: case TKN_AND: case TKN_ASTERISK: case TKN_FORSLASH: case TKN_PERCENT:

		expr_binop *binop_node = (expr_binop *)arena_alloc(&parser_arena, sizeof(expr_binop));

		binop_node->type = binop_tok_to_expr(parser_peek().type);
		binop_node->lhs = node;

		parser_consume(); // consume operation token

		binop_node->rhs = parser_parse_expr((expr_node *)binop_node);

		return parser_parse_binop((expr_node *)binop_node);

	case TKN_EXCL:

		binop_node = (expr_binop *)arena_alloc(&parser_arena, sizeof(expr_binop));

		binop_node->type = binop_tok_to_expr(parser_peek().type);

		parser_consume(); // consume operation token

		binop_node->lhs = parser_parse_expr(0);
		binop_node->rhs = 0; // single param operation

		return (expr_node *)binop_node;

	case TKN_SEMICOL: case TKN_OPENBRACK: case TKN_COMMA: case TKN_CLOSEPAREN:

		return node;

	default:

		fprintf(stderr, "expected token ';' or ')' or a binop, instead got %d, aborting\n", parser_peek().type);
		exit(1);

	}

}

expr_node *parser_parse_expr(expr_node *parent)
{

	while(parser_peek().type)
	{

		switch (parser_peek().type)
		{

		case TKN_CLOSEBRACK:

			return 0;

		case TKN_RET:

			parser_expect_scope(parent);

			parser_consume();

			expr_ret *ret_node = (expr_ret *)arena_alloc(&parser_arena, sizeof(expr_ret));

			ret_node->type = EXPR_RET;
			ret_node->value = parser_parse_expr(0);

			parser_consume_expect(TKN_SEMICOL);

			vect_insert(&((expr_scope *)parent)->body, &ret_node);

			break;

		case TKN_FUNC:
			
			parser_expect_global_exclusive(parent);

			expr_func *func_node = (expr_func *)arena_alloc(&parser_arena, sizeof(expr_func));

			func_node->type = EXPR_FUNC;

			parser_consume();

			parser_peek_expect(TKN_IDENTIFIER);

			func_node->name = arena_strdup(&parser_arena, parser_consume().value);

			expr_scope *func_body = (expr_scope *)arena_alloc(&parser_arena, sizeof(expr_scope));

			func_body->type = EXPR_SCOPE;
			
			vect_init(&func_body->body, sizeof(expr_node *));
			vect_init(&func_body->variables, sizeof(variable));

			func_body->parent = (expr_scope *)parent;

			parser_consume_expect(TKN_OPENPAREN);

			int rbp_off = -16;
			int num_params = 0;

			while(parser_peek().type == TKN_IDENTIFIER)
			{

				variable v;

				v.name = arena_strdup(&parser_arena, parser_consume().value);

				v.rbp_off = rbp_off;

				vect_insert(&func_body->variables,&v);

				rbp_off -= 8;

				num_params++;

				if (parser_peek().type == TKN_COMMA)
					parser_consume();
				else
					break;

			}

			func_node->num_params = num_params;

			parser_consume_expect(TKN_CLOSEPAREN);

			parser_consume_expect(TKN_OPENBRACK);

			parser_parse_expr((expr_node *)func_body);

			parser_consume_expect(TKN_CLOSEBRACK);

			func_node->body = func_body;

			vect_insert(&((expr_scope *)parent)->body, &func_node);

			break;

		case TKN_IF:

			parser_expect_scope(parent);

			parser_consume();

			parser_consume_expect(TKN_OPENPAREN);

			expr_branch *branch_node = (expr_branch *)arena_alloc(&parser_arena, sizeof(expr_branch));

			branch_node->type = EXPR_BRANCH;
			branch_node->condition = parser_parse_expr(0);

			parser_consume_expect(TKN_CLOSEPAREN);

			parser_consume_expect(TKN_OPENBRACK);

			expr_scope *branch_body = (expr_scope *)arena_alloc(&parser_arena, sizeof(expr_scope));

			branch_body->type = EXPR_SCOPE;
			
			vect_init(&branch_body->body, sizeof(expr_node *));
			vect_init(&branch_body->variables, sizeof(variable));

			branch_body->parent = (expr_scope *)parent;

			parser_parse_expr((expr_node *)branch_body);

			parser_consume_expect(TKN_CLOSEBRACK);

			branch_node->if_body = branch_body;

			parser_consume_expect(TKN_ELSE);

			parser_consume_expect(TKN_OPENBRACK);

			expr_scope *branch_else = (expr_scope *)arena_alloc(&parser_arena, sizeof(expr_scope));

			branch_else->type = EXPR_SCOPE;
			
			vect_init(&branch_else->body, sizeof(expr_node *));
			vect_init(&branch_else->variables, sizeof(variable));

			branch_else->parent = (expr_scope *)parent;

			parser_parse_expr((expr_node *)branch_else);

			parser_consume_expect(TKN_CLOSEBRACK);

			branch_node->else_body = branch_else;

			vect_insert(&((expr_scope *)parent)->body, &branch_node);

			break;

		case TKN_EXIT:

			parser_expect_scope(parent);

			parser_consume();

			expr_exit *exit_node = (expr_exit *)arena_alloc(&parser_arena, sizeof(expr_exit));

			exit_node->type = EXPR_EXIT;
			exit_node->exit_code = parser_parse_expr(0);

			parser_consume_expect(TKN_SEMICOL);

			vect_insert(&((expr_scope *)parent)->body, &exit_node);

			break;

		case TKN_VAR:

			parser_expect_global(parent);

			parser_consume();

			parser_peek_expect(TKN_IDENTIFIER);

			char *var_name = parser_consume().value;

			expr_decl *decl_node = (expr_decl *)arena_alloc(&parser_arena, sizeof(expr_decl));

			decl_node->type = EXPR_DECL;
			decl_node->name = arena_strdup(&parser_arena, var_name);

			if(parser_peek().type == TKN_SEMICOL)
			{

				parser_consume(); // consume the semicol token

				decl_node->value = 0;

			}
			else
			{

				parser_consume_expect(TKN_EQUALS);

				decl_node->value = parser_parse_expr(0);

				parser_consume_expect(TKN_SEMICOL);

			}

			vect_insert(&((expr_scope *)parent)->body, &decl_node);

			break;

		case TKN_IDENTIFIER:

			var_name = parser_consume().value;

			if (parser_peek().type == TKN_EQUALS)
			{

				parser_consume(); // consume equals sign

				expr_assign *assign_node = (expr_assign *)arena_alloc(&parser_arena, sizeof(expr_assign));

				assign_node->type = EXPR_ASSIGN;
				assign_node->name = arena_strdup(&parser_arena, var_name);
				assign_node->value = parser_parse_expr(0);

				parser_expect_scope(parent);

				vect_insert(&((expr_scope *)parent)->body, &assign_node);

				parser_consume_expect(TKN_SEMICOL);

			}
			else if(parser_peek().type == TKN_OPENPAREN)
			{

				parser_consume();

				expr_call *call_node = (expr_call *)arena_alloc(&parser_arena, sizeof(expr_call));

				call_node->type = EXPR_CALL;
				call_node->name = arena_strdup(&parser_arena, var_name);
				call_node->use_return = (parent && parent->type == EXPR_SCOPE) ? false : true;
				vect_init(&call_node->params, sizeof(expr_node *));

				expr_node *param = parser_parse_expr(0);

				while(param)
				{

					if (parser_peek().type == TKN_COMMA)
						parser_consume();
					else
					{

						if (param)
							vect_insert(&call_node->params, &param);

						break;
					}

					vect_insert(&call_node->params, &param);

					param = parser_parse_expr(0);

				}

				parser_consume_expect(TKN_CLOSEPAREN);

				if (parent && parent->type == EXPR_SCOPE)
				{
					parser_consume_expect(TKN_SEMICOL);
					vect_insert(&((expr_scope *)parent)->body, &call_node);
				}
				else
					return parser_parse_binop((expr_node *)call_node);

			}
			else
			{

				expr_var *var_node = (expr_var *)arena_alloc(&parser_arena, sizeof(expr_var));

				var_node->type = EXPR_VAR;
				var_node->name = arena_strdup(&parser_arena, var_name);

				if (parent && is_binop_expr(parent->type) && is_binop_tok(parser_peek().type) && binop_expr_to_prec(parent->type) > binop_expr_to_prec(binop_tok_to_expr(parser_peek().type)))
					return (expr_node *)var_node;

				return parser_parse_binop((expr_node *)var_node);

			}

			break;

		case TKN_OPENPAREN:

			parser_consume();

			expr_node *expr =  parser_parse_expr(0);

			parser_consume_expect(TKN_CLOSEPAREN);

			return expr;

		// case TKN_COMMA:

		// 	parser_consume();

		// 	break;

		case TKN_EXCL:

			return parser_parse_binop(0);

		case TKN_CONST:

			token const_tkn = parser_consume();

			expr_const *const_node = (expr_const *)arena_alloc(&parser_arena, sizeof(expr_const));

			const_node->type = EXPR_CONST;
			const_node->value = atoi(const_tkn.value);

			if (parent && is_binop_expr(parent->type) && is_binop_tok(parser_peek().type) && binop_expr_to_prec(parent->type) > binop_expr_to_prec(binop_tok_to_expr(parser_peek().type)))
				return (expr_node *)const_node;

			return parser_parse_binop((expr_node *)const_node);

		default:

			fprintf(stderr, "unknown token %d, aborting\n", parser_peek().type);
			exit(1);

		}

	}

	return 0;

}