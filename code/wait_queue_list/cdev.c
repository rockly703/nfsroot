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

#define SIZE 4 
static int major = 0;
static struct cdev cdev; 

static struct head {
    spinlock_t lock;
    wait_queue_head_t todo;
    struct list_head head;
}work_head;

struct worker{
    struct list_head my_list;
    int data;
};

//通过wake_up唤醒等待队列，队列上的所有成员都会被唤醒
int mythread(void *data)
{
    struct worker *work_entry;
    struct worker *temp; 
    DECLARE_WAITQUEUE(wait, current);
    set_current_state(TASK_INTERRUPTIBLE);

    while(1){
        add_wait_queue(&work_head.todo, &wait);

        spin_lock(&work_head.lock);
        if(list_empty(&work_head.head)){
            spin_unlock(&work_head.lock);
            //在等待对列上休眠
            schedule();  
            spin_lock(&work_head.lock);
        }else{
            set_current_state(TASK_RUNNING);
        }

        //醒来后将自己从队列上移除
        remove_wait_queue(&work_head.todo, &wait);

        //本例子中用了list_for_each_entry_safe代替while循环
        list_for_each_entry_safe(work_entry, temp, &work_head.head, my_list) {

            printk("data is %d\n", work_entry->data);
            list_del(work_head.head.next);
            kfree(work_entry);

            spin_unlock(&work_head.lock);
            spin_lock(&work_head.lock);
        }

        spin_unlock(&work_head.lock);
        set_current_state(TASK_INTERRUPTIBLE);
    }

    return 0;
}

ssize_t my_read(struct file *filp, char __user *userp, size_t size, loff_t *off)
{
    return 0;
}

ssize_t my_write(struct file *filp, const char __user *userp, size_t size, loff_t *off)
{
    struct worker *work_node = kmalloc(sizeof(struct worker), GFP_ATOMIC);
    if(!work_node)
        return -1;

    if(size > SIZE)
        size = SIZE;

    if(copy_from_user((char *)&work_node->data, userp, size)){
        printk("copy failed!\n");
        return -1;
    }  
    
    spin_lock(&work_head.lock);
    list_add_tail(&work_node->my_list, &work_head.head);
    spin_unlock(&work_head.lock);

    wake_up(&work_head.todo);

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

    struct task_struct *p;

    cdev_init(&cdev, &fops);

    err = cdev_add(&cdev, MKDEV(major, 0), 1);
    if(err){
        printk("%s failed!\n", __func__);
        unregister_chrdev(major, "test");
    }

    INIT_LIST_HEAD(&work_head.head);
    spin_lock_init(&work_head.lock);
    init_waitqueue_head(&work_head.todo);

    cdev.owner = THIS_MODULE;

    p = kthread_create(mythread, NULL, "rock_thread");
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
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
