#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MK");
MODULE_DESCRIPTION("Linux kernel modules code-frames.");

struct res {
	struct list_head node;
	void *ptr;
};

spinlock_t lock;
struct list_head pool;

void *alloc(bool uselock)
{
	struct res *res;
	void *r;
	if (uselock) spin_lock(&lock);
	if (list_empty(&pool)) {
		if (uselock) spin_unlock(&lock);
		return NULL;
	}
	res = list_first_entry(&pool, struct res, node);
	list_del(&res->node);

	if (uselock) spin_unlock(&lock);
	r = res->ptr;
	kfree(res);
	return r;
}

void free(void *a, bool uselock)
{
	struct res *res;
	if (a == NULL) return;
	if (uselock) spin_lock(&lock);
	res = kmalloc(sizeof(struct res), GFP_KERNEL);
	res->ptr = a;
	list_add(&res->node, &pool);
	if (uselock) spin_unlock(&lock);
}

void test(bool uselock)
{
	struct timespec start;
	struct timespec end;
	const int N = 100000;
	u64 alloc_nsec = 0;
	u64 free_nsec = 0;
	u64 i;

	spin_lock_init(&lock);
	INIT_LIST_HEAD(&pool);
	for (i = 0; i < N; ++i) {
		free((void *)i + 1, uselock);
	}
	for (i = 0; i < N; ++i) {
		getrawmonotonic(&start);
		alloc(uselock);
		getrawmonotonic(&end);
		alloc_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}
	for (i = 0; i < N; ++i) {
		getrawmonotonic(&start);
		free((void *)i + 1, uselock);
		getrawmonotonic(&end);
		free_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}
	pr_info("uselock=%d alloc %d times costs nsec: %llu\n",
		uselock, N, alloc_nsec);
	pr_info("uselock=%d free  %d times costs nsec: %llu\n",
		uselock, N, free_nsec);
}

int __init hello_init(void)
{
	pr_info("Hello World!\n");
	test(true);
	test(false);
	test(true);
	test(false);
	test(true);
	test(false);
	return 0;
}

void __exit hello_exit(void)
{
	pr_info("Bye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
