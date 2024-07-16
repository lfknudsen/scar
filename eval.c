#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"

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
	if (strcmp(id, "main") == 0) ftable->start_fun_node = node_index;
	for (int i = 0; i < ftable->n; i++) {
		if (strcmp(ftable->fs[i].id,id) == 0) {
			printf("Function override. %s == %s.\n", ftable->fs[i].id, id);
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
		return fbind(ftable, ti->ts[n_tree->nodes[n_index].token_indices[1]].val, n_index);
	}
}

// TODO: Refactor so that ftable is built during parsing or at least using other function.
// This function just makes sure to go through every node, nice if I want to change
// how the node structure works later. It's also less reliant on parsing working correctly,
// making it easier to spot where mistakes are happening.
int careful_build_ftable(struct tree* n_tree, struct token_index* ti,
		FILE* output, struct ftable_index* ftable) {
	for (int i = 0; i < n_tree->n; i++) {
		if (n_tree->nodes[i].nodetype == n_stat &&
			n_tree->nodes[i].specific_type == s_fun_bind) {
			printf("Adding function \"%s\" to ftable.\n", ti->ts[n_tree->nodes[i].token_indices[1]].val);
			fbind(ftable, ti->ts[n_tree->nodes[i].token_indices[1]].val, i);
		}
	}
}

int eval(struct tree* n_tree, int n_index, struct token_index *ti, FILE* output,
		struct vtable_index* vtable, struct ftable_index* ftable) {
	printf("Evaluating (node %d): ", n_index);
	print_node(n_tree, n_index);
	printf("\n");
	if (n_tree->n < n_index) return -1;
	else if (n_tree->nodes[n_index].nodetype == n_stat) {
		if (n_tree->nodes[n_index].specific_type == s_fun_bind) {
			if (n_tree->nodes[n_index].first != -1) {
				struct vtable_index* new_vtable = malloc(sizeof(vtable));
				new_vtable->n = 0;
				//new_vtable->vs = malloc(sizeof(vtable->vs) * vtable->n);
				//memcpy(new_vtable->vs, vtable->vs, sizeof(vtable->vs) * vtable->n);
				//eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
				if (n_tree->nodes[n_index].second != -1) {
					printf("Evaluating function body.\n");
					return eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable);
				}
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
			vbind(new_vtable, ti->ts[n_tree->nodes[n_index].token_indices[1]].val, &value, size_of_val);	
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
			char* result = malloc(sizeof(int));
			int failure = lookup(vtable, ti->ts[n_tree->nodes[n_index].token_indices[0]].val, &result);
			if (failure) {
				printf("Variable \"%s\" not found.\n", ti->ts[n_tree->nodes[n_index].token_indices[0]].val);
			}
			int number = atoi(result);
			free(result);
			return number;
		}	
		else if (n_tree->nodes[n_index].specific_type == e_val) {
			char* result = malloc(sizeof(int));
			memcpy(result, ti->ts[n_tree->nodes[n_index].token_indices[0]].val, sizeof(int));
			int number = atoi(result);
			free(result);
			printf("Returning %d.\n", number);
			return number;
		}
		else if (n_tree->nodes[n_index].specific_type == e_binop) {
			int result1 = eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
			int result2 = eval(n_tree, n_tree->nodes[n_index].second, ti, output, vtable, ftable);
			if (n_tree->nodes[n_index].token_count > 0) {
				char* operator = ti->ts[n_tree->nodes[n_index].token_indices[0]].val;
				printf("Binary operator: %s\n", operator);
				if 		(strcmp(operator, "+") == 0) return result1 + result2;
				else if (strcmp(operator, "-") == 0) return result1 - result2;
				else if (strcmp(operator, "*") == 0) return result1 * result2;
				else if (strcmp(operator, "/") == 0) return result1 / result2;
				else { printf("No BINOP operator specified in node.\n"); }
			}
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
	//assemble_ftable(n_tree, n_index, ti, output, vtable, ftable);
	careful_build_ftable(n_tree, ti, output, ftable);
	printf("Assembled ftable (length %d):\n", ftable->n);
	for (int f = 0; f < ftable->n; f++) {
		print_node(n_tree, ftable->fs[f].node_index);
		printf("\n");
	}
	printf("Now evaluating main node.\n");
	return eval(n_tree, ftable->start_fun_node, ti, output, vtable, ftable);
}