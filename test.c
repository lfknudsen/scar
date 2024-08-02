#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

// alternative:
// use diff --skip-trailing-cr --suppress-common-lines

// Skip . and .. in the file system.
int filter(const struct dirent* a) {
    return (a->d_type == DT_REG);
}

// Execute the "cmp" command on two files with identical names in separate directories.
int compare(const size_t base_comp_len, char* name, const char* base_comp,
    const char* out_dirname, const char* out_ext, const char *exp_dirname)
{
    int success = 0;
    char* comp_cmd = malloc(base_comp_len + strlen(name) * 2 + 1);
    sprintf(comp_cmd, "%s%s%s%s %s%s%s", base_comp, out_dirname,
        name, out_ext, exp_dirname,
        name, out_ext);
    #pragma omp critical
    {
        if (!system(comp_cmd)) {
            printf("\x1b[32mSUCCESS\x1b[m %s\n", name);
            success = 1;
        }
        else {
            char* missing_cmd = malloc(strlen("test -e ") + strlen(exp_dirname) +
                strlen(name) + strlen(out_ext) + 1);
            sprintf(missing_cmd, "test -e %s%s%s", exp_dirname, name, out_ext);
            if (system(missing_cmd))
                printf("\x1b[33mMISSING\x1b[m %s\n", name);
            else
                printf("\x1b[31mFAILURE\x1b[m %s\n", name);
            free(missing_cmd);
        }
    }
    free(comp_cmd);
    return success;
}

int run(const char* out_dirname, char* name, const char* out_ext,
        const char* prog_dirname, const char* base_exec, const char* pipe, const size_t exec_len,
        char check_leak)
{
    char memory_safe = 1;
    char* command;
    if (check_leak) {
        command = malloc(strlen("valgrind -q ") + exec_len + strlen(name) * 2);
        sprintf(command, "valgrind -q %s%s%s%s%s%s%s", base_exec, prog_dirname, name, pipe, out_dirname, name, out_ext);
        #pragma omp critical
        {
            if (system(command)) {
                memory_safe = 0;
            }
        }
    }
    else {
        command = malloc(exec_len + strlen(name) * 2);
        sprintf(command, "%s%s%s%s%s%s%s", base_exec, prog_dirname, name, pipe, out_dirname, name, out_ext);
        #pragma omp critical
        {
            system(command);
        }
    }
    free(command);
    return memory_safe;
}

int single_test(char* name, const char* prog_dirname, const char* out_dirname,
    const char* exp_dirname, const char* out_ext, const char* base_exec, const char* pipe,
    size_t exec_len, char check_leak)
{
    char test_target_exists = 0;
    char included_prog_dir = 0;

    char* test_cmd = malloc(strlen("test -e ") + strlen(prog_dirname) + strlen(name) + 1);
    sprintf(test_cmd, "test -e %s%s", prog_dirname, name);
    if (system(test_cmd)) {
        test_cmd = malloc(strlen("test -e ") + strlen(name) + 1);
        sprintf(test_cmd, "test -e %s", name);
        if (!system(test_cmd)) {
            included_prog_dir = 1;
            test_target_exists = 1;
        }
        else {
            printf("Could not locate %s. Exiting.\n", name);
            return 1;
        }
    }
    else {
        test_target_exists = 1;
    }

    if (test_target_exists) {
        printf("Testing program \"%s\":\n", name);
        if (included_prog_dir) {
            test_cmd = malloc(strlen("cat ") + strlen(name) + 1);
            sprintf(test_cmd, "cat %s", name);
            system(test_cmd);
        }
        else {
            test_cmd = malloc(strlen("cat ") + strlen(prog_dirname) + strlen(name) + 1);
            sprintf(test_cmd, "cat %s%s", prog_dirname, name);
            system(test_cmd);
        }
        printf("\n");

        run(out_dirname, name, out_ext, prog_dirname, base_exec, pipe, exec_len, check_leak);

        test_cmd = malloc(strlen("cat ") + strlen(out_dirname) +
            strlen(name) + strlen(out_ext) + 1);
        sprintf(test_cmd, "cat %s%s%s", out_dirname, name, out_ext);

        printf("\n----------------OUTPUT----------------\n");
        system(test_cmd);
        printf("--------------------------------------\n");

        test_cmd = malloc(strlen("test -e ") + strlen(exp_dirname) +
            strlen(name) + strlen(out_ext) + 1);
        sprintf(test_cmd, "test -e %s%s%s", exp_dirname, name, out_ext);

        if (!system(test_cmd)) {
            test_cmd = malloc(strlen("cat ") + strlen(exp_dirname) +
                strlen(name) + strlen(out_ext) + 1);
            sprintf(test_cmd, "cat %s%s%s", exp_dirname, name, out_ext);
            printf("\n\n---------------EXPECTED---------------\n");
            system(test_cmd);
            printf("--------------------------------------\n");

            test_cmd = malloc(strlen("cmp -s ") + strlen(exp_dirname) + strlen(name) * 2 + strlen(out_dirname) + strlen(out_ext) * 2 + 1);
            sprintf(test_cmd, "cmp -s %s%s%s%s%s%s", exp_dirname, name, out_ext, out_dirname, name, out_ext);
            if (!system(test_cmd)) {
                printf("Create expectation file from output? [y/N]\n");
                char input = getchar();
                if (input == 'y') {
                    test_cmd = malloc(strlen("cp ") + strlen(out_dirname) +
                        strlen(name) + strlen(out_ext) + 1 + strlen(exp_dirname) +
                        strlen(name) + strlen(out_ext) + 1);
                    sprintf(test_cmd, "cp %s%s%s %s%s%s", out_dirname, name, out_ext,
                        exp_dirname, name, out_ext);
                    if (!system(test_cmd))
                        printf("Created expectation file.\n");
                    else
                        printf("Could not create expectation file.\n");
                }
            }
        }
        else {
            printf("\n\n-----------\x1b[33mMISSING EXPECTED\x1b[m-----------\n");
            printf("Create expectation file from output? [y/N]\n");
            char input = getchar();
            if (input == 'y') {
                test_cmd = malloc(strlen("cp ") + strlen(out_dirname) +
                    strlen(name) + strlen(out_ext) + 1 + strlen(exp_dirname) +
                    strlen(name) + strlen(out_ext) + 1);
                sprintf(test_cmd, "cp %s%s%s %s%s%s", out_dirname, name, out_ext,
                    exp_dirname, name, out_ext);
                if (!system(test_cmd))
                    printf("Created expectation file.\n");
                else
                    printf("Could not create expectation file.\n");
            }
        }
    }
    free(test_cmd);
    return 0;
}

int test_missing(struct dirent** progs, int n, const char* prog_dirname,
        const char* out_dirname, const char* exp_dirname, const char* out_ext,
        const char* base_exec, const char* pipe, size_t exec_len, char check_leak) {
    for (int i = 0; i < n; i++) {
        char test_cmd
            [strlen("test -e ") + strlen(exp_dirname) + strlen(progs[i]->d_name) + strlen(out_ext) + 1];
        sprintf(test_cmd, "test -e %s%s%s", exp_dirname, progs[i]->d_name, out_ext);
        if (system(test_cmd)) {
            single_test(progs[i]->d_name, prog_dirname, out_dirname,
                exp_dirname, out_ext, base_exec, pipe, exec_len, check_leak);
            getchar();
        }
    }
    return 0;
}

// Creates necessary directories for test suite.
// Returns 0 if any directories did not exist already, so the main function
// knows to stop the program immediately.
// Otherwise, returns 1.
int check_directories() {
    int can_run = 1;
    if (system("test -d tests")) {
        can_run = 0;
        printf("No tests directory found. Creating directories.\n");
        printf("Place program files in /tests/programs/ and their expected output in /tests/out.\n");
        if (system("mkdir tests"))
            printf("Unable to create tests directory. Check permissions.\n");
    }
    if (system("test -d tests/out")) {
        can_run = 0;
        if (system("mkdir tests/out"))      printf("Unable to create tests/out directory.\n");
    }
    if (system("test -d tests/exp")) {
        can_run = 0;
        if (system("mkdir tests/exp"))      printf("Unable to create tests/exp directory.\n");
    }
    if (system("test -d tests/programs")) {
        can_run = 0;
        if (system("mkdir tests/programs")) printf("Unable to create tests/programs directory.\n");
    }
    return can_run;
}

/*
    Test program for the scar interpreter.
    Runs the ./scar program from the command line for each entry in /tests/programs.
    The expected outputs are in located in /tests/exp/.
    By default, runs the tests in parallel threads (joined when performing IO, so
    there's little benefit currently), then prints alphabetically.
    Returns nothing and requires no arguments.
    Options:
    -v          Print additional information to stdout.
    -p          Print comparison between exp/out directories without running scar first.
    -r          Run scar on the test software, still piping into /tests/exp/, but without printing results.
    -s          Force sequential operation.
    -f          Parallelise running and printing (meaning not alphabetically).
    -m          Individually test each program that doesn't have an output file in /tests/exp/.
    -l          Check for memory leaks with valgrind -q. Takes longer, which is
                especially noticeable without the -f option to print each result immediately.

    <filename>  Individually test the given file in /tests/programs/ (do not include the full path).
                This will print the program, then show the output and expected output next to each
                other. If there is no expected output yet, it will ask the user if they wish to
                make a duplicate of the output and put it into /tests/exp/.
*/
int main (int argc, char** argv) {
    if (system("test -x scar")) {
        if (system("make scar")) {
            printf("Either could not find scar or a Makefile for it, or do have execution privileges.\n");
            return 1;
        }
    }

    if (!check_directories()) return 1;

    const char* base_exec     = "./scar -q "; //Add -q to not report anything.
    const char* pipe          = " > ";
    const char* base_comp     = "cmp -s "; //Add -s to not report anything.
    const char* prog_dirname  = "./tests/programs/";
    const char* out_dirname   = "./tests/out/";
    const char* exp_dirname   = "./tests/exp/";
    const char* out_ext       = ".out";

    const size_t exec_len = strlen(base_exec) + strlen(prog_dirname) +
        strlen(pipe) + strlen(out_dirname) + strlen(out_ext) + 1;
    const size_t base_comp_len = strlen(base_comp) + strlen(out_dirname) +
        strlen(exp_dirname) + strlen(out_ext) + strlen(out_ext) + 1;

    char skip_run       = 0;
    char sequential     = 0;
    char verbosity      = 0;
    char check_missing  = 0;
    char check_leak     = 0;
    int  target         = 0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-v") == 0) verbosity      =  1;
        else if (strcmp(argv[i], "-p") == 0) skip_run       =  1;
        else if (strcmp(argv[i], "-r") == 0) skip_run       = -1;
        else if (strcmp(argv[i], "-s") == 0) sequential     =  1;
        else if (strcmp(argv[i], "-f") == 0) sequential     = -1;
        else if (strcmp(argv[i], "-m") == 0) check_missing  =  1;
        else if (strcmp(argv[i], "-l") == 0) check_leak     =  1;
        else                                 target         =  i;
    }

    if (check_leak && !system("test -x valgrind")) {
        printf("Valgrind not found on PATH. Running without.\n");
        check_leak = 0;
    }

    if (check_missing) target = 0;

    if (verbosity == 1) {
        printf("Printing more information.\n");
        if      (skip_run   ==  0) printf("Running tests as normal.\n");
        else if (skip_run   ==  1) printf("Skipping interpreting. Just printing comparison.\n");
        else if (skip_run   == -1) printf("Skipping stdout printing. Just running tests.\n");
        if      (sequential ==  0) printf("Parallelising running but not printing.\n");
        else if (sequential ==  1) printf("Running tests in the same thread.\n");
        else if (sequential == -1) printf("Parallelising testing and printing.\n");
        if      (check_leak)       printf("Checking memory leakage with Valgrind.\n");
        if      (check_missing)    printf("Individually testing all programs with missing expected outputs.\n");
        else if (target      >  0) printf("Testing only %s\n", argv[target]);
    }

    if (target)
        return single_test(argv[target], prog_dirname, out_dirname, exp_dirname,
            out_ext, base_exec, pipe, exec_len, check_leak);

    int success_count = 0;

    struct dirent** sorted_programs;
    int n = scandir("tests/programs", &sorted_programs, filter, alphasort);

    if (check_missing) return test_missing(sorted_programs, n, prog_dirname, out_dirname,
        exp_dirname, out_ext, base_exec, pipe, exec_len, check_leak);

    volatile int memory_leak = 0;
    if (skip_run != 1) {
        if (sequential == -1) {
            #pragma omp parallel for reduction(+ : success_count)
            for (int i = 0; i < n; i++) {
                if (memory_leak == 0) {
                    int memory_safe = run(out_dirname, sorted_programs[i]->d_name, out_ext, prog_dirname,
                        base_exec, pipe, exec_len, check_leak);
                    success_count += compare(base_comp_len, sorted_programs[i]->d_name,
                        base_comp, out_dirname, out_ext, exp_dirname);
                    free(sorted_programs[i]);
                    if (memory_safe == 0)
                        memory_leak = 1;
                }
                else {
                    continue;
                }
            }
        }
        else {
            #pragma omp parallel for if (sequential == 0)
            for (int i = 0; i < n; i++) {
                if (memory_leak == 0) {
                    int memory_safe = run(out_dirname, sorted_programs[i]->d_name, out_ext,
                        prog_dirname, base_exec, pipe, exec_len, check_leak);
                    if (memory_safe == 0)
                        memory_leak = 1;
                }
                else
                    continue;
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