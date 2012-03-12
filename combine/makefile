all:
	gcc -std=gnu99 -Wall example-target.c -o example-target
	gcc -std=gnu99 -Wall example-embedobj.c -o example-embedobj
	gcc -std=gnu99 -Wall gen_linker_command.c -o gen_linker_command -lbfd

clean:
	rm -f *.o
	rm -f example-target
	rm -f example-embedobj

