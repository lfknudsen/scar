#ifndef SCAR_H
#define SCAR_H

#define END_STATE 128

//#define VERBOSE 0

enum Type {
	int_val,
	float_val,
	string,
	none
};

enum e_token {
	t_type,
	t_type_int,
	t_type_float,
	t_type_string,
	t_type_void,
	t_return,
	t_id,
	t_num_int,
	t_num_float,
	t_par_beg,
	t_par_end,
	t_curl_beg,
	t_curl_end,
	t_eq,
	t_colon,
	t_semicolon,
	t_binop,
	t_comma,
	t_not,
	t_neq,
	t_double_eq,
	t_bool,
	t_if,
	t_else,
	t_proceed,
	t_greater_than,
	t_less_than,
	t_geq,
	t_leq,
	t_comp
};

enum e_binop {
	plus,
	minus,
	times,
	divide
};

enum e_stat {
	s_var_bind,
	s_fun_bind,
	s_function_call,
	s_if,
	s_else,
	s_return,
	s_fun_body,
	s_proceed
};

enum e_expr {
	e_id,
	e_val,
	e_binop,
	e_param,
	e_funcall,
	e_comp,
	e_argument,
	e_not,
	e_condition
};

enum e_nodetype {
	n_none,
	n_stat,
	n_expr,
	n_prog
};

struct node {
	enum e_nodetype node_type;
	int specific_type;
	int token_count;
	int* token_indices;
	int first;
	int second;
	int parent;
	int print_indent;
};

struct node_tree {
	int n;
	struct node* ns;
};

struct Int {
	int val;
};

struct token {
	enum e_token type;
	char* val;
	unsigned long line_number;
	unsigned long char_number;
};

struct token_index {
	unsigned long n;
	struct token* ts;
};

struct function_binding {
	char* id;
	unsigned int node_index;
	unsigned int param_count;
};

struct ftable_index {
	unsigned int n;
	struct function_binding* fs;
	unsigned int start_fun_node;
};

struct var_binding {
	char* id;
	unsigned int val_length;
	int val;
};

struct vtable_index {
	unsigned int n;
	struct var_binding* vs;
};

struct ivar_binding {
	char* id;
	int val;
};

struct ivtable_index {
	unsigned int n;
	struct var_binding* vs;
};

enum error_codes {
	no_error,
	unbound_variable_name,
};

enum out_mode {
    quiet,
    standard,
    verbose
};

struct state {
	struct node_tree* tree;
	struct token_index* ti;
	FILE* output;
	int i;
	enum out_mode out;
	int state;
};

int out;

void print_node(int n_index, struct state* st);

void fprint_node(int n_index, struct state* st);

void free_ivtable(struct ivtable_index* vt);

char* get_val(int n_index, struct state* st);

#endif