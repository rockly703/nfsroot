/*
 * This is a demo program for char device.
 * NOTE:Critical section does not have any protection from lock.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define BUFFER_SIZE 0x1000
#define RLWAIT_NAME "rlwait"

struct rlwait_t {
    struct cdev cdev;
    unsigned char buffer[BUFFER_SIZE];
};

struct rlwait_t rl;
static int rlwait_major = 0;

static int rlwait_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int rlwait_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t rlwait_read(struct file *filp, char __user *userp, size_t size, loff_t *off)
{
    if (*off > BUFFER_SIZE) 
        return 0;
    if (*off + size > BUFFER_SIZE) 
        size = BUFFER_SIZE - *off;
    
    if (copy_to_user(userp, rl.buffer, size)) 
        return -EIO;
    
    *off += size;
    return size; 
}

static ssize_t rlwait_write(struct file *filp, const char __user *userp, size_t size, loff_t *off)
{
    if (*off > BUFFER_SIZE) 
        return 0;
    if (*off + size > BUFFER_SIZE) 
        size = BUFFER_SIZE - *off;
    
    if (copy_from_user(rl.buffer, userp, size)) 
        return -EIO;
    
    *off += size;
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
    
    cdev_init(&rl.cdev, &rlwait_ops);
    rl.cdev.owner = THIS_MODULE;
    if (cdev_add(&rl.cdev, dev_no, 1)) {
        printk("Failed to add cdev %s\n", RLWAIT_NAME);
        goto add_failed;
    }
    
    return 0;
    
add_failed:
    unregister_chrdev_region(dev_no, 1);
    return -ENOMEM;
}

static void __exit rlwait_exit(void)
{
    cdev_del(&rl.cdev);
    unregister_chrdev_region(MKDEV(rlwait_major, 0), 1);
}

module_init(rlwait_init);
module_exit(rlwait_exit);

MODULE_LICENSE("GPL");
