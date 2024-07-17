#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"

// Bind a variable name to a given integer value.
// Returns the index in the vtable.
// Does not detect failure.
int ivbind(struct ivtable_index* vtable, char* id, int val) {
	for (int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id, id) == 0) {
			vtable->vs[i].val = val;
			return i;
		}
	}
	vtable->n += 1;
	vtable->vs = realloc(vtable->vs, sizeof(*vtable->vs) * vtable->n);
	vtable->vs[vtable->n - 1].id = id;
	vtable->vs[vtable->n - 1].val = val;
	return vtable->n - 1;
}

void vbind(struct vtable_index* vtable, char* id, void* val, unsigned int val_length) {
	/*
	for (int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id,id) == 0) {
			vtable->vs[i].val = malloc(val_length);
			memcpy(vtable->vs[i].val, val, val_length);
			vtable->vs[i].val_length = val_length;
			return;
		}
	}
	vtable->n += 1;
	vtable->vs = realloc(vtable->vs, sizeof(*vtable->vs) * vtable->n);
	vtable->vs[vtable->n - 1].id = id;
	vtable->vs[vtable->n - 1].val = malloc(val_length);
	memcpy(vtable->vs[vtable->n - 1].val, val, val_length);
	vtable->vs[vtable->n - 1].val_length = val_length;
	*/
	return;
}

// Find value of variable stored in vtable.
// Result is stored in-place in the 'destination' parameter.
// Returns 0 on success.
// Returns unbound_variable_name on failure.
int lookup(struct ivtable_index* vtable, char* id, int* destination) {
	for (unsigned int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id, id) == 0) {
			*destination = vtable->vs[i].val;
			return 0;
		}
	}
	return 1;
}

// Bind a function definition to the ftable.
// Parameters:
// 	ftable 	= function table.
//  id		= string name of function.
//  n_index = index of the top function node, whose parent is an n_prog, and
//			  whose children are the parameter declaration and function body.
int fbind(struct ftable_index* ftable, char* id, unsigned int node_index) {
	if (strcmp(id, "main") == 0) ftable->start_fun_node = node_index;
	for (int i = 0; i < ftable->n; i++) {
		if (strcmp(ftable->fs[i].id,id) == 0) {
			if (VERBOSE) printf("Function override. %s == %s.\n", ftable->fs[i].id, id);
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
			if (VERBOSE) printf("Adding function \"%s\" to ftable.\n", ti->ts[n_tree->nodes[i].token_indices[1]].val);
			fbind(ftable, ti->ts[n_tree->nodes[i].token_indices[1]].val, i);
		}
	}
}

// Evaluate a single node in the node tree.
// Returns an integer representing either the return value or an error_code if something failed.
// Parameters:
//	'n_tree': tree structure containing nodes.
//  'n_index': the node being evaluated.
//  'ti': the structure of tokens, used for determining information about the node.
//  'output': file-buffer opened with the "w" argument, used to log information to a file.
//  'vtable': the variable table.
//  'ftable': the function table.
int eval(struct tree* n_tree, int n_index, struct token_index *ti, FILE* output,
		struct ivtable_index* vtable, struct ftable_index* ftable) {
	if (VERBOSE) {
		printf("Evaluating (node %d): ", n_index);
		print_node(n_tree, n_index);
		printf("\n");
	}
	if (n_tree->n < n_index) return -1;
	else if (n_tree->nodes[n_index].nodetype == n_stat) {
		if (n_tree->nodes[n_index].specific_type == s_fun_bind) {
			if (n_tree->nodes[n_index].first != -1) {
				struct ivtable_index* new_vtable = malloc(sizeof(vtable));
				new_vtable->n = 0;
				//new_vtable->vs = malloc(sizeof(vtable->vs) * vtable->n);
				//memcpy(new_vtable->vs, vtable->vs, sizeof(vtable->vs) * vtable->n);
				//eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
				if (n_tree->nodes[n_index].second != -1) {
					if (VERBOSE) printf("Evaluating function body.\n");
					return eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable);
				}
				else {
					if (VERBOSE) printf("Function is missing its body.\n");
					return -1;
				}
			}
		}
		// Variable binding statement
		else if (n_tree->nodes[n_index].specific_type == s_var_bind) {
			char* var_name = ti->ts[n_tree->nodes[n_index].token_indices[1]].val;
			//size_t size_of_val = sizeof(int);
			int value = eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable);
			/*
			struct vtable_index* new_vtable = malloc(sizeof(vtable));
			new_vtable->n = vtable->n;
			new_vtable->vs = malloc(sizeof(new_vtable->vs) * new_vtable->n);
			memcpy(new_vtable->vs, vtable->vs, sizeof(vtable->vs) * vtable->n);
			vbind(new_vtable, var_name, &value, size_of_val);	
			return eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable);
			*/
			struct ivtable_index* new_vtable = malloc(sizeof(vtable));
			new_vtable->n = vtable->n;
			new_vtable->vs = malloc(sizeof(new_vtable->vs) * new_vtable->n);
			memcpy(new_vtable->vs, vtable->vs, sizeof(vtable->vs) * vtable->n);
			ivbind(new_vtable, var_name, value);
			return eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable);
		}
		else if (n_tree->nodes[n_index].specific_type == s_return) {
			return eval(n_tree, n_tree->nodes[n_index].second, ti, output, vtable, ftable);
		}
		if (VERBOSE) printf("Missing implementation. Specific type: %d\n", n_tree->nodes[n_index].specific_type);
		return -1;
	}
	// Variable/Value.
	else if (n_tree->nodes[n_index].nodetype == n_expr) {
		if (n_tree->nodes[n_index].specific_type == e_id) {
			/*void* result = malloc(sizeof(int));
			if (n_tree->nodes[n_index].token_count == 0) {
				printf("No variable name info passed along.\n");
			}
			else {
				for (int i = 0; i < vtable->n; i++) {
					if (strcmp(vtable->vs[i].id, seeking) == 0) {
						memcpy(result, vtable->vs[i].val, sizeof(int));
						break;
					}
				}
				printf("Found number\n");
				int number = (int) result;
				free(result);
				return number;
			}*/
			char* seeking = ti->ts[n_tree->nodes[n_index].token_indices[0]].val;
			int result;
			lookup(vtable, seeking, &result);
			return result;
		}	
		else if (n_tree->nodes[n_index].specific_type == e_val) {
			if (n_tree->nodes[n_index].token_count <= 0) {
				if (VERBOSE) printf("Evaluation error: Attempting to bind a value to a variable,\n");
				if (VERBOSE) printf("but no name or value was found. May have roots in an uncaught parsing bug/error.\n");
			}
			char* saved_value = ti->ts[n_tree->nodes[n_index].token_indices[0]].val;
			if (VERBOSE) printf("Found saved value: %s\n", saved_value);
			int value = atoi(saved_value);
			if (VERBOSE) printf("Returning value: %d\n", value);
			return value;
		}
		// Binary operations
		else if (n_tree->nodes[n_index].specific_type == e_binop) {
			struct node* n = &n_tree->nodes[n_index];
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (VERBOSE) printf("Evaluation error: No first child node found in binary operation (node %d).\n", n_index);
					return -5;
				}
				if (n->second == -1) {
					if (VERBOSE) printf("Evaluation error: No second child node found in binary operation (node %d).\n", n_index);
					return -5;
				}
				int result1 = eval(n_tree, n->first, ti, output, vtable, ftable);
				int result2 = eval(n_tree, n->second, ti, output, vtable, ftable);
				char* operator = ti->ts[n->token_indices[0]].val;
				if (VERBOSE) printf("Calculating %d %s %d\n", result1, operator, result2);
				if 		(strcmp(operator, "+") == 0) return result1 + result2;
				else if (strcmp(operator, "-") == 0) return result1 - result2;
				else if (strcmp(operator, "*") == 0) return result1 * result2;
				else if (strcmp(operator, "/") == 0) return result1 / result2;
				else { if (VERBOSE) printf("No BINOP operator specified in node.\n"); }
			}
			else {
				if (VERBOSE) printf("Evaluation error: No operator found for binary operation (node %d). The token is not connected.\n", n_index);
				return -4;
			}
		}
	}
	return -2;
}

int start_eval(struct tree* n_tree, int n_index, struct token_index* ti,
		FILE* output, struct ivtable_index* vtable, struct ftable_index* ftable) {
	if (n_tree->n < n_index) return -1;
	//struct ftable_index* new_ftable = malloc(sizeof(ftable));
	//new_ftable->n = ftable->n;
	//new_ftable->fs = malloc(sizeof(new_table->fs) * new_ftable->n);
	//memcpy(new_ftable->fs, ftable->fs, sizeof(ftable->fs) * ftable->n);
	//assemble_ftable(n_tree, n_index, ti, output, vtable, ftable);
	careful_build_ftable(n_tree, ti, output, ftable);
	if (VERBOSE) {
		printf("Assembled ftable (length %d):\n", ftable->n);
		for (int f = 0; f < ftable->n; f++) {
			print_node(n_tree, ftable->fs[f].node_index);
			printf("\n");
		}
		printf("Now evaluating main node.\n");
	}
	return eval(n_tree, ftable->start_fun_node, ti, output, vtable, ftable);
}