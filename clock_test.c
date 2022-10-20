#define _GNU_SOURCE
#include <sched.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>

#define LENGTH (512UL*1024*1024)
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

    uint64_t cb, ca, c;

    int bytes = 1024;
    int max_bytes = length;
    uint8_t *data = addr;

    double sumcpb = 0;
    double n = 1;
    while (bytes < max_bytes) {
        c = 0;
        int start = rand() % max_bytes;
        int end = start + bytes;
        int iter = 1 * 1000 * 1000;
        int random = start;
        for (int i = 0; i < iter; i++) {
            cb = rdtsc_s ();
            data[random]++;
            ca = rdtsc_e ();
            c += ca - cb;
            random += 64 + (rand() % 64);
            if (random > end) 
                random -= bytes;
        }

        double cpb = (double) c / (double) iter;
        if (sumcpb == 0)
            sumcpb = cpb;
        else if (cpb - (sumcpb / n) > 0.5)
            break;
        else {
            n++;
            sumcpb += cpb;
        }
        bytes *= 2;
    }
    max_bytes = bytes;
    bytes = bytes / 2;
    sumcpb = 0; n = 1;

    while (bytes < max_bytes) {
        c = 0;
        int start = rand() % max_bytes;
        int end = start + bytes;
        int iter = 10 * 1000 * 1000;
        int random = start;
        for (int i = 0; i < iter; i++) {
            cb = rdtsc_s ();
            data[random]++;
            ca = rdtsc_e ();
            c += ca - cb;
            random += 64 + (rand() % 64);
            if (random > end) 
                random -= bytes;
        }

        double cpb = (double) c / (double) iter;
        if (sumcpb == 0)
            sumcpb = 1;
        else if (sumcpb == 1)
            sumcpb = cpb;
        else if (fabs((sumcpb / n) - cpb) > 0.35)
            break;
        else {
            n++;
            sumcpb += cpb;
        }
        bytes += 1000;
    }
    printf("%d\n", bytes);
    fflush(stdout);


	/* munmap() length of MAP_HUGETLB memory must be hugepage aligned */
	if (munmap(addr, length)) {
		perror("munmap");
		exit(1);
	}
	return 0;
}
