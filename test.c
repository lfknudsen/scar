#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

struct test_result {
  char* name;
  int correct;
};

struct result_index {
  int n;
  struct test_result* results;
};

int main (int argc, char** argv) {
    if (system("test -d tests")) {
        system("mkdir tests");
    }
    if (system("test -d tests/out")) {
        system("mkdir tests/out");
    }
    if (system("test -d tests/exp")) {
        system("mkdir tests/exp");
    }
    if (system("test -d tests/programs")) {
        system("mkdir tests/programs");
    }
    int skip_run = 0;
    if (argc == 2 && strcmp(argv[1], "-s") == 0) {
        skip_run = 1;
        printf("Skipping interpreting, just printing previous results.\n");
    }
    char* base_exec = "./scar ";
    char* test_option = " -t > ";
    char* new_extension = ".out";
    char* base_comp = "cmp -s ";
    char* prog_dirname = "./tests/programs/";
    char* out_dirname = "./tests/out/";
    char* exp_dirname = "./tests/exp/";
    char* out_ext = ".out";
    char* exp_ext = ".exp";
    size_t base_exec_len = strlen(base_exec) + strlen(prog_dirname) + strlen(test_option) +
        strlen(out_dirname) + strlen(out_ext);
    size_t base_comp_len = strlen(base_comp) + strlen(out_dirname) + strlen(exp_dirname) +
        strlen(out_ext) + strlen(exp_ext) + 1;
    DIR* input = opendir("tests/programs");
    struct dirent** sorted_programs;
    int n = scandir("tests/programs", &sorted_programs, NULL, alphasort);
    int i = 0;
    while (i < n) {
        if (sorted_programs[i]->d_type == DT_REG) {
            char* name = sorted_programs[i]->d_name;
            if (skip_run == 0) {
                char* exec_cmd = malloc(base_exec_len + strlen(sorted_programs[i]->d_name) * 2 + 4);
                sprintf(exec_cmd, "%s %s%s %s %s%s%s", base_exec, prog_dirname, name, test_option, out_dirname, name, out_ext);
                system(exec_cmd);
            }
            char* comp_cmd = malloc(base_comp_len + strlen(name) * 2 + 1);
            sprintf(comp_cmd, "%s%s%s%s %s%s%s", base_comp, out_dirname, name, out_ext, exp_dirname, name, exp_ext);
            if (!system(comp_cmd)) {
                printf("\x1b[32mSUCCESS\x1b[m %s\n", name);
            } else {
                printf("\x1b[31mFAILURE\x1b[m %s\n", name);
            }
        }
        i++;
    }

    for (int i = 0; i < n; i++) {
        free(sorted_programs[i]);
    }
    return 0;
} 