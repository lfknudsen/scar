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

/// @brief Bind a value to the variable table.
/// @param vtable The current symbol table.
/// @param id The string name of the variable to save.
/// @param value The Val struct whose value and type will be bound to ID.
/// @return Returns 1 if a previous entry was overwritten.
/// Returns 0 if a new entry was created.
/// Returns -1 if the value had type "error".
char vbind(struct vtable_index* vtable, char* id, struct Val value) {
	if (value.type == error) {
		return -1;
	}
	for (int i = 0; i < vtable->n; i++) {
		if (strcmp(vtable->vs[i].id,id) == 0) {
			vtable->vs[i].val = value;
			return 1;
		}
	}
	vtable->n += 1;
	vtable->vs = realloc(vtable->vs, sizeof(*vtable->vs) * vtable->n);
	vtable->vs[vtable->n - 1].id = id;
	vtable->vs[vtable->n - 1].val = value;
	return 0;
}

// Find value of variable stored in vtable.
// The value that matches the variable name is stored in-place in the 'destination' parameter.
// Returns the vtable index on success (so >= 0).
// Returns -1 on failure.
struct Val lookup(struct vtable_index* vtable, char* id, int out) {
	struct Val result;
	result.type = error;
	if (out >= verbose)
		printf("Searching vtable for \"%s\":\n", id);
	for (unsigned int i = 0; i < vtable->n; i++) {
		if (out >= verbose) {
			printf("%2d   %3s   ", i, vtable->vs[i].id);
			print_val(stdout, vtable->vs[i].val);
		}
		if (strcmp(vtable->vs[i].id, id) == 0) {
			result = vtable->vs[i].val;
			if (out >= verbose)
				printf(" <-- FOUND IT!\n");
			return result;
		}
		if (out >= verbose) printf("\n");
	}
	if (out >= verbose) printf("Could not find the variable in the vtable.\n");
	return result;
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
		return fbind(ftable, state->ti->ts[state->tree->ns[n_index].token_indices[1]].val, n_index,
			state->out);
	}
	return ftable->n;
}

// TODO: Report specific errors.
struct Val addv(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;
	switch (a.type) {
		case (int_val):
			if (b.type == int_val)
				result.value.Int = a.value.Int + b.value.Int;
			else if (b.type == float_val)
				result.value.Float = (float)a.value.Int + b.value.Float;
			else
				return result;
			result.type = b.type;
			return result;
		case (float_val):
			if (b.type == int_val)
				result.value.Float = a.value.Float + (float)b.value.Int;
			else if (b.type == float_val)
				result.value.Float = a.value.Float + b.value.Float;
			else
				return result;
			result.type = a.type;
			return result;
		case (char_arr):
			result.value.String = malloc(strlen(a.value.String) + strlen(b.value.String) + 1);
			sprintf(result.value.String, "%s%s", a.value.String, b.value.String);
			result.type = char_arr;
			return result;
		case (bool_val):
			if (b.type == bool_val) {
				result.value.Bool = a.value.Bool + b.value.Bool;
				result.type = bool_val;
			}
		default:
			return result;
	};
	return result;
}

// TODO: Report specific errors.
struct Val subv(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;
	switch (a.type) {
		case (int_val):
			if (b.type == int_val)
				result.value.Int = a.value.Int - b.value.Int;
			else if (b.type == float_val)
				result.value.Float = (float)a.value.Int - b.value.Float;
			else
				return result;
			result.type = b.type;
			return result;
		case (float_val):
			if (b.type == int_val)
				result.value.Float = a.value.Float - (float)b.value.Int;
			else if (b.type == float_val)
				result.value.Float = a.value.Float - b.value.Float;
			else
				return result;
			result.type = a.type;
			return result;
		case (bool_val):
			if (b.type == bool_val) {
				result.value.Bool = a.value.Bool - b.value.Bool;
				result.type = bool_val;
			}
		default:
			return result;
	};
	return result;
}

// TODO: Report specific errors.
struct Val mulv(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;
	if (a.type == float_val) {
		if (b.type == float_val) {
			result.value.Float = a.value.Float * b.value.Float;
		}
		else if (b.type == int_val) {
			result.value.Float = a.value.Float * (float)b.value.Int;
		}
		result.type = a.type;
	}
	else if (a.type == int_val) {
		if (b.type == int_val) {
			result.value.Int = a.value.Int * b.value.Int;;
		}
		if (b.type == float_val) {
			result.value.Int = (float)a.value.Int * b.value.Float;
		}
		result.type = b.type;
	}
	return result;
}

// TODO: Report specific errors.
struct Val divv(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;
	float result_a;
	if (a.type == float_val)
		result_a = a.value.Float;
	else if (a.type == int_val)
		result_a = (float)a.value.Int;
	else
		return result;

	float result_b;
	if (b.type == float_val)
		result_b = b.value.Float;
	else if (b.type == int_val)
		result_b = (float)b.value.Int;
	else
		return result;

	result.type = float_val;
	result.value.Float = result_a / result_b;
	return result;
}

float fpow(float a, int b) {
	if (a == 0.0)
		return 0;
	if (b == 0)
		return 1;
	float result = a;
	for (int i = 1; i < b; i++) {
		result *= a;
	}
	return result;
}

int ipow(int a, int b) {
	if (a == 0)
		return 0;
	if (b == 0)
		return 1;
	int result = a;
	for (int i = 1; i < b; i++) {
		result *= a;
	}
	return result;
}

// TODO: Report specific errors.
struct Val powv(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;
	if (a.type == float_val) {
		if (b.type == float_val) {
			result.value.Float = fpow(a.value.Float, (int)b.value.Float);
			result.type = float_val;
		}
		else if (b.type == int_val) {
			result.value.Float = fpow(a.value.Float,b.value.Int);
			result.type = float_val;
		}
	}
	else if (a.type == int_val) {
		if (b.type == int_val) {
			result.value.Int = ipow(a.value.Int, b.value.Int);
			result.type = int_val;
		}
		if (b.type == float_val) {
			result.value.Int = ipow(a.value.Int, (int)b.value.Float);
			result.type = int_val;
		}
	}
	return result;
}

struct Val not(struct Val a) {
	struct Val result;
	result.type = a.type;
	switch (a.type) {
		case (int_val):
			result.value.Int = 0 - a.value.Int;
			return result;
		case (float_val):
			result.value.Float = 0.0 - a.value.Float;
			return result;
		case (char_arr):
			result.value.String = a.value.String;
			for (int i = 0; result.value.String[i] != '\0'; i++) {
				char c = result.value.String[i];
				if (c == 48)
					result.value.String[i] = 49;
				else if (c >= 49 && c <= 57) {
					result.value.String[i] = 48;
				}
				else if (c >= 65 && c <= 90) {
					result.value.String[i] = c + 32;
				}
				else if (c >= 97 && c <= 122) {
					result.value.String[i] = c - 32;
				}
			}
			return result;
		default:
			return result;
	}
	return result;
}

struct Val deq(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;

	switch (a.type) {
		case (bool_val):
			if (b.type == bool_val)
				result.value.Bool = ((a.value.Bool > 0) == (b.value.Bool > 0));
			else if (b.type == int_val)
				result.value.Bool = ((a.value.Bool > 0) == (b.value.Int > 0));
			else
				return result;
			result.type = bool_val;
			return result;
		case (int_val):
			if (b.type == int_val)
				result.value.Bool = (a.value.Int == b.value.Int);
			else if (b.type == bool_val)
				result.value.Bool = ((a.value.Bool > 0) == (b.value.Int > 0));
			else
				return result;
			result.type = bool_val;
			return result;
		case (float_val):
			if (b.type == float_val)
				result.value.Bool = (a.value.Float == b.value.Float);
			else if (b.type == int_val)
				result.value.Bool = (a.value.Float == (float)b.value.Int);
			else
				return result;
			result.type = bool_val;
			return result;
		case (char_arr):
			if (b.type == char_arr) {
				result.value.Bool = (strcmp(a.value.String,b.value.String) == 0);
				result.type = bool_val;
				return result;
			}
			else
				return result;
		default:
			return result;
	}
	return result;
}

struct Val neq(struct Val a, struct Val b) {
	struct Val result = deq(a,b);
	result.value.Bool = 1 - result.value.Bool;
	return result;
}

struct Val gth(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;

	switch (a.type) {
		case (bool_val):
			if (b.type == bool_val) {
				result.value.Bool = ((a.value.Bool > 0) > (b.value.Bool > 0));
				result.type = bool_val;
				return result;
			}
			else
				return result;
		case (int_val):
			if (b.type == int_val)
				result.value.Bool = (a.value.Int > b.value.Int);
			else if (b.type == float_val)
				result.value.Bool = ((float)a.value.Int > b.value.Float);
			else
				return result;
			result.type = bool_val;
			return result;
		case (float_val):
			if (b.type == float_val)
				result.value.Bool = (a.value.Float > b.value.Float);
			else if (b.type == int_val)
				result.value.Bool = (a.value.Float > (float)b.value.Int);
			else
				return result;
			result.type = bool_val;
			return result;
		default:
			return result;
	}
	return result;
}

struct Val geq(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;

	switch (a.type) {
		case (bool_val):
			if (b.type == bool_val) {
				result.value.Bool = ((a.value.Bool > 0) >= (b.value.Bool > 0));
				result.type = bool_val;
				return result;
			}
			else
				return result;
		case (int_val):
			if (b.type == int_val)
				result.value.Bool = (a.value.Int >= b.value.Int);
			else if (b.type == float_val)
				result.value.Bool = ((float)a.value.Int >= b.value.Float);
			else
				return result;
			result.type = bool_val;
			return result;
		case (float_val):
			if (b.type == float_val)
				result.value.Bool = (a.value.Float >= b.value.Float);
			else if (b.type == int_val)
				result.value.Bool = (a.value.Float >= (float)b.value.Int);
			else
				return result;
			result.type = bool_val;
			return result;
		default:
			return result;
	}
	return result;
}

struct Val lth(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;

	switch (a.type) {
		case (bool_val):
			if (b.type == bool_val) {
				result.value.Bool = ((a.value.Bool > 0) < (b.value.Bool > 0));
				result.type = bool_val;
				return result;
			}
			else
				return result;
		case (int_val):
			if (b.type == int_val)
				result.value.Bool = (a.value.Int < b.value.Int);
			else if (b.type == float_val)
				result.value.Bool = ((float)a.value.Int < b.value.Float);
			else
				return result;
			result.type = bool_val;
			return result;
		case (float_val):
			if (b.type == float_val)
				result.value.Bool = (a.value.Float < b.value.Float);
			else if (b.type == int_val)
				result.value.Bool = (a.value.Float < (float)b.value.Int);
			else
				return result;
			result.type = bool_val;
			return result;
		default:
			return result;
	}
	return result;
}

struct Val leq(struct Val a, struct Val b) {
	struct Val result;
	result.type = error;

	switch (a.type) {
		case (bool_val):
			if (b.type == bool_val) {
				result.value.Bool = ((a.value.Bool > 0) <= (b.value.Bool > 0));
				result.type = bool_val;
				return result;
			}
			else
				return result;
		case (int_val):
			if (b.type == int_val)
				result.value.Bool = (a.value.Int <= b.value.Int);
			else if (b.type == float_val)
				result.value.Bool = ((float)a.value.Int <= b.value.Float);
			else
				return result;
			result.type = bool_val;
			return result;
		case (float_val):
			if (b.type == float_val)
				result.value.Bool = (a.value.Float <= b.value.Float);
			else if (b.type == int_val)
				result.value.Bool = (a.value.Float <= (float)b.value.Int);
			else
				return result;
			result.type = bool_val;
			return result;
		default:
			return result;
	}
	return result;
}

/// @brief Evaluate a single node in the node tree.
/// @return Returns an integer representing either the return value or an error_code if something failed.
/// @param state->nt tree structure containing nodes.
/// @param n_index the node being evaluated.
/// @param ti the structure of tokens, used for determining information about the node.
/// @param output file-buffer opened with the "w" argument, used to log information to a file.
/// @param vtable the variable table.
/// @param ftable the function table.
struct Val eval(int n_index, struct vtable_index* vtable, struct ftable_index* ftable, struct state* state) {
	if (state->out >= verbose) {
		printf("Evaluating (node %d): ", n_index);
		print_node(n_index, state);
		printf("\n");
	}
	struct node* n = &state->tree->ns[n_index];
	struct Val result;
	result.type = error;
	result.value.Int = 0;
	if (state->tree->n < n_index) {
		return result;
	}
	switch(state->tree->ns[n_index].node_type) {
	case (n_stat):
		switch(state->tree->ns[n_index].specific_type) {
		case (s_fun_bind):
			if (state->tree->ns[n_index].first != -1) {
				struct vtable_index* new_vtable = malloc(sizeof(*vtable));
				new_vtable->n = 0;
				if (state->tree->ns[n_index].second != -1) {
					if (state->out >= verbose) printf("Evaluating function body.\n");
					result = eval(state->tree->ns[n_index].second, new_vtable, ftable, state);
					free_vtable(new_vtable);
					return result;
				}
				else {
					if (state->out >= verbose) printf("Function is missing its body.\n");
					free_vtable(new_vtable);
					return result;
				}
			}
			break;
		// Variable binding statement
		case (s_var_bind):
			;
			char* var_name = state->ti->ts[state->tree->ns[n_index].token_indices[1]].val;
			result = eval(state->tree->ns[n_index].first, vtable, ftable, state);
			if (result.type == error)
				return result;
			struct vtable_index* new_vtable = malloc(sizeof(*vtable));
			new_vtable->n = vtable->n;
			new_vtable->vs = malloc(sizeof(*new_vtable->vs) * new_vtable->n);
			memcpy(new_vtable->vs, vtable->vs, sizeof(*vtable->vs) * vtable->n);
			vbind(new_vtable, var_name, result);
			result = eval(state->tree->ns[n_index].second, new_vtable, ftable, state);
			free_vtable(new_vtable);
			return result;
		case (s_return):
			return eval(state->tree->ns[n_index].second, vtable, ftable, state);
		case (s_if):
			result = eval(state->tree->ns[n_index].first, vtable, ftable, state);
			if (result.type == bool_val && result.value.Bool == 1)
				return eval(state->tree->ns[state->tree->ns[n_index].first].second, vtable, ftable, state);
			return eval(state->tree->ns[n_index].second, vtable, ftable, state);

		default:
			if (state->out >= verbose)
				printf("Eval error: Missing implementation. Specific type: %d\n",
					state->tree->ns[n_index].specific_type);
			return result;
		}
	// Variable/Value.
	case n_expr:
		switch(state->tree->ns[n_index].specific_type) {
		case e_id:
			;
			char* seeking = state->ti->ts[state->tree->ns[n_index].token_indices[0]].val;
			if (state->out >= verbose)
				printf("Looking for variable named \"%s\" in the vtable.\n",
					seeking);
			result = lookup(vtable, seeking, state->out);
			return result;
		case e_val:
			if (state->tree->ns[n_index].token_count <= 0) {
				if (state->out >= verbose) {
					printf("Evaluation error: Attempting to bind a value to a variable,\n");
					printf("but no name or value was found. May have roots in an uncaught parsing bug/error.\n");
				}
			}
			char* saved_value = state->ti->ts[state->tree->ns[n_index].token_indices[0]].val;
			switch (state->ti->ts[state->tree->ns[n_index].token_indices[0]].type) {
				case (t_num_int):
					result.type = int_val;
					result.value.Int = atoi(saved_value);
					break;
				case (t_num_float):
					result.type = float_val;
					result.value.Float = atof(saved_value);
					break;
				case (t_id):
					result.type = char_arr;
					result.value.String = saved_value;
					break;
				default:
					result.type = error;
			}
			if (state->out >= verbose) {
				printf("returning value: ");
				print_val(stdout, result);
				printf("\n");
			}
			return result;
		// Binary operations
		case e_binop:
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (state->out >= verbose) printf("Evaluation error: No first child node found in binary operation (node %d).\n", n_index);
					return result;
				}
				if (n->second == -1) {
					if (state->out >= verbose) printf("Evaluation error: No second child node found in binary operation (node %d).\n", n_index);
					return result;
				}
				if (n->token_count <= 0) {
					if (state->out >= standard) printf("Eval error: Binary operation did not have a token specifying its type.\n");
					return result;
				}

				struct Val result1 = eval(n->first, vtable, ftable, state);
				if (result1.type == error)
					return result1;
				struct Val result2 = eval(n->second, vtable, ftable, state);
				if (result2.type == error)
					return result2;

				char* operator = state->ti->ts[n->token_indices[0]].val;
				if 		(strcmp(operator,  "+") == 0) result = addv(result1, result2);
				else if (strcmp(operator,  "-") == 0) result = subv(result1, result2);
				else if (strcmp(operator,  "*") == 0) result = mulv(result1, result2);
				else if (strcmp(operator,  "/") == 0) result = divv(result1, result2);
				else if (strcmp(operator, "**") == 0) result = powv(result1, result2);
				else {
					if (state->out >= verbose)
						printf("Eval error: No BINOP operator specified in node.\n");
					return result;
				}
				if (state->out >= verbose) {
					printf("Calculating ");
					print_val(stdout, result1);
					printf(" %s ", operator);
					print_val(stdout, result2);
					printf(".\n");
				}
				return result;
			}
			else {
				if (state->out >= verbose)
					printf("Evaluation error: No operator found for binary operation (node %d). The token is not connected.\n", n_index);
				return result;
			}
		case (e_comp):
			if (n->token_count > 0) {
				if (n->first == -1) {
					if (state->out >= verbose)
						printf("Evaluation error: No first child node found in comparison operation (node %d).\n", n_index);
					return result;
				}
				if (n->second == -1) {
					if (state->out >= verbose)
						printf("Evaluation error: No second child node found in comparison operation (node %d).\n", n_index);
					return result;
				}
				struct Val result1 = eval(n->first, vtable, ftable, state);
				if (result1.type == error)
					return result1;
				struct Val result2 = eval(n->second, vtable, ftable, state);
				if (result2.type == error)
					return result2;
				if (n->token_count <= 0) {
					if (state->out >= standard)
						printf("Eval error: Comparison operation did not have a token specifying its type.\n");
					return result;
				}
				char* operator = state->ti->ts[n->token_indices[0]].val;
				if (state->out >= verbose) {
					printf("Calculating ");
					print_val(stdout, result1);
					printf(" %s ", operator);
					print_val(stdout, result2);
					printf(".\n");
				}
				if 		(strcmp(operator, "==") == 0) return deq(result1, result2);
				else if (strcmp(operator, "!=") == 0) return neq(result1, result2);
				else if (strcmp(operator, ">=") == 0) return geq(result1, result2);
				else if (strcmp(operator, "<=") == 0) return leq(result1, result2);
				else if (strcmp(operator,  ">") == 0) return gth(result1, result2);
				else if (strcmp(operator,  "<") == 0) return lth(result1, result2);
				else { if (state->out >= verbose)
					printf("Eval error: No comp operator specified in node.\n"); }
			}
			break;
		case (e_funcall):
			if (n->token_count == 0) {
				if (state->out >= standard)
					printf("Eval error: Function call did not have an ID.\n");
				return result;
			}
			int functionbind_node = lookup_f(ftable, state->ti->ts[n->token_indices[0]].val, state->out);
			if (functionbind_node == -1) {
				if (state->out >= standard)
					printf("Eval error: Could not find a function by the name \"%s\".\n",
						state->ti->ts[n->token_indices[0]].val);
				return result;
			}
			struct node* fb = &state->tree->ns[functionbind_node];
			struct vtable_index* new_vtable = malloc(sizeof(*new_vtable));
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
						result = eval(eval_index, vtable, ftable, state);
						if (result.type == error) {
							free_vtable(new_vtable);
							return result;
						}
						vbind(new_vtable,
							  state->ti->ts[state->tree->ns[fb->first].token_indices[i * 2 - 1]].val,
							  result);
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
			return not(eval(state->tree->ns[n_index].first, vtable, ftable, state));
		case (e_condition):
			return eval(state->tree->ns[n_index].first, vtable, ftable, state);
		default:
			return result;
		}
	default:
		break;
	}
	return result;
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
struct Val start_eval(struct vtable_index* vtable, struct ftable_index* ftable, struct state* state) {
	assemble_ftable(0, ftable, state);
	if (state->out >= verbose) {
		printf("Assembled ftable (length %d):\n", ftable->n);
		for (int f = 0; f < ftable->n; f++) {
			print_node(ftable->fs[f].node_index, state);
			printf("\n");
		}
		printf("Now evaluating main node.\n");
	}
	struct Val result = eval(ftable->start_fun_node, vtable, ftable, state);
	if (state->out >= verbose) printf("Finished evaluating. Returning.\n");
	return result;
}
