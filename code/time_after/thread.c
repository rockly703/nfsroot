#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

static int count = 0;

static int mythread(void *unused)
{
    unsigned long timeout = HZ + jiffies;

    for(;;){
        if(time_after(jiffies, timeout)){
            timeout = jiffies + HZ;
            printk("count is k\n");
        }
    }

    return 0;
}

static int __init thread_init(void)
{
    printk("hello world!\n");
    struct task_struct *p;

    p = kthread_run(mythread, "hello", "rocklee");
    /*p = kthread_create(mythread, "hello", "rocklee");*/

    /*wake_up_process(p);*/

    return 0;
}

static void __exit thread_exit(void)
{
    printk("bye world!\n");
}

module_init(thread_init);
module_exit(thread_exit);

MODULE_AUTHOR("ROCKLEE");
MODULE_LICENSE("GPL");
