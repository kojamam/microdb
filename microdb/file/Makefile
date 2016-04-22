test-file: file.o test-file.o
	gcc -Wall -O2 -o test-file file.o test-file.o
	rm -f *.o
file.o: file.c
	gcc -c file.c
test-file.o: test-file.c
	gcc -c test-file.c
clean:
	rm -f test-file *.o
