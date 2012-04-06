all: vm test.0x

vm: vm.c
	gcc vm.c -o vm

test.0x: maketest
	./maketest

maketest: maketest.c
	gcc maketest.c -o maketest

clean:
	rm maketest vm test.0x
