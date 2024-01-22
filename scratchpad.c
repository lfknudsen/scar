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

int parse_state(int state, FILE *f) {
	if (state == 0) return 1;
	if (state == 1) {
		char c = fgetc(f);
		if (c == EOF) {
			return END_STATE;
		}
		ungetc(c, f);
		char *token_line = malloc(20);
		char *line_no = malloc(20);
		char *char_no = malloc(20);
		char *val = malloc(20);
		fgets(token_line, 20, f);
		fgets(line_no, 20, f);
		fgets(char_no, 20, f);
		if (strcmp(token_line, "TOKEN: TYPE") != 0) {
			fprintf(stdout, "Parse error. Expected function type at %lu:%lu.\n", line_no, char_no);
			free(token_line);
			free(line_no);
			free(char_no);
			return -1;
		}
		fgets(val, 20, f);

		fgets(token_line, 20, f);
		fgets(line_no, 20, f);
		fgets(char_no, 20, f);
		if (strcmp(token_line, "ID") != 0) {
			fprintf(stdout, "Parse error. Expected function name at %lu:%lu.\n", line_no, char_no);
			free(token_line);
			free(line_no);
			free(char_no);
			return -1;
		}
		fgets(val, 20, f);

		fgets(token_line, 20, f);
		fgets(line_no, 20, f);
		fgets(char_no, 20, f);
		if (strcmp(token_line, "TOKEN: PAR_BEG") != 0) {
			fprintf(stdout, "Parse error. Expected '(' after function name at %lu:%lu.\n", line_no, char_no);
			free(token_line);
			free(line_no);
			free(char_no);
			return -1;
		}

		free(token_line);
		free(line_no);
		free(char_no);
		free(val);
		return 2;
	}
	if (state == 2) {
		char *token_line = malloc(20);
		char *line_no = malloc(20);
		char *char_no = malloc(20);
		char *val = malloc(20);
		fgets(token_line, 20, f);
		fgets(line_no, 20, f);
		fgets(char_no, 20, f);
		if (strcmp(token_line, "TOKEN: PAR_END") == 0) {
			fgets(token_line, 20, f);
			fgets(line_no, 20, f);
			fgets(char_no, 20, f);
			if (strcmp(token_line, "TOKEN: EQ") == 0) {
				free(token_line);
				free(line_no);
				free(char_no);
				free(val);
				return 4;
			}
			fprintf(stdout, "Parse error. Expected '=' at %lu:%lu.\n", line_no, char_no);
			free(token_line);
			free(line_no);
			free(char_no);
			free(val);
			return -1;
		}
		if (strcmp(token_line, "TOKEN: TYPE") == 0) {
			fgets(val, 20, f);

			fgets(token_line, 20, f);
			fgets(line_no, 20, f);
			fgets(char_no, 20, f);
			if (strcmp(token_line, "ID") == 0) {
				fgets(val, 20, f);

				free(token_line);
				free(line_no);
				free(char_no);
				free(val);
				return 3;
			}
		}
		free(token_line);
		free(line_no);
		free(char_no);
		free(val);
		return -1;
	}
	if (state == 3) {
		char *token_line = malloc(20);
		char *line_no = malloc(20);
		char *char_no = malloc(20);

		fgets(token_line, 20, f);
		fgets(line_no, 20, f);
		fgets(char_no, 20, f);
		if (strcmp(token_line, "TOKEN: COMMA") == 0) {
			free(token_line);
			free(line_no);
			free(char_no);
			return 2;
		}
		if (strcmp(token_line, "TOKEN: PAR_END") != 0) {
			fprintf(stdout, "Parse error. Expected ')' to close function parameters at %lu:%lu.\n", line_no, char_no);
			free(token_line);
			free(line_no);
			free(char_no);
			return -1;
		}

		fgets(token_line, 20, f);
		fgets(line_no, 20, f);
		fgets(char_no, 20, f);
		if (strcmp(token_line, "TOKEN: EQ") != 0) {
			fprintf(stdout, "Parse error. Expected '=' to precede function body declaration at %lu:%lu.\n", line_no, char_no);
			free(token_line);
			free(line_no);
			free(char_no);
			return -1;
		}

		free(token_line);
		free(line_no);
		free(char_no);
		return 4;
	}
	if (state == 4) {

	}
	return 0;
}