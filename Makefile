default: all

setup:
	sudo sysctl -w vm.nr_hugepages=2048
	
all:
	gcc -O0 clock_test.c

run:
	sudo taskset --cpu-list 5 ./a.out

clean:
	rm a.out
