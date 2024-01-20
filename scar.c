#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fail_input() {
	printf("Scar Compiler usage:\n./scar <filename>");
	return 1;
}

int match(FILE *f, char prev, unsigned int *line_no) {
	char c = fgetc(f);
	switch (c) {
		case EOF:
			return -1;
		case ' ':
			return 0;
		case '\n':
			*line_no ++;
			return -2;
		case 'a':
			return match(f, c, line_no);
		case 'o':
			return match(f, c, line_no);
		default:
			if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
				return match(f, c, line_no);
			}
			return -128; // No match
	}
}

int lex(FILE *f, FILE *output) {
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
		else if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
			int token_len = 1;
			char *token_string = malloc(sizeof(char)*2);
			token_string[0] = c;
			c = fgetc(f);
			while ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
				token_len ++;
				token_string = realloc(token_string, sizeof(char) * (token_len + 1));
				token_string[token_len - 1] = c;
				char_no_add ++;
				c = fgetc(f);
			}
			ungetc(c, f);
			token_string[token_len] = '\0';

			if (strcmp(token_string,"void")==0)	fprintf(output, "TOKEN: VOID\n");
			else if (strcmp(token_string,"main")==0) fprintf(output, "TOKEN: MAIN\n");
			else if (strcmp(token_string,"int")==0) fprintf(output, "TOKEN: INT\n");
			else if (strcmp(token_string,"return")==0) fprintf(output, "TOKEN: RETURN\n");
			else {
				fprintf(output, "VARIABLE: %s\n", token_string);
			}
			free(token_string);
			token_count ++;
		}
		else if (c >= 48 && c <= 57) {
			int number = c - 48;
			c = fgetc(f);
			while (c >= 48 && c <= 57) {
				number *= 10;
				number += c - 48;
				char_no_add ++;
				c = fgetc(f);
			}
			ungetc(c, f);
			fprintf(output, "NUM: %d\n", number);
			token_count ++;
		}
		else if (c == '(') {
			fprintf(output, "TOKEN: PAR_BEG\n");
			token_count ++;
		}
		else if (c == ')') {
			fprintf(output, "TOKEN: PAR_END\n");
			token_count ++;
		}
		else if (c == ':') {
			fprintf(output, "TOKEN: COLON\n");
			token_count ++;
		}
		else if (c == '=') {
			fprintf(output, "TOKEN: EQ\n");
			token_count ++;
		}
		else if (c == ';') {
			fprintf(output, "TOKEN: SEMICOLON\n");
			token_count ++;
		}
		else {
			if (c != 10) fprintf(output, "UNKNOWN (%d)\n", c);
		}
		fprintf(output, "%lu\n%lu\n", line_no, char_no);
		char_no += char_no_add;
		c = fgetc(f);
	}
	return token_count;
}

int parse(FILE *f, int token_count) {
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc <= 1 || argv[1] == "--help" || argv[1] == "-h" || argv[1] == "help" || argc != 2 )
	{
		return fail_input();
	}
	printf("Input: %s\n", argv[1]);

	FILE *read_ptr;
	read_ptr = fopen(argv[1], "r");
	if (read_ptr == NULL) { printf("File not found.\n"); return fail_input(); }
	FILE *write_ptr;
	write_ptr = fopen("output", "w");
	if (write_ptr == NULL) {
		printf("Could not create/write to file.\n");
		fclose(read_ptr);
		return 1;
	}
	unsigned long token_count = lex(read_ptr, write_ptr);
	printf("Read %lu tokens.\n", token_count);
	fclose(read_ptr);
	fclose(write_ptr);

	if (token_count) {
		FILE *token_file = fopen("output", "r");
		if (token_file == NULL) { printf("Token file not found.\n"); return 1; }
		parse(read_ptr, token_count);
		fclose(token_file);
	}
	return 0;
}
