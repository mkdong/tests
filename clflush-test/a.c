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

u64 a[1010101];

int main()
{
	u64 t1, t2;
	register int i;
	register int s;

#define TEST(pre, post, prompt) \
	do { \
		/* Warm cache */ \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		s = 0; \
		mfence; \
		/* Start the test */ \
		t1 = rdtscp(); \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		pre; \
		clflush(&a[0]); \
		post; \
		for (i = 8; i < 16; ++i) \
			s += a[i]; \
		t2 = rdtscp(); \
		printf(prompt " = %llu s=%d\n", t2 - t1, s); \
	} while (0)

#define PURE_TEST(pre, post, prompt) \
	do { \
		/* Warm cache */ \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		for (i = 0; i < 16; ++i) \
			a[i] = i; \
		s = 0; \
		mfence; \
		/* Start the test */ \
		t1 = rdtscp(); \
		pre; \
		clflush(&a[0]); \
		post; \
		t2 = rdtscp(); \
		printf(prompt " = %llu s=%d\n", t2 - t1, s); \
	} while (0)


	TEST(      ,       , " . clflush . ===warming===");
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

	PURE_TEST(      ,       , " . clflush . ===warming===");
	PURE_TEST(      ,       , " . clflush . ");
	PURE_TEST(      , lfence, " . clflush l ");
	PURE_TEST(      , sfence, " . clflush s ");
	PURE_TEST(      , mfence, " . clflush m ");
	PURE_TEST(lfence, lfence, " l clflush l ");
	PURE_TEST(lfence, sfence, " l clflush s ");
	PURE_TEST(lfence, mfence, " l clflush m ");
	PURE_TEST(sfence, lfence, " s clflush l ");
	PURE_TEST(sfence, sfence, " s clflush s ");
	PURE_TEST(sfence, mfence, " s clflush m ");
	PURE_TEST(mfence, lfence, " m clflush l ");
	PURE_TEST(mfence, sfence, " m clflush s ");
	PURE_TEST(mfence, mfence, " m clflush m ");
	return 0;
}
