all:
	# compile library
	gcc -Wall -c binary_file.c bf-disasm.c

	# compile target
	cd Target_x86-32;make

	# compile example
	gcc -c main.c
	gcc -Wall -o Example main.o binary_file.o bf-disasm.o -lbfd -lopcodes

clean:
	rm -f *.o
	rm -f Example
	cd Target_x86-32; make clean
