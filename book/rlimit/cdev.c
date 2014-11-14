#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>

static int major = 0;

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

static int __init test_init(void)
{
    //当major为0时，会自动分配可用的major
    int err = register_chrdev(major, "test", &fops);
    printk("rlimit is 0x%lx\n", rlimit(RLIMIT_NOFILE));
    if(err > 0){
        major = err;
        printk("rock lee major is %d\n", major);
        return 0;
    }


    printk("rock lee alloc major failed!\n");
    return -1;
}

static void __exit test_exit(void)
{
    unregister_chrdev(major, "test");
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
