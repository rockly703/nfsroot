#include <linux/init.h> 
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

static struct kobject *parent = NULL;
static struct kobject *child = NULL;

static int __init xxx_init(void)
{
    parent = kobject_create_and_add("pa_obj", NULL);
    child = kobject_create_and_add("ca_obj", parent);

    static struct attribute cld_att = {
        .name = "rocklee",
        .mode = S_IRUGO | S_IWUSR,
    };

    sysfs_create_file(child, &cld_att);

    return 0;
}

static void __exit xxx_exit(void) 
{
    kobject_del(parent);
    kobject_del(child);
    return 0;
}

module_init(xxx_init);
module_exit(xxx_exit);

MODULE_LICENSE("GPL");
