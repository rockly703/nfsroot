#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/hdreg.h>

#define RD_NAME "rockhd"
#define MAX_DEV 4
#define MAX_PART 2
#define HEADS 4
#define SECTOR_SIZE 512
#define SECTORS 16
#define CYLINDERS 256
#define RD_TOTAL (CYLINDERS * HEADS * SECTORS)
#define RD_SIZE (RD_TOTAL * SECTOR_SIZE)

static int major = 0;

static struct rock_disk {
    char *data;
    struct request_queue *queue;
    struct gendisk *gd;
};

static struct rock_disk *rd = NULL; 

static void unset_device(struct rock_disk *rd, int num)
{
    vfree(rd);
}

static int rock_open(struct block_device *device, fmode_t mode)
{
    return 0;
}

static int rock_release(struct gendisk *gd, fmode_t mode)
{
    return 0;
}

static int rock_ioctl(struct block_device *device, fmode_t mode, unsigned cmd, unsigned long args)
{
	struct hd_geometry geo;

	switch(cmd) {
		case HDIO_GETGEO:
			geo.cylinders = CYLINDERS;
			geo.heads = HEADS;
			geo.sectors = SECTORS;
			geo.start = get_start_sect(device);
			if(copy_to_user((void *)args, &geo, sizeof(struct hd_geometry))) 
				return -EACCES;
			break;
		default:
			return -EINVAL;
	}

    return 0;
}

static struct block_device_operations rock_fops = {
    .owner = THIS_MODULE,
    .open = rock_open,
    .release = rock_release,
    .ioctl = rock_ioctl,
};

static int rock_make_request(struct request_queue *q, struct bio *bio)
{
    struct rock_disk *rd = q->queuedata; 
    struct bio_vec *bvec;
    char *rd_buffer;
    char *vec_buffer;
    int err = 0;
	int i;

    if(bio->bi_sector * SECTOR_SIZE + bio->bi_size > RD_SIZE) {
		printk("io error.\n");
        err = -EIO;
        goto out;
    }
    
    rd_buffer = rd->data + (bio->bi_sector * SECTOR_SIZE);

    bio_for_each_segment(bvec, bio, i) {
        vec_buffer = kmap(bvec->bv_page) + bvec->bv_offset;
		
        switch(bio_data_dir(bio)) {
            case READ:
				memcpy(vec_buffer, rd_buffer, bvec->bv_len);
				break;
            case WRITE:
                memcpy(rd_buffer, vec_buffer, bvec->bv_len);
				break;
            default:
                goto unmap;

        }
		
        kunmap(bvec->bv_page);
        rd_buffer += bvec->bv_len;
    }

unmap:
    kunmap(bvec->bv_page);
out:
    bio_endio(bio, err);
    return err;
}

static int setup_device(struct rock_disk *rd, int num)
{
    memset(rd, 0, sizeof(struct rock_disk));

    rd->data = vmalloc(RD_SIZE);
    if(!rd->data) {
        return -ENOMEM;
    }

    rd->queue = blk_alloc_queue(GFP_KERNEL);
    if(!rd->queue)
        goto out_vfree;

    rd->queue->queuedata = rd;
    blk_queue_make_request(rd->queue, rock_make_request);

    rd->gd = alloc_disk(MAX_PART);
    rd->gd->major = major;
    rd->gd->first_minor = num * MAX_PART;
    rd->gd->minors = MAX_PART;
    rd->gd->fops = &rock_fops;
    rd->gd->queue = rd->queue;
    rd->gd->private_data = rd;
    snprintf(rd->gd->disk_name, 32,  "%s%c", RD_NAME, 'a' + num);

	//do not show partition info in /dev/partitions
	rd->gd->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO; 
    set_capacity(rd->gd, RD_TOTAL);
    add_disk(rd->gd);

    return 0;

out_vfree:
    vfree(rd->data);
    return -ENOMEM;
}

static int __init rd_init(void)
{
    int i;
    int err;

    major = register_blkdev(major, RD_NAME);
    if(major <= 0) {
        printk(KERN_WARNING "rock_disk: unable to get major number!\n");
        return -EBUSY;
    }
    
    rd = kmalloc(sizeof(struct rock_disk) * MAX_DEV, GFP_KERNEL);
    if(rd == NULL) {
		printk("kmalloc failed.\n");
        goto out_register;
    }

    for(i = 0; i < MAX_DEV; i++) {
        err = setup_device(rd + i, i);
        
        if(err) {
			printk("err is 0x%x.\n", err);
			goto unset;
		}
           
    }

    return 0;

unset:
    for(--i; i >= 0; i--)
        unset_device(rd + i, i);
out_register:
    unregister_blkdev(major, RD_NAME);
    return -ENOMEM;
}

static void __exit rd_exit(void)
{
	int i;
	for(i = 0; i < MAX_DEV; i++) {
		del_gendisk((rd + i)->gd);
		vfree((rd + i)->data);	
	}
	
	kfree(rd);
    unregister_blkdev(major, RD_NAME);
}

module_init(rd_init);
module_exit(rd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rock Lee");
