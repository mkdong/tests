#include "../common.h"
#include <x86intrin.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define A_LEN (4096)

char *ram;
char *pmm;

#define fatal_errno(err,s)                                         \
do {                                                               \
	fprintf(stderr, "failed mmap ram: %s\n", strerror(err));   \
	exit(-1);                                                  \
} while (0)

#define fatal(s)                                                   \
do {                                                               \
	perror("failed mmap ram");                                 \
	exit(-1);                                                  \
} while (0)

int test_load_miss(char *a)
{
	register int value = 0;
	for (int i = 0; i < A_LEN; ++i)
		a[i] = i % 256;

	for (int i = 0; i < A_LEN; ++i)
		_mm_clflush(&a[i]);

	u64 t1 = rdtscp();
	value = a[2];
	u64 t2 = rdtscp();

	printf(" load miss latency: %llu ticks, value = %d\n",
	       t2 - t1, value);
}

const char *TEST_FILE = "/tmp/testfile";
const char *PMM_FILE = "/mnt/xxx/testfile";

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

int main()
{
	prepare();
	test_load_miss(ram);
	sleep(1);
	test_load_miss(pmm);
	sleep(1);
	test_load_miss(ram);
	sleep(1);
	test_load_miss(pmm);

	return 0;
}
