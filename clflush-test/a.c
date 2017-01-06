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

static inline void sfence(void)
{
	asm volatile("sfence");
}
static inline void mfence(void)
{
	asm volatile("mfence");
}
static inline void lfence(void)
{
	asm volatile("lfence");
}

static inline void clflush(volatile void *p)
{
	asm volatile("clflush %0" : "+m"(*(volatile char *)p));
}

int a[1010101];

int main()
{
	u64 t1, t2;
	register int i;

#define TEST(pre, post, prompt) \
	do { \
		/* Warm cache */ \
		for (i = 0; i < 64; ++i) \
			a[i] = i; \
		mfence(); \
		/* Start the test */ \
		t1 = rdtscp(); \
		for (i = 0; i < 64; ++i) \
			a[i] = i; \
		pre; \
		clflush(&a[0]); \
		post; \
		t2 = rdtscp(); \
		printf(prompt " = %llu \n", t2 - t1); \
	} while (0)


	TEST(      ,       , " . clflush . ");
	TEST(      , lfence, " . clflush l ");
	TEST(      , sfence, " . clflush s ");
	TEST(      , mfence, " . clflush m ");
	TEST(lfence, lfence, " l clflush l ");
	TEST(lfence, sfence, " l clflush s ");
	TEST(lfence, mfence, " l clflush m ");
	TEST(sfence, lfence, " s clflush l ");
	TEST(sfence, sfence, " s clflush s ");
	TEST(sfence, mfence, " s clflush m ");
	TEST(mfence, lfence, " m clflush l ");
	TEST(mfence, sfence, " m clflush s ");
	TEST(mfence, mfence, " m clflush m ");

	return 0;
}
