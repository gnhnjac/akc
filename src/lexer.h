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
	TKN_PLUS,
	TKN_SEMICOL,
	TKN_INT

} tkn_type;

typedef struct
{

	tkn_type type;
	char *value;

} token;

void lex_init(char *fname);
vector lex_tokenize();
void add_token(tkn_type type, char *val, size_t val_len);