#include <linux/types.h>
#include <linux/fs.h>
#include <asm-generic/errno-base.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <asm/uaccess.h>

#include "rlfs.h"

struct rlfs_buffer {
    size_t size;
    loff_t off;
    char *page;
}; 

int rlfs_open(struct inode *inode, struct file *file) 
{
    struct rlfs_buffer *buffer;
    
    if (file->f_mode & FMODE_PREAD) {
        if (!((inode->i_mode) & S_IRUGO)) 
            return -EACCES;
    }
    if (file->f_mode & FMODE_PWRITE) {
        if (!((inode->i_mode) & S_IWUGO)) 
            return -EACCES;
    }
    if (inode->i_private) 
        return 0;
    
    buffer = kzalloc(sizeof(struct rlfs_buffer), GFP_KERNEL); 
    if (!buffer) 
        return -ENOMEM;
   
    inode->i_private = buffer; 
    return 0;
}

ssize_t rlfs_read(struct file *file, char __user *userp, size_t size, loff_t *off) 
{
    size_t retval;
    struct inode *inode = file->f_dentry->d_inode; 
    struct rlfs_buffer *buffer = inode->i_private;
    
    if (!buffer) 
        return -EFAULT;
    if (*off < 0) 
        return -EINVAL;
    if (*off >= buffer->size || size == 0) 
        return 0;
    if (*off + size > buffer->size) 
        size = buffer->size - *off;
    
    retval = copy_to_user(userp, buffer->page + *off, size);
    if (retval == size) 
        return -EINVAL; 
   
    retval = size - retval;
    *off += retval;
    return retval;
}

ssize_t rlfs_write(struct file *file, const char __user *userp, size_t size, loff_t *off) 
{
    size_t retval;
    struct inode *inode = file->f_dentry->d_inode; 
    struct rlfs_buffer *buffer = inode->i_private;
    
    if (!buffer) 
        return -EFAULT;
    if (!buffer->page) 
        buffer->page = (char *)get_zeroed_page(GFP_KERNEL);
    if (!buffer->page) 
        return -ENOMEM;
    
    if (*off < 0) 
        return -EINVAL;
    if (*off > PAGE_SIZE || size == 0) 
        return 0;
    if (*off + size > PAGE_SIZE) 
        size = PAGE_SIZE - *off;
    
    retval = copy_from_user(buffer->page, userp, size);
    if (retval == size) 
        return -EINVAL; 
    
    retval = size - retval;
    buffer->size = retval;
    *off += retval;
    return retval; 
}

const struct super_operations rlfs_sb_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

const struct inode_operations rlfs_link_ops = {

};

const struct file_operations rlfs_file_ops = {
    .open = rlfs_open,
    .read = rlfs_read,
    .write = rlfs_write,
};

const struct inode_operations rlfs_dir_inode_ops = {
    .lookup = simple_lookup,
};

const struct file_operations rlfs_dir_ops = {

}; 
