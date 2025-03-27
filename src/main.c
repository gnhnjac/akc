#include <stdio.h>
#include <stdlib.h>
#include "vector.h"
#include "lexer.h"
#include "parser.h"
#include "generator.h"

int main(int argc, char *argv[]) {

   if (argc != 2)
   {
      fprintf(stderr, "must provide filename\n");
      return 1;
   }

   lex_init(argv[1]);

   vector tokens_vector = lex_tokenize();

   int tkn_idx = 0;

   parser_init((token *)tokens_vector.data, tokens_vector.idx);

   expr_scope *global_node = parser_gen_ast();

   gen_init();

   FILE *fptr = fopen("out.asm", "w");

   fprintf(fptr, "%s", gen_generate_assembly(global_node));

   fclose(fptr);

   system("nasm -felf64 out.asm");
   system("ld -o out out.o");

   return 0;
}