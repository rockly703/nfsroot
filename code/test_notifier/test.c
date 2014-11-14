/**
 * @file test.c
 * @brief 这段代码用来测试notifier_chain
 * @author rockly
 * @version 01
 * @date 2014-06-14
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>


static int __init test_init(void)
{
    char *a = 0;
    *a  = 2;

    return 0;
}

static void __exit test_exit(void)
{
    return;
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
