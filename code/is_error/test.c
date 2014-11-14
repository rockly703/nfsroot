#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <asm-generic/errno-base.h>


static int __init test_init(void)
{
    char *p = NULL;

    if(IS_ERR(p))
        printk("hello world!\n");
    else
        printk("error code is %ld\n", PTR_ERR(p));

    return 0;
}

static void __exit test_exit(void)
{
    printk("bye world!\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_AUTHOR("ROCKLEE");
MODULE_LICENSE("GPL");
