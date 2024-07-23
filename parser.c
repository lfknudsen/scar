#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define NODE node_tree->nodes[node_index]
#define NODES node_tree->nodes
#define TREE node_tree
#define NODE_COUNT node_tree->n

extern int state_1(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_2(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_5(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);

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
s4 -> Type Id = -> s5 (everything up to semicolon is saved to fst. Then next node is saved to snd. Then back to s4 again.)
s4 -> return -> s7
s5 -> Val ; -> s6
s5 -> Val BINOP -> s8
s6 -> Type Id = -> s5
s6 -> return ; -> s1
s6 -> return Id ; -> s1
s7 -> ; -> s1 (warning)
s7 -> Val ; -> s1
s7 -> Val BINOP -> s8
s8 -> Val ; -> (return the new node back up to fst).
s8 -> Val BINOP -> s8
*/

void set_first(struct tree* n_tree, int n_index, int value, int state, int out) {
    if (out >= verbose) {
        if (n_tree->nodes[n_index].first != -1) {
            printf("Node %d's first set to %d while in state %d (OVERRIDE).\n", n_index, value, state);
        }
        else {
            printf("Node %d's first set to %d while in state %d.\n", n_index, value, state);
        }
    }
    n_tree->nodes[n_index].first = value;
}

void set_second(struct tree* n_tree, int n_index, int value, int state, int out) {
    if (out >= verbose) {
        if (n_tree->nodes[n_index].second != -1) {
            printf("Node %d's second set to %d while in state %d (OVERRIDE).\n", n_index, value, state);
        }
        else {
            printf("Node %d's second set to %d while in state %d.\n", n_index, value, state);
        }
    }
    n_tree->nodes[n_index].second = value;
}

void set_parent(struct tree* n_tree, int n_index, int value, int state, int out) {
    if (out >= verbose) {
        if (n_tree->nodes[n_index].parent != -1) {
            printf("Node %d's parent set to %d while in state %d (OVERRIDE).\n", n_index, value, state);
        }
        else {
            printf("Node %d's parent set to %d while in state %d.\n", n_index, value, state);
        }
    }
    n_tree->nodes[n_index].parent = value;
}

void set_indent(struct tree* n_tree, int n_index, int state, int out) {
    if (n_tree->nodes[n_index].parent == -1) {
        if (out >= verbose) printf("Node %d doesn't have a parent but its indent is being set.\n", n_index);
        return;
    }
    n_tree->nodes[n_index].print_indent =
        n_tree->nodes[n_tree->nodes[n_index].parent].print_indent + 1;
    if (out > verbose) {
        printf("Node %d's indentation set to %d while in state %d.\n",
            n_index, n_tree->nodes[n_index].print_indent, state);
    }
}

void debug_print(int state, unsigned int index, int out) {
    if (out >= verbose) printf("Current state: %d\nToken index: %u\n", state, index);
}

// Initialise and add a new node to the tree.
// Return: number of nodes - 1 (i.e. the index of the new node)
// See eval.c for evaluation details, as these differ based on the node's type(s).
int add_node(struct tree* node_tree, int node_type, int specific_type, int state, FILE* output, int out) {
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
    if (out >= verbose) {
        printf("Adding new node %d: ", node_tree->n - 1);
        print_node(node_tree, node_tree->n - 1, out);
        printf("\n");
    }
    fprintf(output, "Created new node in state %d:\n", state);
    fprint_node(node_tree, node_tree->n - 1, output, out);
    fprintf(output, "\n");
    return node_tree->n - 1;
}

void add_token(struct tree* node_tree, int t_index, int n_index, int out) {
    struct node* n = &node_tree->nodes[n_index];
    n->token_count += 1;
    n->token_indices =
        realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
    n->token_indices[n->token_count - 1] = t_index;
    if (out >= verbose) printf("Adding token %d to node %d.\n", t_index, n_index);
}

void connect_token(struct tree* n_tree, int n_index, int t_index, int out) {
    struct node* n = &n_tree->nodes[n_index];
    n->token_count += 1;
    n->token_indices =
        realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
    n->token_indices[n->token_count - 1] = t_index;
    if (out >= verbose) printf("Adding token %d to node %d.\n", t_index, n_index);
}

int check_type(struct token_index *ti, enum e_token expected, int i, int out) {
    if (ti->n <= i) {
        if (out >= standard) printf("Parse error. Code terminates prematurely at %lu:%lu.\n",
        ti->ts[ti->n - 1].line_number, ti->ts[ti->n - 1].char_number);
        return 0;
    }
    return (ti->ts[i].type == expected);
}
int state_11(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 11;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    enum e_expr specific_type = -1;
    if 		(check_type(ti, t_id, *i, out))         specific_type = e_id;
    else if (check_type(ti, t_num_int, *i, out))    specific_type = e_val;
    else if (check_type(ti, t_num_float, *i, out))  specific_type = e_val;
    else {
        if (out >= standard) printf("Parse error. Expected a right operand at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    *i += 1;

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
        set_parent(n_tree, expression, n_index, state, out);
        set_second(n_tree, n_index, expression, state, out);
        set_indent(n_tree, expression, state, out);
        add_token(n_tree, *i - 2, expression, out);
        return state_1(ti, i, n_tree, n_index, parent_func, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semicolon to finish comparison at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
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
int state_10(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 10;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    enum e_expr specific_type = -1;
    if 		(check_type(ti, t_id, *i, out))         specific_type = e_id;
    else if (check_type(ti, t_num_int, *i, out))    specific_type = e_val;
    else if (check_type(ti, t_num_float, *i, out))  specific_type = e_val;
    else {
        if (out >= standard) printf("Parse error. Expected a right operand at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    *i += 1;

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
        set_parent(n_tree, expression, n_index, state, out);
        set_second(n_tree, n_index, expression, state, out);
        set_indent(n_tree, expression, state, out);
        add_token(n_tree, *i - 2, expression, out);
        return parent_func;
    }
    // Parsing the second binary operator in "x (+) y (+) z".
    // node_index is the first binary operator.
    // node_index.first = x
    // node_index.second = not initialised yet
    // node_index.parent is unknown.
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
        add_token(n_tree, *i - 2, expression, out);
        add_token(n_tree, *i - 1, right_operator, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : ti->ts[*i - 3].val;
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : ti->ts[*i - 1].val;
        if ((strcmp(left_symbol,"+") == 0 || strcmp(left_symbol,"-") == 0) &&
            (strcmp(right_symbol,"*") == 0 || strcmp(right_symbol,"/") == 0)) {
                if (out >= verbose) printf("Symbol order of +- _ */\n");
                set_parent(n_tree, right_operator, n_index, state, out);
                set_parent(n_tree, expression, right_operator, state, out);
                set_first(n_tree, right_operator, expression, state, out);
                set_second(n_tree, n_index, right_operator, state, out);
                set_indent(n_tree, right_operator, state, out);
                set_indent(n_tree, expression, state, out);
        }
        else {
            set_parent(n_tree, right_operator, n_tree->nodes[n_index].parent, state, out);
            set_parent(n_tree, n_index, right_operator, state, out);
            set_parent(n_tree, expression, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            set_second(n_tree, n_tree->nodes[right_operator].parent, right_operator, state, out);
            set_second(n_tree, n_index, expression, state, out);
            set_indent(n_tree, expression, state, out);
            set_indent(n_tree, right_operator, state, out);
            set_indent(n_tree, n_index, state, out);
        }
        return state_10(ti, i, n_tree, right_operator, parent_func, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
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
int state_9(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 9;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    enum e_expr specific_type = -1;
    if 		(check_type(ti, t_id, *i, out))         specific_type = e_id;
    else if (check_type(ti, t_num_int, *i, out))    specific_type = e_val;
    else if (check_type(ti, t_num_float, *i, out))  specific_type = e_val;
    else {
        if (out >= standard) printf("Parse error. Expected a right operand at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    *i += 1;

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
        set_parent(n_tree, expression, n_index, state, out);
        set_second(n_tree, n_index, expression, state, out);
        set_indent(n_tree, expression, state, out);
        add_token(n_tree, *i - 2, expression, out);
        return parent_func;
    }
    // Parsing the second binary operator in "x (+) y (+) z".
    // node_index is the first binary operator.
    // node_index.first = x
    // node_index.second = not initialised yet
    // node_index.parent is unknown.
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
        add_token(n_tree, *i - 2, expression, out);
        add_token(n_tree, *i - 1, right_operator, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : ti->ts[*i - 3].val;
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : ti->ts[*i - 1].val;
        if ((strcmp(left_symbol,"+") == 0 || strcmp(left_symbol,"-") == 0) &&
            (strcmp(right_symbol,"*") == 0 || strcmp(right_symbol,"/") == 0)) {
                if (out >= verbose) printf("Symbol order of +- _ */\n");
                set_first(n_tree, right_operator, expression, state, out);
                set_parent(n_tree, expression, right_operator, state, out);
                set_second(n_tree, n_index, right_operator, state, out);
                set_parent(n_tree, right_operator, n_index, state, out);
                set_indent(n_tree, right_operator, state, out);
                set_indent(n_tree, expression, state, out);
                return state_10(ti, i, n_tree, right_operator, n_index, output, out);
        }
        else {
            set_parent(n_tree, right_operator, n_tree->nodes[n_index].parent, state, out);
            set_parent(n_tree, n_index, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            set_second(n_tree, n_tree->nodes[right_operator].parent, right_operator, state, out);
            set_second(n_tree, n_index, expression, state, out);
            set_parent(n_tree, expression, right_operator, state, out);
            set_indent(n_tree, expression, state, out);
            set_indent(n_tree, right_operator, state, out);
            set_indent(n_tree, n_index, state, out);
            return state_10(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
}

// Parsing "y (+) z" in expression "x (+) y (+) z"
// node_index is the binary operator in "x (+) y"
// node_index.first = y
// node_index.parent is the first statement in a function, and is a return statement.
// z is unknown and could be another binop expression.
// z is currently pointed to by *i.
// id/num ; -> s1
// id/num binop -> s8
int state_8(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 8;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    enum e_expr expr_specific_type = -1;
    if 		(check_type(ti, t_id, *i, out)) expr_specific_type = e_id;
    else if (check_type(ti, t_num_int, *i, out)) expr_specific_type = e_val;
    else if (check_type(ti, t_num_float, *i, out)) expr_specific_type = e_val;
    else {
        if (out >= standard) printf("Parse error. Expected a right operand at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    *i += 1;
    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        int expr_node_index = add_node(n_tree, n_expr, expr_specific_type, state, output, out);
        set_parent(n_tree, expr_node_index, n_index, state, out);
        set_second(n_tree, n_index, expr_node_index, state, out);
        set_indent(n_tree, expr_node_index, state, out);
        add_token(n_tree, *i - 2, expr_node_index, out);
        return state_1(ti, i, n_tree, n_index, n_tree->nodes[parent_func].parent, output, out);
    }
    // Create a node for the operator, with the symbol token connected.
    // Create a node for the to operand the left of the operator, with the value symbol connected.
    // The operator's parent is the previously-created node (a binop), and the operator becomes that node's second.
    // The operand's parent is the operator. It is the operator's first.
    // Continue to state 8, with the operator as the node index.
    // This will be evaluated from right to left.
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        int expression = add_node(n_tree, n_expr, expr_specific_type, state, output, out);
        add_token(n_tree, *i - 2, expression, out);
        add_token(n_tree, *i - 1, right_operator, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : ti->ts[*i - 3].val;
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : ti->ts[*i - 1].val;
        if ((strcmp(left_symbol,"+") == 0 || strcmp(left_symbol,"-") == 0) &&
            (strcmp(right_symbol,"*") == 0 || strcmp(right_symbol,"/") == 0)) {
                if (out >= verbose) printf("Symbol order of +- _ */\n");
                set_parent(n_tree, right_operator, n_index, state, out);
                set_parent(n_tree, expression, right_operator, state, out);
                set_first(n_tree, right_operator, expression, state, out);
                set_second(n_tree, n_index, right_operator, state, out);
                set_indent(n_tree, right_operator, state, out);
                set_indent(n_tree, expression, state, out);
        }
        else {
            set_parent(n_tree, right_operator, n_tree->nodes[n_index].parent, state, out);
            set_parent(n_tree, n_index, right_operator, state, out);
            set_parent(n_tree, expression, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            set_second(n_tree, n_tree->nodes[right_operator].parent, right_operator, state, out);
            set_second(n_tree, n_index, expression, state, out);
            set_indent(n_tree, expression, state, out);
            set_indent(n_tree, right_operator, state, out);
            set_indent(n_tree, n_index, state, out);
        }
        return state_8(ti, i, n_tree, right_operator, parent_func, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
}

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
int state_7(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 7;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    enum e_expr expr_specific_type = -1;
    if 		(check_type(ti, t_id, *i, out)) expr_specific_type = e_id;
    else if (check_type(ti, t_num_int, *i, out) || check_type(ti, t_num_float, *i, out)) expr_specific_type = e_val;
    else {
        if (out >= standard) printf("Parse error. Expected a return value or semi-colon to end function body at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }

    *i += 1;
    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        int expr_node_index = add_node(n_tree, n_expr, expr_specific_type, state, output, out);
        set_parent(n_tree, expr_node_index, n_index, state, out);
        set_second(n_tree, n_index, expr_node_index, state, out);
        set_indent(n_tree, expr_node_index, state, out);
        add_token(n_tree, *i - 2, expr_node_index, out);
        return state_1(ti, i, n_tree, expr_node_index, n_tree->nodes[parent_func].parent, output, out);
    }
    // Create a node for the operator, with the symbol token connected.
    // Create a node for the operand to the left of the operator, with the value symbol connected.
    // The operator's parent is the previously-created node (a return), and the operator becomes that node's second.
    // The operand's parent is the operator. It is the operator's first.
    // Continue to state 8, with the operator as the node index.
    // This will be evaluated from right to left.
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        int expression = add_node(n_tree, n_expr, expr_specific_type, state, output, out);
        add_token(n_tree, *i - 2, expression, out);
        add_token(n_tree, *i - 1, operator, out);
        set_parent(n_tree, operator, n_index, state, out);
        set_parent(n_tree, expression, operator, state, out);
        set_first(n_tree, operator, expression, state, out);
        set_second(n_tree, n_index, operator, state, out);
        set_indent(n_tree, operator, state, out);
        set_indent(n_tree, expression, state, out);
        return state_8(ti, i, n_tree, operator, parent_func, output, out);
    }
    else if (check_type(ti, t_neq, *i, out) || check_type(ti, t_double_eq, *i, out)) {
        *i += 1;
        int comparison = add_node(n_tree, n_expr, e_comp, state, output, out);
        int expression = add_node(n_tree, n_expr, expr_specific_type, state, output, out);
        add_token(n_tree, *i - 2, expression, out);
        add_token(n_tree, *i - 1, comparison, out);
        set_parent(n_tree, comparison, n_index, state, out);
        set_parent(n_tree, expression, comparison, state, out);
        set_first(n_tree, comparison, expression, state, out);
        set_second(n_tree, n_index, comparison, state, out);
        set_indent(n_tree, comparison, state, out);
        set_indent(n_tree, expression, state, out);
        return state_11(ti, i, n_tree, comparison, parent_func, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
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
int state_6(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 6;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    if (check_type(ti, t_return, *i, out)) {
        if (out >= verbose) printf("state 6 returning.\n");
        *i += 1;
        int specific_expr_type = -1;
        int return_node_index = add_node(n_tree, n_stat, s_return, state, output, out);
        set_second(n_tree, n_index, return_node_index, state, out);
        set_parent(n_tree, return_node_index, n_index, state, out);
        set_indent(n_tree, return_node_index, state, out);

        if 		(check_type(ti, t_num_int, *i, out)) 	specific_expr_type = e_val;
        else if (check_type(ti, t_num_float, *i, out)) 	specific_expr_type = e_val;
        else if (check_type(ti, t_id, *i, out)) 		specific_expr_type = e_id;
        else {
            if (out >= standard) printf("Parse error. Expected a value or semi-colon to end function body at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, out);
            return -1;
        }
        *i += 1;

        if (check_type(ti, t_semicolon, *i, out)) {
            *i += 1;
            int expr_node_index = add_node(n_tree, n_expr, specific_expr_type, state, output, out);
            set_parent(n_tree, expr_node_index, return_node_index, state, out);
            set_second(n_tree, return_node_index, expr_node_index, state, out);
            set_indent(n_tree, expr_node_index, state, out);
            add_token(n_tree, *i - 2, expr_node_index, out);
            return state_1(ti, i, n_tree, return_node_index, n_tree->nodes[parent_func].parent, output, out);
        }
        else if (check_type(ti, t_binop, *i, out)) {
            *i += 1;
            int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
            int expression = add_node(n_tree, n_expr, specific_expr_type, state, output, out);
            add_token(n_tree, *i - 2, expression, out);
            add_token(n_tree, *i - 1, operator, out);
            set_parent(n_tree, operator, return_node_index, state, out);
            set_parent(n_tree, expression, operator, state, out);
            set_first(n_tree, operator, expression, state, out);
            set_second(n_tree, return_node_index, operator, state, out);
            set_indent(n_tree, operator, state, out);
            set_indent(n_tree, expression, state, out);
            return state_8(ti, i, n_tree, operator, parent_func, output, out);
        }
        else {
            if (out >= standard) printf("Parse error. Expected a semi-colon to end function body at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, out);
            return -1;
        }
    }
    if (check_type(ti, t_type, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    if (check_type(ti, t_id, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a variable name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    if (check_type(ti, t_eq, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    int new_node_index = add_node(n_tree, n_stat, s_var_bind, state, output, out);
    set_parent(n_tree, new_node_index, n_index, state, out);
    set_second(n_tree, n_index, new_node_index, state, out);
    set_indent(n_tree, new_node_index, state, out);
    add_token(n_tree, *i - 3, new_node_index, out);
    add_token(n_tree, *i - 2, new_node_index, out);

    return state_5(ti, i, n_tree, new_node_index, parent_func, output, out);
}

// Binding an expression to a variable.
// Node index is the variable node.
// i is pointing at the first token after the equals sign.
// num ; -> return 6
// num binop -> s9
// id ; -> return 6
// id binop -> s9
int state_5(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 5;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    assert(n_tree->nodes[n_index].nodetype == n_stat
        && n_tree->nodes[n_index].specific_type == s_var_bind);

    int specific_expr_type = -1;
    if 		(check_type(ti, t_num_int,   *i, out)) 	specific_expr_type = e_val;
    else if (check_type(ti, t_num_float, *i, out)) 	specific_expr_type = e_val;
    else if (check_type(ti, t_id,        *i, out)) 	specific_expr_type = e_id;
    else {
        if (out >= standard) printf("Parse error. Expected a value to bind to variable \"%s\"(%lu:%lu) at %lu:%lu.\n",
        ti->ts[*i-2].val, ti->ts[*i-2].line_number, ti->ts[*i-2].char_number, ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    *i += 1;

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        int expression = add_node(n_tree, n_expr, specific_expr_type, state, output, out);
        add_token(n_tree, *i - 2, expression, out);
        set_first(n_tree, n_index, expression, state, out);
        set_parent(n_tree, expression, n_index, state, out);
        set_indent(n_tree, expression, state, out);
        return state_6(ti, i, n_tree, n_index, parent_func, output, out);
    }
    // when binding the results of a binop to a variable.
    // "x (+) ?"
    else if (check_type(ti, t_binop, *i, out)) {
        if (out >= verbose) printf("First BINOP found after variable bind start.\n");
        *i += 1;
        int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, *i - 1, operator, out);
        int left_operand = add_node(n_tree, n_expr, specific_expr_type, state, output, out);
        add_token(n_tree, *i - 2, left_operand, out);
        set_first(n_tree, n_index, operator, state, out);
        set_parent(n_tree, operator, n_index, state, out);
        set_first(n_tree, operator, left_operand, state, out);
        set_parent(n_tree, left_operand, operator, state, out);
        set_indent(n_tree, operator, state, out);
        set_indent(n_tree, left_operand, state, out);

        int result = state_9(ti, i, n_tree, operator, n_index, output, out);
        if (out >= verbose) printf("Come back up, setting variable's fst.\n");
        if (result == -1) return -1;
        /*int idx = n_tree->n - 1;
        while (idx > 0 && idx != n_index) {
            if (n_tree->nodes[idx].parent == n_index && n_tree->nodes[n_index].second != idx && n_tree->nodes[n_index].first != idx) {
                set_first(n_tree, n_index, idx, state, out); //n_tree->nodes[n_index].first = idx;
                break;
            }
            idx--;
        }*/
        if (result != n_index) set_first(n_tree, n_index, result, state, out);
        return state_6(ti, i, n_tree, n_index, parent_func, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator to follow variable value \"%s\"(%lu:%lu) at %lu:%lu.\n",
        ti->ts[*i-2].val, ti->ts[*i-2].line_number, ti->ts[*i-2].char_number, ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
}
// First statement in function body.
// return ; -> s1
// return -> s7
// type id = -> s5 -> ?
// node_index = function binding
// fun_parent = function binding
int state_4(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 4;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    if (check_type(ti, t_return, *i, out)) {
        *i += 1;
        int return_node_index = add_node(n_tree, n_stat, s_return, state, output, out);
        set_second(n_tree, n_index, return_node_index, state, out);
        set_parent(n_tree, return_node_index, n_index, state, out);
        set_indent(n_tree, return_node_index, state, out);

        if (check_type(ti, t_semicolon, *i, out)) {
            if (out == standard) printf("Warning: Empty function body at %lu:%lu.\n", ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, out);
            *i += 1;
            return state_1(ti, i, n_tree, return_node_index, n_tree->nodes[n_index].parent, output, out);
        }
        else {
            return state_7(ti, i, n_tree, return_node_index, n_index, output, out);
        }
    }

    if (check_type(ti, t_type, *i, out)) *i += 1;
    else {
        if (out == standard) printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    if (check_type(ti, t_id, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a variable name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    if (check_type(ti, t_eq, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    int new_node_index = add_node(n_tree, n_stat, s_var_bind, state, output, out);
    set_parent(n_tree, new_node_index, n_index, state, out);
    set_second(n_tree, n_index, new_node_index, state, out);
    set_indent(n_tree, new_node_index, state, out);
    add_token(n_tree, *i - 3, new_node_index, out);
    add_token(n_tree, *i - 2, new_node_index, out);

    return state_5(ti, i, n_tree, new_node_index, n_index, output, out);
}
// Function parameter follow-up (either another parameter or the start of function body)
// , -> s2
// ) = -> s4
// node_index = parameter node.
// fun_parent = function binding.
int state_3(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 3;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    if (check_type(ti, t_comma, *i, out)) {
        *i += 1;
        return state_2(ti, i, n_tree, n_index, parent_func, output, out);
    }
    if (check_type(ti, t_par_end, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected ')' to proceed function parameters at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    if (check_type(ti, t_eq, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected '=' to indicate start of function body at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }

    return state_4(ti, i, n_tree, n_tree->nodes[n_index].parent, parent_func, output, out);
}

// Function parameter(s).
// First check if at the end of the parameter declarations with ") ="
// Otherwise, check for a parameter declaration with "type id".
//    If so, add them as tokens, and go to state
// ) = -> s4
// type id -> s3
// Node index = parameter node
// fun_parent = function binding
int state_2(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 2;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        if (check_type(ti, t_eq, *i, out)) {
            *i += 1;
            return state_4(ti, i, n_tree, n_tree->nodes[n_index].parent, parent_func, output, out);
        }
        else {
            if (out >= standard) printf("Parse error. Expected '=' to precede function body at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, out);
            return -1;
        }
    }
    if (check_type(ti, t_type, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected parameter type at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    if (check_type(ti, t_id, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected parameter name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }

    add_token(n_tree, *i - 2, n_index, out);
    add_token(n_tree, *i - 1, n_index, out);
    return state_3(ti, i, n_tree, n_index, parent_func, output, out);
}
// Expecting a function declaration. State checks type, ID, and beginning parentheses to contain parameters.
// If these are correct, add a program node (n_prog), then a function bind statement node (n_stat + s_fun_bind),
// and finally a parameter expression node. The parameter node will be evaluated first, then the function bind
// node, before returning the result up the chain to evaluate the program node.
// A program contains one or more functions.
// type id ( -> s2
// fun_parent = top node of entire program?
int state_1(struct token_index* ti, int *i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 1;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    if (ti->n <= *i) return END_STATE;
    // Program must consist of only functions at its highest level, and must have at least one function.
    if (!check_type(ti, t_type, *i, out)) {
        if (out >= standard) printf("Parse error. Expected function type at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }

    *i += 1;
    if (!check_type(ti, t_id, *i, out)) {
        if (out >= standard) printf("Parse error. Expected function name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }

    *i += 1;
    if (!check_type(ti, t_par_beg, *i, out)) {
        if (out >= standard) printf("Parse error. Expected '(' to precede function parameters at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, out);
        return -1;
    }
    *i += 1;

    int top_node_index = add_node(n_tree, n_prog, 0, state, output, out);
    set_second(n_tree, parent_func, top_node_index, state, out);
    set_parent(n_tree, top_node_index, parent_func, state, out);
    int fun_declaration = add_node(n_tree, n_stat, s_fun_bind, state, output, out);
    add_token(n_tree, *i - 3, fun_declaration, out);
    add_token(n_tree, *i - 2, fun_declaration, out);
    if (out >= verbose) printf("Added new function: %s %s\n", ti->ts[*i-3].val, ti->ts[*i-2].val);
    set_parent(n_tree, fun_declaration, top_node_index, state, out);
    set_indent(n_tree, fun_declaration, state, out);
    set_first(n_tree, top_node_index, fun_declaration, state, out);
    int new_param_node_index = add_node(n_tree, n_expr, e_param, state, output, out);
    set_parent(n_tree, new_param_node_index, fun_declaration, state, out);
    set_indent(n_tree, new_param_node_index, state, out);
    set_first(n_tree, fun_declaration, new_param_node_index, state, out);
    // TODO: Bind to ftable here, and check for "main" function ID.
    return state_2(ti, i, n_tree, new_param_node_index, fun_declaration, output, out);
}

int state_0(struct token_index* ti, int *i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 0;
    if (out >= verbose) printf("State: %d. Token index: %d\n", state, *i);
    assert(n_tree->n == 0);
    int top_node_index = add_node(n_tree, n_prog, 0, state, output, out);

    return state_1(ti, i, n_tree, top_node_index, parent_func, output, out);
}

struct tree* parse(FILE* f, FILE* output, struct token_index* ti, int out) {
    char c = fgetc(f);
    if (c == EOF) {
        if (out >= standard) printf("Parse error. Empty token file.\n");
        return NULL;
    }
    ungetc(c, f);
    int token_index = 0;
    struct tree *node_tree = malloc(sizeof(*node_tree));
    node_tree->nodes = malloc(sizeof(*node_tree->nodes));
    node_tree->n = 0;
    int final_state = state_0(ti, &token_index, node_tree, 0, 0, output, out);
    if (final_state == END_STATE) return node_tree;
    else return NULL;
}