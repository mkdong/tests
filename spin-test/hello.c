#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MK");
MODULE_DESCRIPTION("Linux kernel modules code-frames.");

struct res {
	struct llist_node lnode;
	struct list_head node;
	void *ptr;
};

spinlock_t lock;
struct list_head pool;
struct llist_head lpool;
const int N = 100000;
struct res res_pool[100000+1];

#define DEF_ALLOC(name, pre, post, empty, first, del) \
void *alloc_##name(void) \
{ \
	struct res *res; \
	void *r; \
	pre; \
	if (empty) { \
		post; \
		return NULL; \
	} \
	res = first; \
	del; \
	post; \
	r = res->ptr; \
	return r; \
}

DEF_ALLOC(vanilla, , ,
	list_empty(&pool),
	list_first_entry(&pool, struct res, node),
	list_del(&res->node))

DEF_ALLOC(spin,
	spin_lock(&lock),
	spin_unlock(&lock),
	list_empty(&pool),
	list_first_entry(&pool, struct res, node),
	list_del(&res->node))

DEF_ALLOC(lockfree, , ,
	llist_empty(&lpool),
	llist_entry(llist_del_first(&lpool), struct res, lnode),
	)



#define DEF_FREE(name, pre, post, add) \
void free_##name(void *a) \
{\
	struct res *res; \
	if (a == NULL) return; \
	pre; \
	res = &res_pool[(u64)a]; \
	res->ptr = a; \
	add; \
	post; \
}

DEF_FREE(vanilla, , ,
	list_add(&res->node, &pool))

DEF_FREE(spin,
	spin_lock(&lock),
	spin_unlock(&lock),
	list_add(&res->node, &pool))

DEF_FREE(lockfree, , ,
	llist_add(&res->lnode, &lpool))

void test(void *(*_alloc)(void), void (*_free)(void *), const char *prompt)
{
	struct timespec start;
	struct timespec end;
	u64 alloc_nsec = 0;
	u64 free_nsec = 0;
	u64 i;

	spin_lock_init(&lock);
	INIT_LIST_HEAD(&pool);
	init_llist_head(&lpool);
	for (i = 0; i < N; ++i) {
		_free((void *)i + 1);
	}
	for (i = 0; i < N; ++i) {
		getrawmonotonic(&start);
		_alloc();
		getrawmonotonic(&end);
		alloc_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}
	for (i = 0; i < N; ++i) {
		getrawmonotonic(&start);
		_free((void *)i + 1);
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
	test(alloc_vanilla, free_vanilla, "vanilla");
	test(alloc_vanilla, free_vanilla, "vanilla");
	test(alloc_vanilla, free_vanilla, "vanilla");
	test(alloc_vanilla, free_vanilla, "vanilla");
	test(alloc_spin, free_spin, "spinlock");
	test(alloc_spin, free_spin, "spinlock");
	test(alloc_spin, free_spin, "spinlock");
	test(alloc_spin, free_spin, "spinlock");
	test(alloc_lockfree, free_lockfree, "lockfree");
	test(alloc_lockfree, free_lockfree, "lockfree");
	test(alloc_lockfree, free_lockfree, "lockfree");
	test(alloc_lockfree, free_lockfree, "lockfree");
	return 0;
}

void __exit hello_exit(void)
{
	pr_info("Bye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
