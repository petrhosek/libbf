all:
	gcc -std=gnu99 -Wall example-target.c -o example-target
	gcc -std=gnu99 -Wall example-embedobj.c -o example-embedobj
	gcc -std=gnu99 -Wall gen_linker_command.c -o gen_linker_command -lbfd
	gcc -std=gnu99 -Wall inject.c -o inject -lbfd

combine:
	# Add linker command to the next command: (-Wall -Wl,...)
	gcc -std=gnu99 -Wall -Wl,--section-start=.interp=0xA02238,--section-start=.note.ABI-tag=0xA02254,--section-start=.note.gnu.build-id=0xA02274,--section-start=.gnu.hash=0xA02298,--section-start=.dynsym=0xA022B8,--section-start=.dynstr=0xA02318,--section-start=.gnu.version=0xA02356,--section-start=.gnu.version_r=0xA02360,--section-start=.rela.dyn=0xA02380,--section-start=.rela.plt=0xA02398,--section-start=.init=0xA023C8,--section-start=.plt=0xA023E0,--section-start=.text=0xA02410,--section-start=.fini=0xA025E8,--section-start=.rodata=0xA025F8,--section-start=.eh_frame_hdr=0xA02604,--section-start=.eh_frame=0xA02638,--section-start=.ctors=0xC02E28,--section-start=.dtors=0xC02E38,--section-start=.jcr=0xC02E48,--section-start=.dynamic=0xC02E50,--section-start=.got=0xC02FE0,--section-start=.got.plt=0xC02FE8,--section-start=.data=0xC03010,--section-start=.bss=0xC03020,--section-start=.comment=0x602000 example-embedobj.c -o example-embedobj
	objcopy --prefix-sections=.new example-embedobj

	./inject example-target example-embedobj example-combined
	objdump -Dslx example-target > example-target.txt
	objdump -Dslx example-combined > example-combined.txt

clean:
	rm -f *.o
	rm -f example-target
	rm -f example-embedobj
	rm -f gen_linker_command
	rm -f inject
