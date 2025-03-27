#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "parser.h"

expr_scope glob_node;

int tkn_idx;

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

	if (parent->type != EXPR_GLOBAL && parent->type != EXPR_FUNCSCOPE && parent->type != EXPR_SUBSCOPE)
	{

		fprintf(stderr, "expected scope as parent, instead got %d, aborting\n", parent->type);
		exit(1);

	}

}

void parser_init(token *tkns, size_t token_count)
{

	tkn_idx = 0;

	glob_node.type = EXPR_GLOBAL;

	vect_init(&glob_node.body, sizeof(expr_node *));
	vect_init(&glob_node.variables, sizeof(variable));
	vect_init(&glob_node.subscope_variables, sizeof(variable));

	tokens = tkns;
	tkn_count = token_count;

}

void parser_peek_expect(tkn_type type)
{

	if (parser_peek().type != type)
	{

		fprintf(stderr, "expected token %d, got %d instead", type, parser_peek().type);
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

	tkn_idx = 0;

	while (tokens[tkn_idx].type)
	{

		if (tokens[tkn_idx].value)
			free(tokens[tkn_idx].value);

		tkn_idx++;

	}

	free(tokens);

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

expr_node *parser_parse_binop(expr_node *node)
{

	switch (parser_peek().type)
	{

	case TKN_PLUS: case TKN_MINUS: case TKN_BIGGER: case TKN_SMALLER: case TKN_EQUAL: case TKN_NEQUAL: case TKN_OR: case TKN_AND:

		expr_binop *binop_node = (expr_binop *)malloc(sizeof(expr_binop));

		binop_node->type = binop_tok_to_expr(parser_peek().type);
		binop_node->lhs = node;

		parser_consume(); // consume operation token

		binop_node->rhs = parser_parse_expr(0);

		return (expr_node *)binop_node;

	case TKN_EXCL:

		binop_node = (expr_binop *)malloc(sizeof(expr_binop));

		binop_node->type = binop_tok_to_expr(parser_peek().type);

		parser_consume(); // consume operation token

		binop_node->lhs = parser_parse_expr(0);
		binop_node->rhs = 0; // single param operation

		return (expr_node *)binop_node;

	case TKN_ASTERISK: case TKN_FORSLASH: case TKN_PERCENT:

		expr_binop *binop_node_inner = (expr_binop *)malloc(sizeof(expr_binop));

		binop_node_inner->type = binop_tok_to_expr(parser_peek().type);
		binop_node_inner->lhs = node;

		parser_consume(); // consume operation token

		binop_node_inner->rhs = parser_parse_expr((expr_node *)binop_node_inner);

		return parser_parse_binop((expr_node *)binop_node_inner);

	case TKN_CLOSEPAREN:

		parser_consume();

		return node;

	case TKN_SEMICOL: case TKN_OPENBRACK:

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

			parser_consume();

			return parent;

		case TKN_IF:

			parser_expect_scope(parent);

			parser_consume();

			parser_peek_expect(TKN_OPENPAREN);

			expr_branch *branch_node = (expr_branch *)malloc(sizeof(expr_branch));

			branch_node->type = EXPR_BRANCH;
			branch_node->condition = parser_parse_expr(0);

			parser_consume_expect(TKN_OPENBRACK);

			expr_scope *branch_body = (expr_scope *)malloc(sizeof(expr_scope));

			branch_body->type = EXPR_SUBSCOPE;
			
			vect_init(&branch_body->body, sizeof(expr_node *));
			vect_init(&branch_body->variables, sizeof(variable));

			branch_body->parent = (expr_scope *)parent;

			parser_parse_expr((expr_node *)branch_body);

			branch_node->if_body = branch_body;

			parser_consume_expect(TKN_ELSE);

			parser_consume_expect(TKN_OPENBRACK);

			expr_scope *branch_else = (expr_scope *)malloc(sizeof(expr_scope));

			branch_else->type = EXPR_SUBSCOPE;
			
			vect_init(&branch_else->body, sizeof(expr_node *));
			vect_init(&branch_else->variables, sizeof(variable));

			branch_else->parent = (expr_scope *)parent;

			parser_parse_expr((expr_node *)branch_else);

			branch_node->else_body = branch_else;

			vect_insert(&((expr_scope *)parent)->body, &branch_node);

			break;

		case TKN_EXIT:

			parser_expect_scope(parent);

			parser_consume();

			expr_exit *exit_node = (expr_exit *)malloc(sizeof(expr_exit));

			exit_node->type = EXPR_EXIT;
			exit_node->exit_code = parser_parse_expr(0);

			parser_consume_expect(TKN_SEMICOL);

			vect_insert(&((expr_scope *)parent)->body, &exit_node);

			break;

		case TKN_VAR:

			parser_expect_scope(parent);

			parser_consume();

			parser_peek_expect(TKN_IDENTIFIER);

			if(parser_peek_ahead(1).type == TKN_SEMICOL)
			{

				char *var_name = parser_consume().value;

				parser_consume(); // consume the semicol token

				expr_decl *decl_node = (expr_decl *)malloc(sizeof(expr_decl));

				decl_node->type = EXPR_DECL;
				decl_node->name = strdup(var_name);

				vect_insert(&((expr_scope *)parent)->body, &decl_node);

			}
			else
			{

				fprintf(stderr, "expected token ';' got %d instead\n", parser_peek().type);
				exit(1);

			}

			break;

		case TKN_IDENTIFIER:

			char *var_name = parser_consume().value;

			if (parser_peek().type == TKN_EQUALS)
			{

				parser_consume(); // consume equals sign

				expr_assign *assign_node = (expr_assign *)malloc(sizeof(expr_assign));

				assign_node->type = EXPR_ASSIGN;
				assign_node->name = strdup(var_name);
				assign_node->value = parser_parse_expr(0);

				parser_expect_scope(parent);

				vect_insert(&((expr_scope *)parent)->body, &assign_node);

				parser_consume_expect(TKN_SEMICOL);

			}
			else
			{

				expr_var *var_node = (expr_var *)malloc(sizeof(expr_var));

				var_node->type = EXPR_VAR;
				var_node->name = strdup(var_name);

				if (parent && (parent->type == EXPR_MULT || parent->type == EXPR_DIV || parent->type == EXPR_MOD))
					return (expr_node *)var_node;

				return parser_parse_binop((expr_node *)var_node);

			}

			break;

		case TKN_OPENPAREN:

			parser_consume();

			return parser_parse_binop(parser_parse_expr(0));

		case TKN_EXCL:

			return parser_parse_binop(0);

		case TKN_CONST:

			token const_tkn = parser_consume();

			expr_const *const_node = (expr_const *)malloc(sizeof(expr_const));

			const_node->type = EXPR_CONST;
			const_node->value = atoi(const_tkn.value);

			if (parent && (parent->type == EXPR_MULT || parent->type == EXPR_DIV || parent->type == EXPR_MOD))
				return (expr_node *)const_node;

			return parser_parse_binop((expr_node *)const_node);

		default:

			fprintf(stderr, "unknown token %d, aborting\n", parser_peek().type);
			exit(1);

		}

	}

	return 0;

}