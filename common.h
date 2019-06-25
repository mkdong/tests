#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

#define fatal_errno(err, s)                        \
do {                                               \
	fprintf(stderr, s " :%s", strerror(err));  \
	exit(-1);                                  \
} while (0)

#define fatal(s)                                   \
do {                                               \
	perror(s);                                 \
	exit(-1);                                  \
} while (0)

typedef uint64_t u64;
typedef uint32_t u32;

static inline u64 realtime(void)
{
	struct timespec ts;
	int r = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (r) fatal("failed clock_gettime");
	return ts.tv_nsec + ts.tv_sec * (long)1e9;
}

static inline u64 rdtscp(void)
{
	u64 msr;
	asm volatile("rdtscp\n\t"
		     "shl $32, %%rdx\n\t"
		     "or %%rdx, %0"
		     : "=a" (msr)
		     :
		     : "rdx");
	return msr;
}

#define sfence asm volatile("sfence":::"memory")
#define mfence asm volatile("mfence":::"memory")
#define lfence asm volatile("lfence":::"memory")

#define clflush(p) _mm_clflush(p)
#define clflushopt(p) _mm_clflushopt(p)
#define clwb(p) _mm_clwb(p)

#endif
