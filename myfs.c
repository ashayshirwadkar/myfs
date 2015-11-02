#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/string.h>

#define MYFS_MAGIC 0xabcd
#define ROOT_INO 1
#define FILE_INO 2

static struct file_operations const myfs_dir_ops;
static struct inode_operations const myfs_inode_ops;
static struct file_operations const myfs_file_ops;

static void myfs_put_super(struct super_block *sb)
{
	pr_debug("myfs super block destroyed\n");
}

static struct dentry *myfs_lookup(struct inode *dir, struct dentry *dentry,
				  unsigned int flags)
{
	struct inode *file_inode = NULL;

	if (dir->i_ino != ROOT_INO ||
	    strlen("hello.txt") != dentry->d_name.len ||
	    strcmp(dentry->d_name.name, "hello.txt"))
		return ERR_PTR(-ENOENT);
	/* Allocate an inode object */
	file_inode = iget_locked(dir->i_sb, FILE_INO);
	if (!file_inode)
		return ERR_PTR(-ENOMEM);
	file_inode->i_size = 0; /* File_size */
	file_inode->i_mode = S_IFREG|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
	file_inode->i_fop = &myfs_file_ops;
	/* Add the inode to the dentry object */
	d_add(dentry, file_inode);
	pr_debug("myfs: inode_operations.lookup called with dentry %s.\n",
		 dentry->d_name.name);
	return NULL;
}

static struct super_operations const myfs_super_ops = {
	.put_super = myfs_put_super
};

static int myfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *root = NULL;

	pr_debug("myfs: fill_super called\n");

	/* Fill the superblock */
	sb->s_magic = MYFS_MAGIC;
	sb->s_op = &myfs_super_ops;

	root = new_inode(sb);
	if (!root) {
		pr_err("inode allocation failed\n");
		return -ENOMEM;
	}

	root->i_ino = ROOT_INO;
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
	/*
	 * mount_bdev will call myfs_fill_sb to fill superblock. This
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
	struct dentry *de = file->f_path.dentry;

	pr_debug("myfs: file_operations.readdir called\n");

	/* This shows that directory structure is filled already */
	if (ctx->pos > 0)
		return 0;

	/* Fill the directory structure for the first time */
	dir_emit(ctx, ".", 1, de->d_inode->i_ino, DT_DIR);
	dir_emit(ctx, "..", 2, de->d_parent->d_inode->i_ino, DT_DIR);
	dir_emit(ctx, "hello.txt", 9, FILE_INO, DT_REG) ;
	ctx->pos = ctx->pos + 14;
	
	return 0;
}

static struct file_operations const myfs_dir_ops = {
	.iterate = myfs_readdir
};

static struct file_operations const myfs_file_ops = {
};

static struct inode_operations const myfs_inode_ops = {
	.lookup = myfs_lookup
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
