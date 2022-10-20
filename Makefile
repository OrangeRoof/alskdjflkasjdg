default: all

setup:
	sudo sysctl -w vm.nr_hugepages=2048
	
all:
	gcc -Os clock_test.c

run:
	sudo taskset --cpu-list 0 ./a.out

clean:
	rm a.out
