#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>

static int count = 0;

static int mythread(void *unused)
{

    while(1){
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ * 3);

        printk("count is %d\n", count++);
    }

    set_current_state(TASK_RUNNING);
    return 0;
}

struct task_struct *p;

static int __init thread_init(void)
{
    printk("hello world!\n");

    p = kthread_run(mythread, "hello", "rocklee");
    /*p = kthread_create(mythread, "hello", "rocklee");*/

    /*wake_up_process(p);*/

    return 0;
}

static void __exit thread_exit(void)
{
    printk("bye world!\n");
    kthread_stop(p);
}

module_init(thread_init);
module_exit(thread_exit);

MODULE_AUTHOR("ROCKLEE");
MODULE_LICENSE("GPL");
