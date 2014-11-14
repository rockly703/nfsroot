#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/hdreg.h>

#define RAMHD_NAME "ramhd"
#define RAMHD_MAX_DEVICE 2
#define RAMHD_MAX_PARTITIONS 4

#define RAMHD_SECTOR_SIZE 512
#define RAMHD_SECTORS 16
#define RAMHD_HEADS 4 
#define RAMHD_CYLINDERS 256

#define RAMHD_SECTORS_TOTAL (RAMHD_SECTORS * RAMHD_HEADS * RAMHD_CYLINDERS)
#define RAMHD_SIZE (RAMHD_SECTORS_TOTAL * RAMHD_SECTOR_SIZE) 

typedef struct {
    unsigned char *data;
	spinlock_t lock; 
    struct request_queue *queue;
    struct gendisk *gd;
}ramhd_dev_t;
    
static ramhd_dev_t *rdev[RAMHD_MAX_DEVICE] = {NULL};
static dev_t ramhd_major;

static int alloc_ramdev(void)
{
    int i;
    
    for (i = 0; i < RAMHD_MAX_DEVICE; i++) {
        rdev[i] = kzalloc(sizeof(ramhd_dev_t), GFP_KERNEL);
        if (!rdev[i]) {
            goto release_rdev;
        }
        
        rdev[i]->data = vmalloc(RAMHD_SIZE);
        if (!rdev[i]->data) {
            vfree(rdev[i]);
            goto release_rdev;
        }
    }
    return 0; 

release_rdev:
    for (i--; i >= 0; i--) {
        vfree(rdev[i]->data);
        kfree(rdev[i]);
    }
    
    return -ENOMEM;
} 

static int ramhd_open(struct inode *inode, struct file *filp)
{
	printk("#### in func %s ###\n", __func__);
    return 0;
}

static int ramhd_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static int ramhd_ioctl(struct inode *inode, struct file *filp, unsigned cmd, unsigned long arg)
{
    int err;
    struct hd_geometry geo;
    struct block_device *bd = I_BDEV(inode);
    
    switch (cmd) {
    case  HDIO_GETGEO:
        err = !access_ok(VERIFY_WRITE, arg, sizeof(geo));
        if (err)
            return -EFAULT;
        
        geo.cylinders = RAMHD_CYLINDERS;
        geo.heads = RAMHD_HEADS;
        geo.sectors = RAMHD_SECTORS;
        geo.start = get_start_sect(bd);
        
        if (copy_to_user((void *)arg, &geo, sizeof(geo))) 
            return -EFAULT;
        
        return 0;
    }
    
    return -ENOTTY;
}

static struct block_device_operations ramhd_fops = {
    .owner = THIS_MODULE,
    .open = ramhd_open,
    .release = ramhd_release,
    .ioctl = ramhd_ioctl,
};

static void clean_ramdev(void)
{
    int i;
    
    for (i = 0; i < RAMHD_MAX_DEVICE; i++) {
        if (rdev[i]) 
            vfree(rdev[i]->data);
            kfree(rdev[i]);
    }
    
}

void ramhd_req_func(struct request_queue *q)
{
	struct request *req;
	ramhd_dev_t *pdev;
	char *pdata;
	unsigned long addr, size, start;
	req = elv_next_request(q);

	while(req){
		start = req->sector;
		pdev = (ramhd_dev_t *)req->rq_disk->private_data;
		pdata = pdev->data;
		addr = (unsigned long)pdata + start * RAMHD_SECTOR_SIZE;
		size = req->data_len;
		if(rq_data_dir(req) == READ)
			memcpy(req->buffer, (char *)addr, size);
		else
			memcpy((char *)addr,req->buffer, size);

		end_request(req, 0); 
	}
}
   
static int __init ramhd_init(void)
{
    int i; 
    
    alloc_ramdev();
   
    //一个主设备号,故只注册一次
    ramhd_major = register_blkdev(0, RAMHD_NAME); 
    for (i = 0; i < RAMHD_MAX_DEVICE; i++) {
		spin_lock_init(&rdev[i]->lock);
        rdev[i]->queue = blk_init_queue(ramhd_req_func, &rdev[i]->lock); 
       
        //在/sys/block/下生成目录,给次分区分配空间
        blk_queue_hardsect_size(rdev[i]->queue, 512);
        rdev[i]->queue->queuedata = rdev[i];
        rdev[i]->gd = alloc_disk(RAMHD_MAX_PARTITIONS);
        if (!rdev[i]->gd) {
            printk (KERN_NOTICE "alloc_disk failure\n");
		    goto out_vfree;
        }
        rdev[i]->gd->major = ramhd_major;
        rdev[i]->gd->first_minor = i * RAMHD_MAX_PARTITIONS; 

        snprintf(rdev[i]->gd->disk_name, strlen(RAMHD_NAME) + 2, "%s%c", RAMHD_NAME, 'a' + i);
 		
        rdev[i]->gd->fops = &ramhd_fops;
        rdev[i]->gd->queue = rdev[i]->queue;
        rdev[i]->gd->private_data = rdev[i];
        rdev[i]->gd->capacity = RAMHD_SIZE;
        rdev[i]->gd->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
        set_capacity(rdev[i]->gd, RAMHD_SECTORS_TOTAL); 
        add_disk(rdev[i]->gd); 
    }
    
    return 0;
out_vfree:
	if (rdev[i]->data)
		vfree(rdev[i]->data);
    
    return -ENOMEM;
}

static void __exit ramhd_exit(void)
{
    int i;
    
    for (i = 0; i < RAMHD_MAX_DEVICE; i++) {
        del_gendisk(rdev[i]->gd);
        put_disk(rdev[i]->gd);
        blk_cleanup_queue(rdev[i]->queue);
        vfree(rdev[i]->data);
        kfree(rdev[i]);
    }
    
    unregister_blkdev(ramhd_major, RAMHD_NAME);
}

module_init(ramhd_init);
module_exit(ramhd_exit);

MODULE_AUTHOR("Rock Lee");
MODULE_LICENSE("GPL");
