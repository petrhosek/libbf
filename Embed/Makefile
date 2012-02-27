all:
	gcc -std=gnu99 -Wall Hello.c -o Hello
	gcc -c Embed.c
	gcc Combine.c -o Combine -lbfd

clean:
	rm -f Combine
	rm -f Hello
	rm *.o
