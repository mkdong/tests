#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

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

volatile int a[1010];

int main()
{
	int status;
	u64 t1, t2;
	int i;

	for (i = 0; i < 64; ++i)
		a[i] = i;
	asm volatile("mfence");
	t1 = rdtscp();
	a[0] = 1;
	t2 = rdtscp();
	printf("set a[0]=1 = %llu ticks\n", t2 - t1);

	for (i = 0; i < 64; ++i)
		a[i] = i;
	asm volatile("mfence");
	t1 = rdtscp();
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		//a[0] = 1;
		_xend();
		t2 = rdtscp();
		printf("set a[0]=1 in rtc = %llu ticks\n", t2 - t1);
	} else {
		puts("Failed!\n");
	}
	return 0;
}
