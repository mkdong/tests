#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/sched.h>
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
/* const int N = 10; */
struct res res_pool[100000+1];

#define DEF_ALLOC(name, pre, post, empty, first, del) \
void *alloc_##name(void) \
{ \
	struct res *res; \
	void *r = NULL; \
	pre; \
	if (empty) { \
		post; \
		return NULL; \
	} \
	res = first; \
	del; \
	post; \
	if (res) r = res->ptr; \
	return r; \
}

DEF_ALLOC(vanilla, , ,
	list_empty(&pool),
	list_first_entry(&pool, struct res, node),
	list_del(&res->node))

DEF_ALLOC(spin,
	spin_lock_irq(&lock),
	spin_unlock_irq(&lock),
	list_empty(&pool),
	list_first_entry(&pool, struct res, node),
	list_del(&res->node))

DEF_ALLOC(lockfree, , ,
	llist_empty(&lpool),
	({
	 struct llist_node *_node = llist_del_first(&lpool);
	 struct res *_res = NULL;
	 if (_node) {
		_res = container_of(_node, struct res, lnode);
	 }
	 _res;
	 }),
	)



#define DEF_FREE(name, pre, post, add) \
void free_##name(void *a) \
{\
	struct res *res; \
	if (a == NULL) return; \
	pre; \
	if ((u64)a > N) pr_warn("!!! a = %p\n", a); \
	res = &res_pool[(u64)a]; \
	res->ptr = a; \
	add; \
	post; \
}

DEF_FREE(vanilla, , ,
	list_add(&res->node, &pool))

DEF_FREE(spin,
	spin_lock_irq(&lock),
	spin_unlock_irq(&lock),
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

void single_thread_test(void)
{
	pr_info("==========> single-thread test <===========\n");
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
}

struct task_spec {
	int worker_id;;
	atomic_t *barrier;
	void **local_pool;
	int top;
	void *(*_alloc)(void);
	void (*_free)(void *);
	u64 alloc_nsec;
	u64 free_nsec;
};


int worker(void *arg)
{
	struct task_spec *spec = arg;
	struct timespec start;
	struct timespec end;
	u64 i;
	u64 alloc_nsec = 0;
	u64 free_nsec = 0;

	atomic_add(-1, spec->barrier);
	while (atomic_read(spec->barrier) > 0);


	spec->alloc_nsec = 0;
	spec->free_nsec = 0;

	for (i = 0; i < N; ++i) {
		void *res;
		getrawmonotonic(&start);
		res = spec->_alloc();
		getrawmonotonic(&end);
		if (!res) break;

		spec->local_pool[spec->top++] = res;
		/*
		pr_info("worker=%d, cpu=%d, allocated %llu=%p\n",
			spec->worker_id, smp_processor_id(), i, res);
			*/

		alloc_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}

	for (i = 0; i < spec->top; ++i) {
		/*
		pr_info("worker=%d, cpu=%d, to free %llu=%p\n",
			spec->worker_id, smp_processor_id(), i,
			spec->local_pool[i]);
			*/
		getrawmonotonic(&start);
		spec->_free(spec->local_pool[i]);
		getrawmonotonic(&end);
		free_nsec += (end.tv_sec - start.tv_sec) * (u64)1e9
			+ (end.tv_nsec - start.tv_nsec);
	}

	spec->alloc_nsec = alloc_nsec;
	spec->free_nsec = free_nsec;

	atomic_add(-1, spec->barrier);
	return 0;
};

void _multi_thread_test(int nthread,
			struct task_struct **tasks,
			struct task_spec *task_specs,
			void *(*_alloc)(void),
			void (*_free)(void *),
			const char *prompt)
{
	atomic_t barrier;
	u64 i;
	u64 alloc_nsec = 0;
	u64 free_nsec = 0;

	/* Bind current thread */
	while (true) {
		volatile int cpu;
		/*
		kthread_bind(current, nthread + 1);
		cond_resched();
		*/
		set_cpus_allowed_ptr(current, cpumask_of(1));
		cpu = smp_processor_id();
		if (cpu == 1)
			break;
		pr_info("try to bind master thread to cpu=%d\n", 1);
	}

	atomic_set(&barrier, nthread + 1);

	spin_lock_init(&lock);
	INIT_LIST_HEAD(&pool);
	init_llist_head(&lpool);
	for (i = 0; i < N; ++i)
		_free((void *)i + 1);

	for (i = 0; i < nthread; ++i) {
		task_specs[i].worker_id = i;
		task_specs[i].barrier = &barrier;
		task_specs[i].top = 0;
		task_specs[i]._alloc = _alloc;
		task_specs[i]._free = _free;

		tasks[i] = kthread_create(worker, task_specs + i,
					  "spin-test-worker %lld", i);
		if (IS_ERR(tasks[i])) {
			pr_err("kthread_create err!");
			return;
		} else
			kthread_bind(tasks[i], 10 + i);
	}
	for (i = 0; i < nthread; ++i) {
		if (!IS_ERR(tasks[i])) {
			wake_up_process(tasks[i]);
		}
	}
	atomic_add(-1, &barrier);
	/* Running here */
	while (atomic_read(&barrier) > -nthread);

	for (i = 0; i < nthread; ++i) {
		alloc_nsec += task_specs[i].alloc_nsec;
		free_nsec += task_specs[i].free_nsec;
	}
	pr_info("%s alloc %d times in %d threads costs nsec: %llu\n",
		prompt, N, nthread, alloc_nsec);
	pr_info("%s free  %d times in %d threads costs nsec: %llu\n",
		prompt, N, nthread, free_nsec);

}

void multi_thread_test(int nthread)
{
	struct task_struct **tasks;
	struct task_spec *task_specs;
	int i;

	pr_info("==========> multi-thread test <===========\n");
	tasks = kmalloc(sizeof(struct task_struct *) * nthread, GFP_KERNEL);
	task_specs = kmalloc(sizeof(struct task_spec) * nthread, GFP_KERNEL);
	for (i = 0; i < nthread; ++i) {
		task_specs[i].local_pool = kmalloc(sizeof(void *) * N,
						   GFP_KERNEL);
	}

	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_spin, free_spin, "spinlock");
	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_lockfree, free_lockfree, "lockfree");

	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_spin, free_spin, "spinlock");
	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_lockfree, free_lockfree, "lockfree");

	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_spin, free_spin, "spinlock");
	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_lockfree, free_lockfree, "lockfree");

	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_spin, free_spin, "spinlock");
	_multi_thread_test(nthread, tasks, task_specs,
			   alloc_lockfree, free_lockfree, "lockfree");

	for (i = 0; i < nthread; ++i) {
		kfree(task_specs[i].local_pool);
	}

	kfree(tasks);
	kfree(task_specs);
}

int __init hello_init(void)
{
	pr_info("Hello World!\n");
	pr_info("==========> struct size <===========\n");
	pr_info("sizeof(spinlock_t) = %lu\n", sizeof(spinlock_t));

	single_thread_test();

	multi_thread_test(2);
	multi_thread_test(4);
	multi_thread_test(6);
	multi_thread_test(8);
	return 0;
}

void __exit hello_exit(void)
{
	pr_info("Bye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
