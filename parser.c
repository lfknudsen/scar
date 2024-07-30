#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

extern int state_1(int n_index, int parent_func, struct state* state);
extern int state_2(int n_index, int parent_func, struct state* state);
extern int state_5(int n_index, int parent_func, struct state* state);
extern int state_6(int n_index, int parent_func, struct state *state);
extern int state_12(int n_index, int parent_func, struct state* state);
extern int state_16(int n_index, int parent_func, struct state* state);

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

void debug_print(int n_index, int parent, struct state* state) {
    if (state->out >= verbose)
        printf("Current state: %d. Token index: %u. Node index: %d. Parent_func: %d.\n",
            state->state, state->i, n_index, parent);
}

enum e_expr check_value_type(struct state* state) {
    switch (state->ti->ts[state->i].type) {
        case t_num_int: return e_val;
        case t_num_float: return e_val;
        case t_id:
            if (state->ti->ts[state->i + 1].type == t_par_beg) return e_funcall;
            return e_id;
        default:
            if (state->out >= standard)
                printf("Parse error. Expected an operand/expression at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
            return -1;
    }
}

void set_indent(int n_index, struct state* state) {
    if (state->tree->ns[n_index].parent == -2) {
        if (state->out >= verbose)
            printf("Node %d doesn't have a parent but its indent is being set.\n", n_index);
        return;
    }
    state->tree->ns[n_index].print_indent =
        state->tree->ns[state->tree->ns[n_index].parent].print_indent + 0;
    if (state->out > verbose) {
        printf("Node %d's indentation set to %d while in state %d.\n",
            n_index, state->tree->ns[n_index].print_indent, state->state);
    }
}

void set_parent(int n_index, int value, struct state* state) {
    if (state->out >= verbose) {
        if (state->tree->ns[n_index].parent != -1) {
            printf("Node %d's parent set to %d while in state %d (OVERRIDE).\n", n_index, value, state->state);
        }
        else {
            printf("Node %d's parent set to %d while in state %d.\n", n_index, value, state->state);
        }
    }
    state->tree->ns[n_index].parent = value;
    set_indent(n_index, state);
}

void set_first(int n_index, int value, struct state* state) {
    if (state->out >= verbose) {
        if (state->tree->ns[n_index].first != -1) {
            printf("Node %d's first set to %d while in state %d (OVERRIDE).\n", n_index, value, state->state);
        }
        else {
            printf("Node %d's first set to %d while in state %d.\n", n_index, value, state->state);
        }
    }
    state->tree->ns[n_index].first = value;
    if (state->out >= verbose)
        printf("  ");
    set_parent(value, n_index, state);
}

void set_second(int n_index, int value, struct state* state) {
    if (state->out >= verbose) {
        if (state->tree->ns[n_index].second != -1) {
            printf("Node %d's second set to %d while in state %d (OVERRIDE).\n", n_index, value, state->state);
        }
        else {
            printf("Node %d's second set to %d while in state %d.\n", n_index, value, state->state);
        }
    }
    state->tree->ns[n_index].second = value;
    if (state->out >= verbose)
        printf("  ");
    set_parent(value, n_index, state);
}

int* parent(int n_index, struct state* state) {
    return &state->tree->ns[n_index].parent;
}

int* first(int n_index, struct state* state) {
    return &state->tree->ns[n_index].first;
}

int* second(int n_index, struct state* state) {
    return &state->tree->ns[n_index].second;
}

enum e_nodetype* ntype(int n_index, struct state* state) {
    return &state->tree->ns[n_index].node_type;
}

int* stype(int n_index, struct state* state) {
    return &state->tree->ns[n_index].specific_type;
}

// Initialise and add a new node to the tree.
// Return: number of nodes - 1 (state->i.e. the index of the new node)
// See eval.c for evaluation details, as these differ based on the node's type(s).
int add_node(int node_type, int specific_type, struct state* state) {
    if (state->output == NULL) state->output = stdout;
    state->tree->n += 1;
    state->tree->ns = realloc(state->tree->ns,
        sizeof(*state->tree->ns) * state->tree->n);
    state->tree->ns[state->tree->n - 1].node_type = node_type;
    state->tree->ns[state->tree->n - 1].specific_type = specific_type;
    state->tree->ns[state->tree->n - 1].token_count = 0;
    state->tree->ns[state->tree->n - 1].first = -1;
    state->tree->ns[state->tree->n - 1].second = -1;
    state->tree->ns[state->tree->n - 1].parent = -1;
    state->tree->ns[state->tree->n - 1].token_indices = malloc(sizeof(state->tree->ns[state->tree->n - 1].token_indices));
    state->tree->ns[state->tree->n - 1].print_indent = 0;
    if (state->out >= verbose) {
        printf("Adding new node %d: ", state->tree->n - 1);
        print_node(state->tree->n - 1, state);
        printf("\n");

    }
    fprintf(state->output, "Created new node in state %d:\n", state->state);
    fprint_node(state->tree->n - 1, state);
    fprintf(state->output, "\n");
    if ((state->tree->n == 0) && (state->out >= verbose))
        printf("\x1b[32mCreating node and returning -1.\x1b[m \n");
    return state->tree->n - 1;
}

void add_token(int n_index, int t_index, struct state* state) {
    struct node* n = &state->tree->ns[n_index];
    n->token_count += 1;
    n->token_indices =
        realloc(n->token_indices, sizeof(*n->token_indices) * n->token_count);
    n->token_indices[n->token_count - 1] = t_index;
    if (state->out >= verbose)
        printf("Adding token %d to node %d.\n", t_index, n_index);
}

int new_operator(enum e_expr e_op, int n_index, int parent_func, struct state* state) {
    int operator = add_node(n_expr, e_op, state);
    add_token(operator, state->i - 1, state);
    set_first(operator, n_index, state);
    set_first(parent_func, operator, state);
    return operator;
}

char check_node(int idx, int ntype, int stype, struct state* state) {
    return (state->tree->ns[idx].node_type == ntype &&
            state->tree->ns[idx].specific_type == stype);
}

/// @brief Check whether the token's type matches the one given.
/// @param state->ti The struct containing the list of tokens.
/// @param expected The type to be checked against.
/// @param state->i The index of the token to be checked.
/// @param state->out Output mode.
/// @return Returns 1 if they're identical. Returns 0 otherwise, or if the index
/// of the token to be checked doesn't match a token at all.
int check_type(enum e_token expected, struct state* state) {
    if (state->ti->n <= state->i) {
        if (out >= standard) printf("Parse error. Code terminates prematurely at %lu:%lu.\n",
        state->ti->ts[state->ti->n - 1].line_number, state->ti->ts[state->ti->n - 1].char_number);
        return 0;
    }
    return (state->ti->ts[state->i].type == expected);
}

int next_expression(int n_index, int parent_func, struct state* state) {
    enum e_expr specific_type = check_value_type(state);
    state->i ++;

    int expression = add_node(n_expr, specific_type, state);
    set_second(n_index, expression, state);
    add_token(expression, state->i - 1, state);

    if (specific_type == e_funcall) {
        state->i += 1;
        int result = state_12(expression, parent_func, state);
        if (result == -1) return -1;
    }
    return expression;
}

// Returns a positive number if the left operator should be calculated before
// the right symbol (and therefore left node should be child of right node).
// Returns 0 if the two have the same precedence.
// Returns a negative number otherwise.
char check_precedence(char* left, char* right) {
    char left_prec = 0;
    char right_prec = 0;

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

char op_precedence(int left, int right, int parent, struct state* state) {
    char* left_symbol = get_val(left, state);
    char* right_symbol = get_val(right, state);

    if (state->out >= verbose)
        printf("Symbol order of '%s'  '%s'", left_symbol, right_symbol);
    int result1 = (check_precedence(left_symbol, right_symbol) < 0);
    if (parent >= 0) {
        char* parent_symbol = get_val(parent, state);
        if (state->out >= verbose)
            printf(" and '%s'  '%s'.\n", parent_symbol, right_symbol);
        return result1 || (check_precedence(parent_symbol, right_symbol) < 0);
    }
    if (state->out >= verbose)
        printf(".\n");
    return result1;
}


struct tuple binop(int n_index, int parent_func, int expression, struct state* state) {
    int operator = add_node(n_expr, e_binop, state);
    add_token(operator, state->i - 1, state);

    if (op_precedence(n_index, operator, -1, state)) {
        set_first(operator, expression, state);
        set_second(n_index, operator, state);
        struct tuple result;
        result.fst = operator;
        result.snd = parent_func;
        return result;
    }
    else {
        set_first(state->tree->ns[n_index].parent, operator, state);
        set_first(operator, n_index, state);
        struct tuple result;
        result.fst = operator;
        result.snd = parent_func;
        return result;
    }
}

// Continued right side of the comparison operator.
int state_26(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
           state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
           state->tree->ns[parent_func].specific_type == e_binop);

    state->state = 26;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return parent_func;
    }

    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, parent_func, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_26(operator, parent_func, state);
        }
        else {
            if (state->tree->ns[state->tree->ns[parent_func].parent].specific_type == e_comp)
                set_second(state->tree->ns[parent_func].parent, operator, state);
            else
                set_first(state->tree->ns[parent_func].parent, operator, state);
            set_first(operator, parent_func, state);
            return state_26(operator, operator, state);
        }
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Second state for parsing right side of the comparison operator.
int state_25(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
           state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
           state->tree->ns[parent_func].specific_type == e_comp);
    assert(state->tree->ns[parent_func].second == n_index);

    state->state = 25;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return parent_func;
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, -1, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_26(operator, n_index, state);
        }
        else {
            set_second(parent_func, operator, state);
            set_first(operator, n_index, state);
            return state_26(operator, operator, state);
        }
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// First state for parsing right side of comparison operator.
// Therefore only ends upon par_end, and cannot accept another comparison operator.
int state_24(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
           state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
           state->tree->ns[parent_func].specific_type == e_comp);
    assert(state->tree->ns[state->tree->ns[parent_func].parent].node_type == n_stat &&
           state->tree->ns[state->tree->ns[parent_func].parent].specific_type == s_return);

    state->state = 24;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return parent_func;
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, expression, state);
        set_second(n_index, operator, state);
        return state_25(operator, n_index, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Continued right side of the comparison operator.
int state_23(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
           state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
           state->tree->ns[parent_func].specific_type == e_binop);

    state->state = 23;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }

    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, parent_func, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_23(operator, parent_func, state);
        }
        else {
            if (state->tree->ns[state->tree->ns[parent_func].parent].specific_type == e_comp)
                set_second(state->tree->ns[parent_func].parent, operator, state);
            else
                set_first(state->tree->ns[parent_func].parent, operator, state);
            set_first(operator, parent_func, state);
            return state_23(operator, operator, state);
        }
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Second state for parsing right side of the comparison operator.
int state_22(int n_index, int parent_func, struct state* state) {
    assert(check_node(n_index, n_expr, e_binop, state));
    assert(check_node(parent_func, n_expr, e_comp, state));
    assert(state->tree->ns[parent_func].second == n_index);

    state->state = 22;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, -1, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_23(operator, n_index, state);
        }
        else {
            set_second(parent_func, operator, state);
            set_first(operator, n_index, state);
            return state_23(operator, operator, state);
        }
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// First state for parsing right side of comparison operator.
// Therefore only ends upon par_end, and cannot accept another comparison operator.
int state_21(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
           state->tree->ns[n_index].specific_type == e_comp);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
           state->tree->ns[parent_func].specific_type == e_comp);
    assert(state->tree->ns[state->tree->ns[n_index].parent].node_type == n_expr &&
           state->tree->ns[state->tree->ns[n_index].parent].specific_type == e_condition);
    assert(parent_func == n_index);

    state->state = 21;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, expression, state);
        set_second(n_index, operator, state);
        return state_22(operator, n_index, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}
// else
// can be followed by another if statement, or a continue (into state 6), or anything else.
// when inside the else branch, it doesn't come back up and continue to state 6 until it hits
// a continue! Could make do with only an if and an anything else branch from this state?
int state_20(int n_index, int parent_func, struct state* state) {
    state->state = 20;
    debug_print(n_index, parent_func, state);

    if (check_type(t_else, state)) {
        state->i += 1;
        if (state->out >= verbose)
            printf("Found an else-token!");
    }
    if (check_type(t_if, state)) {
        state->i += 1;
        int if_node = add_node(n_stat, s_if, state);
        add_token(if_node, state->i - 1, state);
        set_second(n_index, if_node, state);

        return state_16(if_node, parent_func, state);
    }
    if (check_type(t_proceed, state)) {
        state->i += 1;
        return state_6(n_index, parent_func, state);
    }
    return state_6(n_index, parent_func, state);
    if (state->out >= standard) {
        printf("Parse error. Expected a continuation of an if-statement at %lu:%lu.\n",
            state->ti->ts[state->tree->ns[n_index].token_indices[0]].line_number,
            state->ti->ts[state->tree->ns[n_index].token_indices[0]].char_number);
    }
    return -1;
}

// Still left side of comparison operator.
// ONLY difference between state 18 and 19 is whether to also check binop precedence compared to the "parent" node.
// ONLY difference between 18/19 and 9/10 is whether to stop on semicolon or par_end.
int state_19(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
        state->tree->ns[parent_func].specific_type == e_binop);
    assert(state->tree->ns[state->tree->ns[parent_func].parent].node_type == n_expr &&
        state->tree->ns[state->tree->ns[parent_func].parent].specific_type == e_condition);

    state->state = 19;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }

    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, parent_func, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_19(operator, parent_func, state);
        }
        else {
            set_first(state->tree->ns[parent_func].parent, operator, state);
            set_first(operator, parent_func, state);
            return state_19(operator, operator, state);
        }
    }
    else if (check_type(t_comp, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_comp, state);
        add_token(operator, state->i - 1, state);
        int condition_index = state->tree->ns[parent_func].parent;
        set_first(operator, parent_func, state);
        set_first(condition_index, operator, state);
        int result = state_21(operator, operator, state);
        if (result == -1)
            return -1;
        return parent_func;
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected an end-parenthesis or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Left side of a comparison operator.
int state_18(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
        state->tree->ns[parent_func].specific_type == e_condition);
    assert(state->tree->ns[parent_func].first == n_index);

    state->state = 18;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, -1, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_19(operator, parent_func, state);
        }
        else {
            set_first(state->tree->ns[n_index].parent, operator, state);
            set_first(operator, n_index, state);
            return state_19(operator, operator, state);
        }
    }
    else if (check_type(t_comp, state)) {
        state->i += 1;
        int operator = new_operator(e_comp, n_index, parent_func, state);
        return state_21(operator, operator, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected an end-parenthesis, comparison, or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// First state to parse left side of comparison operator in if-statement condition.
int state_17(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
    state->tree->ns[n_index].specific_type == e_condition);

    state->state = 17;
    debug_print(n_index, parent_func, state);

    enum e_expr specific_type = check_value_type(state);
    if (specific_type == -1)
        return -1;
    state->i += 1;

    int expression = add_node(n_expr, specific_type, state);
    set_first(n_index, expression, state); // first differs from next_expression()
    add_token(expression, state->i - 1, state);

    if (specific_type == e_funcall) {
        state->i += 1;
        int result = state_12(expression, parent_func, state);
        if (result == -1) return -1;
    }

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }

    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(n_index, operator, state);
        set_first(operator, expression, state);

        int result = state_18(operator, n_index, state);
        if (state->out >= verbose)
            printf("Came back up from if-statement conditional tangent. n_index = %d. result = %d.\n",
                n_index, result);
        if (result == -1)
            return -1;
        return parent_func;

    }
    else if (check_type(t_comp, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_comp, state);
        add_token(operator, state->i - 1, state);
        set_first(n_index, operator, state);
        set_first(operator, expression, state);

        int result = state_21(operator, operator, state);
        if (state->out >= verbose)
            printf("Came back up from if-statement conditional tangent. n_index = %d. result = %d.\n",
                n_index, result);
        if (result == -1)
            return -1;
        return parent_func;
    }

    else {
        if (state->out >= standard)
            printf("Parse error. Expected an end-parenthesis, comparison, or binary operator at %lu:%lu.\n",
                state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

int state_16(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_stat &&
    state->tree->ns[n_index].specific_type == s_if);

    state->state = 16;
    debug_print(n_index, parent_func, state);

    if (!check_type(t_par_beg, state)) {
        if (state->out >= standard) {
            printf("Parse error. Expected opening parentheses to precede conditional expression\n ");
            printf("for the if-statement at %lu:%lu.\n",
                state->ti->ts[state->tree->ns[n_index].token_indices[0]].line_number,
                state->ti->ts[state->tree->ns[n_index].token_indices[0]].char_number);
        }
        debug_print(n_index, parent_func, state);
        return -1;
    }
    state->i += 1;

    int condition = add_node(n_expr, e_condition, state);
    set_first(n_index, condition, state);
    add_token(condition, state->i - 1, state);

    // Parse full conditional expression, returning upon seeing t_par_end.
    int result = state_17(condition, parent_func, state);
    if (result == -1)
        return -1;


    result = state_6(condition, n_index, state);
    if (state->out >= verbose)
        printf("Came back up state->out of if-statement's body tangent.\n");
    if (result == -1)
        return -1;
    // Parse (presumed) else statement.
    // This will become (this) n_index's second.
    return state_20(n_index, n_index, state);
}

// Continuous tree construction of chained binary operators.
// Inspired by state_10.
int state_15(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
    state->tree->ns[n_index].specific_type == e_binop);

    state->state = 15;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_comma, state) || check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }

    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, parent_func, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_15(operator, parent_func, state);
        }
        else {
            set_first(state->tree->ns[parent_func].parent, operator, state);
            set_first(operator, parent_func, state);
            return state_15(operator, operator, state);
        }
    }

    else {
        if (state->out >= standard)
            printf("Parse error. Expected a comma, end-parenthesis or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// The "tangent" for building the nodes that allow evaluation of a function call argument.
// Equivalent to the process for binding a value to a variable. Inspired by state_9.
int state_14(int n_index, int parent_func, struct state* state) {
    assert(*ntype(n_index, state) == n_expr &&
           *stype(n_index, state) == e_binop);

    state->state = 14;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_comma, state) || check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }

    else if (check_type(t_binop, state)) {
        state->i += 1;
        struct tuple result = binop(n_index, parent_func, expression, state);
        return state_15(result.fst, result.snd, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a comma, end-parenthesis, or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Follow-up to a complete argument, either look for next argument or continue.
int state_13(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
    state->tree->ns[n_index].specific_type == e_funcall);

    state->state = 13;
    debug_print(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        if (check_type(t_semicolon, state)) state->i += 1;
        return parent_func;
    }
    if (check_type(t_comma, state)) state->i += 1;
    else {
        if (state->out >= standard) {
            printf("Parse error. Expected comma or closing parentheses to follow the argument\n");
            printf("during function call at %lu:%lu.\n", state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        }
    }
    return state_12(n_index, parent_func, state);
}

// Begin parsing arguments for function call. Inspired by state_5.
int state_12(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_funcall);

    state->state = 12;
    debug_print(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        return parent_func;
    }
    enum e_expr specific_type = check_value_type(state);
    state->i += 1;

    int argument = add_node(n_expr, e_argument, state);
    int expression = add_node(n_expr, specific_type, state);
    set_first(n_index, argument, state);
    set_first(argument, expression, state);
    add_token(expression, state->i - 1, state);

    if (specific_type == e_funcall) {
        state->i += 1;
        int result = state_12(expression, parent_func, state);
        if (result == -1) return -1;
    }

    if (check_type(t_binop, state)) {
        state->i += 1;

        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(argument, operator, state);
        set_first(operator, expression, state);

        int result = state_14(operator, operator, state);
        if (state->out >= verbose)
            printf("Came back up from argument tangent. n_index = %d. result = %d.\n", n_index, result);
        if (result == -1) return -1;
    }
    return state_13(n_index, parent_func, state);
}

// Right side of comparison operator, but only from state 7...
int state_11(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_comp);

    state->state = 11;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);
    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return parent_func; //state_1(n_index, parent_func, state);
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, expression, state);
        set_second(n_index, operator, state);
        int result = state_24(operator, n_index, state);
        if (result == -1)
            return -1;
        return n_index;
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semicolon to finish comparison at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Parsing "y" in expression "return x (+) y"
// node_index is "x (+)"
// node_index.first = x
// node_index.parent = return
// y is unknown and could be another binop expression.
// y is currently pointed to by state->i.
// id/num ; -> s1
// id/num binop -> s10
// ONLY difference between state 9 and 10 is whether to also check precedence compared to the "parent" node.
int state_10(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_binop);

    state->state = 10;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return parent_func;
    }
    // Parsing the second binary operator in "x (+) y (+) z".
    // node_index is the first binary operator.
    // node_index.first = x
    // node_index.second = not initialised yet
    // node_index.parent is unknown.
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, parent_func, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_10(operator, parent_func, state);
        }
        else {
            set_first(state->tree->ns[parent_func].parent, operator, state);
            set_first(operator, parent_func, state);
            return state_10(operator, operator, state);
        }
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon, function call, or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Parsing "y" in expression "x (+) y"
// node_index is "x (+)"
// node_index.first = x
// node_index.parent = unknown
// y is unknown and could be another binop expression.
// y is currently pointed to by state->i.
// id/num ; -> return 6
// id/num binop -> s9
int state_9(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_binop);

    state->state = 9;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return parent_func;
    }
    // Parsing the second binary operator in "x (+) y (+) z".
    // node_index is the first binary operator.
    // node_index.first = x
    // node_index.second = not initialised yet
    // node_index.parent is unknown.
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, -1, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_10(operator, parent_func, state);
        }
        else {
            set_first(state->tree->ns[n_index].parent, operator, state);
            set_first(operator, n_index, state);
            return state_10(operator, operator, state);
        }
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

int state_27(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_expr &&
        state->tree->ns[parent_func].specific_type == e_binop);

    state->state = 27;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return state->tree->ns[parent_func].parent; //state_1(n_index, state->nt->nodes[parent_func].parent, state);
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, parent_func, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_27(operator, parent_func, state);
        }
        else {
            set_second(state->tree->ns[parent_func].parent, operator, state);
            set_first(operator, parent_func, state);
            return state_27(operator, operator, state);
        }
    }
    else if (check_type(t_comp, state)) {
        state->i += 1;
        int operator = new_operator(e_comp, n_index, parent_func, state);
        return state_11(operator, operator, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// Parsing "y (+) z" in expression "x (+) y (+) z"
// node_index is the binary operator in "x (+) y"
// node_index.first = y
// node_index.parent is the first statement in a function, and is a return statement.
// z is unknown and could be another binop expression.
// z is currently pointed to by state->i.
// id/num ; -> s1
// id/num binop -> s8
int state_8(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_expr &&
        state->tree->ns[n_index].specific_type == e_binop);
    assert(state->tree->ns[parent_func].node_type == n_stat &&
        state->tree->ns[parent_func].specific_type == s_return);
    assert(state->tree->ns[parent_func].second == n_index);

    state->state = 8;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return state->tree->ns[parent_func].parent; //state_1(n_index, state->nt->nodes[parent_func].parent, state);
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        if (op_precedence(n_index, operator, -1, state)) {
            set_first(operator, expression, state);
            set_second(n_index, operator, state);
            return state_27(operator, operator, state);
        }
        else {
            set_second(parent_func, operator, state);
            set_first(operator, n_index, state);
            return state_27(operator, operator, state);
        }
    }
    else if (check_type(t_comp, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_comp, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, n_index, state);
        set_second(parent_func, operator, state);
        return state_11(operator, operator, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
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
int state_7(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_stat &&
        state->tree->ns[n_index].specific_type == s_return);
    assert(state->tree->ns[parent_func].node_type == n_stat &&
        state->tree->ns[parent_func].specific_type == s_fun_bind);

    state->state = 7;
    debug_print(n_index, parent_func, state);

    int expression = next_expression(n_index, parent_func, state);

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return state->tree->ns[parent_func].parent; //state_1(expression, state->nt->nodes[parent_func].parent, state);
    }
    else if (check_type(t_binop, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, expression, state);
        set_second(n_index, operator, state);
        return state_8(operator, n_index, state);
    }
    else if (check_type(t_comp, state)) {
        state->i += 1;
        int operator = add_node(n_expr, e_comp, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, expression, state);
        set_second(n_index, operator, state);
        return state_11(operator, n_index, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

int state_6_ret(int n_index, int parent_func, struct state* state)
{
    if (state->out >= verbose)
        printf("state 6 returning.\n");
    state->i += 1;
    int specific_type = check_value_type(state);
    state->i += 1;

    int return_node_index = add_node(n_stat, s_return, state);
    set_second(n_index, return_node_index, state);

    int expression = add_node(n_expr, specific_type, state);
    set_second(return_node_index, expression, state);
    add_token(expression, state->i - 1, state);

    if (specific_type == e_funcall)
    {
        state->i += 1;
        int result = state_12(expression, parent_func, state);
        if (result == -1)
            return -1;
    }

    if (check_type(t_semicolon, state))
    {
        state->i += 1;
        if (state->ti->n > state->i && check_type(t_else, state))
        {
            // state->i += 1;
            return parent_func;
        }
        return state->tree->ns[parent_func].parent; // state_1(return_node_index, state->nt->nodes[parent_func].parent, state);
    }
    else if (check_type(t_binop, state))
    {
        state->i += 1;
        int operator= add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(operator, expression, state);
        set_second(return_node_index, operator, state);
        int result = state_8(operator, return_node_index, state);
        if (result == -1)
            return -1;
        return parent_func;
    }
    else
    {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon to end function body at %lu:%lu.\n",
                   state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
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
int state_6(int n_index, int parent_func, struct state* state) {
    state->state = 6;
    debug_print(n_index, parent_func, state);

    if (check_type(t_return, state)) {
        return state_6_ret(n_index, parent_func, state);
    }
    if (check_type(t_if, state)) {
        state->i += 1;
        int if_node = add_node(n_stat, s_if, state);
        set_second(n_index, if_node, state);
        add_token(if_node, state->i - 1, state);
        return state_16(if_node, n_index, state);
    }
    if (check_type(t_type, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a variable type at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    if (check_type(t_id, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a variable name at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    if (check_type(t_eq, state)) {
        state->i += 1;
        int new_node_index = add_node(n_stat, s_var_bind, state);
        set_second(n_index, new_node_index, state);
        add_token(new_node_index, state->i - 3, state);
        add_token(new_node_index, state->i - 2, state);

        return state_5(new_node_index, parent_func, state);
    }
    else if (check_type(t_par_beg, state)) {
        state->i -= 1;
        int funcall = add_node(n_stat, s_function_call, state);
        set_second(n_index, funcall, state);
        add_token(funcall, state->i - 3, state);
        add_token(funcall, state->i - 2, state);
        int result = state_12(funcall, parent_func, state);
        if (result == -1)
            return -1;
        return state_6(funcall, parent_func, state);
    }
    return -1;
}

// Binding an expression to a variable.
// Node index is the variable node.
// state->i is pointing at the first token after the equals sign.
// num ; -> return 6
// num binop -> s9
// id ; -> return 6
// id binop -> s9
int state_5(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_stat
        && state->tree->ns[n_index].specific_type == s_var_bind);

    state->state = 5;
    debug_print(n_index, parent_func, state);

    int specific_type = check_value_type(state);
    state->i += 1;

    int expression = add_node(n_expr, specific_type, state);
    set_first(n_index, expression, state);
    add_token(expression, state->i - 1, state);

    if (specific_type == e_funcall) {
        state->i += 1;
        int result = state_12(expression, parent_func, state);
        if (result == -1) return -1;
    }

    if (check_type(t_semicolon, state)) {
        state->i += 1;
        return state_6(n_index, parent_func, state);
    }
    // when binding the results of a binop to a variable.
    // "x (+) ?"
    else if (check_type(t_binop, state)) {
        if (state->out >= verbose)
            printf("First BINOP found after variable bind start.\n");
        state->i += 1;
        int operator = add_node(n_expr, e_binop, state);
        add_token(operator, state->i - 1, state);
        set_first(n_index, operator, state);
        set_first(operator, expression, state);

        int result = state_9(operator, operator, state);
        if (state->out >= verbose)
            printf("Came back up from variable tangent. n_index = %d. result = %d.\n",
                n_index, result);
        if (result == -1) return -1;
        return state_6(n_index, parent_func, state);
    }
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a semi-colon or binary operator to follow variable value \"%s\"(%lu:%lu) at %lu:%lu.\n",
            state->ti->ts[state->i-2].val, state->ti->ts[state->i-2].line_number,
            state->ti->ts[state->i-2].char_number, state->ti->ts[state->i].line_number,
            state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
}

// First statement in function body.
// return ; -> s1
// return -> s7
// type id = -> s5 -> ?
// node_index = function binding
// fun_parent = function binding
int state_4(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_stat
        && state->tree->ns[n_index].specific_type == s_fun_bind);
    assert(n_index == parent_func);

    state->state = 4;
    debug_print(n_index, parent_func, state);

    if (check_type(t_return, state)) {
        state->i += 1;
        int return_node_index = add_node(n_stat, s_return, state);
        set_second(n_index, return_node_index, state);

        if (check_type(t_semicolon, state)) {
            if (state->out >= standard)
                printf("Warning: Empty function body at %lu:%lu.\n",
                state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
            debug_print(n_index, parent_func, state);
            state->i += 1;
            return state->tree->ns[n_index].parent;
            //return state_1(return_node_index, state->nt->nodes[n_index].parent, state);
        }
        else {
            return state_7(return_node_index, n_index, state);
        }
    }
    if (check_type(t_if, state)) {
        state->i += 1;
        int if_node = add_node(n_stat, s_if, state);
        set_second(n_index, if_node, state);
        add_token(if_node, state->i - 1, state);
        return state_16(if_node, n_index, state);
    }
    if (check_type(t_type, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    if (check_type(t_id, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected a variable name at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    if (check_type(t_eq, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    int new_node_index = add_node(n_stat, s_var_bind, state);
    set_second(n_index, new_node_index, state);
    add_token(new_node_index, state->i - 3, state);
    add_token(new_node_index, state->i - 2, state);

    return state_5(new_node_index, n_index, state);
}

// Function parameter follow-up (either another parameter or the start of function body)
// , -> s2
// ) = -> s4
// node_index = parameter node.
// fun_parent = function binding.
int state_3(int n_index, int parent_func, struct state* state) {
    state->state = 3;
    debug_print(n_index, parent_func, state);

    if (check_type(t_comma, state)) {
        state->i += 1;
        return state_2(n_index, parent_func, state);
    }
    if (check_type(t_par_end, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected ')' to proceed function parameters at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    if (check_type(t_eq, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected '=' to indicate start of function body at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }

    return state_4(state->tree->ns[n_index].parent, parent_func, state);
}

// Function parameter(s).
// First check if at the end of the parameter declarations with ") ="
// Otherwise, check for a parameter declaration with "type id".
//    If so, add them as tokens, and go to state
// ) = -> s4
// type id -> s3
// Node index = parameter node
// fun_parent = function binding
int state_2(int n_index, int parent_func, struct state* state) {
    state->state = 2;
    debug_print(n_index, parent_func, state);

    if (check_type(t_par_end, state)) {
        state->i += 1;
        if (check_type(t_eq, state)) {
            state->i += 1;
            return state_4(state->tree->ns[n_index].parent, parent_func, state);
        }
        else {
            if (state->out >= standard)
                printf("Parse error. Expected '=' to precede function body at %lu:%lu.\n",
                    state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
            debug_print(n_index, parent_func, state);
            return -1;
        }
    }
    if (check_type(t_type, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected parameter type at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }
    if (check_type(t_id, state)) state->i += 1;
    else {
        if (state->out >= standard)
            printf("Parse error. Expected parameter name at %lu:%lu.\n",
            state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
        debug_print(n_index, parent_func, state);
        return -1;
    }

    add_token(n_index, state->i - 2, state);
    add_token(n_index, state->i - 1, state);
    return state_3(n_index, parent_func, state);
}

// Expecting a function declaration. State checks type, ID, and beginning parentheses to contain parameters.
// If these are correct, add a program node (n_prog), then a function bind statement node (n_stat + s_fun_bind),
// and finally a parameter expression node. The parameter node will be evaluated first, then the function bind
// node, before returning the result up the chain to evaluate the program node.
// A program contains one or more functions.
// type id ( -> s2
// fun_parent = top node of entire program?
int state_1(int n_index, int parent_func, struct state* state) {
    assert(state->tree->ns[n_index].node_type == n_prog &&
        state->tree->ns[parent_func].node_type == n_prog);

    state->state = 1;
    debug_print(n_index, parent_func, state);

    int new_parent = parent_func;

    while (state->ti->n > state->i) {
        // Program must consist of only functions at its highest level, and must have at least one function.
        if (!check_type(t_type, state)) {
            if (state->out >= standard)
                printf("Parse error. Expected function type at %lu:%lu.\n",
                    state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
            debug_print(n_index, parent_func, state);
            return -1;
        }
        state->i += 1;

        if (!check_type(t_id, state)) {
            if (state->out >= standard)
                printf("Parse error. Expected function name at %lu:%lu.\n",
                    state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
            debug_print(n_index, parent_func, state);
            return -1;
        }
        state->i += 1;

        if (!check_type(t_par_beg, state)) {
            if (state->out >= standard)
                printf("Parse error. Expected '(' to precede function parameters at %lu:%lu.\n",
                    state->ti->ts[state->i].line_number, state->ti->ts[state->i].char_number);
            debug_print(n_index, parent_func, state);
            return -1;
        }
        state->i += 1;

        int top_node_index = add_node(n_prog, 0, state);
        set_second(new_parent, top_node_index, state);

        int fun_declaration = add_node(n_stat, s_fun_bind, state);
        add_token(fun_declaration, state->i - 3, state);
        add_token(fun_declaration, state->i - 2, state);
        set_first(top_node_index, fun_declaration, state);

        int new_param_node_index = add_node(n_expr, e_param, state);
        set_first(fun_declaration, new_param_node_index, state);

        if (state->out >= verbose)
            printf("Added new function: %s %s\n", state->ti->ts[state->i-3].val, state->ti->ts[state->i-2].val);
        // TODO: Bind to ftable here, and check for "main" function ID.
        state_2(new_param_node_index, fun_declaration, state);
        new_parent = top_node_index;
    }
    return END_STATE;
}

/// @brief Initial state from which the tree structure is created.
/// @param state->ti
/// @param state->i
/// @param state->nt
/// @param n_index
/// @param parent_func
/// @param state->output
/// @param state->out
/// @return END_STATE on success, -1 otherwise.
int state_0(int n_index, int parent_func, struct state* state) {
    assert(n_index == 0);
    assert(parent_func == 0);

    state->state = 0;
    debug_print(n_index, parent_func, state);

    assert(state->tree->n == 0);
    int top_node_index = add_node(n_prog, 0, state);

    return state_1(top_node_index, parent_func, state);
}

struct node_tree* parse(FILE* f, struct state* state) {
    char c = fgetc(f);
    if (c == EOF) {
        if (state->out >= standard)
            printf("Parse error. Empty token file.\n");
        return NULL;
    }
    ungetc(c, f);
    state->i = 0;
    state->tree = malloc(sizeof(*state->tree));
    state->tree->ns = malloc(sizeof(*state->tree->ns));
    state->tree->n = 0;
    int final_state = state_0(0, 0, state);
    if (final_state == END_STATE) return state->tree;
    else return NULL;
}