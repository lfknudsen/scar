#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"

// Bind a variable name to a given integer value.
// Returns the index in the vtable.
// Does not detect failure.
int ivbind(struct ivtable_index* vtable, char* id, int val, int out) {
	if (out >= verbose) printf("Entering vtable bind with id %s and value %d.\n", id, val);
	for (int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id, id) == 0) {
			vtable->vs[i].val = val;
			if (out >= verbose) {
				printf("ID already existed in vtable. Updated vtable:\n");
				for (int j = 0; j < vtable->n; j++) {
					printf("%2d   %3s   %2d\n", j, vtable->vs[j].id, vtable->vs[j].val);
				}
			}
			return i;
		}
	}
	vtable->n += 1;
	vtable->vs = realloc(vtable->vs, sizeof(*vtable->vs) * vtable->n);
	vtable->vs[vtable->n - 1].id = id;
	vtable->vs[vtable->n - 1].val = val;
	if (out >= verbose) {
		printf("ID not found in vtable already, so vtable is extended. Updated vtable:\n");
		for (int i = 0; i < vtable->n; i++) {
			printf("%2d   %3s   %2d\n", i, vtable->vs[i].id, vtable->vs[i].val);
		}
	}
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
// The value that matches the variable name is stored in-place in the 'destination' parameter.
// Returns the vtable index on success (so >= 0).
// Returns -1 on failure.
int lookup(struct ivtable_index* vtable, char* id, int* destination, int out) {
	if (out >= verbose) {
		printf("Searching vtable for \"%s\":\n", id);
	}
	for (unsigned int i = 0; i < vtable->n; i++) {
		if (out >= verbose) {
			printf("%2d   %3s   %3d\n", i, vtable->vs[i].id, vtable->vs[i].val);
		}
		if (strcmp(vtable->vs[i].id, id) == 0) {
			*destination = vtable->vs[i].val;
			return i;
		}
	}
	if (out >= verbose) printf("Could not find the variable in the vtable.\n");
	return -1;
}

// Bind a function definition to the ftable.
// Parameters:
// 	ftable 	= function table.
//  id		= string name of function.
//  n_index = index of the top function node, whose parent is an n_prog, and
//			  whose children are the parameter declaration and function body.
int fbind(struct ftable_index* ftable, char* id, unsigned int node_index, int out) {
	if (strcmp(id, "main") == 0) ftable->start_fun_node = node_index;
	for (int i = 0; i < ftable->n; i++) {
		if (strcmp(ftable->fs[i].id,id) == 0) {
			if (out >= verbose) printf("Function override. %s == %s.\n", ftable->fs[i].id, id);
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

// Find the node index of the entry in the ftable which matches the
// id argument.
// Returns the node index.
int lookup_f(struct ftable_index* ftable, char* id, int out) {
	for (int i = 0; i < ftable->n; i++) {
		if (strcmp(ftable->fs[i].id,id) == 0) {
			return ftable->fs[i].node_index;
		}
	}
	return -1;
}

int assemble_ftable(struct tree* n_tree, int n_index, struct token_index* ti,
		FILE* output, struct vtable_index* vtable, struct ftable_index* ftable, int out) {
	if (n_tree->nodes[n_index].nodetype == n_prog) {
		if (n_tree->nodes[n_index].first != -1) {
			return assemble_ftable(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable, out);
		}
		if (n_tree->nodes[n_index].second != -1) {
			return assemble_ftable(n_tree, n_tree->nodes[n_index].second, ti, output, vtable, ftable, out);
		}
	}
	else if (n_tree->nodes[n_index].nodetype == n_stat &&
		n_tree->nodes[n_index].specific_type == s_fun_bind) {
		return fbind(ftable, ti->ts[n_tree->nodes[n_index].token_indices[1]].val, n_index, out);
	}
	if (out >= standard) {
		printf("Cannot bind non-function/program node to the ftable:\n%d ", n_index);
		print_node(n_tree, n_index, out);
	}
	return -1;
}

// TODO: Refactor so that ftable is built during parsing or at least using other function.
// This function just makes sure to go through every node, nice if I want to change
// how the node structure works later. It's also less reliant on parsing working correctly,
// making it easier to spot where mistakes are happening.
int careful_build_ftable(struct tree* n_tree, struct token_index* ti,
		FILE* output, struct ftable_index* ftable, int out) {
	for (int i = 0; i < n_tree->n; i++) {
		if (n_tree->nodes[i].nodetype == n_stat &&
			n_tree->nodes[i].specific_type == s_fun_bind) {
			if (out >= verbose) printf("Adding function \"%s\" to ftable.\n", ti->ts[n_tree->nodes[i].token_indices[1]].val);
			fbind(ftable, ti->ts[n_tree->nodes[i].token_indices[1]].val, i, out);
		}
	}
	if (ftable->n == 0) {
		if (out >= standard) printf("No functions were found in the program.\n");
		return ftable->n;
	}
	return ftable->n;
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
		struct ivtable_index* vtable, struct ftable_index* ftable, int out) {
	if (out >= verbose) {
		printf("Evaluating (node %d): ", n_index);
		print_node(n_tree, n_index, out);
		printf("\n");
	}
	if (n_tree->n < n_index) return -1;
	else if (n_tree->nodes[n_index].nodetype == n_stat) {
		if (n_tree->nodes[n_index].specific_type == s_fun_bind) {
			if (n_tree->nodes[n_index].first != -1) {
				struct ivtable_index* new_vtable = malloc(sizeof(vtable));
				new_vtable->n = 0;
				if (n_tree->nodes[n_index].second != -1) {
					if (out >= verbose) printf("Evaluating function body.\n");
					int result = eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable, out);
					free_ivtable(new_vtable);
					return result;
				}
				else {
					if (out >= verbose) printf("Function is missing its body.\n");
					free_ivtable(new_vtable);
					return -1;
				}
			}
		}
		// Variable binding statement
		else if (n_tree->nodes[n_index].specific_type == s_var_bind) {
			char* var_name = ti->ts[n_tree->nodes[n_index].token_indices[1]].val;
			int value = eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable, out);
			struct ivtable_index* new_vtable = malloc(sizeof(vtable));
			new_vtable->n = vtable->n;
			new_vtable->vs = malloc(sizeof(*new_vtable->vs) * new_vtable->n);
			memcpy(new_vtable->vs, vtable->vs, sizeof(*vtable->vs) * vtable->n);
			ivbind(new_vtable, var_name, value, out);
			int result = eval(n_tree, n_tree->nodes[n_index].second, ti, output, new_vtable, ftable, out);
			free_ivtable(new_vtable);
			return result;
		}
		else if (n_tree->nodes[n_index].specific_type == s_return) {
			return eval(n_tree, n_tree->nodes[n_index].second, ti, output, vtable, ftable, out);
		}
		if (out >= verbose) printf("Eval error: Missing implementation. Specific type: %d\n", n_tree->nodes[n_index].specific_type);
		return -1;
	}
	// Variable/Value.
	else if (n_tree->nodes[n_index].nodetype == n_expr) {
		if (n_tree->nodes[n_index].specific_type == e_id) {
			char* seeking = ti->ts[n_tree->nodes[n_index].token_indices[0]].val;
			if (out >= verbose) printf("Looking for variable named \"%s\" in the vtable.\n",
				ti->ts[n_tree->nodes[n_index].token_indices[0]].val);
			int result = -1;
			lookup(vtable, seeking, &result, out);
			return result;
		}
		else if (n_tree->nodes[n_index].specific_type == e_val) {
			if (n_tree->nodes[n_index].token_count <= 0) {
				if (out >= verbose) {
					printf("Evaluation error: Attempting to bind a value to a variable,\n");
					printf("but no name or value was found. May have roots in an uncaught parsing bug/error.\n");
				}
			}
			char* saved_value = ti->ts[n_tree->nodes[n_index].token_indices[0]].val;
			if (out >= verbose) printf("Found saved value: %s\n", saved_value);
			int value = atoi(saved_value);
			if (out >= verbose) printf("Returning value: %d\n", value);
			return value;
		}
		// Binary operations
		else if (n_tree->nodes[n_index].specific_type == e_binop) {
			struct node* n = &n_tree->nodes[n_index];
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (out >= verbose) printf("Evaluation error: No first child node found in binary operation (node %d).\n", n_index);
					return -5;
				}
				if (n->second == -1) {
					if (out >= verbose) printf("Evaluation error: No second child node found in binary operation (node %d).\n", n_index);
					return -5;
				}
				int result1 = eval(n_tree, n->first, ti, output, vtable, ftable, out);
				int result2 = eval(n_tree, n->second, ti, output, vtable, ftable, out);
				if (n->token_count <= 0) {
					if (out >= standard) printf("Eval error: Binary operation did not have a token specifying its type.\n");
					return -1;
				}
				char* operator = ti->ts[n->token_indices[0]].val;
				if (out >= verbose) printf("Calculating %d %s %d\n", result1, operator, result2);
				if 		(strcmp(operator, "+") == 0) return (result1 + result2);
				else if (strcmp(operator, "-") == 0) return (result1 - result2);
				else if (strcmp(operator, "*") == 0) return (result1 * result2);
				else if (strcmp(operator, "/") == 0) return (result1 / result2);
				else if (strcmp(operator, "==") == 0) return (result1 == result2);
				else if (strcmp(operator, "!=") == 0) return (result1 != result2);
				else if (strcmp(operator, ">=") == 0) return (result1 >= result2);
				else if (strcmp(operator, "<=") == 0) return (result1 <= result2);
				else if (strcmp(operator, ">") == 0) return (result1 > result2);
				else if (strcmp(operator, "<") == 0) return (result1 < result2);
				else if (strcmp(operator, "**") == 0) {
					if (result2 == 0) return 1;
					int result = result1;
					for (int i = 1; i < result2; i++)
						result *= result1;
					return result;
				}
				else { if (out >= verbose) printf("Eval error: No BINOP operator specified in node.\n"); }
			}
			else {
				if (out >= verbose) printf("Evaluation error: No operator found for binary operation (node %d). The token is not connected.\n", n_index);
				return -4;
			}
		}
		else if (n_tree->nodes[n_index].specific_type == e_comp) {
			struct node* n = &n_tree->nodes[n_index];
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (out >= verbose) printf("Evaluation error: No first child node found in comparison operation (node %d).\n", n_index);
					return -5;
				}
				if (n->second == -1) {
					if (out >= verbose) printf("Evaluation error: No second child node found in comparison operation (node %d).\n", n_index);
					return -5;
				}
				int result1 = eval(n_tree, n->first, ti, output, vtable, ftable, out);
				int result2 = eval(n_tree, n->second, ti, output, vtable, ftable, out);
				if (n->token_count <= 0) {
					if (out >= standard)
						printf("Eval error: Comparison operation did not have a token specifying its type.\n");
					return -1;
				}
				char* operator = ti->ts[n->token_indices[0]].val;
				if (out >= verbose) printf("Calculating %d %s %d\n", result1, operator, result2);
				if 		(strcmp(operator, "==") == 0) return (result1 == result2);
				else if (strcmp(operator, "!=") == 0) return (result1 != result2);
				else { if (out >= verbose) printf("Eval error: No comp operator specified in node.\n"); }
			}
		}
		else if (n_tree->nodes[n_index].specific_type == e_funcall) {
			struct node* n = &n_tree->nodes[n_index];
			if (n->token_count == 0) {
				if (out >= standard) printf("Eval error: Function call did not have an ID.\n");
				return -1;
			}
			int functionbind_node = lookup_f(ftable, ti->ts[n->token_indices[0]].val, out);
			if (functionbind_node == -1) {
				if (out >= standard)
					printf("Eval error: Could not find a function by the name \"%s\".\n",
						ti->ts[n->token_indices[0]].val);
				return -1;
			}
			struct node* fb = &n_tree->nodes[functionbind_node];
			struct ivtable_index* new_vtable = malloc(sizeof(*new_vtable));
			new_vtable->n = 0;
			if (fb->first == -1) {
				if (out >= standard)
					printf("Eval warning: No parameter node for \"%s\" function.\n",
						ti->ts[n->token_indices[0]].val);
			} else {
				// should be possible to do this non-imperically when/if I change the way evaluation works...
				int eval_index = n->first;
				if (eval_index != -1) {
					for (int i = 1; i < fb->token_count; i++) {
						int value_to_bind = eval(n_tree, eval_index, ti, output, vtable, ftable, out);
						ivbind(new_vtable,
							  ti->ts[n_tree->nodes[fb->first].token_indices[i * 2 - 1]].val,
							  value_to_bind, out);
						if (n_tree->nodes[eval_index].second == -1) break;
						else eval_index = n_tree->nodes[eval_index].second;
					}
				}
			}

			int result = eval(n_tree, functionbind_node, ti, output, new_vtable, ftable, out);
			free(new_vtable);
			return result;
		}
		else if (n_tree->nodes[n_index].specific_type == e_argument) {
			return eval(n_tree, n_tree->nodes[n_index].first, ti, output, vtable, ftable, out);
		}
	}
	return -2;
}

int start_eval(struct tree* n_tree, int n_index, struct token_index* ti,
		FILE* output, struct ivtable_index* vtable, struct ftable_index* ftable, int out) {
	if (n_tree->n < n_index) return -1;
	//struct ftable_index* new_ftable = malloc(sizeof(ftable));
	//new_ftable->n = ftable->n;
	//new_ftable->fs = malloc(sizeof(new_table->fs) * new_ftable->n);
	//memcpy(new_ftable->fs, ftable->fs, sizeof(ftable->fs) * ftable->n);
	//assemble_ftable(n_tree, n_index, ti, output, vtable, ftable);
	careful_build_ftable(n_tree, ti, output, ftable, out);
	if (out >= verbose) {
		printf("Assembled ftable (length %d):\n", ftable->n);
		for (int f = 0; f < ftable->n; f++) {
			print_node(n_tree, ftable->fs[f].node_index, out);
			printf("\n");
		}
		printf("Now evaluating main node.\n");
	}
	int result = eval(n_tree, ftable->start_fun_node, ti, output, vtable, ftable, out);
	if (out >= verbose) printf("Finished evaluating. Returning.\n");
	return result;
}