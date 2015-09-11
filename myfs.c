#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dcache.h>

#define MYFS_MAGIC 0xabcd
#define FILE_INODE_NUMBER 2

static struct file_operations const myfs_dir_ops; 
static void myfs_put_super(struct super_block *sb)
{
	pr_debug("myfs super block destroyed\n");
}

static struct inode_operations const myfs_inode_ops = {
#if 0
	.lookup = myfs_lookup
#endif
};

static struct super_operations const myfs_super_ops = {
	.put_super = myfs_put_super
};

static int myfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *root = NULL;

	/* Fill the superblock */
	sb->s_magic = MYFS_MAGIC;
	sb->s_op = &myfs_super_ops;

	root = new_inode(sb);
	if (!root) {
		pr_err("inode allocation failed\n");
		return -ENOMEM;
	}

	root->i_ino = 1;
	root->i_sb = sb;
	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
	root->i_op = &myfs_inode_ops;
	root->i_fop = &myfs_dir_ops;
	inode_init_owner(root, NULL, S_IFDIR);

	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		pr_err("root creation failed\n");
		return -ENOMEM;
	}
	return 0;
}


static struct dentry *myfs_mount(struct file_system_type *type, int flags,
				 char const *dev, void *data)
{
	/* mount_bdev will call myfs_fill_sb to fill superblock. This
	 * function will return dentry of fs root node
	 */
	struct dentry *const entry = mount_bdev(type, flags, dev, data,
						myfs_fill_sb);
	if (IS_ERR(entry))
		pr_err("myfs mounting failed\n");
	else
		pr_debug("myfs mounted\n");
	return entry;
}

static struct file_system_type myfs_type = {
	.owner = THIS_MODULE,
	.name = "myfs",
	.mount = myfs_mount,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};

static int myfs_readdir(struct file *file,  struct dir_context *ctx)
{
	struct dentry *de = file->f_dentry;
	
	pr_debug("myfs: file_operations.readdir called\n");
	if(file->f_pos > 0)
		return 1;
	if(ctx->actor(ctx, ".", 1, file->f_pos++, de->d_inode->i_ino, DT_DIR) ||
	   (ctx->actor(ctx, "..", 2, file->f_pos++, de->d_parent->d_inode->i_ino, DT_DIR)))
		return 0;
	if(ctx->actor(ctx, "hello.txt", 9, file->f_pos++, FILE_INODE_NUMBER, DT_REG))
		return 0;
	return 1;
}

static struct file_operations const myfs_dir_ops = {
	.iterate = myfs_readdir
};

static int __init myfs_init(void)
{
	int ret = 0;
	ret = register_filesystem(&myfs_type);
	if (ret != 0) {
		pr_err("cannot register filesystem\n");
		return ret;
	}
	pr_debug("myfs module loaded\n");
	return 0;
}
static void __exit myfs_fini(void)
{
	int const ret = unregister_filesystem(&myfs_type);
	if (ret != 0)
		pr_err("cannot unregister filesystem\n");
	pr_debug("myfs module unloaded\n");
}

module_init(myfs_init);
module_exit(myfs_fini);
