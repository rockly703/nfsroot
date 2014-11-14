#ifndef __RLFS_H__
#define __RLFS_H__

extern const struct super_operations rlfs_sb_ops;
extern const struct inode_operations rlfs_link_ops; 
extern const struct file_operations rlfs_file_ops; 
extern const struct inode_operations rlfs_dir_inode_ops; 
extern const struct file_operations rlfs_dir_ops; 

#endif 
