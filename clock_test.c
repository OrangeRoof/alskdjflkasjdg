#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>

#define LENGTH (2048UL*1024)
#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000 /* arch specific */
#endif

#ifndef MAP_HUGE_MASK
#define MAP_HUGE_MASK 0x3f
#endif

/* Only ia64 requires this */
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_FIXED)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#endif

static __inline__ int64_t rdtsc_s(void)
{
  unsigned a, d; 
  asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  asm volatile("rdtsc" : "=a" (a), "=d" (d)); 
  return ((unsigned long)a) | (((unsigned long)d) << 32); 
}

static __inline__ int64_t rdtsc_e(void)
{
  unsigned a, d; 
  asm volatile("rdtscp" : "=a" (a), "=d" (d)); 
  asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  return ((unsigned long)a) | (((unsigned long)d) << 32); 
}

/**
 * Sets CPU affinity to a core
 * 
 * @param core_idx The index of the core to be set as the affinity core
 * @return the status value of sched_setaffinity(), non-zero are error values
 */
int set_core(int core_idx) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core_idx, &mask);

	// 0 in the first arg means current process
	int result = sched_setaffinity(0, sizeof(mask), &mask);
	if (result != 0) {
		printf("sched_setaffinity() failed!\n");
	}
	return result;
}

/**
 * Sets nice value
 * 
 * @param nice_val The nice value to be set, range from [-20, 19]
 * @return Nice value of the program
 */
int set_nice(int nice_val) {
	int result = nice(nice_val);
	if (result == -1) {
		printf("Set nice() failed, remember to run with root access (ignore if you set nice value to %d)\n", result);
	}
	return result;
}

int main(int argc, char **argv)
{
    // setup nice & cpu core affinity 
    set_core(1);
    set_nice(-20);

    srand(time(0));
	void *addr;
	size_t length = LENGTH;
	int flags = FLAGS;

	addr = mmap(ADDR, length, PROTECTION, flags, -1, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
    uint8_t *data = addr;

    uint64_t cb, ca, c;
    //int bytes = 1024;
    int bytes = 48000;
    int max_bytes = 262144;

    while (bytes < 49500) {
        c = 0;
        int start = rand() % (max_bytes - bytes);
        // load as much data as 
        for (int i = start; i < start+bytes; i++) {
            data[i] += data[i];
        }
        int iter = 20 * 1000 * 1000;
        int random;
        for (int i = 0; i < iter; i++) {
            random = start + rand() % bytes;
            cb = rdtsc_s ();
            data[random] += data[random];
            ca = rdtsc_e ();
            c += ca - cb;
        }

        double cpb = (double) c / (double) iter;
        printf("%lf ", cpb);
        //printf("%" PRIu64 " ", c);
        printf("%d \n", bytes);
        fflush(stdout);
        bytes += 100;
    }


	/* munmap() length of MAP_HUGETLB memory must be hugepage aligned */
	if (munmap(addr, length)) {
		perror("munmap");
		exit(1);
	}
	return 0;
}
