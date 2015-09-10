#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dcache.h>

static void myfs_put_super(struct super_block *sb)
{
	pr_debug("myfs super block destroyed\n");
}

static struct super_operations const myfs_super_ops = {
	.put_super = myfs_put_super
};

static int myfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *root = NULL;

	/* Fill the superblock */
	sb->s_magic = 1207;
	sb->s_op = &myfs_super_ops;

	root = new_inode(sb);
	if (!root) {
		pr_err("inode allocation failed\n");
		return -ENOMEM;
	}

	root->i_ino = 0;
	root->i_sb = sb;
	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
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
	struct dentry *const entry = mount_bdev(type, flags, dev,
						data, myfs_fill_sb);
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
