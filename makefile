all:
	# compile library
	gcc -std=gnu99 -Wall -c binary_file.c bf_insn_decoder.c bf_disasm.c bf_insn.c bf_basic_blk.c bf_func.c bf_cfg.c bf_sym.c bf_mem_manager.c

	# compile target
	cd Target;make

	# compile example
	gcc -std=gnu99 -Wall -c main.c
	gcc -std=gnu99 -Wall -o Example main.o binary_file.o bf_insn_decoder.o bf_disasm.o bf_insn.o bf_basic_blk.o bf_func.o bf_cfg.o bf_sym.o bf_mem_manager.o -lbfd -lopcodes

docs:
	doxygen Doxyfile

dot:
	./Example
	dot -Tps graph.dot -o graph.pdf

clean:
	rm -f *.o
	rm -f Example
	rm -f graph.dot
	cd Target; make clean
