#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdebug.h>
#include <linux/cdev.h>
#include <linux/netdevice.h>

static int major = 0;
struct cdev cdev;

static int  my_die_event_handler(struct notifier_block *self, unsigned long val, void *data)
{
    struct die_args *args = (struct die_args *)data;

    if(val == 1){
        printk("my_die_evnet: oops, pc = 0x%lx\n", args->regs->uregs[15]);
    }
    else
        printk("this is rocklee!, val = %ld\n", val);
    
    return 0;
}

static int  my_net_event_handler(struct notifier_block *self, unsigned long val, void *data)
{
        printk("my net event, val is %ld, interface is %s ", val, ((struct net_device *)data)->name);
    return 0;
}

static struct notifier_block my_die_notifier = {
    .notifier_call = my_die_event_handler,
};

static struct notifier_block my_net_notifier = {
    .notifier_call = my_net_event_handler,
};

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

static BLOCKING_NOTIFIER_HEAD(my_noti_chain);

int my_event_handler(struct notifier_block *self, unsigned long val, void *data)
{
    printk("my_event: val = %ld\n", val);

    return 0;
}

static struct notifier_block my_notifier = {
    .notifier_call = my_event_handler,
};

static int __init test_init(void)
{
    //当major为0时，会自动分配可用的major
    int err = register_chrdev(major, "test", &fops);
    if(err <= 0){
        printk("register_chrdev failed!\n");
        goto reg;
    }

    major = err;
    printk("rock lee major is %d\n", major);

    cdev_init(&cdev, &fops);

    printk("rock lee 2major is %d\n", major);

    err = cdev_add(&cdev, MKDEV(major, 0), 1);
    if(err){
        printk("cdev_add failed!\n");
        goto add;
    }

    register_die_notifier(&my_die_notifier);
    register_netdevice_notifier(&my_net_notifier);
    blocking_notifier_chain_register(&my_noti_chain, &my_notifier);

    return 0;
add:
    unregister_chrdev(major, "test");
reg:
    return -1;
}

static void __exit test_exit(void)
{
    unregister_chrdev(major, "test");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
