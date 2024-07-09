#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  printf("Entered main().\n");
  if (argc != 3) {
    printf("Wrong number of arguments.\n");
  }
  FILE *code_file = fopen(argv[1], "r");
  if (code_file == NULL) {
    printf("Could not open %s.\n", argv[1]);
    return 1;
  }
  FILE *token_file = fopen(argv[2], "r");
  if (token_file == NULL) {
    printf("Could not open %s.\n", argv[2]);
    fclose(code_file);
    return 1;
  }
  char c = fgetc(token_file);
  while (c != EOF) {
    int token_len = 0;
    char *token = malloc(0);
    while (c != '\n' && c != '\0' && c != 10 && c != EOF) {
      token_len ++;
      token = realloc(token, token_len + 1);
      token[token_len - 1] = c;
      c = fgetc(token_file);
    }
    if (token_len > 0) token[token_len] = '\0';
    ungetc(c, token_file);
    printf("%s\n", token);
    free(token);
  }

  fclose(code_file);
  fclose(token_file);
  return 0;
}