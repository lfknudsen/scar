scar: scar.c lexer.c parser.c eval.c scar.h lexer.h parser.h eval.h
	gcc -o scar scar.c lexer.c parser.c eval.c -g
test: test.c
	gcc -o test test.c -g
debug:
	gdb scar
	break main
	run test_program
clean:
	rm scar
	rm tokens.out