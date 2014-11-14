#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>
#include <linux/user_namespace.h>

static int __init test_init(void)
{
    int uid = current->nsproxy->user_ns->root_user->uid;
    printk("current uid is %d\n", uid);

    return 0;
}

static void __exit test_exit(void)
{
    /*unregister_chrdev(major, "test");*/
    return;
}

module_init(test_init);
module_exit(test_exit);
