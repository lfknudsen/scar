#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#include <assert.h>

int lex(FILE *f, FILE *output, struct token_index *ti) {
	//struct Token *ts = &ti->ts;
	int ts_bytes = 0;
	int val_bytes = 0;

	unsigned long line_no = 1;
	unsigned long char_no = 1;
	unsigned long token_count = 0;
	char c = fgetc(f);
	while (c != EOF && c != '\0') {
		unsigned int char_no_add = 1;
		if (c == '\n') {
			line_no ++;
			char_no = 1;
			c = fgetc(f);
			continue;
		}
		if (c == ' ') {
			char_no ++;
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
				char_no_add ++;
				c = fgetc(f);
			}
			letter_string[string_len] = '\0';

			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			if (strcmp(letter_string,"void")==0)	{
				fprintf(output, "TOKEN: TYPE\n");
				ti->ts[token_count].type = t_type;
			}
			else if (strcmp(letter_string,"int")==0) {
				fprintf(output, "TOKEN: TYPE\n");
				ti->ts[token_count].type = t_type;
			}
			else if (strcmp(letter_string,"float")==0) {
				fprintf(output, "TOKEN: TYPE\n");
				ti->ts[token_count].type = t_type;
			}
			else if (strcmp(letter_string,"return")==0) {
				fprintf(output, "TOKEN: RETURN\n");
				ti->ts[token_count].type = t_return;
			}
			else {
				fprintf(output, "ID\n");
				ti->ts[token_count].type = t_id;
			}
			fprintf(output, "%lu\n%lu\n", line_no, char_no);
			if (strcmp(letter_string,"return") != 0) {
				fprintf(output, "%s\n", letter_string);
			}
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * (string_len));
			val_bytes += sizeof(*ti->ts[token_count].val) * (string_len);
			strcpy(ti->ts[token_count].val, letter_string);
			token_count ++;
			char_no += char_no_add;
			free(letter_string);
			continue;
		}
		else if (c >= 48 && c <= 57) { // [0-9]
			char is_floating_type = 0;
			char *number_string = malloc(sizeof(char) * 2);
			int string_len = 1;
			number_string[0] = c;
			c = fgetc(f);
			while (is_floating_type <= 1 && ((c >= 48 && c <= 57) || (c == 46))) { // [0-9.]
				if (c == 46) {
					if (is_floating_type) {
						is_floating_type ++;
						char_no_add ++;
						printf("Lexer error. Extra '.' in floating-point number");
						printf(" at %lu:%lu\n", line_no, char_no + char_no_add);
					} else {
						is_floating_type ++;
						string_len ++;
						number_string = realloc(number_string, sizeof(*number_string) * (string_len));
						number_string[string_len - 1] = c;
						char_no_add ++;
						c = fgetc(f);
					}
				}
				else {
					string_len ++;
					number_string = realloc(number_string, sizeof(*number_string) * (string_len));
					number_string[string_len - 1] = c;
					char_no_add ++;
					c = fgetc(f);
				}
			}
			printf("String: %s\nString length: %d\n", number_string, string_len);
			if (is_floating_type) {
				fprintf(output, "NUM: FLOAT\n%lu\n%lu\n%s\n", line_no, char_no, number_string);
			}
			else {
				fprintf(output, "NUM: INT\n%lu\n%lu\n%s\n", line_no, char_no, number_string);
			}
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = (is_floating_type) ? t_num_float : t_num_int;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * (string_len));
			val_bytes += sizeof(*ti->ts[token_count].val) * (string_len);
			strcpy(ti->ts[token_count].val, number_string);
			ti->ts[token_count].val[string_len] = '\0';
			token_count ++;

			free(number_string);
			char_no += char_no_add;
			continue;
		}
		else if (c == '+') {
			fprintf(output, "BINOP\n+\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "+");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '-') {
			fprintf(output, "BINOP\n-\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "-");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '*') {
			fprintf(output, "BINOP\n*\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			printf("%s\n", strcpy(ti->ts[token_count].val, "*"));
			//ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '/') {
			fprintf(output, "BINOP\n/\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_binop;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "/");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '{') {
			fprintf(output, "TOKEN: CURL_BEG\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_curl_beg;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "{");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '}') {
			fprintf(output, "TOKEN: CURL_END\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_curl_end;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "}");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '(') {
			fprintf(output, "TOKEN: PAR_BEG\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_par_beg;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "(");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ')') {
			fprintf(output, "TOKEN: PAR_END\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_par_end;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, ")");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ':') {
			fprintf(output, "TOKEN: COLON\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_colon;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, ":");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == '=') {
			fprintf(output, "TOKEN: EQ\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_eq;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, "=");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ';') {
			fprintf(output, "TOKEN: SEMICOLON\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_semicolon;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
			strcpy(ti->ts[token_count].val, ";");
			ti->ts[token_count].val[1] = '\0';
			token_count ++;
		}
		else if (c == ',') {
			fprintf(output, "TOKEN: COMMA\n");
			ti->ts = realloc(ti->ts, sizeof(*ti->ts) * (token_count + 1));
			ts_bytes += sizeof(*ti->ts);
			ti->ts[token_count].type = t_comma;
			ti->ts[token_count].line_no = line_no;
			ti->ts[token_count].char_no = char_no;
			ti->ts[token_count].val = malloc(sizeof(*ti->ts[token_count].val) * 2);
			val_bytes += 2;
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
		fprintf(output, "%lu\n%lu\n", line_no, char_no);
		char_no += char_no_add;
		c = fgetc(f);
	}
	printf("%d\n%d\n", ts_bytes, val_bytes);
	return token_count;
}