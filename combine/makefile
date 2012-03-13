all:
	gcc -std=gnu99 -Wall example-target.c -o example-target
	gcc -std=gnu99 -Wall example-embedobj.c -o example-embedobj
	gcc -std=gnu99 -Wall gen_linker_command.c -o gen_linker_command -lbfd
	gcc -std=gnu99 -Wall inject.c -o inject -lbfd
	gcc -std=gnu99 -g -Wall inject-simple.c -o inject-simple -lbfd

combine:
	# Add linker command to the next command: (-Wall -Wl,...)
	gcc -std=gnu99 -Wall example-embedobj.c -o example-embedobj
	objcopy --prefix-sections=.new example-embedobj

	./inject example-target example-embedobj example-combined
	# ./inject-simple example-target example-embedobj example-combined
	objdump -Dslx example-target > example-target.txt
	objdump -Dslx example-combined > example-combined.txt

clean:
	rm -f *.o
	rm -f example-target
	rm -f example-embedobj
	rm -f gen_linker_command
	rm -f inject
