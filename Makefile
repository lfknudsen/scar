scar: scar.c lexer.c parser.c
	gcc -o scar scar.c lexer.c parser.c
test:
	./scar test_program

clean:
	rm scar
	rm tokens.out