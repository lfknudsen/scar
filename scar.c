#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "scar.h"
#include "lexer.h"
#include "parser.h"

// For now, grammar is as follows:
// FunDec 		-> Type Id ( FunArgs ) = { BodyStats }
// BodyStats 	-> Stats ; BodyStats
// BodyStats 	-> return Exp ;
// BodyStats	-> return ;
// Type 		-> int
// Type 		-> float
// FunArgs 		-> Type Id, FunArgs
// FunArgs 		-> Type Id
// FunArgs 		->
// Stats 		-> Stat ; Stats
// Stats 		-> Stat ;
// Stat 		-> Type Id = Val
// Stat			-> return Exp ;
// Stat			-> return ;
// Val			-> num
// Val			-> id
// Exp 			-> Exp Binop Exp
// Exp			-> Val
// Binop		-> +
// Binop		-> -
// Binop		-> *
// Binop		-> /

int eval(struct tree* n_tree, int n_index, struct token_index *ti, void* output) {
	if (n_tree->n < n_index) return -1;
	if (n_tree->nodes[n_index].nodetype == n_prog) {
		if (n_tree->nodes[n_index].first != -1)
			eval(n_tree, n_tree->nodes[n_index].first, ti, output);
		if (n_tree->nodes[n_index].second != -1)
			eval(n_tree, n_tree->nodes[n_index].second, ti, output);
		return 0;
	}
	if (n_tree->nodes[n_index].nodetype == n_stat && n_tree->nodes[n_index].specific_type == s_var_bind) {
		char* var_name = ti->ts[n_tree->nodes[n_index].token_indices[1]].val;
		size_t size_of_var_val;
		switch (ti->ts[n_tree->nodes[n_index].token_indices[1]].type) {
			case t_type_int:
				size_of_var_val = sizeof(int);
				break;
			case t_type_float:
				size_of_var_val = sizeof(float);
				break;
			case t_type_string:
				size_of_var_val = sizeof(char);
				break;
			default:
				size_of_var_val = 0;
				break;
		}
		void* var_val = malloc(size_of_var_val);
		int result = eval(n_tree, n_tree->nodes[n_index].second, ti, var_val);
		if (result == -1) return -1;
		else return 1;
	}
	if (n_tree->nodes[n_index].nodetype == n_expr &&
		(n_tree->nodes[n_index].specific_type == e_val || n_tree->nodes[n_index].specific_type == e_id))
		{

		}
	return -1;
}

void print_node(struct tree* node_tree, int n_index) {
	switch (node_tree->nodes[n_index].nodetype) {
		case n_stat:
			switch (node_tree->nodes[n_index].specific_type) {
				case s_var_bind:
					printf("Statement: Variable bind");
					return;
				case s_fun_bind:
					printf("Statement: Function bind");
					return;
				case s_function_call:
					printf("Statement: Function call");
					return;
				case s_if:
					printf("Statement: If");
					return;
				case s_else:
					printf("Statement: Else");
					return;
				case s_return:
					printf("Statement: Return");
					return;
				case s_fun_body:
					printf("Statement: Function body");
					return;
			}
		case n_expr:
			switch (node_tree->nodes[n_index].specific_type) {
				case e_id:
					printf("Expression: Variable name");
					return;
				case e_val:
					printf("Expression: Value");
					return;
				case e_binop:
					printf("Expression: Binary operation");
					return;
				case e_param:
					printf("Expression: Parameter declaration");
					return;
			}
		case n_prog:
			printf("Top-level program");
			return;
		default:
			return;
	}
	return;
}

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
			printf("Token #%d: \n", i);
			for (int c = 0; c < strlen(ti->ts[i].val); c++) {
				printf("%c", ti->ts[i].val[c]);
			}
			printf("\n");
			printf("%s (%u)\n", ti->ts[i].val, ti->ts[i].type);
		}
		FILE *token_file = fopen("tokens.out", "r");
		if (token_file == NULL) { printf("Token file not found.\n"); return 1; }
		struct tree* node_tree = parse(token_file, ti);
		if (node_tree == NULL) { printf("Could not parse file.\n"); return 1; }
		printf("Nodes: %d\n", node_tree->n);
		for (int i = 0; i < node_tree->n; i++) {
			int indent = node_tree->nodes[i].print_indent;
			for (int ind = 0; ind < indent; ind++) {
					if (ind == node_tree->nodes[node_tree->nodes[i].parent].print_indent) printf("| ");
					else printf("  ");
			}

			printf("Node #%d. ", i);
			print_node(node_tree, i);
			printf(". Parent: %d. ", node_tree->nodes[i].parent);
			printf("1: %d. 2: %d\n", node_tree->nodes[i].first, node_tree->nodes[i].second);

			if (node_tree->nodes[i].token_count > 0) {
				for (int ind = 0; ind < indent; ind++) {
					if (ind == node_tree->nodes[node_tree->nodes[i].parent].print_indent) printf("| ");
					else printf("  ");
			}
				for (int j = 0; j < node_tree->nodes[i].token_count; j++) {
					for (int c = 0;
						c < strlen(ti->ts[node_tree->nodes[i].token_indices[j]].val); c++) {
							putchar(ti->ts[node_tree->nodes[i].token_indices[j]].val[c]);
						}
						printf(" ");
				}
				printf("\n");
			}
		}
		fclose(token_file);
	}
	free_tokens(ti);
	return 0;
}
