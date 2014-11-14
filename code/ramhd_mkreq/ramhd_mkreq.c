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

static int ramhd_make_request(request_queue_t *q, struct bio *bio)
{   
    char *kbuffer;
    char *ubuffer;
    struct bio_vec *bvec;
    int i;
    int err = 0;
    
    struct block_device *bdev = bio->bi_bdev;
    ramhd_dev_t *pdev = bdev->bd_disk->private_data;
    
    if (bio->bi_sector * RAMHD_SECTOR_SIZE + bio->bi_size > RAMHD_SIZE) {
        err = -EIO;
        goto OUT_OF_RANGE;
    }
    
    kbuffer = pdev->data + bio->bi_sector * RAMHD_SECTOR_SIZE;
    
    bio_for_each_segment(bvec, bio, i){
        ubuffer = kmap(bvec->bv_page) + bvec->bv_offset;
        switch (bio_data_dir(bio)) {
        case READ:
            memcpy(ubuffer, kbuffer, bvec->bv_len);
            flush_dcache_page(bvec->bv_page);
            break;
        case WRITE:
            flush_dcache_page(bvec->bv_page);
            memcpy(kbuffer, ubuffer, bvec->bv_len);
            break;
        default:
            kunmap(bvec->bv_page);
            goto OUT_OF_RANGE;
        }
        kunmap(bvec->bv_page);
        kbuffer += bvec->bv_len;
    }

OUT_OF_RANGE:
    bio_endio(bio, bio->bi_size, err);
    return 0;
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
   
static int __init ramhd_init(void)
{
    int i;
    dev_t ramhd_major_test;
    
    alloc_ramdev();
   
    //一个主设备号,故只注册一次
    ramhd_major = register_blkdev(0, RAMHD_NAME); 
    for (i = 0; i < RAMHD_MAX_DEVICE; i++) {
		//采用make request方式,需要alloc a queue
        rdev[i]->queue = blk_alloc_queue(GFP_KERNEL);
        blk_queue_make_request(rdev[i]->queue, ramhd_make_request);
        
        //在/sys/block/下生成目录,给次分区分配空间
        rdev[i]->gd = alloc_disk(RAMHD_MAX_PARTITIONS);
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
