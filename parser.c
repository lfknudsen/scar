#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define NODE node_tree->nodes[node_index]
#define NODES node_tree->nodes
#define TREE node_tree
#define NODE_COUNT node_tree->n

enum state {
	program_start,
	function_head,
	function_param,
	function_param_followup,
	function_body_start,
	function_body_expression,
	function_body_statement
};

/*
s0 -> s1
s1 -> Type Id ( -> s2
s1 -> -> END
s2 -> Type Id -> s3
s2 -> ) = -> s4
s3 -> , -> s2
s3 -> ) = -> s4
s4 -> Type Id = -> s5
s4 -> return -> s7
s5 -> Num ; -> s6
s5 -> Id ; -> s6
s5 -> Num/Val BINOP -> s9
s6 -> Type Id = -> s5
s6 -> return ; -> s1
s6 -> return Id ; -> s1
s7 -> ; -> s1 (warning)
s7 -> Val -> s8
s8 -> BINOP -> s8 -> 
s9 -> 
*/

/*
Bug with an opening return statement having one or more binary operators.
*/

void set_first(struct tree *n_tree, int n_index, int value, int state) {
	n_tree->nodes[n_index].first = value;
	if (VERBOSE) {
		printf("Node %d's first set to %d while in state %d.\n", n_index, value, state);
	}
}

void set_second(struct tree *n_tree, int n_index, int value, int state) {
	n_tree->nodes[n_index].second = value;
	if (VERBOSE) {
		printf("Node %d's second set to %d while in state %d.\n", n_index, value, state);
	}
}

void set_parent(struct tree *n_tree, int n_index, int value, int state) {
	n_tree->nodes[n_index].parent = value;
	if (VERBOSE) {
		printf("Node %d's parent set to %d while in state %d.\n", n_index, value, state);
	}
}

void set_indent(struct tree *n_tree, int n_index, int state) {
	if (n_tree->nodes[n_index].parent == -1) {
		printf("NO PARENT!\n");
		return;
	}
	n_tree->nodes[n_index].print_indent =
		n_tree->nodes[n_tree->nodes[n_index].parent].print_indent + 1;
	if (VERBOSE) {
		printf("Node %d's indentation set to %d while in state %d.\n",
			n_index, n_tree->nodes[n_index].print_indent, state);
	}
}

void debug_print(int state, unsigned int index) {
	//if (VERBOSE) printf("Current state: %d\nToken index: %u\n", state, index);
}

// Add a new node to the tree.
// Return: number of nodes - 1 (i.e. the index of the new node)
// Nodes are evaluated first -> second -> self (since evaluating self *is* evaluating the children)
int add_node(struct tree *node_tree, int node_type, int specific_type, int state, FILE* output) {
	printf("About to enter new node.\n");
	if (output == NULL) output = stdout;
	node_tree->n += 1;
	node_tree->nodes = realloc(node_tree->nodes,
		sizeof(*node_tree->nodes) * node_tree->n);
	node_tree->nodes[node_tree->n - 1].nodetype = node_type;
	node_tree->nodes[node_tree->n - 1].specific_type = specific_type;
	node_tree->nodes[node_tree->n - 1].token_count = 0;
	node_tree->nodes[node_tree->n - 1].extra_info = 0;
	node_tree->nodes[node_tree->n - 1].first = -1;
	node_tree->nodes[node_tree->n - 1].second = -1;
	node_tree->nodes[node_tree->n - 1].parent = -1;
	node_tree->nodes[node_tree->n - 1].token_indices = malloc(1);
	node_tree->nodes[node_tree->n - 1].print_indent = 0;
	if (VERBOSE) printf("Adding new node %d.\n", node_tree->n - 1);
	fprintf(output, "Created new node in state %d:\n", state);
	fprint_node(node_tree, node_tree->n - 1, output);
	fprintf(output, "\n");
	return node_tree->n - 1;
}

void add_token(struct tree* node_tree, int t_index, int n_index) {
	struct node* n = &node_tree->nodes[n_index];
	n->token_count += 1;
	n->token_indices =
		realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
	n->token_indices[n->token_count - 1] = t_index;
	if (VERBOSE) printf("Adding token %d to node %d.\n", t_index, n_index);
}

void connect_token(struct tree* n_tree, int n_index, int t_index) {
	struct node* n = &n_tree->nodes[n_index];
	n->token_count += 1;
	n->token_indices =
		realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
	n->token_indices[n->token_count - 1] = t_index;
	if (VERBOSE) printf("Adding token %d to node %d.\n", t_index, n_index);
}

// n = elements in token_index's (ti) array of tokens (ts).
// i = the index of the token in ti->ts that is currently being parsed.
int check_type(struct token_index *ti, enum e_token expected, int i) {
	if (ti->n <= i) {
		printf("Parse error. Code terminates prematurely at %lu:%lu.\n",
		ti->ts[ti->n - 1].line_number, ti->ts[ti->n - 1].char_number);
		return 0;
	}
	return (ti->ts[i].type == expected);
}

int parse_s(int state, struct token_index* ti, int* i, struct tree* node_tree, int node_index, int fun_parent, FILE* output) {
	if (VERBOSE) printf("State: %d. Token index: %d\n", state, *i);

	struct token* t = ti->ts;
	if (state == 0) {
		assert(node_tree->n == 0);
		int top_node_index = add_node(node_tree, n_prog, 0, state, output);
		//fprintf(output, "%3d: Program node\n  Created in state %d\n", top_node_index, state);

		return parse_s(1, ti, i, node_tree, top_node_index, top_node_index, output);
	}

	// Expecting a function declaration. State checks type, ID, and beginning parentheses to contain parameters.
	// If these are correct, add a program node (n_prog), then a function bind statement node (n_stat + s_fun_bind),
	// and finally a parameter expression node. The parameter node will be evaluated first, then the function bind
	// node, before returning the result up the chain to evaluate the program node.
	// A program contains one or more functions.
	// type id ( -> s2
	// fun_parent = top node of entire program?
	else if (state == 1) {
		if (ti->n == *i) return END_STATE;
		// Program must consist of only functions at its highest level, and must have at least one function.
		if (!check_type(ti, t_type, *i)) {
			printf("Parse error. Expected function type at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		if (!check_type(ti, t_id, *i)) {
			printf("Parse error. Expected function name at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		if (!check_type(ti, t_par_beg, *i)) {
			printf("Parse error. Expected '(' to precede function parameters at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;

		int top_node_index = add_node(node_tree, n_prog, 0, state, output);
		set_second(node_tree, fun_parent, top_node_index, state);
		set_parent(node_tree, top_node_index, fun_parent, state);

		int fun_declaration = add_node(node_tree, n_stat, s_fun_bind, state, output);
		add_token(node_tree, *i - 3, fun_declaration);
		add_token(node_tree, *i - 2, fun_declaration);
		if (VERBOSE) printf("Added new function: %s %s\n", ti->ts[*i-3].val, ti->ts[*i-2].val);
		set_parent(node_tree, fun_declaration, top_node_index, state);
		set_indent(node_tree, fun_declaration, state);
		set_first(node_tree, top_node_index, fun_declaration, state);

		int new_param_node_index = add_node(node_tree, n_expr, e_param, state, output);
		set_parent(node_tree, new_param_node_index, fun_declaration, state);
		set_indent(node_tree, new_param_node_index, state);

		set_first(node_tree, fun_declaration, new_param_node_index, state);

		return parse_s(2, ti, i, node_tree, new_param_node_index, fun_declaration, output);
	}

	// Function parameter(s).
	// First check if at the end of the parameter declarations with ") ="
	// Otherwise, check for a parameter declaration with "type id".
	//    If so, add them as tokens, and go to state 
	// ) = -> s4
	// type id -> s3
	// Node index = parameter node
	// fun_parent = function binding
	else if (state == 2) {
		if (check_type(ti, t_par_end, *i)) {
			*i += 1;
			if (check_type(ti, t_eq, *i)) {
				*i += 1;
				return parse_s(4, ti, i, node_tree, node_tree->nodes[node_index].parent, fun_parent, output);
			}
			else {
				printf("Parse error. Expected '=' to precede function body at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				return -1;
			}
		}
		if (check_type(ti, t_type, *i)) *i += 1;
		else {
			printf("Parse error. Expected parameter type at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		if (check_type(ti, t_id, *i)) *i += 1;
		else {
			printf("Parse error. Expected parameter name at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}

		add_token(node_tree, *i - 2, node_index);
		add_token(node_tree, *i - 1, node_index);
		return parse_s(3, ti, i, node_tree, node_index, fun_parent, output);
	}
	// Function parameter follow-up (either another parameter or the start of function body)
	// , -> s2
	// ) = -> s4
	// node_index = parameter node.
	// fun_parent = function binding.
	else if (state == 3) {
		if (check_type(ti, t_comma, *i)) {
			*i += 1;
			return parse_s(2, ti, i, node_tree, node_index, fun_parent, output);
		}
		if (check_type(ti, t_par_end, *i)) *i += 1;
		else {
			printf("Parse error. Expected ')' to proceed function parameters at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		if (check_type(ti, t_eq, *i)) *i += 1;
		else {
			printf("Parse error. Expected '=' to indicate start of function body at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		return parse_s(4, ti, i, node_tree, node_tree->nodes[node_index].parent, fun_parent, output);
	}
	// First statement in function body.
	// return ; -> s1
	// return -> s7
	// type id = -> s5 -> ?
	// node_index = function binding
	// fun_parent = function binding
	else if (state == 4) {
		if (check_type(ti, t_return, *i)) {
			*i += 1;
			int return_node_index = add_node(node_tree, n_stat, s_return, state, output);
			set_second(node_tree, node_index, return_node_index, state);
			set_parent(node_tree, return_node_index, node_index, state);
			set_indent(node_tree, return_node_index, state);

			if (check_type(ti, t_semicolon, *i)) {
				printf("Warning: Empty function body at %lu:%lu.\n", t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				*i += 1;
				return parse_s(1, ti, i, node_tree, return_node_index, node_tree->nodes[node_index].parent, output);
			}
			else {
				return parse_s(7, ti, i, node_tree, return_node_index, node_index, output);
			}
		}
		// TODO: THIS ONLY SUPPORTS LET-STATEMENTS FOR NOW
		if (check_type(ti, t_type, *i)) *i += 1;
		else {
			printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		if (check_type(ti, t_id, *i)) *i += 1;
		else {
			printf("Parse error. Expected a variable name at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		if (check_type(ti, t_eq, *i)) *i += 1;
		else {
			printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		int new_node_index = add_node(node_tree, n_stat, s_var_bind, state, output);
		set_parent(node_tree, new_node_index, node_index, state);
		set_second(node_tree, node_index, new_node_index, state);
		set_indent(node_tree, new_node_index, state);
		add_token(node_tree, *i - 3, new_node_index);
		add_token(node_tree, *i - 2, new_node_index);

		return
			parse_s (
				parse_s(5, ti, i, node_tree, new_node_index, node_index, output),
			ti, i, node_tree, new_node_index, node_index, output
			);
	}
	// State used to evaluate the value of the right side of an equals sign.
	// num ; -> return 6
	// num binop -> s9
	// id ; -> return 6
	// id binop -> s9
	else if (state == 5) {
		assert(
			!(node_tree->nodes[node_index].nodetype == n_stat
			&& node_tree->nodes[node_index].specific_type == s_fun_bind));

		int specific_expr_type = -1;
		if 		(check_type(ti, t_num_int, *i)) 	specific_expr_type = e_val;
		else if (check_type(ti, t_num_float, *i)) 	specific_expr_type = e_val;
		else if (check_type(ti, t_id, *i)) 			specific_expr_type = e_id;
		else {
			printf("Parse error. Expected a value to bind to variable \"%s\"(%lu:%lu) at %lu:%lu.\n",
			t[*i-2].val, t[*i-2].line_number, t[*i-2].char_number, t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			int left_operand = add_node(node_tree, n_expr, specific_expr_type, state, output);
			add_token(node_tree, *i - 2, left_operand);
			set_first(node_tree, node_index, left_operand, state);
			set_parent(node_tree, left_operand, node_index, state);
			set_indent(node_tree, left_operand, state);
			return 6;
		}
		// "x (+) ?"
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int operator = add_node(node_tree, n_expr, e_binop, state, output);
			add_token(node_tree, *i - 1, operator);
			int left_operand = add_node(node_tree, n_expr, specific_expr_type, state, output);
			add_token(node_tree, *i - 2, left_operand);

			set_second(node_tree, node_index, operator, state);
			set_parent(node_tree, left_operand, operator, state);
			set_parent(node_tree, operator, node_index, state);
			set_first(node_tree, operator, left_operand, state);
			set_indent(node_tree, operator, state);
			set_indent(node_tree, left_operand, state);

			return parse_s(9, ti, i, node_tree, operator, fun_parent, output);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator to follow variable value \"%s\"(%lu:%lu) at %lu:%lu.\n",
			t[*i-2].val, t[*i-2].line_number, t[*i-2].char_number, t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	// return num(int) ; -> s1
	// return num(float) ; -> s1
	// return id ; -> s1
	// return num(int) binop -> s10
	// return num(float) binop -> s10
	// return id binop -> s10
	// return ; -> s1
	// type id = -> s5 -> ?
	else if (state == 6) {
		if (check_type(ti, t_return, *i)) {
			printf("state 6 returning.\n");
			*i += 1;
			int specific_expr_type = -1;
			int return_node_index = add_node(node_tree, n_stat, s_return, state, output);
			set_second(node_tree, node_index, return_node_index, state);
			set_parent(node_tree, return_node_index, node_index, state);
			set_indent(node_tree, return_node_index, state);

			if 		(check_type(ti, t_num_int, *i)) 	specific_expr_type = e_val;
			else if (check_type(ti, t_num_float, *i)) 	specific_expr_type = e_val;
			else if (check_type(ti, t_id, *i)) 			specific_expr_type = e_id;
			else {
				printf("Parse error. Expected a value or semi-colon to end function body at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				return -1;
			}
			*i += 1;

			if (check_type(ti, t_semicolon, *i)) {
				*i += 1;
				int expr_node_index = add_node(node_tree, n_expr, specific_expr_type, state, output);
				set_parent(node_tree, expr_node_index, return_node_index, state);
				set_second(node_tree, return_node_index, expr_node_index, state);
				set_indent(node_tree, expr_node_index, state);
				add_token(node_tree, *i - 2, expr_node_index);
				return parse_s(1, ti, i, node_tree, return_node_index, node_tree->nodes[fun_parent].parent, output);
			}
			else if (check_type(ti, t_binop, *i)) {
				*i += 1;
				int operator = add_node(node_tree, n_expr, e_binop, state, output);
				set_second(node_tree, return_node_index, operator, state);
				set_parent(node_tree, operator, return_node_index, state);
				set_indent(node_tree, operator, state);
				add_token(node_tree, *i - 1, operator);
				fprintf(output, "%3d: Binop (%s) node.\n  Created in state %d.\n",
					operator, ti->ts[operator].val, state);
				int left_operand = add_node(node_tree, n_expr, specific_expr_type, state, output);
				set_first(node_tree, operator, left_operand, state);
				set_parent(node_tree, left_operand, operator, state);
				set_indent(node_tree, left_operand, state);
				add_token(node_tree, *i - 2, left_operand);
				return parse_s(7, ti, i, node_tree, operator, fun_parent, output);
			}
			else {
				printf("Parse error. Expected a semi-colon to end function body at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				return -1;
			}
		}
		else if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			int new_node_index = add_node(node_tree, n_stat, s_return, state, output);
			set_second(node_tree, node_index, new_node_index, state);
			set_parent(node_tree, new_node_index, node_index, state);
			set_indent(node_tree, new_node_index, state);
			return parse_s(1, ti, i, node_tree, new_node_index, fun_parent, output);
		}
		// TODO: THIS ONLY SUPPORTS LET-STATEMENTS FOR NOW
		if (check_type(ti, t_type, *i)) *i += 1;
		else {
			printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		if (check_type(ti, t_id, *i)) *i += 1;
		else {
			printf("Parse error. Expected a variable name at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		if (check_type(ti, t_eq, *i)) *i += 1;
		else {
			printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		int new_node_index = add_node(node_tree, n_stat, s_var_bind, state, output);
		set_parent(node_tree, new_node_index, node_index, state);
		set_indent(node_tree, new_node_index, state);
		add_token(node_tree, *i - 3, new_node_index);
		add_token(node_tree, *i - 2, new_node_index);

		return parse_s(parse_s(5, ti, i, node_tree, new_node_index, fun_parent, output),
			ti, i, node_tree, node_index, fun_parent, output);
	}
	// return value expression from first function body statement
	// id ; -> s1
	// id binop id ; -> s1
	// id binop id binop -> s7
	// id binop num(int) ; -> s1
	// id binop num(int) binop -> s7
	// num(int) ; -> s1
	// num(int) binop id ; -> s1
	// num(int) binop id binop -> s7
	// num(int) binop num(int) ; -> s1
	// num(int) binop num(int) binop -> s7
	else if (state == 7) {
		enum e_expr expr_specific_type = -1;
		if 		(check_type(ti, t_id, *i)) expr_specific_type = e_id;
		else if (check_type(ti, t_num_int, *i) || check_type(ti, t_num_float, *i)) expr_specific_type = e_val;
		else {
			printf("Parse error. Expected a return value or semi-colon to end function body at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}

		*i += 1;
		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			int expr_node_index = add_node(node_tree, n_expr, expr_specific_type, state, output);
			set_parent(node_tree, expr_node_index, node_index, state);
			set_second(node_tree, node_index, expr_node_index, state);
			set_indent(node_tree, expr_node_index, state);
			add_token(node_tree, *i - 2, expr_node_index);
			return parse_s(1, ti, i, node_tree, expr_node_index, node_tree->nodes[fun_parent].parent, output);
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int operator = add_node(node_tree, n_expr, e_binop, state, output);
			int expr_node_index = add_node(node_tree, n_expr, expr_specific_type, state, output);
			add_token(node_tree, *i - 2, expr_node_index);
			add_token(node_tree, *i - 1, operator);
			set_parent(node_tree, operator, node_index, state);
			set_parent(node_tree, expr_node_index, operator, state);
			set_first(node_tree, operator, expr_node_index, state);
			set_second(node_tree, node_index, operator, state);
			set_indent(node_tree, operator, state);
			set_indent(node_tree, expr_node_index, state);
			return parse_s(8, ti, i, node_tree, operator, fun_parent, output);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	// Parsing "y (+) z" in expression "x (+) y (+) z"
	// node_index is "y (+) z"
	// node_index.first = y
	// node_index.parent = binop expression x (+) node_index
	// z is unknown and could be another binop expression.
	// z is currently pointed to by *i.
	// id/num ; -> s1
	// id/num binop -> s8
	else if (state == 8) {
		enum e_expr expr_specific_type = -1;
		if 		(check_type(ti, t_id, *i)) expr_specific_type = e_id;
		else if (check_type(ti, t_num_int, *i)) expr_specific_type = e_val;
		else if (check_type(ti, t_num_float, *i)) expr_specific_type = e_val;
		else {
			printf("Parse error. Expected a right operand at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			int expr_node_index = add_node(node_tree, n_expr, expr_specific_type, state, output);
			set_parent(node_tree, expr_node_index, node_index, state);
			set_second(node_tree, node_index, expr_node_index, state);
			set_indent(node_tree, expr_node_index, state);
			add_token(node_tree, *i - 2, expr_node_index);
			return parse_s(1, ti, i, node_tree, node_index, node_tree->nodes[fun_parent].parent, output);
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int operator = add_node(node_tree, n_expr, e_binop, state, output);
			int expr_node_index = add_node(node_tree, n_expr, expr_specific_type, state, output);
			add_token(node_tree, *i - 2, expr_node_index);
			add_token(node_tree, *i - 1, operator);
			set_parent(node_tree, operator, node_index, state);
			set_parent(node_tree, expr_node_index, operator, state);
			set_first(node_tree, operator, expr_node_index, state);
			set_second(node_tree, node_index, operator, state);
			set_indent(node_tree, operator, state);
			set_indent(node_tree, expr_node_index, state);

			char *left_operator = (node_tree->nodes[node_index].token_count > 0)
				? ti->ts[node_tree->nodes[node_index].token_indices[0]].val
				: ti->ts[*i - 3].val;

			return parse_s(8, ti, i, node_tree, operator, fun_parent, output);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}

	// Parsing "y" in expression "x (+) y"
	// node_index is "x (+)"
	// node_index.first = x
	// node_index.parent = unknown
	// y is unknown and could be another binop expression.
	// y is currently pointed to by *i.
	// id/num ; -> return 6
	// id/num binop -> s9
	else if (state == 9) {
		enum e_expr specific_type = -1;
		if 		(check_type(ti, t_id, *i)) specific_type = e_id;
		else if (check_type(ti, t_num_int, *i)) specific_type = e_val;
		else if (check_type(ti, t_num_float, *i)) specific_type = e_val;
		else {
			printf("Parse error. Expected a right operand at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		int middle_operand = add_node(node_tree, n_expr, specific_type, state, output);
		set_parent(node_tree, middle_operand, node_index, state);
		set_indent(node_tree, middle_operand, state);
		add_token(node_tree, *i - 1, middle_operand);

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			return 6;
		}
		// Parsing the second binary operator in "x (+) y (+) z".
		// node_index is the first binary operator.
		// node_index.first = x
		// node_index.second = not initialised yet
		// node_index.parent is unknown.
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int right_binop = add_node(node_tree, n_expr, e_binop, state, output);
			set_parent(node_tree, right_binop, node_index, state);
			set_indent(node_tree, right_binop, state);
			add_token(node_tree, *i - 1, right_binop);

			set_parent(node_tree, middle_operand, right_binop, state);
			set_first(node_tree, right_binop, middle_operand, state);

			char *left_operator = (node_tree->nodes[node_index].token_count > 0)
				? ti->ts[node_tree->nodes[node_index].token_indices[0]].val
				: ti->ts[*i - 3].val;
			char *right_operator = ti->ts[*i - 1].val;

			if ((strcmp(left_operator, "+") == 0) || (strcmp(left_operator, "-") == 0)) {
				// x + y * z
				if (strcmp(right_operator, "*") == 0 || strcmp(right_operator, "/") == 0) {
					set_second(node_tree, node_index, right_binop, state);
				}
				// x + y + z
				else {
					set_second(node_tree, node_tree->nodes[node_index].parent, right_binop, state);
					set_parent(node_tree, node_index, right_binop, state);
					set_second(node_tree, node_index, middle_operand, state);
					set_first(node_tree, right_binop, node_index, state);
					set_parent(node_tree, middle_operand, node_index, state);
					return parse_s(9, ti, i, node_tree, right_binop, fun_parent, output);
				}
			}
			// x * y + z
			else {
				if (VERBOSE) {
					printf("Switching the order of operations to align with PEMDAS.\n");
				}
				set_second(node_tree, node_tree->nodes[node_index].parent, right_binop, state);
				set_parent(node_tree, node_index, right_binop, state);
				set_second(node_tree, node_index, middle_operand, state);
				set_first(node_tree, right_binop, node_index, state);
				set_parent(node_tree, middle_operand, node_index, state);
				return parse_s(9, ti, i, node_tree, right_binop, fun_parent, output);
			}
			return parse_s(9, ti, i, node_tree, right_binop, fun_parent, output);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	// Parsing "y" in expression "return x (+) y"
	// node_index is "x (+)"
	// node_index.first = x
	// node_index.parent = return
	// y is unknown and could be another binop expression.
	// y is currently pointed to by *i.
	// id/num ; -> s1
	// id/num binop -> s10
	else if (state == 10) {
		enum e_expr specific_type = -1;
		if 		(check_type(ti, t_id, *i)) specific_type = e_id;
		else if (check_type(ti, t_num_int, *i)) specific_type = e_val;
		else if (check_type(ti, t_num_float, *i)) specific_type = e_val;
		else {
			printf("Parse error. Expected a right operand at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		int right_operand = add_node(node_tree, n_expr, specific_type, state, output);
		set_parent(node_tree, right_operand, node_index, state);
		set_indent(node_tree, right_operand, state);
		add_token(node_tree, *i - 1, right_operand);

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			return parse_s(1, ti, i, node_tree, node_index, fun_parent, output);
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int right_binop = add_node(node_tree, n_expr, e_binop, state, output);
			set_parent(node_tree, right_binop, node_index, state);
			set_indent(node_tree, right_binop, state);
			add_token(node_tree, *i - 1, right_binop);

			set_parent(node_tree, right_operand, right_binop, state);
			set_first(node_tree, right_binop, right_operand, state);

			char *left_operator = (node_tree->nodes[node_index].token_count > 0)
				? ti->ts[node_tree->nodes[node_index].token_indices[0]].val
				: ti->ts[*i - 3].val;
			char *right_operator = ti->ts[*i - 1].val;

			if (strcmp(left_operator, "*") == 0 || strcmp(left_operator, "/") == 0) {
				set_second(node_tree, node_index, right_binop, state);
			}
			else {
				set_first(node_tree, node_index, right_binop, state);
				set_second(node_tree, node_index, node_tree->nodes[node_index].first, state);
			}
			return parse_s(10, ti, i, node_tree, right_binop, fun_parent, output);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	else if (state == 11) {
		return -4;
	}
	else return -1;
}

struct tree* parse(FILE* f, FILE* output, struct token_index* ti) {
	char c = fgetc(f);
	if (c == EOF) {
		printf("Parse error. Empty token file.\n");
		return NULL;
	}
	ungetc(c, f);
	int token_index = 0;
	struct tree *node_tree = malloc(sizeof(*node_tree));
	node_tree->nodes = malloc(sizeof(*node_tree->nodes));
	node_tree->n = 0;
	int final_state = parse_s(0, ti, &token_index, node_tree, 0, 0, output);
	if (final_state == END_STATE) return node_tree;
	else return NULL;
}