#pragma once

#include "vector.h"

typedef enum
{

	TKN_EXIT = 1,
	TKN_IDENTIFIER,
	TKN_CONST,
	TKN_OPENPAREN,
	TKN_CLOSEPAREN,
	TKN_OPENBRACK,
	TKN_CLOSEBRACK,
	TKN_EQUALS,
	TKN_EXCL,
	TKN_PLUS,
	TKN_MINUS,
	TKN_ASTERISK,
	TKN_FORSLASH,
	TKN_PERCENT,
	TKN_SEMICOL,
	TKN_PIPE,
	TKN_AMPR,
	TKN_EQUAL,
	TKN_NEQUAL,
	TKN_BIGGER,
	TKN_SMALLER,
	TKN_OR,
	TKN_AND,
	TKN_VAR,
	TKN_IF,
	TKN_ELSE

} tkn_type;

typedef struct
{

	tkn_type type;
	char *value;

} token;

void lex_init(char *fname);
vector lex_tokenize();
void add_token(tkn_type type, char *val, size_t val_len);