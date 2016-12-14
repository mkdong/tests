#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MK");
MODULE_DESCRIPTION("Linux kernel modules code-frames.");

int __init hello_init(void)
{
	pr_info("Hello World!\n");
	pr_info("==========> struct size <===========\n");
	pr_info("sizeof(spinlock_t) = %lu\n", sizeof(spinlock_t));

	return 0;
}

void __exit hello_exit(void)
{
	pr_info("Bye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
