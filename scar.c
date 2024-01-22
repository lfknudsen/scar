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

void free_tokens(struct token_index *ti) {
	for (int i = 0; i < ti->n; i++) {
		free(ti->ts[i].val);
	}
	free(ti->ts);
	free(ti);
}

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
	struct token_index *ti = malloc(sizeof(*ti));
	ti->ts = malloc(sizeof(*ti->ts));
	ti->n = 0;
	ti->n = lex(read_ptr, write_ptr, ti);
	printf("Read %lu tokens.\n", ti->n);
	fclose(read_ptr);
	fclose(write_ptr);

	if (ti->n) {
		for (int i = 0; i < ti->n; i++) {
			printf("Token #%d: %s (%u)\n", i, ti->ts[i].val, ti->ts[i].type);
		}
		FILE *token_file = fopen("tokens.out", "r");
		if (token_file == NULL) { printf("Token file not found.\n"); return 1; }
		printf("%d\n", parse(token_file, ti));
		fclose(token_file);
	}
	free_tokens(ti);
	return 0;
}
