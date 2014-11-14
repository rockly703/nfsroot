/*
 * Wait queue test program.This program test wait queue fuction. read processes
 * block until the buffer is not empty. A writer process fills the buffer and
 * wakes up one reader process waited on the wait queue.
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <asm/bitops.h>

#define BUFFER_SIZE 0x1000
#define RLWAIT_NAME "rlwait"

enum rl_state_bits {
    RL_NEW,
    RL_WR_DONE,
};

struct rlwait_t {
    struct cdev r_cdev;
    struct mutex r_lock;
    unsigned char r_buffer[BUFFER_SIZE];
    unsigned long r_len;
    //Used to distinguish whether r_buffer has been written by a writer process
    unsigned long r_flag;
    wait_queue_head_t r_wait_head;
};

#define RL_FLAG_FNS(bit, name)    \
static void set_rl_##name(struct rlwait_t *rl)           \
{                                                   \
    set_bit(RL_##bit, &rl->r_flag);                  \
}                                                   \
static void clear_rl_##name(struct rlwait_t *rl)         \
{                                                   \
    clear_bit(RL_##bit, &rl->r_flag);                \
}                                                   \
static int rl_##name(struct rlwait_t *rl)           \
{                                                   \
    return test_bit(RL_##bit, &rl->r_flag);                  \
}
                                                   
RL_FLAG_FNS(NEW, new)
RL_FLAG_FNS(WR_DONE, write_done)                                                   

struct rlwait_t rl;
static int rlwait_major = 0;

static int rlwait_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &rl;
    return 0;
}

static int rlwait_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t rlwait_read(struct file *filp, char __user *userp, size_t size, loff_t *off)
{
    struct rlwait_t *rock = (struct rlwait_t *)filp->private_data;
    DECLARE_WAITQUEUE(wait, current);
    
    add_wait_queue_exclusive(&rock->r_wait_head, &wait);
    
    while (rock->r_flag == 0) {
        
        printk("read is going to sleep\n");
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        printk("read is awake\n");
    }

    remove_wait_queue(&rock->r_wait_head, &wait);
    set_current_state(TASK_RUNNING);
    return 0;
}

static ssize_t rlwait_write(struct file *filp, const char __user *userp, size_t size, loff_t *off)
{
    struct rlwait_t *rock = (struct rlwait_t *)filp->private_data;
    
    //The param of wake_up_interruptible is a wait queue head.
    rock->r_flag = 1;
    wake_up_interruptible(&rock->r_wait_head);
    
    return size; 
}

static struct file_operations rlwait_ops = {
    .owner = THIS_MODULE,
    .open = rlwait_open,
    .release = rlwait_close,
    .read = rlwait_read,
    .write = rlwait_write
};

static int __init rlwait_init(void)
{
    dev_t dev_no = 0;
    int err;
    
    err = alloc_chrdev_region(&dev_no, 0, 1, RLWAIT_NAME);
    if (err < 0) 
        return err;
    
    rlwait_major = MAJOR(dev_no); 
    printk("rlwait_major is %d\n", rlwait_major);
    
    cdev_init(&rl.r_cdev, &rlwait_ops);
    rl.r_cdev.owner = THIS_MODULE;
    if (cdev_add(&rl.r_cdev, dev_no, 1)) {
        printk("Failed to add cdev %s\n", RLWAIT_NAME);
        goto add_failed;
    }
    
    rl.r_len = 0;
    memset(rl.r_buffer, 0, BUFFER_SIZE);
    mutex_init(&rl.r_lock);
    /*set_rl_new(&rl);*/
    rl.r_flag = 0;
    init_waitqueue_head(&rl.r_wait_head);
    
    return 0;
    
add_failed:
    unregister_chrdev_region(dev_no, 1);
    return -ENOMEM;
}

static void __exit rlwait_exit(void)
{
    cdev_del(&rl.r_cdev);
    unregister_chrdev_region(MKDEV(rlwait_major, 0), 1);
}

module_init(rlwait_init);
module_exit(rlwait_exit);

MODULE_LICENSE("GPL");
