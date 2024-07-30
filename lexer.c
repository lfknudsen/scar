#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#include <assert.h>

int init_token(struct token_index* ti, unsigned long* token_count, int* sum_sizeof_ts,
		int* sum_sizeof_val, int line_number, int char_number, char* string, enum e_token token_type) {
	ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (*token_count + 1));
	*sum_sizeof_ts += sizeof(*ti->ts);
	ti->ts[*token_count].type = token_type;
	ti->ts[*token_count].line_number = line_number;
	ti->ts[*token_count].char_number = char_number;
	size_t sizeof_val = sizeof(*ti->ts[*token_count].val) * (strlen(string) + 1);
	*sum_sizeof_val += sizeof_val;
	ti->ts[*token_count].val = malloc(sizeof_val);
	strcpy(ti->ts[*token_count].val, string);
	ti->ts[*token_count].val[strlen(string)] = '\0';
	*token_count += 1;
	return *token_count;
}

// Check the value of the next non-whitespace character.
// Returns 1 if the next non-whitespace character is the one expected.
// Returns -1 on EOF.
// Returns 0 otherwise.
// Modifies the char and line numbers in-place to correspond to whitespace,
// but not other characters.
int read_ahead(FILE* f, char expected, unsigned long* char_number, unsigned long* line_number, int out) {
	char c = fgetc(f);
	while ((c != EOF) && (c == ' ' || c == '\n')) {
		if (c == ' ') *char_number += 1;
		else if (c == '\n') {
			*char_number = 1;
			*line_number += 1;
		}
		c = fgetc(f);
	}
	if (c == EOF) return -1;
	if (c == expected) {
		return 1;
	}
	ungetc(c, f);

	return 0;
}

// Read through the given file, assembling a list of tokens.
// Also writes its results to an state->output file.
// It is caller's responsibility to open and close file buffers, as
// well as check for validity.
// return: Number of tokens found.
int lex(FILE *f, struct state* st) {
	//struct Token *ts = &state->ti->ts;
	int sum_sizeof_ts = 0;
	int sum_sizeof_val = 0;

	unsigned long line_number = 1;
	unsigned long char_number = 1;
	unsigned long token_count = 0;
	char c = fgetc(f);
	while (c != EOF && c != '\0') {
		unsigned int char_number_add = 1;
		if (c == '\n') {
			line_number ++;
			char_number = 1;
			c = fgetc(f);
			continue;
		}
		if (c == ' ') {
			char_number ++;
			c = fgetc(f);
			continue;
		}
		// [a-zA-Z_]+[a-zA-Z0-9_]*
		else if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c == 95)) { // [a-zA-Z_]
			int string_len = 1;
			char *letter_string = malloc(sizeof(char) * 2);
			letter_string[0] = c;
			c = fgetc(f);
			while ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == '_' || (c >= 48 && c <= 57)) { // [a-zA-Z0-9_]
				string_len ++;
				letter_string = realloc(letter_string, sizeof(char) * (string_len + 1));
				letter_string[string_len - 1] = c;
				char_number_add ++;
				c = fgetc(f);
			}
			letter_string[string_len] = '\0';

			enum e_token string_type;
			if (strcmp(letter_string,"void")==0)	{
				fprintf(st->output, "%3lu: TOKEN: TYPE\n", token_count);
				string_type = t_type;
			}
			else if (strcmp(letter_string,"int")==0) {
				fprintf(st->output, "%3lu: TOKEN: TYPE\n", token_count);
				string_type = t_type;
			}
			else if (strcmp(letter_string,"float")==0) {
				fprintf(st->output, "%3lu: TOKEN: TYPE\n", token_count);
				string_type = t_type;
			}
			else if (strcmp(letter_string,"return")==0) {
				fprintf(st->output, "%3lu: TOKEN: RETURN\n", token_count);
				string_type = t_return;
			}
			else if (strcmp(letter_string,"if")==0) {
				fprintf(st->output, "%3lu: TOKEN: IF\n", token_count);
				string_type = t_if;
			}
			else if (strcmp(letter_string,"else")==0) {
				fprintf(st->output, "%3lu: TOKEN: ELSE\n", token_count);
				string_type = t_else;
			}
			else if (strcmp(letter_string,"proceed")==0) {
				fprintf(st->output, "%3lu: TOKEN: PROCEED\n", token_count);
				string_type = t_proceed;
			}
			else {
				fprintf(st->output, "%3lu: TOKEN: ID\n", token_count);
				string_type = t_id;
			}
			fprintf(st->output, " Line#: %lu\n Char#: %lu\n", line_number, char_number);
			if (strcmp(letter_string,"return") != 0) {
				fprintf(st->output, " Value: %s\n", letter_string);
			}

			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, letter_string, string_type);

			char_number += char_number_add;
			free(letter_string);
			continue;
		}
		// Number token
		else if ((c >= 48 && c <= 57)) { // [0-9]
			char is_floating_type = 0;
			int negative = 0;
			if (token_count > 2 && st->ti->ts[token_count - 1].type == t_binop &&
					strcmp(st->ti->ts[token_count - 1].val,"-") == 0 &&
					st->ti->ts[token_count - 2].type != t_num_int &&
					st->ti->ts[token_count - 2].type != t_num_float &&
					st->ti->ts[token_count - 2].type != t_id &&
					st->ti->ts[token_count - 2].type != t_par_end) {
				negative = 1;
				st->ti->ts = realloc(st->ti->ts, (token_count - 1) * sizeof(*st->ti->ts));
				token_count --;
			}
			char *number_string = malloc(sizeof(char) * (2 + negative));
			int string_len = 1 + negative;
			if (negative) number_string[0] = '-';
			number_string[0 + negative] = c;
			c = fgetc(f);
			while (is_floating_type <= 1 && ((c >= 48 && c <= 57) || (c == 46))) { // [0-9.]
				if (c == 46) {
					if (is_floating_type) {
						is_floating_type ++;
						char_number_add ++;
						if (out >= standard) {
							printf("Lexer error. Extra '.' in floating-point number");
							printf(" at %lu:%lu\n", line_number, char_number + char_number_add);
						}
					} else {
						is_floating_type ++;
						string_len ++;
						number_string = realloc(number_string, sizeof(*number_string) * string_len + 1);
						number_string[string_len - 1] = c;
						char_number_add ++;
						c = fgetc(f);
					}
				}
				else {
					string_len ++;
					number_string = realloc(number_string, sizeof(*number_string) * string_len + 1);
					number_string[string_len - 1] = c;
					char_number_add ++;
					c = fgetc(f);
				}
			}
			number_string[string_len] = '\0';
			if (out >= verbose) printf("String: \"%s\"\nString length: %d\n", number_string, string_len);
			if (is_floating_type) {
				fprintf(st->output, "%3lu: TOKEN: NUM(FLOAT)\n Line#: %lu\n Char#: %lu\n Value: %s\n", token_count, line_number, char_number, number_string);
			}
			else {
				fprintf(st->output, "%3lu: TOKEN: NUM(INT)\n Line#: %lu\n Char#: %lu\n Value: %s\n", token_count, line_number, char_number, number_string);
			}

			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val, line_number,
				char_number, number_string, (is_floating_type) ? t_num_float : t_num_int);

			free(number_string);
			char_number += char_number_add;
			continue;
		}
		else if (c == '+') {
			fprintf(st->output, "%3lu: TOKEN: BINOP\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "+", t_binop);
		}
		else if (c == '-') {
			fprintf(st->output, "%3lu: TOKEN: BINOP\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "-", t_binop);
		}
		else if (c == '*') {
			if (read_ahead(f, '*', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: BINOP\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "**", t_binop);
			}
			fprintf(st->output, "%3lu: TOKEN: BINOP\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "*", t_binop);
		}
		else if (c == '/') {
			fprintf(st->output, "%3lu: TOKEN: BINOP\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "/", t_binop);
		}
		else if (c == '{') {
			fprintf(st->output, "%3lu: TOKEN: CURL_BEG\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "{", t_curl_beg);
		}
		else if (c == '}') {
			fprintf(st->output, "%3lu: TOKEN: CURL_END\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "}", t_curl_end);
		}
		else if (c == '(') {
			fprintf(st->output, "%3lu: TOKEN: PAR_BEG\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, "(", t_par_beg);
		}
		else if (c == ')') {
			fprintf(st->output, "%3lu: TOKEN: PAR_END\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, ")", t_par_end);
		}
		else if (c == ':') {
			fprintf(st->output, "%3lu: TOKEN: COLON\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, ":", t_colon);
		}
		else if (c == '=') {
			if (read_ahead(f, '=', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: DEQ\n", token_count - 1);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "==", t_comp);
			}
			else if (read_ahead(f, '>', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: GREATER THAN OR EQUAL\n", token_count - 1);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, ">=", t_comp);
			}
			else if (read_ahead(f, '<', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: LESS THAN OR EQUAL\n", token_count - 1);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "<=", t_comp);
			}
			else {
				fprintf(st->output, "%3lu: TOKEN: EQ\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "=", t_eq);
			}
		}
		else if (c == ';') {
			fprintf(st->output, "%3lu: TOKEN: SEMICOLON\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, ";", t_semicolon);
		}
		else if (c == ',') {
			fprintf(st->output, "%3lu: TOKEN: COMMA\n", token_count);
			init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
				line_number, char_number, ",", t_comma);
		}
		else if (c == '!') {
			if (read_ahead(f, '=', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: NOT EQUAL\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "!=", t_comp);
			} else {
				fprintf(st->output, "%3lu: TOKEN: NOT\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "!", t_not);
			}
		}
		else if (c == '>') {
			if (read_ahead(f, '=', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: GREATER THAN OR EQUAL\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, ">=", t_comp);
			}
			else {
				fprintf(st->output, "%3lu: TOKEN: GREATER THAN\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, ">", t_comp);
			}
		}
		else if (c == '<') {
			if (read_ahead(f, '=', &char_number, &line_number, out) == 1) {
				fprintf(st->output, "%3lu: TOKEN: LESS THAN OR EQUAL\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "<=", t_comp);
			}
			else {
				fprintf(st->output, "%3lu: TOKEN: LESS THAN\n", token_count);
				init_token(st->ti, &token_count, &sum_sizeof_ts, &sum_sizeof_val,
					line_number, char_number, "<", t_comp);
			}
		}
		else {
			if (c != 10 && c != 13) fprintf(st->output, "UNKNOWN (%d)\n", c);
			else {
				c = fgetc(f);
				continue;
			}
		}
		fprintf(st->output, " Line#: %lu\n Char#: %lu\n", line_number, char_number);
		if (st->ti->ts[token_count - 1].type == t_binop) {
			fprintf(st->output, " Value: %s\n", st->ti->ts[token_count - 1].val);
		}
		char_number += char_number_add;
		c = fgetc(f);
	}
	if (out >= verbose) printf("Total size of tokens: %d\nTotal size of vals:%d\n", sum_sizeof_ts, sum_sizeof_val);
	return token_count;
}