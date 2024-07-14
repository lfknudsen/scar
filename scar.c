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

void vbind(struct vtable_index* vtable, char* id, void* val, unsigned int val_length) {
	for (int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id,id) == 0) {
			vtable->vs[i].val = val;
			vtable->vs[i].val_length = val_length;
			return;
		}
	}
	vtable->n += 1;
	vtable->vs = realloc(vtable->vs, sizeof(*vtable->vs) * vtable->n);
	vtable->vs[vtable->n - 1].id = id;
	vtable->vs[vtable->n - 1].val = val;
	vtable->vs[vtable->n - 1].val_length = val_length;
	return;
}

int lookup(struct vtable_index* vtable, char* id, void* destination) {
	for (int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id, id) == 0) {
			memcpy(destination, vtable->vs[i].val, vtable->vs[i].val_length);
			return 0;
		}
	}
	return 1;
}

int fbind(struct ftable_index* ftable, char* id, unsigned int node_index) {
	if (strcmp(id, "main") == 0) ftable->main_function = node_index;
	for (int i = 0; i < ftable->n; i++) {
		if (strcmp(ftable->fs[i].id,id) == 0) {
			ftable->fs[i].node_index = node_index;
			return 0;
		}
	}
	ftable->n += 1;
	ftable->fs = realloc(ftable->fs, sizeof(*ftable->fs) * ftable->n);
	ftable->fs[ftable->n - 1].id = id;
	ftable->fs[ftable->n - 1].node_index = node_index;
	return 0;
}

int assemble_ftable(struct tree* n_tree, int n_index, struct token_index* ti,
		FILE* output, struct vtable_index* vtable, struct ftable_index* ftable) {
	if (n_tree->nodes[n_index].nodetype == n_prog) {
		if (n_tree->nodes[n_index].first != -1) {
			return assemble_ftable(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
		}
		if (n_tree->nodes[n_index].second != -1) {
			return assemble_ftable(n_tree, n_tree->nodes[n_index].second, ti, output, vtable, ftable);
		}
	}
	else if (n_tree->nodes[n_index].nodetype == n_stat &&
		n_tree->nodes[n_index].specific_type == s_fun_bind) {
		return fbind(ftable, ti->ts[n_tree->nodes[n_index].token_indices[0]].val, n_index);
	}
}

int eval(struct tree* n_tree, int n_index, struct token_index *ti, FILE* output,
		struct vtable_index* vtable, struct ftable_index* ftable) {
	if (n_tree->n < n_index) return -1;
	else if (n_tree->nodes[n_index].nodetype == n_stat) {
		if (n_tree->nodes[n_index].specific_type == s_fun_bind) {
			if (n_tree->nodes[n_index].first != -1) {
				struct vtable_index* new_vtable = malloc(sizeof(vtable));
				new_vtable->n = 0;
				//new_vtable->vs = malloc(sizeof(vtable->vs) * vtable->n);
				//memcpy(new_vtable->vs, vtable->vs, sizeof(vtable->vs) * vtable->n);
				//eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
				if (n_tree->nodes[n_index].second != -1)
					return eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable);
				else {
					printf("Function is missing its body.\n"); return -1;
				}
			}
		}
		// Variable binding statement
		else if (n_tree->nodes[n_index].specific_type == s_var_bind) {
			char* var_name = ti->ts[n_tree->nodes[n_index].token_indices[1]].val;
			size_t size_of_val = sizeof(int);
			int value = eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
			struct vtable_index* new_vtable = malloc(sizeof(vtable));
			new_vtable->n = vtable->n;
			new_vtable->vs = malloc(sizeof(new_vtable->vs) * new_vtable->n);
			memcpy(new_vtable->vs, vtable->vs, sizeof(vtable->vs) * vtable->n);
			vbind(new_vtable, ti->ts[n_tree->nodes[n_index].token_indices[0]].val, &value, size_of_val);	
			return eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable);
		}
		else if (n_tree->nodes[n_index].specific_type == s_return) {
			return eval(n_tree, n_tree->nodes[n_index].second, ti, output, vtable, ftable);
		}
		printf("Missing implementation. Specific type: %d\n", n_tree->nodes[n_index].specific_type);
		return -1;
	}
	// Variable/Value
	else if (n_tree->nodes[n_index].nodetype == n_expr) {
		if (n_tree->nodes[n_index].specific_type == e_id) {
			int result;
			int success = lookup(vtable, ti->ts[n_tree->nodes[n_index].token_indices[0]].val, &result);
			return result;
		}	
		else if (n_tree->nodes[n_index].specific_type == e_val) {
			int result;
			memcpy(&result, ti->ts[n_tree->nodes[n_index].token_indices[0]].val, sizeof(int));
			return result;
		}
	}
	return -2;
}

int start_eval(struct tree* n_tree, int n_index, struct token_index* ti,
		FILE* output, struct vtable_index* vtable, struct ftable_index* ftable) {
	if (n_tree->n < n_index) return -1;
	//struct ftable_index* new_ftable = malloc(sizeof(ftable));
	//new_ftable->n = ftable->n;
	//new_ftable->fs = malloc(sizeof(new_table->fs) * new_ftable->n);
	//memcpy(new_ftable->fs, ftable->fs, sizeof(ftable->fs) * ftable->n);
	assemble_ftable(n_tree, n_index, ti, output, vtable, ftable);
	return eval(n_tree, ftable->fs[ftable->main_function].node_index, ti, output, vtable, ftable);
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
		printf("Could not create/write to file. Check permissions.\n");
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
		FILE* token_file = fopen("tokens.out", "r");
		if (token_file == NULL) { printf("Token file not found.\n"); return 1; }
		FILE* node_file = fopen("nodes.out", "w");
		if (node_file == NULL) {
			printf("Could not create node file.\n");
			fclose(token_file);
			return 1;
		}
		struct tree* node_tree = parse(token_file, node_file, ti);
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
		fclose(node_file);
		struct vtable_index* vtable = malloc(sizeof(vtable));
		struct ftable_index* ftable = malloc(sizeof(ftable));
		ftable->main_function = 0;
		ftable->n = 0;
		FILE* eval_output = fopen("eval.out", "w");
		printf("Evaluated: %c\n", start_eval(node_tree, 0, ti, eval_output, vtable, ftable));
		fclose(eval_output);
		for (int f = 0; f < ftable->n; f++) {
			printf("%3d %s %d", f, ftable->fs[f].id, ftable->fs[f].node_index);
			if (ftable->main_function == f) printf(" (main)\n");
			else printf("\n");
		}
	}
	free_tokens(ti);
	return 0;
}
