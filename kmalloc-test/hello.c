#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mkdong");
MODULE_DESCRIPTION("Evaluate the perforamnce of kmalloc, kmem_cache_alloc and"
		   " vanilla pre-allocated memory."
		   "Code is based on Linux kernel modules code-frames.");

struct res {
	struct llist_node lnode;
	struct list_head node;
	void *ptr;
};

struct kmem_cache *cachep;
const int N = 100000;
struct res res_pool[100000+1];
int res_pool_used;
struct res *res_ptrs[100000+1];

void *alloc_kmalloc(void)
{
	return kmalloc(sizeof(struct res), GFP_KERNEL | GFP_ATOMIC);
}

void *alloc_kmem_cache_alloc(void)
{
	return kmem_cache_alloc(cachep, GFP_KERNEL | GFP_ATOMIC);
}

void *alloc_vanilla_pre_allocated(void)
{
	return &res_pool[(res_pool_used++) % N];
}

void free_kmalloc(void *p)
{
	kfree(p);
}

void free_kmem_cache_alloc(void *p)
{
	kmem_cache_free(cachep, p);
}

void free_vanilla_pre_allocated(void *p)
{
	/* nothing */
}

void test(void *(*_alloc)(void), void (*_free)(void *), const char *prompt)
{
	struct timespec start;
	struct timespec end;
	u64 alloc_nsec = 0;
	u64 free_nsec = 0;
	u64 i;

	for (i = 0; i < N; ++i) {
		getrawmonotonic(&start);
		res_ptrs[i] = _alloc();
		getrawmonotonic(&end);
		alloc_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}
	for (i = 0; i < N; ++i) {
		getrawmonotonic(&start);
		_free((void *)res_ptrs[i]);
		getrawmonotonic(&end);
		free_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}
	pr_info("%s alloc %d times costs nsec: %llu\n",
		prompt, N, alloc_nsec);
	pr_info("%s free  %d times costs nsec: %llu\n",
		prompt, N, free_nsec);
}

int __init hello_init(void)
{
	pr_info("Hello World!\n");
	cachep = kmem_cache_create("struct res cachep", sizeof(struct res), 0,
				   SLAB_HWCACHE_ALIGN, NULL);
	pr_info("==========> result <===========\n");
	test(alloc_kmalloc, free_kmalloc, "kmalloc");
	test(alloc_kmalloc, free_kmalloc, "kmalloc");
	test(alloc_kmalloc, free_kmalloc, "kmalloc");
	test(alloc_kmalloc, free_kmalloc, "kmalloc");
	test(alloc_kmem_cache_alloc, free_kmem_cache_alloc, "kmem_cache_alloc");
	test(alloc_kmem_cache_alloc, free_kmem_cache_alloc, "kmem_cache_alloc");
	test(alloc_kmem_cache_alloc, free_kmem_cache_alloc, "kmem_cache_alloc");
	test(alloc_kmem_cache_alloc, free_kmem_cache_alloc, "kmem_cache_alloc");
	test(alloc_vanilla_pre_allocated, free_vanilla_pre_allocated,
	     "vanilla_pre_allocated");
	test(alloc_vanilla_pre_allocated, free_vanilla_pre_allocated,
	     "vanilla_pre_allocated");
	test(alloc_vanilla_pre_allocated, free_vanilla_pre_allocated,
	     "vanilla_pre_allocated");
	test(alloc_vanilla_pre_allocated, free_vanilla_pre_allocated,
	     "vanilla_pre_allocated");
	kmem_cache_destroy(cachep);
	return 0;
}

void __exit hello_exit(void)
{
	pr_info("Bye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
