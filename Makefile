default: all

setup:
	sudo sysctl -w vm.nr_hugepages=2048
	
all:
	gcc -O0 clock_test.c

run:
	sudo ./a.out
	
benchmark: clean setup all
	cat /proc/cmdline
	sudo ./a.out
	sudo ./a.out
	sudo ./a.out

clean:
	rm a.out
	sudo sysctl -w vm.nr_hugepages=0
