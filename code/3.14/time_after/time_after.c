#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/processor.h>
#include <linux/kthread.h>

//#define KTHREAD_RUN
#define KTHREAD_CREATE

static DECLARE_WAIT_QUEUE_HEAD(myevent_waitqueue);
rwlock_t myevent_lock;
static struct task_struct *p = NULL;
static int count = 0;

extern unsigned int myevent_id;

static int mythread(void *unused)
{
    unsigned int event_id = 0;
    DECLARE_WAITQUEUE(wait, current);

//    allow_signal(SIGKILL);

    add_wait_queue(&myevent_waitqueue, &wait);
    unsigned long timeout = jiffies +  HZ;

    for(;;) {
        if(time_after(jiffies , timeout)){
            count++;
            timeout = jiffies + HZ;
            printk("count: %d\n", count);
        }
        
#if 0
        if(signal_pending(current))
        {
            printk("in signal_pendging\n");
            break;
        }

        printk("before read_lock\n");
        /*read_lock(&myevent_lock);*/

        if(myevent_id) {
            event_id = myevent_id;
            /*read_unlock(&myevent_lock);*/
            /*run_umode_handler(event_id);*/
        }else{
            /*read_unlock(&myevent_lock);*/
        }
#endif
    }

    /*set_current_state(TASK_RUNNING);*/
    /*remove_wait_queue(&myevent_waitqueue, &wait);*/
    return 0;
}

static int __init mythread_init(void)
{

#ifdef KTHREAD_RUN
     p = kthread_run(mythread, "hello world","rocklee_run"); 
#endif

#ifdef KTHREAD_CREATE
     p = kthread_create(mythread, "hello world","rocklee_create"); 
#endif

    if (!IS_ERR(p))
        printk("hello world!\n");
    else
        printk("bad world!\n");

#ifdef KTHREAD_CREATE
    wake_up_process(p);
#endif
    
    return 0;
}

static void __exit mythread_exit(void)
{
    /*read_unlock(&myevent_lock);*/
    kthread_stop(p);
    printk("bye bye world!\n");
}

module_init(mythread_init);
module_exit(mythread_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rocklee");
