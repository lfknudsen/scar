#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "scar.h"
#include "lexer.h"
#include "parser.h"

#define END_STATE 7

// For now, grammar is as follows:
// FunDec 		-> Type Id ( FunArgs ) = { BodyStats }
// BodyStats 	-> Stats ; BodyStats
// BodyStats 	-> return Exp ;
// BodyStats	-> return ;
// Type 		-> int
// Type 		-> float
// Type 		-> string
// FunArgs 		-> Type Id, FunArgs
// FunArgs 		-> Type Id
// Stats 		-> Stat ; Stats
// Stats 		-> Stat ;
// Stat 		-> Type Id = Exp
// Exp 			-> Exp Binop Exp
// Exp			-> Val
// Val			-> num
// Val			-> id
// Binop		-> +
// Binop		-> -

/*
DFA:
s0 -> s1
s1 -> Type Id ( -> s2
s1 -> -> END
s2 -> Type Id -> s3
s3 -> , -> s2
s3 -> ) = { -> s4
s4 -> Type Id = Exp ; -> s5
s5 -> Type Id = Exp ; -> s4
s5 -> return ; -> s1
*/

int fail_input() {
	printf("Scar Compiler usage:\n./scar <filename>");
	return 1;
}

int main(int argc, char* argv[]) {
	if (argc <= 1 || strcmp(argv[1],"--help") == 0 || strcmp(argv[1],"-h") == 0 || argc != 2 ) {
		return fail_input();
	}
	printf("Input: %s\n", argv[1]);

	FILE *read_ptr;
	read_ptr = fopen(argv[1], "r");
	if (read_ptr == NULL) { printf("File not found.\n"); return fail_input(); }
	FILE *write_ptr;
	write_ptr = fopen("tokens.out", "w");
	if (write_ptr == NULL) {
		printf("Could not create/write to file.\n");
		fclose(read_ptr);
		return 1;
	}
	struct token_index *t_i = malloc(sizeof(*t_i));
	t_i->ts = malloc(sizeof(*t_i->ts));
	t_i->n = 0;
	t_i->n = lex(read_ptr, write_ptr, t_i);
	printf("Read %lu tokens.\n", t_i->n);
	fclose(read_ptr);
	fclose(write_ptr);

	if (t_i->n) {
		for (int i = 0; i < t_i->n; i++) {
			printf("Token #%d: %s (%u)\n", i, t_i->ts[i].val, t_i->ts[i].type);
		}
		FILE *token_file = fopen("tokens.out", "r");
		if (token_file == NULL) { printf("Token file not found.\n"); return 1; }
		printf("%d\n", parse(token_file, t_i));
		fclose(token_file);
	}
	return 0;
}
