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

// print one character per bit.
void to_binary(char number) {
    char* result = malloc(sizeof(number) * 8); //we're not printing as string, so no need for \0.
    unsigned char c = 0;
    for (c = 0; c < sizeof(number) * 8; c ++) {
        unsigned char num = number >> (sizeof(number) * 8 - c);
        result[c] = num & 1;
    }

    for (c = 0; c < sizeof(number) * 8; c++) {
        printf("%u", result[c]);
    }
    return;
}

char is_v(char options) {
    return options & 1;
}

char is_p(char options) {
    return options >> (sizeof(options) * 8 - 1) && 1;
}

char is_r(char options) {
    return options >> (sizeof(options) * 8 - 2) && 1;
}

char is_s(char options) {
    return options >> (sizeof(options) * 8 - 3) && 1;
}

char is_f(char options) {
    return options >> (sizeof(options) * 8 - 4) && 1;
}

char is_m(char options) {
    return options >> (sizeof(options) * 8 - 5) && 1;
}

char is_l(char options) {
    return options >> (sizeof(options) * 8 - 6) && 1;
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

    /*
    char skip_run       = 0;
    char sequential     = 0;
    char verbosity      = 0;
    char check_missing  = 0;
    char check_leak     = 0;
    */
    int  target         = 0;

    unsigned char options = 0;
    /*
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
    */
    if (argc > 1) {
        int i = 1;
        do {
            if (argv[i][0] == '-') {
                switch (argv[i][1]) {
                    case 'v':
                        options = options | (unsigned char)   1;
                        break;
                    case 'p':
                        options = options | (unsigned char)   2;
                        break;
                    case 'r':
                        options = options | (unsigned char)   4;
                        break;
                    case 's':
                        options = options | (unsigned char)   8;
                        break;
                    case 'f':
                        options = options | (unsigned char)   16;
                        break;
                    case 'm':
                        options = options | (unsigned char)   32;
                        break;
                    case 'l':
                        options = options | (unsigned char)   64;
                        break;
                }
            }
            else
                target = i;
            i ++;
        } while (i < argc);
    }
        /*
        if      (strcmp(argv[i], "-v") == 0) options = options | (unsigned char)   1;
        else if (strcmp(argv[i], "-p") == 0) options = options | (unsigned char)   2;
        else if (strcmp(argv[i], "-r") == 0) options = options | (unsigned char)   4;
        else if (strcmp(argv[i], "-s") == 0) options = options | (unsigned char)   8;
        else if (strcmp(argv[i], "-f") == 0) options = options | (unsigned char)  16;
        else if (strcmp(argv[i], "-m") == 0) options = options | (unsigned char)  32;
        else if (strcmp(argv[i], "-l") == 0) options = options | (unsigned char) 128;
        else                                 target            =   i;
        */


    // -v -l -p => 1000 0011
    /*
    printf("Options: ");
    to_binary(options);
    printf("\n");
    return 0;
    */

    if (is_l(options) && !system("test -x valgrind")) {
        printf("Valgrind not found on PATH. Running without.\n");
        options = options | (unsigned char) 128;
    }

    if (is_m(options)) target = 0;

    if (is_v(options) == 1) {
        printf("Printing more information.\n");
        if      (is_p(options) && !is_r(options)) printf("Skipping interpreting. Just printing comparison.\n");
        else if (!is_p(options) && is_r(options)) printf("Skipping stdout printing. Just running tests.\n");
        else                                      printf("Running tests as normal.\n");
        if      (is_s(options) && !is_f(options)) printf("Running tests in the same thread.\n");
        else if (!is_s(options) && is_f(options)) printf("Parallelising testing and printing.\n");
        else                                      printf("Parallelising running but not printing.\n");
        if      (is_l(options))                   printf("Checking memory leakage with Valgrind.\n");
        if      (is_m(options))                   printf("Individually testing all programs with missing expected outputs.\n");
        else if (target   >  0)                   printf("Testing only %s\n", argv[target]);
    }

    /*
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
    */

    if (target)
        return single_test(argv[target], prog_dirname, out_dirname, exp_dirname,
            out_ext, base_exec, pipe, exec_len, is_l(options));

    int success_count = 0;

    struct dirent** sorted_programs;
    int n = scandir("tests/programs", &sorted_programs, filter, alphasort);

    if (is_m(options)) return test_missing(sorted_programs, n, prog_dirname, out_dirname,
        exp_dirname, out_ext, base_exec, pipe, exec_len, is_l(options));

    volatile int memory_leak = 0;
    if (!is_p(options)) {
        if (is_f(options)) {
            #pragma omp parallel for reduction(+ : success_count)
            for (int i = 0; i < n; i++) {
                if (memory_leak == 0) {
                    int memory_safe = run(out_dirname, sorted_programs[i]->d_name, out_ext, prog_dirname,
                        base_exec, pipe, exec_len, is_l(options));
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
            #pragma omp parallel for if (!is_s(options) && !is_f(options))
            for (int i = 0; i < n; i++) {
                if (memory_leak == 0) {
                    int memory_safe = run(out_dirname, sorted_programs[i]->d_name, out_ext,
                        prog_dirname, base_exec, pipe, exec_len, is_l(options));
                    if (memory_safe == 0)
                        memory_leak = 1;
                }
                else
                    continue;
            }
        }
    }
    if (!is_f(options)) {
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