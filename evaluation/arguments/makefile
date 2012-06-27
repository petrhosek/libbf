all:
	gcc -std=gnu99 -Wall arguments.c -o arguments -lbf
	gcc -std=gnu99 -Wall -m32 target.c -o target32
	gcc -std=gnu99 -Wall -m64 target.c -o target64

clean:
	rm -f *.o
	rm -f target32
	rm -f target64
	rm -f arguments

	# removing possible output files
	rm -f target32-output
	rm -f target64-output
