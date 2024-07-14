scar: scar.c lexer.c parser.c eval.c
	gcc -o scar scar.c lexer.c parser.c eval.c -g
test:
	./scar test_program
debug:
	gdb scar
	break main
	run test_program
clean:
	rm scar
	rm tokens.out