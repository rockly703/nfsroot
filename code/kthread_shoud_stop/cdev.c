#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/completion.h>

static int major = 0;
static struct cdev cdev; 

static DECLARE_COMPLETION(my_com);
static DECLARE_WAIT_QUEUE_HEAD(my_wait);

int mythread(void *data)
{
    printk("line %d\n", __LINE__);
    DECLARE_WAITQUEUE(wait, current);
    add_wait_queue(&my_wait, &wait);

    while(!kthread_should_stop()){
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();

        printk("fuck!\n");
    }

    printk("i am waken up!\n");

    set_current_state(TASK_RUNNING);
    remove_wait_queue(&my_wait, &wait);

    return 0;
}

ssize_t my_read(struct file *filp, char __user *userp, size_t size, loff_t *off)
{
    return 0;
}

ssize_t my_write(struct file *filp, const char __user *userp, size_t size, loff_t *off)
{
    return 0;
}

int my_open(struct inode *inodep, struct file *filp)
{
    return 0;
}

int my_release(struct inode *inodep, struct file *filep)
{
    return 0;
}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static struct task_struct *p;

static int __init test_init(void)
{
    //当major为0时，会自动分配可用的major
    int err = register_chrdev(major, "test", &fops);
    if(err > 0){
        major = err;
        printk("rock lee major is %d\n", major);
    }else
        goto failed;

    cdev_init(&cdev, &fops);

    err = cdev_add(&cdev, MKDEV(major, 0), 1);
    if(err){
        printk("%s failed!\n", __func__);
        unregister_chrdev(major, "test");
    }

    cdev.owner = THIS_MODULE;

    p = kthread_create(mythread, NULL, "rock_thread");
    /*kthread_run(mythread, NULL, "rock_thread");*/
    wake_up_process(p);

    return 0;

failed:
    printk("rock lee alloc major failed!\n");
    return -1;
}

static void __exit test_exit(void)
{
    cdev_del(&cdev);
    unregister_chrdev(major, "test");

    kthread_stop(p);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
