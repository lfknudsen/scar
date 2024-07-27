# scar
An interpreter for a small programming language which I've dubbed "Scar".

## The Language
Scar is a small, statically-typed language with pass-by-value function calls.
Program execution starts in the "main" function.

## Compile
A Makefile is included. The compilation method therein uses the GNU C compiler for GNU/Linux.
To build the project, simply type 'make' into a terminal emulator.

Alternative options include 'make err' for more stringent errors, and
'make opt' for optimisation flags to be set.

## Run
To run the compiler:
    ./scar [options] <code file>

options:
    -h      Show a basic help screen.
    -q      Hide any error messages that would be printed.
    -v      Log extra debugging information to standard output while program runs.

## Tests
A test suite is included as well, as a separate program. Compile it with
'make test'.

To run the test suite:
    ./test [options]

options:
    -v      Log extra debugging information to standard output while program runs.
    -p      Do not run the test programs. Instead, simply print the comparison
            between their expected output and the last output they gave.
            Mutually exclusive with -r.
    -r      Do not perform comparisons. Instead, simply run the test programs.
            Mutually exclusive with -p.
    -s      Run the test suite single-threaded. Mutually exclusive with -f.
    -f      Run test suite and compare results in multiple threads. Mutually exclusive with -s.

By default, ./test will multi-thread running the actual test programs, but will
compare them single-threaded so as to print them alphabetically.

## Motivation
This was a personal project, the idea being that I wanted to see how I would
approach writing an interpreter (or compiler) if I just had to sit down and
go about it without text-books and course notes - as any other project.

As such, I very intentionally avoided looking things up. Next time, I think
I'll definitely be a lot more structured in my approach, constructing a
DFA and minimising it, and so on.

The casual nature of the project also means I didn't give the language
too much thought. I don't imagine I'll develop this interpreter beyond turing
completeness.

