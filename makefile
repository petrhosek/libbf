all:
	# compile library
	gcc -Wall -c binary_file.c -lbfd

	# compile target
	cd Target;make

	# compile example
	gcc -c main.c
	gcc -Wall -o Example main.o binary_file.o -lbfd

clean:
	rm -f *.o
	rm -f Example
	cd Target; make clean
