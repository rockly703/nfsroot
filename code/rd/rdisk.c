#include <linux/init.h>
#include <linux/module.h>
#include <linux/genhd.h>

#define NR_DISK 2
#define NR_PART 2

static struct gendisk *rdisk[NR_DISK];

static int __init rdisk_init(void)
{
    int i;
    
    for (i = 0; i < NR_DISK; i++) {
        rdisk[i] = alloc_disk(NR_PART);
        if (!rdisk[i]) 
            goto alloc_fail;
    }
    
    return 0;
alloc_fail:
    for (--i; i >= 0; i--) {
        put_disk(rdisk[i]);
    }
    
    return -1;
}

static void __exit rdisk_exit(void)
{
    
}
                        
subsys_initcall(rdisk_init);
module_init(rdisk_init);
module_exit(rdisk_exit);
MODULE_LICENSE("GPL");
