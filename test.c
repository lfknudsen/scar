#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

// alternative:
// use diff --skip-trailing-cr --suppress-common-lines

struct test_result {
  char* name;
  int correct;
};

struct result_index {
  int n;
  struct test_result* results;
};

// Skip . and .. in the file system.
int filter(const struct dirent* a) {
    return (a->d_type == DT_REG);
}

// Execute the "cmp" command on two files with identical names in separate directories.
int compare(const size_t base_comp_len, char* name,
    const char* base_comp, const char* out_dirname, const char* out_ext, const char *exp_dirname)
{
    int success = 0;
    char *comp_cmd = malloc(base_comp_len + strlen(name) * 2 + 1);
    sprintf(comp_cmd, "%s%s%s%s %s%s%s", base_comp, out_dirname,
        name, out_ext, exp_dirname,
        name, out_ext);
    #pragma omp critical
    {
        if (!system(comp_cmd))
        {
            printf("\x1b[32mSUCCESS\x1b[m %s\n", name);
            success = 1;
        }
        else printf("\x1b[31mFAILURE\x1b[m %s\n", name);
    }
    free(comp_cmd);
    return success;
}

void run(const char* out_dirname, char* name, const char* out_ext,
        const char* prog_dirname, const char* base_exec, const char* pipe, const size_t exec_len)
{
    char* command = malloc(exec_len + strlen(name) * 2);
    sprintf(command, "%s%s%s%s%s%s%s", base_exec, prog_dirname, name, pipe, out_dirname, name, out_ext);
    #pragma omp critical
    {
        system(command);
    }
    free(command);
    return;
}

/*
    Test program for the scar interpreter.
    Runs the ./scar program from the command line for each entry in /tests/programs.
    The expected outputs are in located in /tests/exp/.
    By default, runs the tests in parallel threads (joined when performing IO, so
    there's little benefit currently), then prints alphabetically.
    Returns nothing and requires no arguments.
    Options:
    -p          Print comparison between exp/out directories without running scar first.
    -s          Force sequential operation.
    -r          Run scar on the test software, still piping into /tests/exp/, but without printing results.
    -f          Parallelise running and printing (meaning not alphabetically).
    -v          Print additional information to stdout.
*/
int main (int argc, char** argv) {
    if (system("test -e scar")) {
        if (system("make scar")) {
            printf("Could not find scar or a Makefile for it.\n");
            return 1;
        }
    }

    const char* base_exec     = "./scar -q "; //Add -q to not report anything.
    const char* pipe          = " > ";
    const char* base_comp     = "cmp -s "; //Add -s to not report anything.
    const char* prog_dirname  = "./tests/programs/";
    const char* out_dirname   = "./tests/out/";
    const char* exp_dirname   = "./tests/exp/";
    const char* out_ext       = ".out";

    if (system("test -d tests"))
        if (system("mkdir tests"))          printf("Unable to create tests directory.");
    if (system("test -d tests/out"))
        if (system("mkdir tests/out"))      printf("Unable to create tests/out directory.");
    if (system("test -d tests/exp"))
        if (system("mkdir tests/exp"))      printf("Unable to create tests/exp directory.");
    if (system("test -d tests/programs"))
        if (system("mkdir tests/programs")) printf("Unable to create tests/programs directory.");

    char skip_run   = 0;
    char sequential = 0;
    char verbosity  = 0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-v") == 0) verbosity  =   1;
        else if (strcmp(argv[i], "-p") == 0) skip_run   =   1;
        else if (strcmp(argv[i], "-r") == 0) skip_run   =  -1;
        else if (strcmp(argv[i], "-s") == 0) sequential =   1;
        else if (strcmp(argv[i], "-f") == 0) sequential =  -1;
    }

    if (verbosity == 1) {
        printf("Printing more information.\n");
        if      (skip_run   ==  0) printf("Running tests as normal.\n");
        else if (skip_run   ==  1) printf("Skipping interpreting. Just printing comparison.\n");
        else if (skip_run   == -1) printf("Skipping stdout printing. Just running tests.\n");
        if      (sequential ==  0) printf("Parallelising running but not printing.\n");
        else if (sequential ==  1) printf("Running tests in the same thread.\n");
        else if (sequential == -1) printf("Parallelising testing and printing.\n");
    }

    const size_t exec_len = strlen(base_exec) + strlen(prog_dirname) +
        strlen(pipe) + strlen(out_dirname) + strlen(out_ext) + 1;
    const size_t base_comp_len = strlen(base_comp) + strlen(out_dirname) +
        strlen(exp_dirname) + strlen(out_ext) + strlen(out_ext) + 1;

    int success_count = 0;

    struct dirent** sorted_programs;
    int n = scandir("tests/programs", &sorted_programs, filter, alphasort);
    if (skip_run != 1) {
        if (sequential == -1) {
            #pragma omp parallel for reduction(+ : success_count)
            for (int i = 0; i < n; i++) {
                run(out_dirname, sorted_programs[i]->d_name, out_ext, prog_dirname,
                    base_exec, pipe, exec_len);
                success_count += compare(base_comp_len, sorted_programs[i]->d_name,
                    base_comp, out_dirname, out_ext, exp_dirname);
                free(sorted_programs[i]);
            }
        }
        else {
            #pragma omp parallel for if (sequential == 0)
            for (int i = 0; i < n; i++) {
                run(out_dirname, sorted_programs[i]->d_name, out_ext,
                    prog_dirname, base_exec, pipe, exec_len);
            }
        }
    }
    if (sequential != -1) {
        for (int i = 0; i < n; i++) {
            success_count += compare(base_comp_len, sorted_programs[i]->d_name,
                base_comp, out_dirname, out_ext, exp_dirname);
            free(sorted_programs[i]);
        }
    }
    free(sorted_programs);
    printf("%d/%d\n", success_count, n);
    return 0;
}