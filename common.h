#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;

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

#define clflush(p) asm volatile("clflush %0" : "+m"(*(volatile char *)p))

#endif
