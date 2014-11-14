#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

extern wait_queue_head_t myevent_waitqueue;

static int __init wake_thread_up(void)
{
    wake_up_interruptible(&myevent_waitqueue);

    return 0;
}

static void __exit wake_thread_down(void)
{
    return;
}

module_init(wake_thread_up);
module_exit(wake_thread_down);

MODULE_LICENSE("GPL");
