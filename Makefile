all: vm

vm: vm.c
	gcc vm.c -o vm

test_bin: test.0x

test.0x: maketest
	./maketest

maketest: maketest.c
	gcc maketest.c -o maketest

clean:
	rm maketest vm test.0x
