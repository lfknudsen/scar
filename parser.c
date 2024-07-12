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
	function_declaration_head,
	function_declaration_params,
	function_declaration_body,

};

/*
s0 -> s1
s1 -> Type Id ( -> s2
s1 -> -> END
s2 -> Type Id -> s3
s2 -> ) = -> s4
s3 -> , -> s2
s3 -> ) = -> s4
s4 -> return ; -> s1 (warning)
s4 -> return Id ; -> s1
s4 -> Type Id = -> s5
s5 -> Num ; -> s6
s5 -> Id ; -> s6
s5 -> Num/Val BINOP Num/Val ; -> s6 *unimplemented
s6 -> Type Id = -> s5
s6 -> return ; -> s1
s6 -> return Id ; -> s1
*/

void debug_print(int state, unsigned int index) {
	if (VERBOSE) printf("Current state: %d\nToken index: %u\n", state, index);
}

// Nodes are evaluated first -> second -> self (since evaluating self *is* evaluating the children)
int add_node(struct tree *node_tree, int node_type, int specific_type) {
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
	if (VERBOSE) printf("Adding new node.\n");
	return node_tree->n - 1;
}

void add_token(struct tree *node_tree, int t_index, int n_index) {
	struct node *n = &node_tree->nodes[n_index];
	n->token_count += 1;
	n->token_indices =
		realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
	n->token_indices[n->token_count - 1] = t_index;
	if (VERBOSE) printf("Adding to node %d: %d\n", n_index, t_index);
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

int parse_s(int state, struct token_index *ti, int *i, struct tree* node_tree, int node_index, int fun_parent) {
	if (VERBOSE) printf("State: %d. Token index: %d\n", state, *i);

	struct token *t = ti->ts;
	if (state == program_start) {
		assert(node_tree->n == 0);
		int top_node_index = add_node(node_tree, n_prog, 0);

		return parse_s(1, ti, i, node_tree, 0, top_node_index);
	}

	// Expecting a function declaration. State checks type, ID, and beginning parentheses to contain parameters.
	// If these are correct, add a program node (n_prog), then a function bind statement node (n_stat + s_fun_bind),
	// and finally a parameter expression node. The parameter node will be evaluated first, then the function bind
	// node, before returning the result up the chain to evaluate the program node.
	// A program contains one or more functions.
	else if (state == function_declaration_head) {
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

		int top_node_index = add_node(node_tree, n_prog, 0);
		node_tree->nodes[fun_parent].second = top_node_index;
		node_tree->nodes[top_node_index].parent = fun_parent;

		int fun_declaration = add_node(node_tree, n_stat, s_fun_bind);
		add_token(node_tree, *i - 3, fun_declaration);
		add_token(node_tree, *i - 2, fun_declaration);
		node_tree->nodes[fun_declaration].parent = top_node_index;
		node_tree->nodes[fun_declaration].print_indent = 1;
		node_tree->nodes[top_node_index].first = fun_declaration;

		int new_param_node_index = add_node(node_tree, n_expr, e_param);
		node_tree->nodes[new_param_node_index].parent = fun_declaration;
		node_tree->nodes[new_param_node_index].print_indent = 2;

		node_tree->nodes[fun_declaration].first = new_param_node_index;

		return parse_s(2, ti, i, node_tree, new_param_node_index, top_node_index);
	}

	// Function parameter(s).
	// First check if at the end of the parameter declarations with ") ="
	// Otherwise, check for a parameter declaration with "type id".
	//    If so, add them as tokens, and go to state 
	else if (state == function_declaration_params) {
		if (check_type(ti, t_par_end, *i)) {
			*i += 1;
			if (check_type(ti, t_eq, *i)) {
				*i += 1;
				return parse_s(4, ti, i, node_tree, node_tree->nodes[node_index].parent, fun_parent);
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
		return parse_s(3, ti, i, node_tree, node_index, fun_parent);
	}
	// Function parameter follow-up (either another parameter or the start of function body)
	else if (state == 3) {
		if (check_type(ti, t_comma, *i)) {
			*i += 1;
			return parse_s(2, ti, i, node_tree, node_index, fun_parent);
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
		return parse_s(4, ti, i, node_tree, node_tree->nodes[node_index].parent, fun_parent);
	}
	// First statement in function body.
	else if (state == 4) {
		if (check_type(ti, t_return, *i)) {
			*i += 1;
			int return_node_index = add_node(node_tree, n_stat, s_return);
			node_tree->nodes[node_index].second = return_node_index;
			node_tree->nodes[return_node_index].parent = node_index;
			node_tree->nodes[return_node_index].print_indent =
				node_tree->nodes[node_tree->nodes[return_node_index].parent].print_indent + 1;

			if (check_type(ti, t_semicolon, *i)) {
				printf("Warning: Empty function body at %lu:%lu.\n", t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				*i += 1;
				return parse_s(1, ti, i, node_tree, return_node_index, fun_parent);
			}
			else {
				return parse_s(7, ti, i, node_tree, return_node_index, fun_parent);
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
		int new_node_index = add_node(node_tree, n_stat, s_var_bind);
		node_tree->nodes[new_node_index].parent = node_index;
		node_tree->nodes[node_index].second = new_node_index;
		node_tree->nodes[new_node_index].print_indent =
			node_tree->nodes[node_tree->nodes[new_node_index].parent].print_indent + 1;

		add_token(node_tree, *i - 3, new_node_index);
		add_token(node_tree, *i - 2, new_node_index);

		return
			parse_s (
				parse_s(5, ti, i, node_tree, new_node_index, fun_parent),
			ti, i, node_tree, node_index, fun_parent
			);
	}
	else if (state == 5) {
		assert(
			!(node_tree->nodes[node_index].nodetype == n_stat
			&& node_tree->nodes[node_index].specific_type == s_fun_bind));

		int specific_expr_type = -1;
		if 			(check_type(ti, t_num_int, *i)) 	specific_expr_type = e_val;
		else if (check_type(ti, t_num_float, *i)) specific_expr_type = e_val;
		else if (check_type(ti, t_id, *i)) 				specific_expr_type = e_id;
		else {
			printf("Parse error. Expected a value to bind to variable at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			int left_operand = add_node(node_tree, n_expr, specific_expr_type);
			add_token(node_tree, *i - 2, left_operand);
			node_tree->nodes[node_index].second = left_operand;
			node_tree->nodes[left_operand].parent = node_index;
			node_tree->nodes[left_operand].print_indent =
				node_tree->nodes[node_tree->nodes[left_operand].parent].print_indent + 1;
			return 6;
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int operator = add_node(node_tree, n_expr, e_binop);
			add_token(node_tree, *i - 1, operator);
			int left_operand = add_node(node_tree, n_expr, specific_expr_type);
			add_token(node_tree, *i - 2, left_operand);

			node_tree->nodes[node_index].second = operator;
			node_tree->nodes[left_operand].parent = operator;
			node_tree->nodes[operator].parent = node_index;
			node_tree->nodes[operator].first = left_operand;

			node_tree->nodes[operator].print_indent =
				node_tree->nodes[node_tree->nodes[operator].parent].print_indent + 1;
			node_tree->nodes[left_operand].print_indent =
				node_tree->nodes[node_tree->nodes[left_operand].parent].print_indent + 1;

			return parse_s(9, ti, i, node_tree, operator, fun_parent);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator to follow variable value at %lu:%lu.\n",
			t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	else if (state == 6) {
		if (check_type(ti, t_return, *i)) {
			*i += 1;
			int specific_expr_type = -1;
			if 			(check_type(ti, t_num_int, *i)) 	specific_expr_type = e_val;
			else if (check_type(ti, t_num_float, *i)) specific_expr_type = e_val;
			else if (check_type(ti, t_id, *i)) 				specific_expr_type = e_id;
			else {
					printf("Parse error. Expected a value or semi-colon to end function body at %lu:%lu.\n",
					t[*i].line_number, t[*i].char_number);
					debug_print(state, *i);
					return -1;
			}
			*i += 1;
			int return_node_index = add_node(node_tree, n_stat, s_return);
			node_tree->nodes[node_index].first = return_node_index;
			node_tree->nodes[return_node_index].parent = node_index;
			node_tree->nodes[return_node_index].print_indent =
				node_tree->nodes[node_tree->nodes[return_node_index].parent].print_indent + 1;
			if (check_type(ti, t_semicolon, *i)) {
				*i += 1;
				add_token(node_tree, *i - 2, return_node_index);
				return parse_s(1, ti, i, node_tree, return_node_index, fun_parent);
			}
			else if (check_type(ti, t_binop, *i)) {
				*i += 1;
				int operator = add_node(node_tree, n_expr, e_binop);
				node_tree->nodes[return_node_index].first = operator;
				node_tree->nodes[operator].parent = return_node_index;
				node_tree->nodes[operator].print_indent =
					node_tree->nodes[node_tree->nodes[operator].parent].print_indent + 1;
				add_token(node_tree, *i - 1, operator);
				int left_operand = add_node(node_tree, n_expr, specific_expr_type);
				node_tree->nodes[operator].first = left_operand;
				node_tree->nodes[left_operand].parent = operator;
				node_tree->nodes[left_operand].print_indent =
					node_tree->nodes[node_tree->nodes[left_operand].parent].print_indent + 1;
				add_token(node_tree, *i - 2, left_operand);
				return parse_s(10, ti, i, node_tree, operator, fun_parent);
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
			int new_node_index = add_node(node_tree, n_stat, s_return);
			node_tree->nodes[node_index].second = new_node_index;
			node_tree->nodes[new_node_index].parent = node_index;
			node_tree->nodes[new_node_index].print_indent =
				node_tree->nodes[node_tree->nodes[new_node_index].parent].print_indent + 1;
			return parse_s(1, ti, i, node_tree, new_node_index, fun_parent);
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
		int new_node_index = add_node(node_tree, n_stat, s_var_bind);
		node_tree->nodes[new_node_index].parent = node_index;
		node_tree->nodes[new_node_index].print_indent =
			node_tree->nodes[node_tree->nodes[new_node_index].parent].print_indent + 1;
		add_token(node_tree, *i - 3, new_node_index);
		add_token(node_tree, *i - 2, new_node_index);

		return parse_s(parse_s(5, ti, i, node_tree, new_node_index, fun_parent),
			ti, i, node_tree, node_index, fun_parent);
	}
	// return value expression from first function body statement
	else if (state == 7) {
		enum e_expr expr_specific_type = -1;
		if (check_type(ti, t_id, *i)) expr_specific_type = e_id;
		else if (check_type(ti, t_num_int, *i) || check_type(ti, t_num_float, *i)) expr_specific_type = e_val;
		else {
			printf("Parse error. Expected a return value or semi-colon to end function body at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}

		*i += 1;
		int expr_node_index = add_node(node_tree, n_expr, expr_specific_type);
		node_tree->nodes[expr_node_index].parent = node_index;
		node_tree->nodes[node_index].second = expr_node_index;
		node_tree->nodes[expr_node_index].print_indent =
			node_tree->nodes[node_tree->nodes[expr_node_index].parent].print_indent + 1;

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			add_token(node_tree, *i - 2, expr_node_index);
			return parse_s(1, ti, i, node_tree, expr_node_index, fun_parent);
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			node_tree->nodes[expr_node_index].specific_type = e_binop;
			add_token(node_tree, *i - 1, expr_node_index);

			int left_operand = add_node(node_tree, n_expr, expr_specific_type);
			node_tree->nodes[left_operand].parent = expr_node_index;
			node_tree->nodes[left_operand].print_indent =
				node_tree->nodes[node_tree->nodes[left_operand].parent].print_indent + 1;
			add_token(node_tree, *i - 2, left_operand);

			if (check_type(ti, t_id, *i)) expr_specific_type = e_id;
			else if (check_type(ti, t_num_int, *i) || check_type(ti, t_num_float, *i)) expr_specific_type = e_val;
			else {
				printf("Parse error. Expected a right operand at %lu:%lu.\n",
					t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				return -1;
			}
			*i += 1;
			int right_operand = add_node(node_tree, n_expr, expr_specific_type);
			node_tree->nodes[right_operand].parent = expr_node_index;
			node_tree->nodes[right_operand].print_indent =
				node_tree->nodes[node_tree->nodes[right_operand].parent].print_indent + 1;
			add_token(node_tree, *i - 1, right_operand);

			if (check_type(ti, t_semicolon, *i)) {
				*i += 1;
				node_tree->nodes[expr_node_index].first = left_operand;
				node_tree->nodes[expr_node_index].second = right_operand;
				return parse_s(1, ti, i, node_tree, expr_node_index, fun_parent);
			}
			else if (check_type(ti, t_binop, *i)) {
				*i += 1;
				int right_binop = add_node(node_tree, n_expr, e_binop);
				node_tree->nodes[right_binop].parent = expr_node_index;
				node_tree->nodes[right_binop].print_indent =
					node_tree->nodes[node_tree->nodes[right_binop].parent].print_indent + 1;
				node_tree->nodes[right_binop].first = right_operand;
				node_tree->nodes[right_operand].parent = right_binop;
				add_token(node_tree, *i - 1, right_binop);

				char *left_operator = ti->ts[*i - 3].val;
				char *right_operator = ti->ts[*i - 1].val;

				if (strcmp(left_operator, "*") == 0 || strcmp(left_operator, "/") == 0) {
					node_tree->nodes[expr_node_index].first = left_operand;
					node_tree->nodes[expr_node_index].second = right_binop;
				}
				else {
					node_tree->nodes[expr_node_index].first = right_binop;
					node_tree->nodes[expr_node_index].second = left_operand;
				}

				return parse_s(7, ti, i, node_tree, right_binop, fun_parent);
			}
			else {
				printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
					t[*i].line_number, t[*i].char_number);
				debug_print(state, *i);
				return -1;
			}
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	// Parsing "y (+) z" in expression x (+) y (+) z
	// node_index is "y (+) z"
	// node_index.first = y
	// node_index.parent = binop expression x (+) node_index
	// z is unknown and could be another binop expression.
	// z is currently pointed to by *i.
	else if (state == 8) {
		enum e_expr specific_type = -1;
		if (check_type(ti, t_id, *i)) specific_type = e_id;
		else if (check_type(ti, t_num_int, *i)) specific_type = e_val;
		else if (check_type(ti, t_num_float, *i)) specific_type = e_val;
		else {
			printf("Parse error. Expected a right operand at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		int right_operand = add_node(node_tree, n_expr, specific_type);
		node_tree->nodes[right_operand].parent = node_index;
		node_tree->nodes[right_operand].print_indent =
			node_tree->nodes[node_tree->nodes[right_operand].parent].print_indent + 1;
		add_token(node_tree, *i - 1, right_operand);

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			return parse_s(1, ti, i, node_tree, node_index, fun_parent);
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int right_binop = add_node(node_tree, n_expr, e_binop);
			node_tree->nodes[right_binop].parent = node_index;
			node_tree->nodes[right_binop].print_indent =
				node_tree->nodes[node_tree->nodes[right_binop].parent].print_indent + 1;
			add_token(node_tree, *i - 1, right_binop);

			node_tree->nodes[right_operand].parent = right_binop;
			node_tree->nodes[right_binop].first = right_operand;

			char *left_operator = (node_tree->nodes[node_index].token_count > 0)
				? ti->ts[node_tree->nodes[node_index].token_indices[0]].val
				: ti->ts[*i - 3].val;
			char *right_operator = ti->ts[*i - 1].val;

			if (strcmp(left_operator, "*") == 0 || strcmp(left_operator, "/") == 0) {
				node_tree->nodes[node_index].second = right_binop;
			}
			else {
				node_tree->nodes[node_index].second = node_tree->nodes[node_index].first;
				node_tree->nodes[node_index].first = right_binop;
			}
			return parse_s(8, ti, i, node_tree, right_binop, fun_parent);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}

	// Parsing "y" in expression x (+) y
	// node_index is "x (+)"
	// node_index.first = x
	// node_index.parent = unknown
	// y is unknown and could be another binop expression.
	// y is currently pointed to by *i.
	else if (state == 9) {
		enum e_expr specific_type = -1;
		if (check_type(ti, t_id, *i)) specific_type = e_id;
		else if (check_type(ti, t_num_int, *i)) specific_type = e_val;
		else if (check_type(ti, t_num_float, *i)) specific_type = e_val;
		else {
			printf("Parse error. Expected a right operand at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		int right_operand = add_node(node_tree, n_expr, specific_type);
		node_tree->nodes[right_operand].parent = node_index;
		node_tree->nodes[right_operand].print_indent =
			node_tree->nodes[node_tree->nodes[right_operand].parent].print_indent + 1;
		add_token(node_tree, *i - 1, right_operand);

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			return 6;
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int right_binop = add_node(node_tree, n_expr, e_binop);
			node_tree->nodes[right_binop].parent = node_index;
			node_tree->nodes[right_binop].print_indent =
				node_tree->nodes[node_tree->nodes[right_binop].parent].print_indent + 1;
			add_token(node_tree, *i - 1, right_binop);

			node_tree->nodes[right_operand].parent = right_binop;
			node_tree->nodes[right_binop].first = right_operand;

			char *left_operator = (node_tree->nodes[node_index].token_count > 0)
				? ti->ts[node_tree->nodes[node_index].token_indices[0]].val
				: ti->ts[*i - 3].val;
			char *right_operator = ti->ts[*i - 1].val;

			if (strcmp(left_operator, "*") == 0 || strcmp(left_operator, "/") == 0) {
				node_tree->nodes[node_index].second = right_binop;
			}
			else {
				node_tree->nodes[node_index].second = node_tree->nodes[node_index].first;
				node_tree->nodes[node_index].first = right_binop;
			}
			return parse_s(9, ti, i, node_tree, right_binop, fun_parent);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	// Parsing "y" in expression return x (+) y
	// node_index is "x (+)"
	// node_index.first = x
	// node_index.parent = return
	// y is unknown and could be another binop expression.
	// y is currently pointed to by *i.
	else if (state == 10) {
		enum e_expr specific_type = -1;
		if (check_type(ti, t_id, *i)) specific_type = e_id;
		else if (check_type(ti, t_num_int, *i)) specific_type = e_val;
		else if (check_type(ti, t_num_float, *i)) specific_type = e_val;
		else {
			printf("Parse error. Expected a right operand at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
		*i += 1;
		int right_operand = add_node(node_tree, n_expr, specific_type);
		node_tree->nodes[right_operand].parent = node_index;
		node_tree->nodes[right_operand].print_indent =
			node_tree->nodes[node_tree->nodes[right_operand].parent].print_indent + 1;
		add_token(node_tree, *i - 1, right_operand);

		if (check_type(ti, t_semicolon, *i)) {
			*i += 1;
			return parse_s(1, ti, i, node_tree, node_index, fun_parent);
		}
		else if (check_type(ti, t_binop, *i)) {
			*i += 1;
			int right_binop = add_node(node_tree, n_expr, e_binop);
			node_tree->nodes[right_binop].parent = node_index;
			node_tree->nodes[right_binop].print_indent =
				node_tree->nodes[node_tree->nodes[right_binop].parent].print_indent + 1;
			add_token(node_tree, *i - 1, right_binop);

			node_tree->nodes[right_operand].parent = right_binop;
			node_tree->nodes[right_binop].first = right_operand;

			char *left_operator = (node_tree->nodes[node_index].token_count > 0)
				? ti->ts[node_tree->nodes[node_index].token_indices[0]].val
				: ti->ts[*i - 3].val;
			char *right_operator = ti->ts[*i - 1].val;

			if (strcmp(left_operator, "*") == 0 || strcmp(left_operator, "/") == 0) {
				node_tree->nodes[node_index].second = right_binop;
			}
			else {
				node_tree->nodes[node_index].second = node_tree->nodes[node_index].first;
				node_tree->nodes[node_index].first = right_binop;
			}
			return parse_s(10, ti, i, node_tree, right_binop, fun_parent);
		}
		else {
			printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
				t[*i].line_number, t[*i].char_number);
			debug_print(state, *i);
			return -1;
		}
	}
	else return -1;
}

struct tree* parse(FILE *f, struct token_index *ti) {
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
	int final_state = parse_s(0, ti, &token_index, node_tree, 0, 0);
	if (final_state == END_STATE) return node_tree;
	else return NULL;
}