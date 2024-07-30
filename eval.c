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
	if (vtable->n == 1)
		vtable->vs = malloc(sizeof(*vtable->vs));
	else
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
	if (out >= verbose)
		printf("Searching vtable for \"%s\":\n", id);
	for (unsigned int i = 0; i < vtable->n; i++) {
		if (out >= verbose)
			printf("%2d   %3s   %3d", i, vtable->vs[i].id, vtable->vs[i].val);
		if (strcmp(vtable->vs[i].id, id) == 0) {
			*destination = vtable->vs[i].val;
			if (out >= verbose)
				printf(" <-- FOUND IT!\n");
			return i;
		}
		if (out >= verbose) printf("\n");
	}
	if (out >= verbose) printf("Could not find the variable in the vtable.\n");
	return -1;
}

/// @brief Bind a function definition to the ftable.
/// @param ftable 	function table.
/// @param id 		string name of function.
/// @param n_index 	index of the top function node, whose parent is an n_prog,
/// and whose children are the parameter declaration and function body.
/// @return The function's index into the ftable.
int fbind(struct ftable_index* ftable, char* id, unsigned int node_index, int out) {
	if (strcmp(id, "main") == 0) ftable->start_fun_node = node_index;
	for (int i = 0; i < ftable->n; i++) {
		if (strcmp(ftable->fs[i].id,id) == 0) {
			if (out >= verbose) printf("Function override. %s == %s.\n", ftable->fs[i].id, id);
			ftable->fs[i].node_index = node_index;
			return i;
		}
	}
	if (out >= verbose)
		printf("Binding new function \"%s\" to the ftable.\n", id);
	ftable->n += 1;
	if (ftable->n == 1)
		ftable->fs = malloc(sizeof(*ftable->fs));
	else
		ftable->fs = realloc(ftable->fs, sizeof(*ftable->fs) * ftable->n);
	ftable->fs[ftable->n - 1].id = id;
	ftable->fs[ftable->n - 1].node_index = node_index;
	return ftable->n - 1;
}

/// @brief Find the node index of the entry in the ftable which matches the
/// id argument.
/// @param id The name of the function to look for.
/// @return Returns the node index if it was found, otherwise -1.
int lookup_f(struct ftable_index* ftable, char* id, int out) {
	if (out >= verbose)
		printf("Searching ftable for \"%s\":\n", id);
	for (int i = 0; i < ftable->n; i++) {
		if (out >= verbose)
			printf("%2d   %3s   %3d", i, ftable->fs[i].id, ftable->fs[i].node_index);
		if (strcmp(ftable->fs[i].id,id) == 0) {
			if (out >= verbose)
				printf(" <-- FOUND IT!\n");
			return ftable->fs[i].node_index;
		}
		if (out >= verbose) printf("\n");
	}
	if (out >= verbose) printf("Could not find the function in the ftable.\n");
	return -1;
}

/// @brief Construct the function table used for look-ups later.
/// The starting function "main" is located in the process, as part of
/// the fbind() function.
/// @param state->nt
/// @param n_index
/// @param ti
/// @param output
/// @param ftable
/// @param out
/// @return The size of the final function table.
int assemble_ftable(int n_index, struct ftable_index* ftable, struct state* state) {
	if (state->out >= verbose) { printf("Checking node %d.\n", n_index); }
	if (state->tree->ns[n_index].node_type == n_prog) {
		if (state->tree->ns[n_index].first != -1) {
			assemble_ftable(state->tree->ns[n_index].first, ftable, state);
		}
		if (state->tree->ns[n_index].second != -1) {
			assemble_ftable(state->tree->ns[n_index].second, ftable, state);
		}
	}
	else if (state->tree->ns[n_index].node_type == n_stat &&
		state->tree->ns[n_index].specific_type == s_fun_bind) {
		if (state->out >= verbose)
			printf("Adding function \"%s\" to ftable.\n",
				state->ti->ts[state->tree->ns[n_index].token_indices[1]].val);
		return fbind(ftable, state->ti->ts[state->tree->ns[n_index].token_indices[1]].val, n_index, out);
	}
	return ftable->n;
}

/// @brief Evaluate a single node in the node tree.
/// @return Returns an integer representing either the return value or an error_code if something failed.
/// @param state->nt tree structure containing nodes.
/// @param n_index the node being evaluated.
/// @param ti the structure of tokens, used for determining information about the node.
/// @param output file-buffer opened with the "w" argument, used to log information to a file.
/// @param vtable the variable table.
/// @param ftable the function table.
int eval(int n_index, struct ivtable_index* vtable, struct ftable_index* ftable, struct state* state) {
	if (state->out >= verbose) {
		printf("Evaluating (node %d): ", n_index);
		print_node(n_index, state);
		printf("\n");
	}
	struct node* n = &state->tree->ns[n_index];
	if (state->tree->n < n_index) return -1;
	switch(state->tree->ns[n_index].node_type) {
	case (n_stat):
		switch(state->tree->ns[n_index].specific_type) {
		case (s_fun_bind):
			if (state->tree->ns[n_index].first != -1) {
				struct ivtable_index* new_vtable = malloc(sizeof(*vtable));
				new_vtable->n = 0;
				if (state->tree->ns[n_index].second != -1) {
					if (state->out >= verbose) printf("Evaluating function body.\n");
					int result = eval(state->tree->ns[n_index].second, new_vtable, ftable, state);
					free_ivtable(new_vtable);
					return result;
				}
				else {
					if (state->out >= verbose) printf("Function is missing its body.\n");
					free_ivtable(new_vtable);
					return -1;
				}
			}
			break;
		// Variable binding statement
		case (s_var_bind):
			;
			char* var_name = state->ti->ts[state->tree->ns[n_index].token_indices[1]].val;
			int value = eval(state->tree->ns[n_index].first, vtable, ftable, state);
			struct ivtable_index* new_vtable = malloc(sizeof(*vtable));
			new_vtable->n = vtable->n;
			new_vtable->vs = malloc(sizeof(*new_vtable->vs) * new_vtable->n);
			memcpy(new_vtable->vs, vtable->vs, sizeof(*vtable->vs) * vtable->n);
			ivbind(new_vtable, var_name, value, out);
			int result = eval(state->tree->ns[n_index].second, new_vtable, ftable, state);
			free_ivtable(new_vtable);
			return result;
		case (s_return):
			return eval(state->tree->ns[n_index].second, vtable, ftable, state);
		case (s_if):
			result = eval(state->tree->ns[n_index].first, vtable, ftable, state);
			if (result)
				return eval(state->tree->ns[state->tree->ns[n_index].first].second, vtable, ftable, state);
			return eval(state->tree->ns[n_index].second, vtable, ftable, state);

		default:
			if (state->out >= verbose)
				printf("Eval error: Missing implementation. Specific type: %d\n",
					state->tree->ns[n_index].specific_type);
			return -1;
		}
	// Variable/Value.
	case n_expr:
		switch(state->tree->ns[n_index].specific_type) {
		case e_id:
			;
			char* seeking = state->ti->ts[state->tree->ns[n_index].token_indices[0]].val;
			if (state->out >= verbose) printf("Looking for variable named \"%s\" in the vtable.\n",
				state->ti->ts[state->tree->ns[n_index].token_indices[0]].val);
			int result = -1;
			lookup(vtable, seeking, &result, out);
			return result;
		case e_val:
			if (state->tree->ns[n_index].token_count <= 0) {
				if (state->out >= verbose) {
					printf("Evaluation error: Attempting to bind a value to a variable,\n");
					printf("but no name or value was found. May have roots in an uncaught parsing bug/error.\n");
				}
			}
			char* saved_value = state->ti->ts[state->tree->ns[n_index].token_indices[0]].val;
			int value = atoi(saved_value);
			if (state->out >= verbose)
				printf("Returning value: %d\n", value);
			return value;
		// Binary operations
		case e_binop:
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (state->out >= verbose) printf("Evaluation error: No first child node found in binary operation (node %d).\n", n_index);
					return -5;
				}
				if (n->second == -1) {
					if (state->out >= verbose) printf("Evaluation error: No second child node found in binary operation (node %d).\n", n_index);
					return -5;
				}
				if (n->token_count <= 0) {
					if (out >= standard) printf("Eval error: Binary operation did not have a token specifying its type.\n");
					return -1;
				}

				int result1 = eval(n->first, vtable, ftable, state);
				int result2 = eval(n->second, vtable, ftable, state);

				char* operator = state->ti->ts[n->token_indices[0]].val;
				int output;
				if 		(strcmp(operator,  "+") == 0) output = (result1  + result2);
				else if (strcmp(operator,  "-") == 0) output = (result1  - result2);
				else if (strcmp(operator,  "*") == 0) output = (result1  * result2);
				else if (strcmp(operator,  "/") == 0) output = (result1  / result2);
				else if (strcmp(operator, "**") == 0) {
					if (result1 == 0 || result2 == 0)
						output = 0;
					int result = result1;
					for (int i = 1; i < result2; i++)
						result *= result1;
					output = result;
				}
				else {
					if (state->out >= verbose) printf("Eval error: No BINOP operator specified in node.\n");
					return -1;
				}
				if (state->out >= verbose) printf("Calculating %d %s %d = %d\n", result1, operator, result2, output);
				return output;
			}
			else {
				if (state->out >= verbose) printf("Evaluation error: No operator found for binary operation (node %d). The token is not connected.\n", n_index);
				return -4;
			}
		case (e_comp):
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (state->out >= verbose) printf("Evaluation error: No first child node found in comparison operation (node %d).\n", n_index);
					return -5;
				}
				if (n->second == -1) {
					if (state->out >= verbose) printf("Evaluation error: No second child node found in comparison operation (node %d).\n", n_index);
					return -5;
				}
				int result1 = eval(n->first, vtable, ftable, state);
				int result2 = eval(n->second, vtable, ftable, state);
				if (n->token_count <= 0) {
					if (state->out >= standard)
						printf("Eval error: Comparison operation did not have a token specifying its type.\n");
					return -1;
				}
				char* operator = state->ti->ts[n->token_indices[0]].val;
				if (state->out >= verbose) printf("Calculating %d %s %d\n", result1, operator, result2);
				if 		(strcmp(operator, "==") == 0) return (result1 == result2);
				else if (strcmp(operator, "!=") == 0) return (result1 != result2);
				else if (strcmp(operator, ">=") == 0) return (result1 >= result2);
				else if (strcmp(operator, "<=") == 0) return (result1 <= result2);
				else if (strcmp(operator,  ">") == 0) return (result1  > result2);
				else if (strcmp(operator,  "<") == 0) return (result1  < result2);
				else { if (state->out >= verbose) printf("Eval error: No comp operator specified in node.\n"); }
			}
			break;
		case (e_funcall):
			if (n->token_count == 0) {
				if (state->out >= standard) printf("Eval error: Function call did not have an ID.\n");
				return -1;
			}
			int functionbind_node = lookup_f(ftable, state->ti->ts[n->token_indices[0]].val, state->out);
			if (functionbind_node == -1) {
				if (state->out >= standard)
					printf("Eval error: Could not find a function by the name \"%s\".\n",
						state->ti->ts[n->token_indices[0]].val);
				return -1;
			}
			struct node* fb = &state->tree->ns[functionbind_node];
			struct ivtable_index* new_vtable = malloc(sizeof(*new_vtable));
			new_vtable->n = 0;
			if (fb->first == -1) {
				if (state->out >= standard)
					printf("Eval warning: No parameter node for \"%s\" function.\n",
						state->ti->ts[n->token_indices[0]].val);
			} else {
				// should be possible to do this non-imperically when/if I change the way evaluation works...
				int eval_index = n->first;
				if (eval_index != -1) {
					for (int i = 1; i < fb->token_count; i++) {
						int value_to_bind = eval(eval_index, vtable, ftable, state);
						ivbind(new_vtable,
							  state->ti->ts[state->tree->ns[fb->first].token_indices[i * 2 - 1]].val,
							  value_to_bind, out);
						if (state->tree->ns[eval_index].second == -1) break;
						else eval_index = state->tree->ns[eval_index].second;
					}
				}
			}
			result = eval(functionbind_node, new_vtable, ftable, state);
			free(new_vtable);
			return result;
		case (e_argument):
			return eval(state->tree->ns[n_index].first, vtable, ftable, state);
		case (e_not):
			return !(eval(state->tree->ns[n_index].first, vtable, ftable, state));
		case (e_condition):
			return eval(state->tree->ns[n_index].first, vtable, ftable, state);
		}
	default:
		break;
	}
	return -2;
}

/// @brief Begin the evaluation process by building a function table and then
/// evaluating the first node in the node tree.
/// @param state->nt
/// @param n_index
/// @param ti
/// @param output
/// @param vtable
/// @param ftable
/// @param out
/// @return
int start_eval(struct ivtable_index* vtable, struct ftable_index* ftable, struct state* state) {
	assemble_ftable(0, ftable, state);
	if (state->out >= verbose) {
		printf("Assembled ftable (length %d):\n", ftable->n);
		for (int f = 0; f < ftable->n; f++) {
			print_node(ftable->fs[f].node_index, state);
			printf("\n");
		}
		printf("Now evaluating main node.\n");
	}
	int result = eval(ftable->start_fun_node, vtable, ftable, state);
	if (state->out >= verbose) printf("Finished evaluating. Returning.\n");
	return result;
}