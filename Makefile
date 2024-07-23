scar: scar.c lexer.c parser.c eval.c scar.h lexer.h parser.h eval.h
	gcc -o scar scar.c lexer.c parser.c eval.c -g -Og
opt: scar.c lexer.c parser.c eval.c scar.h lexer.h parser.h eval.h
	gcc -o scar scar.c lexer.c parser.c eval.c -O3
mix: scar.c lexer.c parser.c eval.c scar.h lexer.h parser.h eval.h
	gcc -o scar scar.c lexer.c parser.c eval.c -g -O3
err: scar.c lexer.c parser.c eval.c scar.h lexer.h parser.h eval.h
	gcc -o scar scar.c lexer.c parser.c eval.c -g -O3 -Wall -Wpedantic
test: test.c
	gcc -o test test.c -g -O3 -fopenmp
testdb: test.c
	gcc -o test test.c -g -Og -fopenmp
testerr: test.c
	gcc -o test test.c -g -Og -fopenmp -Wall -Wpedantic
both:
	make scar
	make test
debug:
	gdb scar
	break main
	run test_program
clean:
	rm scar
	rm test
	rm tokens.out
	rm nodes.out
	rm eval.out