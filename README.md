# scar
Development repository of the interpreter for the Scar programming language.

This was a personal project, the idea being that I wanted to see how I would
approach writing an interpreter (or compiler) if I just had to sit down and
go about it without text-books and course notes - as any other project.

As such, I very intentionally avoided looking things up. Next time, I think
I'll definitely be a lot more structured in my approach, constructing a
DFA and minimising it, and so on.

The casual nature of the project also means I didn't give the language
too much thought. I don't imagine I'll develop this interpreter beyond turing
completeness.

Scar is a small, statically-typed language with pass-by-value function calls.
Program execution starts in the "main" function.