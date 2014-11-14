#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/module.h>
#include <linux/anon_inodes.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/magic.h>

#define ROCK_MAGIC 0x1ee57a1e
#define ROCK_NAME "rocklee"

#define ANON_ERROR(fmt, ...) printk("anon_inodefs error:"fmt, ##__VA_ARGS__)
#define ANON_INFO(fmt, ...) printk("anon_inodefs info:"fmt, ##__VA_ARGS__)

#define DEBUGFS_ERROR(fmt, ...) printk("debugfs error:"fmt, ##__VA_ARGS__)
#define DEBUGFS_INFO(fmt, ...) printk("debugfs info:"fmt, ##__VA_ARGS__)

static struct dentry *dir_a;
static struct dentry *dir_b1, *dir_b2;
static struct dentry *dir_c1, *dir_c2; 

static int test_anon_inodefs(void)
{
    int test_priv = ROCK_MAGIC;
    const char *name = ROCK_NAME;
    int flags = 0;
    struct file *fp = anon_inode_getfile(name, (const struct file_operations *)&simple_dir_operations,
                                          (void *)&test_priv, flags);
    
    if (!fp->f_dentry) {
         ANON_ERROR("f_dentry is NULL.\n");
         return -1;
    }
    
    if (strcmp(name, fp->f_dentry->d_name.name)) {
        ANON_ERROR("filename does not match.\n"); 
        return -1;
    }
    ANON_INFO("file name is %s\n", fp->f_dentry->d_name.name);
    
    if (fp->f_vfsmnt->mnt_sb != fp->f_dentry->d_sb) {
        ANON_ERROR("f_dentry is NULL.\n");
        return -1;
    }
    ANON_INFO("anon_inodefs magic is 0x%x, current fs magic is 0x%lx\n",
               ANON_INODE_FS_MAGIC, fp->f_dentry->d_sb->s_magic); 
    
    if ((FMODE_READ | FMODE_WRITE) != fp->f_mode) {
        ANON_ERROR("fmode does not match.\n");
        return -1;
    }
    
    if (fp->f_op != &simple_dir_operations) {
        ANON_ERROR("f_op does not match.\n");
        return -1;
    }
    
    if (fp->f_mapping != fp->f_dentry->d_inode->i_mapping) {
        ANON_ERROR("mapping does not match.\n");
        return -1;
    }
    
    if (fp->f_pos != 0) {
        ANON_ERROR("pos does not match.\n");
        return -1;
    }
    
    if (fp->f_flags != (O_RDWR | (flags & O_NONBLOCK))) {
        ANON_ERROR("anon_inodefs:f_flags does not match.\n");
        return -1;
    }
    
    if (fp->f_version != 0) {
        ANON_ERROR("version does not match.\n");
        return -1;
    }
    
    if (*(int *)fp->private_data != test_priv) {
        ANON_ERROR("priavate data does not match.\n");
        return -1;
    }
    ANON_INFO("private data is 0x%x\n", *(int *)fp->private_data);
    
    return 0;
}

void debugfs_tranverse(struct dentry *root) 
{
    struct dentry *parent = root;
    struct dentry *dentry = NULL; 
    struct list_head *dir;

    if (root) {
        printk("### root name is %s ###\n", root->d_name.name);
    } else {
        printk("root is null \n");
        return;
    }

repeat:
    printk("### file %s, func %s, line %d ###\n", __FILE__, __func__, __LINE__); 
    dir = parent->d_subdirs.next;
    printk("### file %s, func %s, line %d ###\n", __FILE__, __func__, __LINE__);
resume:
    printk("### file %s, func %s, line %d ###\n", __FILE__, __func__, __LINE__);
    while (dir != &parent->d_subdirs) {
    printk("### file %s, func %s, line %d ###\n", __FILE__, __func__, __LINE__);
        dentry = list_entry(dir, struct dentry, d_u.d_child);
        dir = dir->next;
        if (!list_empty(&dentry->d_subdirs)) {
            parent = dentry;
            goto repeat;
        }

        printk("#### func %s, dir name is %s , d_count %d, i_count %d ###\n",
                __func__, dentry->d_name.name, dentry->d_count, dentry->d_inode->i_count);
    }
    printk("### file %s, func %s, line %d ###\n", __FILE__, __func__, __LINE__);

    /*printk("#### func %s, dir name is %s , d_count %d, i_count %d ###\n",*/
                            /*__func__, dentry->d_name.name, dentry->d_count, dentry->d_inode->i_count);*/
    dentry = list_entry(dir, struct dentry, d_subdirs);
    printk("#### func %s, dir name is %s ###\n",  
           __func__, dentry->d_name.name); 

    if (dentry->d_inode)
        printk("has a inode\n");
    else
        printk("has no inode\n");
    
    if (parent != root) {
        dir = parent->d_u.d_child.next;
        parent = parent->d_parent;
        goto resume;
    }
}

static int create_test_tree(void) 
{
    dir_a = debugfs_create_dir("dir_a", NULL);
    if (IS_ERR(dir_a)) {
        DEBUGFS_ERROR("create dir_a failed.\n");
        goto err;
    }
    
    dir_b1 = debugfs_create_dir("dir_b1", dir_a);
    if (IS_ERR(dir_b1)) {
        DEBUGFS_ERROR("create dir_b1 failed.\n");
        goto release_a;
    }
    
    dir_b2 = debugfs_create_dir("dir_b2", dir_a);
    if (IS_ERR(dir_b2)) {
        DEBUGFS_ERROR("create dir_b2 failed.\n");
        goto release_b1;
    }
    
    dir_c1 = debugfs_create_dir("dir_c1", dir_b1);
    if (IS_ERR(dir_c1)) {
        DEBUGFS_ERROR("create dir_c1 failed.\n");
        goto release_b2;
    }
    
    dir_c2 = debugfs_create_dir("dir_c2", dir_b1);
    if (IS_ERR(dir_c1)) {
        DEBUGFS_ERROR("create dir_c2 failed.\n");
        goto release_c1;
    }
    return 0;

release_c1:
    debugfs_remove(dir_c1);
release_b2:
    debugfs_remove(dir_b2);
release_b1:
    debugfs_remove(dir_b1);
release_a:
    debugfs_remove(dir_a);
err:
    return -1;
}

static int test_debugfs(void)
{
    int err;
    struct dentry *root;
    err = create_test_tree(); 
    if (err) 
        return err;
    
    root = dir_a->d_sb->s_root;
    /*debugfs_tranverse(dir_a);*/
    debugfs_tranverse(dir_a);
#if 0
    debugfs_remove_recursive(dir_a);
    printk("########### debugfs_remove_recursive ##########\n");
    debugfs_tranverse(dir_a); 
#endif
    
#if 0
    debugfs_remove(dir_c1);
    debugfs_remove(dir_c2);
    debugfs_remove(dir_b1); 
    debugfs_remove(dir_b2); 
#endif    
    debugfs_remove_recursive(dir_a); 
    printk("########## debugfs_remove ###########\n");
    debugfs_tranverse(dir_a);
#if 0 
    debugfs_remove_old(dir_c1);
    printk("######### debugfs_remove_old ############\n");
    debugfs_tranverse(dir_a); 
    
    debugfs_remove_old(dir_c1);
    printk("######### debugfs_remove_old 2 ############\n");
    debugfs_tranverse(dir_a); 
#endif
    
    return err;
}

static int __init anon_debug_init(void)
{
    int err;

    err = test_anon_inodefs();
    if (err) {
        printk("anon_inodefs test: get file failed.\n");
        return -1;
    }
    printk("anon_inodefs test: get file successfully.\n"); 
    
    err = test_debugfs();
    if (err) {
        printk("debugfs test: debugfs_remove_recursive failed.\n"); 
        return -1;
    }
    printk("debugfs test: debugfs_remove_recursive successfully.\n"); 
    
    return 0;
}

static void __exit anon_debug_exit(void)
{
    
}

module_init(anon_debug_init);
module_exit(anon_debug_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rock Lee");
