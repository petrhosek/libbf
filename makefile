SHELL := /bin/bash

all:
	# compile library
	make lib

	# compile target
	cd Target;make

	# compile example
	gcc -std=gnu99 -Wall -c main.c
	gcc -std=gnu99 -Wall -o Example main.o binary_file.o bf_insn_decoder.o bf_disasm.o bf_insn.o bf_basic_blk.o bf_func.o bf_cfg.o bf_sym.o bf_mem_manager.o -lbfd -lopcodes

lib:
	gcc -std=gnu99 -Wall -c binary_file.c bf_insn_decoder.c bf_disasm.c bf_insn.c bf_basic_blk.c bf_func.c bf_cfg.c bf_sym.c bf_mem_manager.c

tests:
	#compile library
	make lib

	#compile coreutils-cfg
	gcc -std=gnu99 -Wall -c coreutils-cfg.c
	gcc -std=gnu99 -Wall -o coreutils-cfg-tests coreutils-cfg.o binary_file.o bf_insn_decoder.o bf_disasm.o bf_insn.o bf_basic_blk.o bf_func.o bf_cfg.o bf_sym.o bf_mem_manager.o -lbfd -lopcodes

get-coreutils:
	#get package
	wget http://ftp.gnu.org/gnu/coreutils/coreutils-8.15.tar.xz
	tar -x --xz -f coreutils-8.15.tar.xz
	rm coreutils-8.15.tar.xz

	#make binaries and extract them to a folder
	mkdir coreutils
	cd coreutils-8.15; ./configure; make; find src -type f -perm /a+x -exec cp {} ../coreutils \;
	rm -rf coreutils-8.15
	rm coreutils/*.pl
	rm coreutils/*.so
	rm coreutils/dcgen

run-tests:
	rm -rf tests
	mkdir tests
	./coreutils-cfg-tests
	# for i in tests/*.dot ; do dot -Tpdf $i -o "tests/"$(basename $i dot)pdf ; done

docs:
	doxygen Doxyfile

dot:
	./Example
	dot -Tpdf graph.dot -o graph.pdf

clean:
	rm -f *.o
	rm -f Example
	rm -rf tests
	rm -rf coreutils
	rm -f coreutils-cfg-tests
	rm -f graph.dot
	cd Target; make clean
