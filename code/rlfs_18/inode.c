#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <asm-generic/errno-base.h>
#include <linux/stat.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include "rlfs.h"

#define RLFS_MAGIC 0x1ee57a1e
struct vfsmount *rl_mount;

static struct inode *rlfs_get_inode(struct super_block *sb, int mode, dev_t dev,
                                    void *data, const struct file_operations *fops)
{
    struct inode *inode = new_inode(sb);

    if (inode) {
        inode->i_mode = mode;
        inode->i_blocks = 0;
        inode->i_ctime = inode->i_mtime = inode->i_atime = CURRENT_TIME;

        switch (mode & S_IFMT) {
        case S_IFLNK:
            inode->i_op = &rlfs_link_ops;
            inode->i_fop = fops; 
            break;
        case S_IFREG:
            inode->i_fop = fops ? fops : &rlfs_file_ops;
            break;
        case S_IFDIR:
            inode->i_op = &rlfs_dir_inode_ops;
            //inode->i_fop = &rlfs_dir_ops;
            inode->i_fop = &simple_dir_operations; 
            //目录的i_nlink从2开始
            inode->i_nlink++;
            break;
        default:
            init_special_inode(inode, mode, dev);
            break;
        }
    }

    return inode;
}

static int rl_mknod(struct inode *inode, struct dentry *dentry, mode_t mode,
                    dev_t dev, void *data, const struct file_operations *fops) 
{
    int err = -EPERM;
    struct super_block *sb = rl_mount->mnt_sb; 
    
    if (dentry->d_inode) 
        return -EEXIST;
    
    inode = rlfs_get_inode(sb, mode, dev, data, fops); 
    if (inode) {
        d_instantiate(dentry, inode);
        dget(dentry);
        err = 0;
    }
    
    return err;
}

static int rl_link(struct inode *inode, struct dentry *dentry, mode_t mode,
                    void *data, const struct file_operations *fops) 
{
    return 0;
}

static int rl_mkdir(struct inode *inode, struct dentry *dentry, mode_t mode,
                    void *data, const struct file_operations *fops)
{
    int err;
    
    mode = (mode & (S_IRWXUGO | S_ISVTX)) | S_IFDIR;
    err = rl_mknod(inode, dentry, mode, 0, data, fops);
    if (!err)
        //增加父目录的i_nlink
         inode->i_nlink++;
        
    return err;
}

static int rl_create(struct inode *inode, struct dentry *dentry, mode_t mode,
                     void *data, const struct file_operations *fops)
{
    int err;
    
    mode = (mode & S_IALLUGO) | S_IFREG;
    err = rl_mknod(inode, dentry, mode, 0, data, fops); 
    
    return err;
}

static int rl_create_by_name(const char *name, mode_t mode,
                              struct dentry *parent, struct dentry **dentry,
                              void *data, const struct file_operations *fops)
{
    int err;
    
    if (!parent) {
        if (rl_mount && rl_mount->mnt_sb) {
            parent = rl_mount->mnt_sb->s_root;  
        }
    }
    
    if (!parent) {
        printk("Ah! Cant't find a parent.\n");
        return -EFAULT;
    }
    
    *dentry = NULL;
    mutex_lock(&parent->d_inode->i_mutex); 
    *dentry = lookup_one_len(name, parent, strlen(name));
    if (!IS_ERR(*dentry)) {
        switch (mode & S_IFMT) {
        case S_IFDIR:
            err = rl_mkdir(parent->d_inode, *dentry, mode, data, fops);
            break;
        case S_IFLNK:
            err = rl_link(parent->d_inode, *dentry, mode, data, fops); 
            break;
        default:
            err = rl_create(parent->d_inode, *dentry, mode, data, fops);
            break; 
        }
    } else 
        err = PTR_ERR(*dentry);
    mutex_unlock(&parent->d_inode->i_mutex);
    
    return err; 
}

void rl_remove_file(struct dentry *dentry)
{
    
}

struct dentry *rl_create_file(const char *name, mode_t mode, struct dentry *parent,
                               void *data, const struct file_operations *fops)
{
    int err;
    struct dentry *dentry = NULL;
    
    err = rl_create_by_name(name, mode, parent, &dentry, data, fops);
    return dentry;
}

struct dentry *rl_create_dir(const char * name, struct dentry *parent)
{
    return rl_create_file(name, S_IFDIR | S_IRWXU | S_IRUGO | S_IXUGO,
                          parent, NULL, NULL);
}

static int rlfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *inode = NULL;
	struct dentry *dentry;
	
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = RLFS_MAGIC;
	sb->s_op = &rlfs_sb_ops;

	inode = rlfs_get_inode(sb, S_IFDIR | 0755, 0, NULL, NULL);
	if(!inode)
		return -ENOMEM;

	dentry = d_alloc_root(inode);
	if(!dentry) {
		iput(inode);
		return -ENOMEM;
	}

	sb->s_root = dentry;
	return 0;
}

static int  rlfs_get_sb(struct file_system_type *fs_type, int fs_flag,
		       const char *fs_name, void *fs_data, struct vfsmount *fs_mnt)
{
	return get_sb_single(fs_type, fs_flag, fs_data, rlfs_fill_sb, fs_mnt);
}

static void rlfs_kill_sb(struct super_block *sb)
{

}

static struct file_system_type rlfs_type = {
	.name = "rlfs",
	.get_sb = rlfs_get_sb,
	.kill_sb = rlfs_kill_sb,
	.owner = THIS_MODULE,
};

struct dentry *test_file;

static int __init rlfs_init(void)
{
	register_filesystem(&rlfs_type);

    rl_mount = kern_mount(&rlfs_type);
    if (IS_ERR(rl_mount))
        goto unreg;
    
    test_file = rl_create_file("rocklee", 0755, NULL, NULL, NULL);

    return 0;
unreg:
	unregister_filesystem(&rlfs_type);
    return -1;
}

static void __exit rlfs_exit(void)
{
    rl_remove_file(test_file);
	unregister_filesystem(&rlfs_type);
}

module_init(rlfs_init);
module_exit(rlfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rock Lee");
