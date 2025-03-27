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

void parser_init(token *tkns, size_t token_count)
{

	tkn_idx = 0;

	glob_node.type = EXPR_GLOBAL;

	vect_init(&glob_node.body, sizeof(expr_node *));
	vect_init(&glob_node.variables, sizeof(variable));

	tokens = tkns;
	tkn_count = token_count;

}

void parser_consume_expect(tkn_type type)
{

	if (parser_peek().type != type)
	{

		fprintf(stderr, "expected token %d, got %d instead", type, parser_peek().type);
		exit(1);

	}

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

expr_node *parser_parse_expr(expr_node *parent)
{

	while(parser_peek().type)
	{

		switch (parser_peek().type)
		{

		case TKN_EXIT:

			parser_consume();

			expr_exit *exit_node = (expr_exit *)malloc(sizeof(expr_exit));

			exit_node->type = EXPR_EXIT;
			exit_node->exit_code = parser_parse_expr(0);

			parser_consume_expect(TKN_SEMICOL);

			vect_insert(&((expr_scope *)parent)->body, &exit_node);

			break;

		case TKN_INT:

			parser_consume();

			if (parser_peek().type != TKN_IDENTIFIER)
			{

				fprintf(stderr, "expected identifier got %d instead", parser_peek().type);
				exit(1);

			}

			if (parser_peek_ahead(1).type == TKN_EQUALS)
			{

				char *var_name = parser_consume().value;

				parser_consume(); // consume the equals token

				expr_assign *assign_node = (expr_assign *)malloc(sizeof(expr_assign));

				assign_node->type = EXPR_ASSIGN;
				assign_node->var_type = DATA_INT;
				assign_node->name = strdup(var_name);
				assign_node->value = parser_parse_expr(0);

				vect_insert(&((expr_scope *)parent)->body, &assign_node);

				parser_consume_expect(TKN_SEMICOL);

			}
			else if(parser_peek_ahead(1).type == TKN_SEMICOL)
			{

				char *var_name = parser_consume().value;

				parser_consume(); // consume the semicol token

				expr_decl *decl_node = (expr_decl *)malloc(sizeof(expr_decl));

				decl_node->type = EXPR_ASSIGN;
				decl_node->var_type = DATA_INT;
				decl_node->name = strdup(var_name);

				vect_insert(&((expr_scope *)parent)->body, &decl_node);

			}
			else
			{

				fprintf(stderr, "expected ';' or '=' got %d instead", parser_peek().type);
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
				assign_node->var_type = 0;
				assign_node->name = strdup(var_name);
				assign_node->value = parser_parse_expr(0);

				vect_insert(&((expr_scope *)parent)->body, &assign_node);

				parser_consume_expect(TKN_SEMICOL);

			}
			else
			{

				expr_var *var_node = (expr_var *)malloc(sizeof(expr_var));

				var_node->type = EXPR_VAR;
				var_node->name = strdup(var_name);

				return (expr_node *)var_node;

			}

			break;

		case TKN_CONST:

			token const_tkn = parser_consume();

			expr_const *const_node = (expr_const *)malloc(sizeof(expr_const));

			const_node->type = EXPR_CONST;
			const_node->value = atoi(const_tkn.value);

			return (expr_node *)const_node;

		default:

			fprintf(stderr, "unknown token %d, aborting\n", parser_peek().type);
			exit(1);

		}

	}

	return 0;

}