#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#include <assert.h>

int init_read_token(struct token_index* ti, unsigned long* token_count, int* sum_sizeof_ts, int line_number,
		int char_number, int* sum_sizeof_val, char** string, int string_len, enum e_token token_type) {
	ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (*token_count + 1));
	*sum_sizeof_ts += sizeof(*ti->ts);
	ti->ts[*token_count].type = token_type;
	ti->ts[*token_count].line_number = line_number;
	ti->ts[*token_count].char_number = char_number;
	size_t sizeof_val = sizeof(*ti->ts[*token_count].val) * (string_len);
	*sum_sizeof_val += sizeof_val;
	ti->ts[*token_count].val = malloc(sizeof(*ti->ts[*token_count].val) * (string_len));
	strcpy(ti->ts[*token_count].val, *string);
	ti->ts[*token_count].val[string_len] = '\0';
	*token_count ++;
}

// Read through the given file, assembling a list of tokens.
// Also writes its results to an output file.
// It is caller's responsibility to open and close file buffers, as
// well as check for validity.
// return: Number of tokens found.
int lex(FILE *f, FILE *output, struct token_index *ti) {
	//struct Token *ts = &ti->ts;
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

			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			if (strcmp(letter_string,"void")==0)	{
				fprintf(output, "%3lu: TOKEN: TYPE\n", token_count);
				ti->ts[token_count].type = t_type;
			}
			else if (strcmp(letter_string,"int")==0) {
				fprintf(output, "%3lu: TOKEN: TYPE\n", token_count);
				ti->ts[token_count].type = t_type;
			}
			else if (strcmp(letter_string,"float")==0) {
				fprintf(output, "%3lu: TOKEN: TYPE\n", token_count);
				ti->ts[token_count].type = t_type;
			}
			else if (strcmp(letter_string,"return")==0) {
				fprintf(output, "%3lu: TOKEN: RETURN\n", token_count);
				ti->ts[token_count].type = t_return;
			}
			else {
				fprintf(output, "%3lu: TOKEN: ID\n", token_count);
				ti->ts[token_count].type = t_id;
			}
			fprintf(output, " Line#: %lu\n Char#: %lu\n", line_number, char_number);
			if (strcmp(letter_string,"return") != 0) {
				fprintf(output, " Value: %s\n", letter_string);
			}
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = strlen(letter_string) + 1;
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strncpy(ti->ts[token_count].val, letter_string, sizeof_val);
			token_count ++;
			char_number += char_number_add;
			free(letter_string);
			continue;
		}
		// Number token
		else if (c >= 48 && c <= 57) { // [0-9]
			char is_floating_type = 0;
			int negative = 0;
			if (token_count > 2 && ti->ts[token_count - 1].type == t_binop &&
					strcmp(ti->ts[token_count - 1].val,"-") == 0 &&
					ti->ts[token_count - 2].type != t_num_int &&
					ti->ts[token_count - 2].type != t_num_float &&
					ti->ts[token_count - 2].type != t_id) {
				negative = 1;
				ti->ts = realloc(ti->ts, (token_count - 1) * sizeof(*ti->ts));
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
						number_string = realloc(number_string, sizeof(*number_string) * (string_len));
						number_string[string_len - 1] = c;
						char_number_add ++;
						c = fgetc(f);
					}
				}
				else {
					string_len ++;
					number_string = realloc(number_string, sizeof(*number_string) * (string_len));
					number_string[string_len - 1] = c;
					char_number_add ++;
					c = fgetc(f);
				}
			}
			number_string[string_len] = '\0';
			if (out >= verbose) printf("String: \"%s\"\nString length: %d\n", number_string, string_len);
			if (is_floating_type) {
				fprintf(output, "%3lu: TOKEN: NUM(FLOAT)\n Line#: %lu\n Char#: %lu\n Value: %s\n", token_count, line_number, char_number, number_string);
			}
			else {
				fprintf(output, "%3lu: TOKEN: NUM(INT)\n Line#: %lu\n Char#: %lu\n Value: %s\n", token_count, line_number, char_number, number_string);
			}
			
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = (is_floating_type) ? t_num_float : t_num_int;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = strlen(number_string) + 1;
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strncpy(ti->ts[token_count].val, number_string, sizeof_val);
			ti->ts[token_count].val[string_len] = '\0';
			token_count ++;

			free(number_string);
			char_number += char_number_add;
			continue;
		}
		else if (c == '+') {
			fprintf(output, "%3lu: TOKEN: BINOP\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("+");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "+");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '-') {
			/*if (token_count > 1) {
				enum e_token prev = ti->ts[token_count - 1].type;
				if (prev != t_num_int && prev != t_num_float && prev != t_id) {
					char_number ++;
					c = fgetc(f);	
				}
			}*/
			//else {
			fprintf(output, "%3lu: TOKEN: BINOP\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("-");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "-");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
			//}
		}
		else if (c == '*') {
			fprintf(output, "%3lu: TOKEN: BINOP\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("*");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "*");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '/') {
			fprintf(output, "%3lu: TOKEN: BINOP\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("/");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "/");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '{') {
			fprintf(output, "%3lu: TOKEN: CURL_BEG\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_curl_beg;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("{");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "{");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '}') {
			fprintf(output, "%3lu: TOKEN: CURL_END\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_curl_end;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("}");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "}");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '(') {
			fprintf(output, "%3lu: TOKEN: PAR_BEG\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_par_beg;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("(");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "(");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ')') {
			fprintf(output, "%3lu: TOKEN: PAR_END\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_par_end;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof(")");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, ")");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ':') {
			fprintf(output, "%3lu: TOKEN: COLON\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_colon;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof(":");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, ":");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '=') {
			fprintf(output, "%3lu: TOKEN: EQ\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_eq;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof("=");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, "=");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ';') {
			fprintf(output, "%3lu: TOKEN: SEMICOLON\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_semicolon;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof(";");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, ";");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ',') {
			fprintf(output, "%3lu: TOKEN: COMMA\n", token_count);
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			sum_sizeof_ts += sizeof(*ti->ts);
			ti->ts[token_count].type = t_comma;
			ti->ts[token_count].line_number = line_number;
			ti->ts[token_count].char_number = char_number;
			size_t sizeof_val = sizeof(",");
			sum_sizeof_val += sizeof_val;
			ti->ts[token_count].val = malloc(sizeof_val);
			strcpy(ti->ts[token_count].val, ",");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else {
			if (c != 10 && c != 13) fprintf(output, "UNKNOWN (%d)\n", c);
			else {
				c = fgetc(f);
				continue;
			}
		}
		fprintf(output, " Line#: %lu\n Char#: %lu\n", line_number, char_number);
		if (ti->ts[token_count - 1].type == t_binop) {
			fprintf(output, " Value: %s\n", ti->ts[token_count - 1].val);
		}
		char_number += char_number_add;
		c = fgetc(f);
	}
	if (out >= verbose) printf("Total size of tokens: %d\nTotal size of vals:%d\n", sum_sizeof_ts, sum_sizeof_val);
	return token_count;
}