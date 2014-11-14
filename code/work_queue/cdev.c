/**
 * @file cdev.c
 * @brief 此程序模拟了一个消费者的情况，mythread用于处理write提供的数据，
 * 一般情况，mythread在等待队列上休眠，当write提供数据后，就会把mythread叫
 * 醒，mythread醒来后将自己从等待队列上移除，当处理完数据后继续将自己加入
 * 等待队列，休眠
 * @author rockly
 * @version 0.1
 * @date 2014-06-07
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/nsproxy.h>

#define SIZE 4 
static int major = 0;
static struct cdev cdev; 

static struct head {
    spinlock_t lock;
    struct list_head head;
}work_head;

struct worker{
    struct list_head my_list;
    int data;
};

struct workqueue_struct *wq;

int mythread(struct work_struct *work)
{
    struct worker *work_entry;

    spin_lock(&work_head.lock);
    while(!list_empty(&work_head.head)){

        work_entry = list_entry(work_head.head.next, struct worker, my_list);

        printk("data is %d\n", work_entry->data);

        list_del(work_head.head.next);
        spin_unlock(&work_head.lock);
        kfree(work_entry);

        spin_lock(&work_head.lock);
    }

    spin_unlock(&work_head.lock);

    return 0;
}

ssize_t my_read(struct file *filp, char __user *userp, size_t size, loff_t *off)
{
    return 0;
}

ssize_t my_write(struct file *filp, const char __user *userp, size_t size, loff_t *off)
{
    struct work_struct *hardwork = kmalloc(sizeof(struct worker), GFP_KERNEL);
    if(!hardwork){
        printk("hardwork kmalloc failed!\n");
        return -1;
    }

    struct worker *node = kmalloc(sizeof(struct worker), GFP_KERNEL);
    if(!node){
        printk("node kmalloc failed!\n");
        return -1;
    }

    if(copy_from_user((char *)&node->data, userp, size)){
        printk("copy failed!\n");
        return -1;
    }  
    
    spin_lock(&work_head.lock);
    list_add_tail(&node->my_list, &work_head.head);
    spin_unlock(&work_head.lock);

    INIT_WORK(hardwork, mythread);
    queue_work(wq, hardwork);

    return size;
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

    INIT_LIST_HEAD(&work_head.head);
    spin_lock_init(&work_head.lock);

    cdev.owner = THIS_MODULE;

    wq = create_singlethread_workqueue("rocklee");
    return 0;

failed:
    printk("rock lee alloc major failed!\n");
    return -1;
}

static void __exit test_exit(void)
{
    cdev_del(&cdev);
    unregister_chrdev(major, "test");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
