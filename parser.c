#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

void debug_print(int state, unsigned int index) {
	printf("Current state: %d\nToken index: %u\n", state, index);
}

int check_type(struct token_index *ti, enum e_token expected, int i) {
	if (ti->n <= i) {
		printf("Parse error. Code terminates prematurely at %lu:%lu.\n",
		ti->ts[ti->n - 1].line_no, ti->ts[ti->n - 1].char_no);
		return 0;
	}
	return (ti->ts[i].type == expected);
}

int parse_s(int state, struct token_index *ti, int *i) {
	printf("State: %d. Token index: %d\n", state, *i);
	struct Token *t = ti->ts;
	switch (state) {
		case 0:
			return 1;
		case 1:
			if (ti->n == *i) return END_STATE;
			if (check_type(ti, t_type, *i)) *i += 1;
			else {
				printf("Parse error. Expected function type at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_id, *i)) *i += 1;
			else {
				printf("Parse error. Expected function name at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_par_beg, *i)) *i += 1;
			else {
				printf("Parse error. Expected '(' to precede function parameters at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			return 2;
		case 2:
			if (check_type(ti, t_par_end, *i)) {
				*i += 1;
				if (check_type(ti, t_eq, *i)) {
					*i += 1;
					return 4;
				}
			}
			if (check_type(ti, t_type, *i)) *i += 1;
			else {
				printf("Parse error. Parameter name must be preceded by a type at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_id, *i)) *i += 1;
			else {
				printf("Parse error. Expected parameter name at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			return 3;
		case 3:
			if (check_type(ti, t_comma, *i)) {
				*i += 1;
				return 2;
			}
			if (check_type(ti, t_par_end, *i)) *i += 1;
			else {
				printf("Parse error. Expected ')' to proceed function parameters at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_eq, *i)) *i += 1;
			else {
				printf("Parse error. Expected '=' to indicate start of function body at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			return 4;
		case 4:
			if (check_type(ti, t_return, *i)) {
				*i += 1;
				if (check_type(ti, t_id, *i) || check_type(ti, t_num_int, *i) || check_type(ti, t_num_float, *i)) {
					*i += 1;
					return 1;
				}
				else if (check_type(ti, t_semicolon, *i)) {
					*i += 1;
					printf("Warning: Empty function body at %lu:%lu.\n", t[*i].line_no, t[*i].char_no);
					debug_print(state, *i);
					return 1;
				}
				else {
					printf("Parse error. Expected a return value or semi-colon to end function body at %lu:%lu.\n",
					t[*i].line_no, t[*i].char_no);
					debug_print(state, *i);
					return -1;
				}
			}
			// TODO: THIS ONLY SUPPORTS LET-STATEMENTS FOR NOW
			if (check_type(ti, t_type, *i)) *i += 1;
			else {
				printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_id, *i)) *i += 1;
			else {
				printf("Parse error. Expected a variable name at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_eq, *i)) *i += 1;
			else {
				printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			return 5;
		case 5:
			if (check_type(ti, t_num_int, *i) || check_type(ti, t_num_float, *i)) *i += 1;
			else {
				printf("Parse error. Expected a number to bind to variable at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_semicolon, *i)) *i += 1;
			else {
				printf("Parse error. Expected a semi-colon to proceed variable binding statement at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			return 6;
		case 6:
			if (check_type(ti, t_return, *i)) {
				*i += 1;
				if (check_type(ti, t_id, *i) || check_type(ti, t_num_int, *i) || check_type(ti, t_num_float, *i)) {
					*i += 1;
					if (check_type(ti, t_semicolon, *i)) {
						*i += 1;
						return 1;
					}
					else {
					printf("Parse error. Expected a semi-colon to end function body at %lu:%lu.\n",
					t[*i].line_no, t[*i].char_no);
					debug_print(state, *i);
					return -1;
					}
				}
				else {
					printf("Parse error. Expected a return value or semi-colon to end function body at %lu:%lu.\n",
					t[*i].line_no, t[*i].char_no);
					debug_print(state, *i);
					return -1;
				}
			}
			// TODO: THIS ONLY SUPPORTS LET-STATEMENTS FOR NOW
			if (check_type(ti, t_type, *i)) *i += 1;
			else {
				printf("Parse error. Expected a type to begin variable declaration at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_id, *i)) *i += 1;
			else {
				printf("Parse error. Expected a variable name at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			if (check_type(ti, t_eq, *i)) *i += 1;
			else {
				printf("Parse error. Expected an equals symbol for variable declaration at %lu:%lu.\n",
				t[*i].line_no, t[*i].char_no);
				debug_print(state, *i);
				return -1;
			}
			return 5;
		default:
			return -1;
	};
}

int parse(FILE *f, struct token_index *ti) {
	char c = fgetc(f);
	if (c == EOF) {
		printf("Parse error. Empty token file.\n");
		return -1;
	}
	ungetc(c, f);
	int goto_state = 0;
	int index = 0;
	while (goto_state >= 0 && goto_state != END_STATE) {
		//goto_state = parse_state(goto_state, f);
		goto_state = parse_s(goto_state, ti, &index);
	}
	return goto_state;
}