all:
	gcc -std=gnu99 -Wall Hello.c -o Hello
	gcc -c Embed.c
	gcc -std=gnu99 -Wall Combine.c -o Combine -lbfd

clean:
	rm -f Combine
	rm -f Hello
	rm *.o
