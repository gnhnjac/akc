#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "vector.h"
#include "allocator.h"

size_t text_idx;
char *text;
size_t line = 1;
size_t col = 0;

static arena lex_arena;

static vector tokens;

static inline char lex_peek()
{

	return text[text_idx];

}

static inline char lex_peek_ahead(int ahead)
{

	return text[text_idx + ahead];

}

static inline char lex_consume()
{

	col++;

	return text[text_idx++];

}

void lex_init(char *fname)
{

	lex_arena = arena_create();

	text_idx = 0;

	FILE *fptr = fopen(fname, "r");

	if (!fptr)
	{
	  fprintf(stderr, "unable to open file %s\n", fname);
	  exit(1);
	}

	// read file size
	fseek(fptr, 0L, SEEK_END);
	size_t fsize = ftell(fptr);
	rewind(fptr);

	// read file text into buffer
	text = (char *)arena_alloc(&lex_arena, sizeof(char) * fsize + 1);
	fread(text, fsize, 1, fptr);

	fclose(fptr);

	text[fsize] = 0;

	vect_init(&tokens, sizeof(token));

}

void lex_finalize()
{

	arena_destroy(&lex_arena);

}

vector lex_tokenize()
{

	while(lex_peek())
	{

		char c = lex_peek();

		if (isalpha(c))
		{

			int start_idx = text_idx;

			while(lex_peek() && isalpha(lex_peek()))
				lex_consume();

			size_t str_size = text_idx - start_idx;

			tkn_type type = TKN_IDENTIFIER;

			if (str_size == 4 && strncmp(&text[start_idx], "exit", str_size) == 0)
			{

				type = TKN_EXIT;

			}
			else if (str_size == 3 && strncmp(&text[start_idx], "var", str_size) == 0)
			{

				type = TKN_VAR;

			}
			else if (str_size == 2 && strncmp(&text[start_idx], "if", str_size) == 0)
			{
				
				type = TKN_IF;

			}
			else if (str_size == 4 && strncmp(&text[start_idx], "else", str_size) == 0)
			{

				type = TKN_ELSE;

			}
			else if (str_size == 4 && strncmp(&text[start_idx], "func", str_size) == 0)
			{

				type = TKN_FUNC;

			}
			else if (str_size == 6 && strncmp(&text[start_idx], "return", str_size) == 0)
			{

				type = TKN_RET;

			}

			col -= str_size;

			if (type == TKN_IDENTIFIER)
				add_token(type, &text[start_idx], str_size);
			else
				add_token(type, 0, 0);

			col += str_size;

		}
		else if(isdigit(c))
		{

			int start_idx = text_idx;

			while(lex_peek() && isdigit(lex_peek()))
				lex_consume();

			size_t str_size = text_idx - start_idx;

			add_token(TKN_CONST, &text[start_idx], str_size);

		}
		else if(isspace(c))
		{

			lex_consume();

			if (c == '\n')
			{
				line++;
				col = 0;
			}

		}
		else
		{

			switch (c)
			{
			case ';':
				add_token(TKN_SEMICOL,0,0);
				break;
			case '=':
				if (lex_peek_ahead(1) == '=')
				{
					lex_consume();
					add_token(TKN_EQUAL,0,0);
				}
				else
					add_token(TKN_EQUALS,0,0);
				break;
			case '(':
				add_token(TKN_OPENPAREN,0,0);
				break;
			case ')':
				add_token(TKN_CLOSEPAREN,0,0);
				break;
			case '{':
				add_token(TKN_OPENBRACK,0,0);
				break;
			case '}':
				add_token(TKN_CLOSEBRACK,0,0);
				break;
			case ',':
				add_token(TKN_COMMA,0,0);
				break;
			case '+':
				add_token(TKN_PLUS,0,0);
				break;
			case '-':
				add_token(TKN_MINUS,0,0);
				break;
			case '*':
				add_token(TKN_ASTERISK,0,0);
				break;
			case '/':
				add_token(TKN_FORSLASH,0,0);
				break;
			case '%':
				add_token(TKN_PERCENT,0,0);
				break;
			case '>':
				add_token(TKN_BIGGER,0,0);
				break;
			case '<':
				add_token(TKN_SMALLER,0,0);
				break;
			case '!':
				if (lex_peek_ahead(1) == '=')
				{
					lex_consume();
					add_token(TKN_NEQUAL,0,0);
				}
				else
					add_token(TKN_EXCL,0,0);
				break;
			case '|':
				if (lex_peek_ahead(1) == '|')
				{
					lex_consume();
					add_token(TKN_OR,0,0);
				}
				else
					add_token(TKN_PIPE,0,0);
				break;

			case '&':
				if (lex_peek_ahead(1) == '&')
				{
					lex_consume();
					add_token(TKN_AND,0,0);
				}
				else
					add_token(TKN_AMPR,0,0);
				break;
			default:
				fprintf(stderr, "unknown symbol %c, aborting\n", c);
				exit(1);
			}

			lex_consume();


		}

	}

	token v = { 0 };

	vect_insert(&tokens, &v);

	return tokens;

}

void add_token(tkn_type type, char *val, size_t val_len)
{

	token v = { 0 };

	v.type = type;

	if (val)
	{

		v.value = (char *)arena_alloc(&lex_arena, sizeof(char) * val_len + 1);
		memcpy(v.value, val, val_len);

		v.value[val_len] = 0;

	}

	v.line = line;
	v.col = col;

	vect_insert(&tokens, &v);

}