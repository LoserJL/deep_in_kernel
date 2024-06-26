#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>

#include <linux/kfifo.h>

#define DEMO_NAME "my_demo_dev"
static dev_t dev;
static struct cdev *demo_cdev;
static signed count = 1;
static struct class *my_class;

DEFINE_KFIFO(mydemo_fifo, char, 64);

static int demodrv_open(struct inode *inode, struct file *file)
{
	int major = MAJOR(inode->i_rdev);
	int minor = MINOR(inode->i_rdev);

	printk("%s: major = %d, minor = %d\n", __func__, major, minor);

	return 0;
}

static int demodrv_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t
demodrv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int actual_readed;
	int ret;

	if (kfifo_is_empty(&mydemo_fifo)) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
	}

	ret = kfifo_to_user(&mydemo_fifo, buf, count, &actual_readed);
	if (ret)
	return -EIO;

	printk("%s, actual_readed = %d, pos = %lld\n", __func__, actual_readed,
			*ppos);

	return actual_readed;
}

static ssize_t
demodrv_write(struct file *file, const char __user *buf, size_t count,
				loff_t *ppos)
{
	unsigned int actual_write;
	int ret;

	if (kfifo_is_full(&mydemo_fifo)) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
	}

	ret = kfifo_from_user(&mydemo_fifo, buf, count, &actual_write);
	if (ret)
		return -EIO;

	printk("%s: actual_write = %d, ppos = %lld\n", __func__, actual_write,
			*ppos);

	return actual_write;
}

static const struct file_operations demodrv_fops = {
	.owner = THIS_MODULE,
	.open = demodrv_open,
	.release = demodrv_release,
	.read = demodrv_read,
	.write = demodrv_write
};

static int __init my_demo_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&dev, 0, count, DEMO_NAME);
	if (ret) {
		printk("failed to allocate char device region");
		return ret;
	}

	demo_cdev = cdev_alloc();
	if (!demo_cdev) {
		printk("cdev_alloc failed\n");
		return ret;
	}

	cdev_init(demo_cdev, &demodrv_fops);

	ret = cdev_add(demo_cdev, dev, count);
	if (ret) {
		printk("cdev_add failed\n");
		goto cdev_fail;
	}

	printk("succeeded register char device: %s\n", DEMO_NAME);
	printk("Major number = %d, Minor number = %d\n",
			MAJOR(dev), MINOR(dev));

	my_class = class_create(THIS_MODULE, "my_class");
	if (IS_ERR(my_class)) {
		printk("Failed to create class\n");
		ret = PTR_ERR(my_class);
		goto class_fail;
	}

	device_create(my_class, NULL, dev, NULL, DEMO_NAME);

	return 0;

class_fail:
	cdev_del(demo_cdev);

cdev_fail:
	unregister_chrdev_region(dev, count);

	return ret;
}

static void __exit my_demo_exit(void)
{
	printk("removing device\n");

	if (my_class) {
		device_destroy(my_class, dev);
		class_destroy(my_class);
	}

	if (demo_cdev)
		cdev_del(demo_cdev);

	unregister_chrdev_region(dev, count);
}

module_init(my_demo_init);
module_exit(my_demo_exit);
MODULE_LICENSE("GPL v2");
