#ifndef SCAR_H
#define SCAR_H

#define END_STATE 7

enum Type {
	int_val,
	float_val,
	string,
	none
};

enum e_token {
	t_type,
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
	t_comma
};

/*
char const*const token_str[7] = {
	[t_type] = "int",
	[t_type] = "float",
	[t_type] = "string",
	[t_type] = "void",
	[t_return] = "return",
	[t_id] = "id",
	[t_num] = "num"
};
*/

struct Int {
	int val;
};

struct Token {
	enum e_token type;
	char *val;
	unsigned long line_no;
	unsigned long char_no;
};

struct token_index {
	unsigned long n;
	struct Token *ts;
};

#endif