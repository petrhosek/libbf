all:
	# compile library
	c99 -Wall -c binary_file.c bf-disasm.c bf_insn.c

	# compile target
	cd Target;make

	# compile example
	c99 -c main.c
	c99 -Wall -o Example main.o binary_file.o bf-disasm.o bf_insn.c -lbfd -lopcodes

clean:
	rm -f *.o
	rm -f Example
	cd Target; make clean
