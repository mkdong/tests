#include "../common.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define A_LEN (4096 * 1024 * 4)
#define CL_SIZE (64)


#define measure realtime
#define MEASURE_UNIT "ns"
char *ram;
char *pmm;
int *idx = NULL;


int measure_lat(void)
{
	_mm_mfence();
	u64 t1 = measure();
	u64 t2 = measure();
	_mm_mfence();
	printf(" empty measurement = %llu "MEASURE_UNIT"\n", t2 - t1);
}

int test_load_miss(char *a, const char *tag)
{
	register int value = 0;
	for (int i = 0; i < A_LEN; ++i)
		a[i] = i % 256;

	for (int i = 0; i < A_LEN; ++i)
		_mm_clflush(&a[i]);

	_mm_mfence();
	u64 t1 = measure();
	value = a[2];
	u64 t2 = measure();
	_mm_mfence();

	printf("[%s] load miss latency: %llu "MEASURE_UNIT", value = %d\n",
	       tag, t2 - t1, value);
}

const char *TEST_FILE = "/tmp/testfile";
const char *PMM_FILE = "/mnt/mem/testfile";

void prepare(void)
{
	ram = mmap(NULL, A_LEN, PROT_READ | PROT_WRITE,
		   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (ram == MAP_FAILED)
		fatal("failed mmap ram");
	int fd = open(TEST_FILE, O_CREAT | O_RDWR, 0666);
	if (fd == -1)
		fatal("failed creating file");
	int r = posix_fallocate(fd, 0, A_LEN);
	if (r)
		fatal_errno(r, "failed fallocate");
	pmm = mmap(NULL, A_LEN, PROT_READ | PROT_WRITE,
		   MAP_SHARED, fd, 0);
	if (pmm == MAP_FAILED)
		fatal("failed mmap pmm");
}


void single_test(void)
{
	puts("= single test =");
	test_load_miss(ram, "ram 0.");
	test_load_miss(ram, "ram 1.");
	test_load_miss(ram, "ram 2.");
	test_load_miss(ram, "ram 3.");
	test_load_miss(ram, "ram 4.");
	puts("-");
	sleep(1);
	test_load_miss(pmm, "pmm 0.");
	test_load_miss(pmm, "pmm 1.");
	test_load_miss(pmm, "pmm 2.");
	test_load_miss(pmm, "pmm 3.");
	test_load_miss(pmm, "pmm 4.");
}

void test_seq_load(char *a, const char *tag)
{
	register int chksum = 0;
	register int sum = 0;
	register int count = 0;
	for (int i = 0; i < A_LEN; ++i)
		a[i] = i % 256;
	for (int i = 0; i < A_LEN; ++i)
		_mm_clflush(&a[i]);

	for (int i = 0; i < A_LEN; i += CL_SIZE) {
		_mm_mfence();
		u64 t1 = measure();
		chksum += a[i];
		u64 t2 = measure();
		_mm_mfence();
		sum += t2 - t1;
		count++;
	}

	printf("[%s] seq load miss latency: %llu "MEASURE_UNIT
	       ", count=%lld, avg=%llu "MEASURE_UNIT", chksum=0x%lx\n",
	       tag, sum, count, sum / count, chksum);
}

void seq_test(void)
{
	puts("= seq test =");
	puts("-");
	test_seq_load(ram, "ram 0.");
	test_seq_load(ram, "ram 1.");
	test_seq_load(ram, "ram 2.");
	test_seq_load(ram, "ram 3.");
	test_seq_load(ram, "ram 4.");
	puts("-");
	sleep(1);
	test_seq_load(pmm, "pmm 0.");
	test_seq_load(pmm, "pmm 1.");
	test_seq_load(pmm, "pmm 2.");
	test_seq_load(pmm, "pmm 3.");
	test_seq_load(pmm, "pmm 4.");
}

void test_rand_load(char *a, const char *tag)
{
	if (!idx) {
		idx = malloc(A_LEN / CL_SIZE * sizeof(int));
		for (int i = 0; i < A_LEN / CL_SIZE; ++i) {
			idx[i] = i * CL_SIZE;
		}
		/* shuffle */
		for (int i = 0; i < A_LEN; ++i) {
			int x = rand() % A_LEN *
				rand() % A_LEN *
				rand() % (A_LEN / CL_SIZE);
			if (x < 0) x += A_LEN / CL_SIZE;
			int y = rand() % A_LEN *
				rand() % A_LEN *
				rand() % (A_LEN / CL_SIZE);
			if (y < 0) y += A_LEN / CL_SIZE;
			int t = idx[x];
			idx[x] = idx[y];
			idx[y] = t;
		}
	}
	for (int i = 0; i < A_LEN / CL_SIZE; ++i) {
		volatile int t = idx[i];
	}
	register int chksum = 0;
	register int sum = 0;
	register int count = 0;
	for (int i = 0; i < A_LEN; ++i)
		a[i] = i % 256;
	for (int i = 0; i < A_LEN; ++i)
		_mm_clflush(&a[i]);

	for (int i = 0; i < A_LEN / CL_SIZE / 2; ++i) {
		_mm_mfence();
		u64 t1 = measure();
		chksum += a[idx[i]];
		u64 t2 = measure();
		_mm_mfence();
		sum += t2 - t1;
		count++;
	}

	printf("[%s] seq load miss latency: %llu "MEASURE_UNIT
	       ", count=%lld, avg=%llu "MEASURE_UNIT", chksum=0x%lx\n",
	       tag, sum, count, sum / count, chksum);
}


void rand_test(void)
{
	puts("= rand test =");
	puts("-");
	test_rand_load(ram, "ram 0.");
	test_rand_load(ram, "ram 1.");
	test_rand_load(ram, "ram 2.");
	test_rand_load(ram, "ram 3.");
	test_rand_load(ram, "ram 4.");
	puts("-");
	sleep(1);
	test_rand_load(pmm, "pmm 0.");
	test_rand_load(pmm, "pmm 1.");
	test_rand_load(pmm, "pmm 2.");
	test_rand_load(pmm, "pmm 3.");
	test_rand_load(pmm, "pmm 4.");
}

int main()
{
	measure_lat();
	measure_lat();
	measure_lat();
	measure_lat();
	prepare();
	single_test();
	seq_test();
	rand_test();
	return 0;
}
