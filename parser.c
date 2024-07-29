#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

extern int state_1(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_2(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_5(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_6(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_12(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);
extern int state_16(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out);

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

struct tuple {
    int fst;
    int snd;
};

void debug_print(int state, unsigned int t_index, int n_index, int parent, int out) {
    if (out >= verbose)
        printf("Current state: %d. Token index: %u. Node index: %d. Parent_func: %d.\n",
            state, t_index, n_index, parent);
}

enum e_expr check_value_type(struct token_index* ti, int i, int state, int out) {
    switch (ti->ts[i].type) {
        case t_num_int: return e_val;
        case t_num_float: return e_val;
        case t_id:
            if (ti->ts[i + 1].type == t_par_beg) return e_funcall;
            return e_id;
        default:
            if (out >= standard) printf("Parse error. Expected an operand/expression at %lu:%lu.\n",
            ti->ts[i].line_number, ti->ts[i].char_number);
            return -1;
    }
}

void set_indent(struct tree* n_tree, int n_index, int state, int out) {
    if (n_tree->nodes[n_index].parent == -2) {
        if (out >= verbose) printf("Node %d doesn't have a parent but its indent is being set.\n", n_index);
        return;
    }
    n_tree->nodes[n_index].print_indent =
        n_tree->nodes[n_tree->nodes[n_index].parent].print_indent + 0;
    if (out > verbose) {
        printf("Node %d's indentation set to %d while in state %d.\n",
            n_index, n_tree->nodes[n_index].print_indent, state);
    }
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
    set_indent(n_tree, n_index, state, out);
}

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
    if (out >= verbose) printf("  ");
    set_parent(n_tree, value, n_index, state, out);
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
    if (out >= verbose) printf("  ");
    set_parent(n_tree, value, n_index, state, out);
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
    if ((node_tree->n == 0) && (out >= verbose)) printf("\x1b[32mCreating node and returning -1.\x1b[m \n");
    return node_tree->n - 1;
}

void add_token(struct tree* node_tree, int n_index, int t_index, int out) {
    struct node* n = &node_tree->nodes[n_index];
    n->token_count += 1;
    n->token_indices =
        realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
    n->token_indices[n->token_count - 1] = t_index;
    if (out >= verbose) printf("Adding token %d to node %d.\n", t_index, n_index);
}

/// @brief Check whether the token's type matches the one given.
/// @param ti The struct containing the list of tokens.
/// @param expected The type to be checked against.
/// @param i The index of the token to be checked.
/// @param out Output mode.
/// @return Returns 1 if they're identical. Returns 0 otherwise, or if the index
/// of the token to be checked doesn't match a token at all.
int check_type(struct token_index *ti, enum e_token expected, int i, int out) {
    if (ti->n <= i) {
        if (out >= standard) printf("Parse error. Code terminates prematurely at %lu:%lu.\n",
        ti->ts[ti->n - 1].line_number, ti->ts[ti->n - 1].char_number);
        return 0;
    }
    return (ti->ts[i].type == expected);
}

int next_expression(struct token_index* ti, int* i, struct tree* n_tree, int n_index,
        int parent_func, FILE* output, int out, int state) {
    enum e_expr specific_type = check_value_type(ti, *i, state, out);
    *i += 1;

    int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
    set_second(n_tree, n_index, expression, state, out);
    add_token(n_tree, expression, *i - 1, out);

    if (specific_type == e_funcall) {
        *i += 1;
        int result = state_12(ti, i, n_tree, expression, parent_func, output, out);
        if (result == -1) return -1;
    }
    return expression;
}

// Returns a positive number if the left operator should be calculated before
// the right symbol (and therefore left node should be child of right node).
// Returns 0 if the two have the same precedence.
// Returns a negative number otherwise.
int check_precedence(char* left, char* right) {
    int left_prec = 0;
    int right_prec = 0;

    if (strcmp(left,"**") == 0)
        left_prec = 5;
    else if (strcmp(left,"*") == 0 || strcmp(left,"/") == 0)
        left_prec = 4;
    else if (strcmp(left,"+") == 0 || strcmp(left,"-") == 0)
        left_prec = 3;

    if (strcmp(right,"**") == 0)
        right_prec = 5;
    else if (strcmp(right,"*") == 0 || strcmp(right,"/") == 0)
        right_prec = 4;
    else if (strcmp(right,"+") == 0 || strcmp(right,"-") == 0)
        right_prec = 3;

    return left_prec - right_prec;
}

struct tuple binop(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func,
        int state, FILE* output, int out, int expression) {
    int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
    add_token(n_tree, right_operator, *i - 1, out);
    char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
        ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
        : "";
    char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
        ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
        : "";
    if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
    if (check_precedence(left_symbol, right_symbol) < 0) {
        set_first(n_tree, right_operator, expression, state, out);
        set_second(n_tree, n_index, right_operator, state, out);
        struct tuple result;
        result.fst = right_operator;
        result.snd = parent_func;
        return result;
    }
    else {
        set_first(n_tree, n_tree->nodes[n_index].parent, right_operator, state, out);
        set_first(n_tree, right_operator, n_index, state, out);
        struct tuple result;
        result.fst = right_operator;
        result.snd = parent_func;
        return result;
    }
}

// Continued right side of the comparison operator.
int state_26(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
           n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
           n_tree->nodes[parent_func].specific_type == e_binop);

    static const int state = 26;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return parent_func;
    }

    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        char* parent_symbol = ti->ts[n_tree->nodes[parent_func].token_indices[0]].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0 ||
            check_precedence(parent_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_26(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            if (n_tree->nodes[n_tree->nodes[parent_func].parent].specific_type == e_comp)
                set_second(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            else
                set_first(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            set_first(n_tree, right_operator, parent_func, state, out);
            return state_26(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// Second state for parsing right side of the comparison operator.
int state_25(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
           n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
           n_tree->nodes[parent_func].specific_type == e_comp);
    assert(n_tree->nodes[parent_func].second == n_index);

    static const int state = 25;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return parent_func;
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_26(ti, i, n_tree, right_operator, n_index, output, out);
        }
        else {
            set_second(n_tree, parent_func, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            return state_26(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// First state for parsing right side of comparison operator.
// Therefore only ends upon par_end, and cannot accept another comparison operator.
int state_24(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
           n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
           n_tree->nodes[parent_func].specific_type == e_comp);
    assert(n_tree->nodes[n_tree->nodes[parent_func].parent].nodetype == n_stat &&
           n_tree->nodes[n_tree->nodes[parent_func].parent].specific_type == s_return);

    static const int state = 24;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return parent_func;
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        set_first(n_tree, right_operator, expression, state, out);
        set_second(n_tree, n_index, right_operator, state, out);
        return state_25(ti, i, n_tree, right_operator, n_index, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// Continued right side of the comparison operator.
int state_23(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
           n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
           n_tree->nodes[parent_func].specific_type == e_binop);

    static const int state = 23;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }

    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        char* parent_symbol = ti->ts[n_tree->nodes[parent_func].token_indices[0]].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0 ||
            check_precedence(parent_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_23(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            if (n_tree->nodes[n_tree->nodes[parent_func].parent].specific_type == e_comp)
                set_second(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            else
                set_first(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            set_first(n_tree, right_operator, parent_func, state, out);
            return state_23(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// Second state for parsing right side of the comparison operator.
int state_22(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
           n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
           n_tree->nodes[parent_func].specific_type == e_comp);
    assert(n_tree->nodes[parent_func].second == n_index);

    static const int state = 22;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_23(ti, i, n_tree, right_operator, n_index, output, out);
        }
        else {
            set_second(n_tree, parent_func, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            return state_23(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// First state for parsing right side of comparison operator.
// Therefore only ends upon par_end, and cannot accept another comparison operator.
int state_21(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
           n_tree->nodes[n_index].specific_type == e_comp);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
           n_tree->nodes[parent_func].specific_type == e_comp);
    assert(n_tree->nodes[n_tree->nodes[n_index].parent].nodetype == n_expr &&
           n_tree->nodes[n_tree->nodes[n_index].parent].specific_type == e_condition);
    assert(parent_func == n_index);

    static const int state = 21;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        set_first(n_tree, right_operator, expression, state, out);
        set_second(n_tree, n_index, right_operator, state, out);
        return state_22(ti, i, n_tree, right_operator, n_index, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}
// else
// can be followed by another if statement, or a continue (into state 6), or anything else.
// when inside the else branch, it doesn't come back up and continue to state 6 until it hits
// a continue! Could make do with only an if and an anything else branch from this state?
int state_20(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 20;
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_else, *i, out)) {
        *i += 1;
        if (out >= verbose)
            printf("Found an else-token!");
    }
    if (check_type(ti, t_if, *i, out)) {
        *i += 1;
        int if_node = add_node(n_tree, n_stat, s_if, state, output, out);
        add_token(n_tree, if_node, *i - 1, out);
        set_second(n_tree, n_index, if_node, state, out);

        return state_16(ti, i, n_tree, if_node, parent_func, output, out);
    }
    if (check_type(ti, t_continue, *i, out)) {
        *i += 1;
        return state_6(ti, i, n_tree, n_index, parent_func, output, out);
    }
    return state_6(ti, i, n_tree, n_index, parent_func, output, out);
    if (out >= standard) {
        printf("Parse error. Expected a continuation of an if-statement at %lu:%lu.\n",
            ti->ts[n_tree->nodes[n_index].token_indices[0]].line_number,
            ti->ts[n_tree->nodes[n_index].token_indices[0]].char_number);
    }
    return -1;
}

// Still left side of comparison operator.
// ONLY difference between state 18 and 19 is whether to also check binop precedence compared to the "parent" node.
// ONLY difference between 18/19 and 9/10 is whether to stop on semicolon or par_end.
int state_19(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
        n_tree->nodes[parent_func].specific_type == e_binop);
    assert(n_tree->nodes[n_tree->nodes[parent_func].parent].nodetype == n_expr &&
        n_tree->nodes[n_tree->nodes[parent_func].parent].specific_type == e_condition);

    static const int state = 19;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }

    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        char* parent_symbol = ti->ts[n_tree->nodes[parent_func].token_indices[0]].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0 ||
            check_precedence(parent_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_19(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            set_first(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            set_first(n_tree, right_operator, parent_func, state, out);
            return state_19(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else if (check_type(ti, t_comp, *i, out)) {
        *i += 1;
        int comp_operator = add_node(n_tree, n_expr, e_comp, state, output, out);
        add_token(n_tree, comp_operator, *i - 1, out);
        int condition_index = n_tree->nodes[parent_func].parent;
        set_first(n_tree, comp_operator, parent_func, state, out);
        set_first(n_tree, condition_index, comp_operator, state, out);
        int result = state_21(ti, i, n_tree, comp_operator, comp_operator, output, out);
        if (result == -1)
            return -1;
        return parent_func;
    }
    else {
        if (out >= standard) printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// Left side of a comparison operator.
int state_18(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
        n_tree->nodes[parent_func].specific_type == e_condition);
    assert(n_tree->nodes[parent_func].first == n_index);

    static const int state = 18;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_19(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            set_first(n_tree, n_tree->nodes[n_index].parent, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            return state_19(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else if (check_type(ti, t_comp, *i, out)) {
        *i += 1;
        int comp_operator = add_node(n_tree, n_expr, e_comp, state, output, out);
        add_token(n_tree, comp_operator, *i - 1, out);
        set_first(n_tree, comp_operator, n_index, state, out);
        set_first(n_tree, parent_func, comp_operator, state, out);
        return state_21(ti, i, n_tree, comp_operator, comp_operator, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected an end-parenthesis, comparison, or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// First state to parse left side of comparison operator in if-statement condition.
int state_17(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
    n_tree->nodes[n_index].specific_type == e_condition);

    static const int state = 17;
    debug_print(state, *i, n_index, parent_func, out);

    enum e_expr specific_type = check_value_type(ti, *i, state, out);
    if (specific_type == -1)
        return -1;
    *i += 1;

    int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
    set_first(n_tree, n_index, expression, state, out); // first differs from next_expression()
    add_token(n_tree, expression, *i - 1, out);

    if (specific_type == e_funcall) {
        *i += 1;
        int result = state_12(ti, i, n_tree, expression, parent_func, output, out);
        if (result == -1) return -1;
    }

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }

    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, operator, *i - 1, out);
        set_first(n_tree, n_index, operator, state, out);
        set_first(n_tree, operator, expression, state, out);

        int result = state_18(ti, i, n_tree, operator, n_index, output, out);
        if (out >= verbose)
            printf("Came back up from if-statement conditional tangent. n_index = %d. result = %d.\n",
                n_index, result);
        if (result == -1)
            return -1;
        return parent_func;

    }
    else if (check_type(ti, t_comp, *i, out)) {
        *i += 1;
        int operator = add_node(n_tree, n_expr, e_comp, state, output, out);
        add_token(n_tree, operator, *i - 1, out);
        set_first(n_tree, n_index, operator, state, out);
        set_first(n_tree, operator, expression, state, out);

        int result = state_21(ti, i, n_tree, operator, operator, output, out);
        if (out >= verbose)
            printf("Came back up from if-statement conditional tangent. n_index = %d. result = %d.\n",
                n_index, result);
        if (result == -1)
            return -1;
        return parent_func;
    }

    else {
        if (out >= standard)
            printf("Parse error. Expected an end-parenthesis, comparison, or binary operator at %lu:%lu.\n",
                ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

int state_16(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_stat &&
    n_tree->nodes[n_index].specific_type == s_if);

    static const int state = 16;
    debug_print(state, *i, n_index, parent_func, out);

    if (!check_type(ti, t_par_beg, *i, out)) {
        if (out >= standard) {
            printf("Parse error. Expected opening parentheses to precede conditional expression\n ");
            printf("for the if-statement at %lu:%lu.\n",
                ti->ts[n_tree->nodes[n_index].token_indices[0]].line_number,
                ti->ts[n_tree->nodes[n_index].token_indices[0]].char_number);
        }
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    *i += 1;

    int condition = add_node(n_tree, n_expr, e_condition, state, output, out);
    set_first(n_tree, n_index, condition, state, out);
    add_token(n_tree, condition, *i - 1, out);

    // Parse full conditional expression, returning upon seeing t_par_end.
    int result = state_17(ti, i, n_tree, condition, parent_func, output, out);
    if (result == -1)
        return -1;


    result = state_6(ti, i, n_tree, condition, n_index, output, out);
    if (out >= verbose)
        printf("Came back up out of if-statement's body tangent.\n");
    if (result == -1)
        return -1;
    // Parse (presumed) else statement.
    // This will become (this) n_index's second.
    return state_20(ti, i, n_tree, n_index, n_index, output, out);
}

// Continuous tree construction of chained binary operators.
// Inspired by state_10.
int state_15(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
    n_tree->nodes[n_index].specific_type == e_binop);

    static const int state = 15;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_comma, *i, out) || check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }

    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        char* parent_symbol = ti->ts[n_tree->nodes[parent_func].token_indices[0]].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0 ||
            check_precedence(parent_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_15(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            set_first(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            set_first(n_tree, right_operator, parent_func, state, out);
            return state_15(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }

    else {
        if (out >= standard) printf("Parse error. Expected a comma, end-parenthesis or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// The "tangent" for building the nodes that allow evaluation of a function call argument.
// Equivalent to the process for binding a value to a variable. Inspired by state_9.
int state_14(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
    n_tree->nodes[n_index].specific_type == e_binop);

    static const int state = 14;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_comma, *i, out) || check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }

    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        struct tuple result = binop(ti, i, n_tree, n_index, parent_func, state, output, out, expression);
        return state_15(ti, i, n_tree, result.fst, result.snd, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a comma, end-parenthesis, or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

// Follow-up to a complete argument, either look for next argument or continue.
int state_13(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
    n_tree->nodes[n_index].specific_type == e_funcall);

    static const int state = 13;
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        if (check_type(ti, t_semicolon, *i, out)) *i += 1;
        return parent_func;
    }
    if (check_type(ti, t_comma, *i, out)) *i += 1;
    else {
        if (out >= standard) {
            printf("Parse error. Expected comma or closing parentheses to follow the argument\n");
            printf("during function call at %lu:%lu.\n", ti->ts[*i].line_number, ti->ts[*i].char_number);
        }
    }
    return state_12(ti, i, n_tree, n_index, parent_func, output, out);
}

// Begin parsing arguments for function call. Inspired by state_5.
int state_12(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_funcall);

    static const int state = 12;
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        return parent_func;
    }
    enum e_expr specific_type = check_value_type(ti, *i, state, out);
    *i += 1;

    int argument = add_node(n_tree, n_expr, e_argument, state, output, out);
    int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
    set_first(n_tree, n_index, argument, state, out);
    set_first(n_tree, argument, expression, state, out);
    add_token(n_tree, expression, *i - 1, out);

    if (specific_type == e_funcall) {
        *i += 1;
        int result = state_12(ti, i, n_tree, expression, parent_func, output, out);
        if (result == -1) return -1;
    }

    if (check_type(ti, t_binop, *i, out)) {
        *i += 1;

        int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, operator, *i - 1, out);
        set_first(n_tree, argument, operator, state, out);
        set_first(n_tree, operator, expression, state, out);

        int result = state_14(ti, i, n_tree, operator, operator, output, out);
        if (out >= verbose) printf("Came back up from argument tangent. n_index = %d. result = %d.\n", n_index, result);
        if (result == -1) return -1;
    }
    return state_13(ti, i, n_tree, n_index, parent_func, output, out);
}

// Right side of comparison operator, but only from state 7...
int state_11(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_comp);

    static const int state = 11;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);
    expression += 0;
    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return parent_func; //state_1(ti, i, n_tree, n_index, parent_func, output, out);
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        set_first(n_tree, right_operator, expression, state, out);
        set_second(n_tree, n_index, right_operator, state, out);
        int result = state_24(ti, i, n_tree, right_operator, n_index, output, out);
        if (result == -1)
            return -1;
        return n_index;
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semicolon to finish comparison at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
// ONLY difference between state 9 and 10 is whether to also check precedence compared to the "parent" node.
int state_10(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_binop);

    static const int state = 10;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
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
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        char* parent_symbol = ti->ts[n_tree->nodes[parent_func].token_indices[0]].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);

        if (check_precedence(left_symbol, right_symbol) < 0 ||
            check_precedence(parent_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_10(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            set_first(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            set_first(n_tree, right_operator, parent_func, state, out);
            return state_10(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon, function call, or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_binop);

    static const int state = 9;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
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
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : "";
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : "";
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_10(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            set_first(n_tree, n_tree->nodes[n_index].parent, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            return state_10(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
}

int state_27(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_expr &&
        n_tree->nodes[parent_func].specific_type == e_binop);

    static const int state = 27;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return n_tree->nodes[parent_func].parent; //state_1(ti, i, n_tree, n_index, n_tree->nodes[parent_func].parent, output, out);
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : ti->ts[*i - 3].val;
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : ti->ts[*i - 1].val;
         char* parent_symbol = ti->ts[n_tree->nodes[parent_func].token_indices[0]].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);

        if (check_precedence(left_symbol, right_symbol) < 0 ||
            check_precedence(parent_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_27(ti, i, n_tree, right_operator, parent_func, output, out);
        }
        else {
            set_second(n_tree, n_tree->nodes[parent_func].parent, right_operator, state, out);
            set_first(n_tree, right_operator, parent_func, state, out);
            return state_27(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else if (check_type(ti, t_comp, *i, out)) {
        *i += 1;
        int comp_operator = add_node(n_tree, n_expr, e_comp, state, output, out);
        add_token(n_tree, comp_operator, *i - 1, out);
        set_first(n_tree, comp_operator, n_index, state, out);
        set_first(n_tree, parent_func, comp_operator, state, out);
        return state_11(ti, i, n_tree, comp_operator, comp_operator, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
    assert(n_tree->nodes[n_index].nodetype == n_expr &&
        n_tree->nodes[n_index].specific_type == e_binop);
    assert(n_tree->nodes[parent_func].nodetype == n_stat &&
        n_tree->nodes[parent_func].specific_type == s_return);
    assert(n_tree->nodes[parent_func].second == n_index);

    static const int state = 8;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return n_tree->nodes[parent_func].parent; //state_1(ti, i, n_tree, n_index, n_tree->nodes[parent_func].parent, output, out);
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int right_operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, right_operator, *i - 1, out);
        char* left_symbol = (n_tree->nodes[n_index].token_count > 0)
            ? ti->ts[n_tree->nodes[n_index].token_indices[0]].val
            : ti->ts[*i - 3].val;
        char* right_symbol = (n_tree->nodes[right_operator].token_count > 0)
            ? ti->ts[n_tree->nodes[right_operator].token_indices[0]].val
            : ti->ts[*i - 1].val;
        if (out >= verbose) printf("Symbol order of '%s'  '%s'.\n", left_symbol, right_symbol);
        if (check_precedence(left_symbol, right_symbol) < 0) {
            set_first(n_tree, right_operator, expression, state, out);
            set_second(n_tree, n_index, right_operator, state, out);
            return state_27(ti, i, n_tree, right_operator, right_operator, output, out);
        }
        else {
            set_second(n_tree, parent_func, right_operator, state, out);
            set_first(n_tree, right_operator, n_index, state, out);
            return state_27(ti, i, n_tree, right_operator, right_operator, output, out);
        }
    }
    else if (check_type(ti, t_comp, *i, out)) {
        *i += 1;
        int comp_operator = add_node(n_tree, n_expr, e_comp, state, output, out);
        add_token(n_tree, comp_operator, *i - 1, out);
        set_first(n_tree, comp_operator, n_index, state, out);
        set_second(n_tree, parent_func, comp_operator, state, out);
        return state_11(ti, i, n_tree, comp_operator, comp_operator, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
    assert(n_tree->nodes[n_index].nodetype == n_stat &&
        n_tree->nodes[n_index].specific_type == s_return);
    assert(n_tree->nodes[parent_func].nodetype == n_stat &&
        n_tree->nodes[parent_func].specific_type == s_fun_bind);

    static const int state = 7;
    debug_print(state, *i, n_index, parent_func, out);

    int expression = next_expression(ti, i, n_tree, n_index, parent_func, output, out, state);

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return n_tree->nodes[parent_func].parent; //state_1(ti, i, n_tree, expression, n_tree->nodes[parent_func].parent, output, out);
    }
    else if (check_type(ti, t_binop, *i, out)) {
        *i += 1;
        int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        set_first(n_tree, operator, expression, state, out);
        set_second(n_tree, n_index, operator, state, out);
        add_token(n_tree, operator, *i - 1, out);
        return state_8(ti, i, n_tree, operator, n_index, output, out);
    }
    else if (check_type(ti, t_comp, *i, out)) {
        *i += 1;
        int comparison = add_node(n_tree, n_expr, e_comp, state, output, out);
        add_token(n_tree, comparison, *i - 1, out);
        set_first(n_tree, comparison, expression, state, out);
        set_second(n_tree, n_index, comparison, state, out);
        return state_11(ti, i, n_tree, comparison, n_index, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_return, *i, out)) {
        if (out >= verbose) printf("state 6 returning.\n");
        *i += 1;
        int specific_type = check_value_type(ti, *i, state, out);
        *i += 1;

        int return_node_index = add_node(n_tree, n_stat, s_return, state, output, out);
        set_second(n_tree, n_index, return_node_index, state, out);

        int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
        set_second(n_tree, return_node_index, expression, state, out);
        add_token(n_tree, expression, *i - 1, out);

        if (specific_type == e_funcall) {
            *i += 1;
            int result = state_12(ti, i, n_tree, expression, parent_func, output, out);
            if (result == -1) return -1;
        }

        if (check_type(ti, t_semicolon, *i, out)) {
            *i += 1;
            if (ti->n > *i && check_type(ti, t_else, *i, out)) {
                //*i += 1;
                return parent_func;
            }
            return n_tree->nodes[parent_func].parent; //state_1(ti, i, n_tree, return_node_index, n_tree->nodes[parent_func].parent, output, out);
        }
        else if (check_type(ti, t_binop, *i, out)) {
            *i += 1;
            int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
            add_token(n_tree, operator, *i - 1, out);
            set_first(n_tree, operator, expression, state, out);
            set_second(n_tree, return_node_index, operator, state, out);
            int result = state_8(ti, i, n_tree, operator, return_node_index, output, out);
            if (result == -1)
                return -1;
            return parent_func;
        }
        else {
            if (out >= standard) printf("Parse error. Expected a semi-colon to end function body at %lu:%lu.\n",
            ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, n_index, parent_func, out);
            return -1;
        }
    }
    if (check_type(ti, t_if, *i, out)) {
        *i += 1;
        int if_node = add_node(n_tree, n_stat, s_if, state, output, out);
        set_second(n_tree, n_index, if_node, state, out);
        add_token(n_tree, if_node, *i - 1, out);
        return state_16(ti, i, n_tree, if_node, n_index, output, out);
    }
    if (check_type(ti, t_type, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a variable type at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    if (check_type(ti, t_id, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a variable name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    if (check_type(ti, t_eq, *i, out)) {
        *i += 1;
        int new_node_index = add_node(n_tree, n_stat, s_var_bind, state, output, out);
        set_second(n_tree, n_index, new_node_index, state, out);
        add_token(n_tree, new_node_index, *i - 3, out);
        add_token(n_tree, new_node_index, *i - 2, out);

        return state_5(ti, i, n_tree, new_node_index, parent_func, output, out);
    }
    else if (check_type(ti, t_par_beg, *i, out)) {
        *i -= 1;
        int funcall = add_node(n_tree, n_stat, s_function_call, state, output, out);
        set_second(n_tree, n_index, funcall, state, out);
        add_token(n_tree, funcall, *i - 3, out);
        add_token(n_tree, funcall, *i - 2, out);
        int result = state_12(ti, i, n_tree, funcall, parent_func, output, out);
        if (result == -1)
            return -1;
        return state_6(ti, i, n_tree, funcall, parent_func, output, out);
    }
    return -1;
}

// Binding an expression to a variable.
// Node index is the variable node.
// i is pointing at the first token after the equals sign.
// num ; -> return 6
// num binop -> s9
// id ; -> return 6
// id binop -> s9
int state_5(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    assert(n_tree->nodes[n_index].nodetype == n_stat
        && n_tree->nodes[n_index].specific_type == s_var_bind);

    static const int state = 5;
    debug_print(state, *i, n_index, parent_func, out);

    int specific_type = check_value_type(ti, *i, state, out);
    *i += 1;

    int expression = add_node(n_tree, n_expr, specific_type, state, output, out);
    set_first(n_tree, n_index, expression, state, out);
    add_token(n_tree, expression, *i - 1, out);

    if (specific_type == e_funcall) {
        *i += 1;
        int result = state_12(ti, i, n_tree, expression, parent_func, output, out);
        if (result == -1) return -1;
    }

    if (check_type(ti, t_semicolon, *i, out)) {
        *i += 1;
        return state_6(ti, i, n_tree, n_index, parent_func, output, out);
    }
    // when binding the results of a binop to a variable.
    // "x (+) ?"
    else if (check_type(ti, t_binop, *i, out)) {
        if (out >= verbose) printf("First BINOP found after variable bind start.\n");
        *i += 1;
        int operator = add_node(n_tree, n_expr, e_binop, state, output, out);
        add_token(n_tree, operator, *i - 1, out);
        set_first(n_tree, n_index, operator, state, out);
        set_first(n_tree, operator, expression, state, out);

        int result = state_9(ti, i, n_tree, operator, operator, output, out);
        if (out >= verbose) printf("Came back up from variable tangent. n_index = %d. result = %d.\n",
            n_index, result);
        if (result == -1) return -1;
        return state_6(ti, i, n_tree, n_index, parent_func, output, out);
    }
    else {
        if (out >= standard) printf("Parse error. Expected a semi-colon or binary operator to follow variable value \"%s\"(%lu:%lu) at %lu:%lu.\n",
        ti->ts[*i-2].val, ti->ts[*i-2].line_number, ti->ts[*i-2].char_number, ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
    assert(n_tree->nodes[n_index].nodetype == n_stat
        && n_tree->nodes[n_index].specific_type == s_fun_bind);
    assert(n_index == parent_func);

    static const int state = 4;
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_return, *i, out)) {
        *i += 1;
        int return_node_index = add_node(n_tree, n_stat, s_return, state, output, out);
        set_second(n_tree, n_index, return_node_index, state, out);

        if (check_type(ti, t_semicolon, *i, out)) {
            if (out >= standard) printf("Warning: Empty function body at %lu:%lu.\n",
                ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, n_index, parent_func, out);
            *i += 1;
            return n_tree->nodes[n_index].parent;
            //return state_1(ti, i, n_tree, return_node_index, n_tree->nodes[n_index].parent, output, out);
        }
        else {
            return state_7(ti, i, n_tree, return_node_index, n_index, output, out);
        }
    }
    if (check_type(ti, t_if, *i, out)) {
        *i += 1;
        int if_node = add_node(n_tree, n_stat, s_if, state, output, out);
        set_second(n_tree, n_index, if_node, state, out);
        add_token(n_tree, if_node, *i - 1, out);
        return state_16(ti, i, n_tree, if_node, n_index, output, out);
    }
    if (check_type(ti, t_type, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    if (check_type(ti, t_id, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected a variable name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    if (check_type(ti, t_eq, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    int new_node_index = add_node(n_tree, n_stat, s_var_bind, state, output, out);
    set_second(n_tree, n_index, new_node_index, state, out);
    add_token(n_tree, new_node_index, *i - 3, out);
    add_token(n_tree, new_node_index, *i - 2, out);

    return state_5(ti, i, n_tree, new_node_index, n_index, output, out);
}

// Function parameter follow-up (either another parameter or the start of function body)
// , -> s2
// ) = -> s4
// node_index = parameter node.
// fun_parent = function binding.
int state_3(struct token_index* ti, int* i, struct tree* n_tree, int n_index, int parent_func, FILE* output, int out) {
    static const int state = 3;
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_comma, *i, out)) {
        *i += 1;
        return state_2(ti, i, n_tree, n_index, parent_func, output, out);
    }
    if (check_type(ti, t_par_end, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected ')' to proceed function parameters at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    if (check_type(ti, t_eq, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected '=' to indicate start of function body at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
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
    debug_print(state, *i, n_index, parent_func, out);

    if (check_type(ti, t_par_end, *i, out)) {
        *i += 1;
        if (check_type(ti, t_eq, *i, out)) {
            *i += 1;
            return state_4(ti, i, n_tree, n_tree->nodes[n_index].parent, parent_func, output, out);
        }
        else {
            if (out >= standard) printf("Parse error. Expected '=' to precede function body at %lu:%lu.\n",
                ti->ts[*i].line_number, ti->ts[*i].char_number);
                debug_print(state, *i, n_index, parent_func, out);
                return -1;
        }
    }
    if (check_type(ti, t_type, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected parameter type at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }
    if (check_type(ti, t_id, *i, out)) *i += 1;
    else {
        if (out >= standard) printf("Parse error. Expected parameter name at %lu:%lu.\n",
        ti->ts[*i].line_number, ti->ts[*i].char_number);
        debug_print(state, *i, n_index, parent_func, out);
        return -1;
    }

    add_token(n_tree, n_index, *i - 2, out);
    add_token(n_tree, n_index, *i - 1, out);
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
    assert(n_tree->nodes[n_index].nodetype == n_prog &&
        n_tree->nodes[parent_func].nodetype == n_prog);

    static const int state = 1;
    debug_print(state, *i, n_index, parent_func, out);

    int new_parent = parent_func;

    while (ti->n > *i) {
        // Program must consist of only functions at its highest level, and must have at least one function.
        if (!check_type(ti, t_type, *i, out)) {
            if (out >= standard)
                printf("Parse error. Expected function type at %lu:%lu.\n",
                    ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, n_index, parent_func, out);
            return -1;
        }
        *i += 1;

        if (!check_type(ti, t_id, *i, out)) {
            if (out >= standard)
                printf("Parse error. Expected function name at %lu:%lu.\n",
                    ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, n_index, parent_func, out);
            return -1;
        }
        *i += 1;

        if (!check_type(ti, t_par_beg, *i, out)) {
            if (out >= standard)
                printf("Parse error. Expected '(' to precede function parameters at %lu:%lu.\n",
                    ti->ts[*i].line_number, ti->ts[*i].char_number);
            debug_print(state, *i, n_index, parent_func, out);
            return -1;
        }
        *i += 1;

        int top_node_index = add_node(n_tree, n_prog, 0, state, output, out);
        set_second(n_tree, new_parent, top_node_index, state, out);

        int fun_declaration = add_node(n_tree, n_stat, s_fun_bind, state, output, out);
        add_token(n_tree, fun_declaration, *i - 3, out);
        add_token(n_tree, fun_declaration, *i - 2, out);
        set_first(n_tree, top_node_index, fun_declaration, state, out);

        int new_param_node_index = add_node(n_tree, n_expr, e_param, state, output, out);
        set_first(n_tree, fun_declaration, new_param_node_index, state, out);

        if (out >= verbose)
            printf("Added new function: %s %s\n", ti->ts[*i-3].val, ti->ts[*i-2].val);
        // TODO: Bind to ftable here, and check for "main" function ID.
        state_2(ti, i, n_tree, new_param_node_index, fun_declaration, output, out);
        new_parent = top_node_index;
    }
    return END_STATE;
}

/// @brief Initial state from which the tree structure is created.
/// @param ti
/// @param i
/// @param n_tree
/// @param n_index
/// @param parent_func
/// @param output
/// @param out
/// @return END_STATE on success, -1 otherwise.
int state_0(struct token_index* ti, int *i, struct tree* n_tree, int n_index,
        int parent_func, FILE* output, int out) {
    assert(n_index == 0);
    assert(parent_func == 0);

    static const int state = 0;
    debug_print(state, *i, n_index, parent_func, out);

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