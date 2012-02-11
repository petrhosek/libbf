all:
	# compile library
	gcc -std=gnu99 -Wall -c binary_file.c bf_insn_decoder.c bf_disasm.c bf_insn.c bf_basic_blk.c bf_cfg.c

	# compile target
	cd Target;make

	# compile example
	gcc -std=gnu99 -Wall -c main.c
	gcc -std=gnu99 -Wall -o Example main.o binary_file.o bf_insn_decoder.o bf_disasm.o bf_insn.o bf_basic_blk.o bf_cfg.o -lbfd -lopcodes

clean:
	rm -f *.o
	rm -f Example
	cd Target; make clean
