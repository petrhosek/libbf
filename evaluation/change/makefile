all:
	gcc -std=gnu99 -Wall change.c -o change -lbind -lkern
	gcc -std=gnu99 -Wall target1.c -o target1
	gcc -std=gnu99 -Wall target2.c -o target2
	gcc -std=gnu99 -Wall coreutils-change.c logger.c func_analysis.c -o coreutils-change -lbind -lkern -lbfd

clean:
	rm -f change
	rm -f target1
	rm -f target2
	rm -f coreutils-change
