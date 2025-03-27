#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "vector.h"

int text_idx;
char *text;

static vector tokens;

static inline char lex_peek()
{

	return text[text_idx];

}

static inline char lex_consume()
{

	return text[text_idx++];

}

void lex_init(char *fname)
{

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
	text = (char *)malloc(sizeof(char) * fsize + 1);
	fread(text, fsize, 1, fptr);

	fclose(fptr);

	text[fsize] = 0;

	vect_init(&tokens, sizeof(token));

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

			if (strncmp(&text[start_idx], "exit", str_size) == 0)
			{

				type = TKN_EXIT;

			}
			else if (strncmp(&text[start_idx], "int", str_size) == 0)
			{

				type = TKN_INT;

			}

			if (type == TKN_IDENTIFIER)
				add_token(type, &text[start_idx], str_size);
			else
				add_token(type, 0, 0);

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
		}
		else
		{

			switch (c)
			{
			case ';':
				add_token(TKN_SEMICOL,0,0);
				break;
			case '=':
				add_token(TKN_EQUALS,0,0);
				break;
			default:
				fprintf(stderr, "unknown symbol %c, aborting\n", c);
				exit(1);
			}

			lex_consume();


		}

	}

	free(text);

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

		v.value = (char *)malloc(sizeof(char) * val_len + 1);
		memcpy(v.value, val, val_len);

		v.value[val_len] = 0;

	}

	vect_insert(&tokens, &v);

}