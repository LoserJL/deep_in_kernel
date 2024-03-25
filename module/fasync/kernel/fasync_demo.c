#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/poll.h>

#include <linux/kfifo.h>

#define MYDEMO_FIFO_SIZE	64

struct mydemo_device {
	char name[64];
	struct device *dev;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
	struct kfifo mydemo_fifo;
	struct fasync_struct *fasync;
};

struct mydemo_private_data {
	struct mydemo_device *device;
	char name[64];
};

#define MYDEMO_MAX_DEVICES	8
static struct mydemo_device *mydemo_device[MYDEMO_MAX_DEVICES];

#define DEMO_NAME "my_demo_dev"
static dev_t dev;
static struct cdev *demo_cdev;
static struct class *my_class;

static int demodrv_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct mydemo_private_data *data;
	struct mydemo_device *device = mydemo_device[minor];

	printk("%s: major = %d, minor = %d, device = %s\n", __func__,
		MAJOR(inode->i_rdev), MINOR(inode->i_rdev), device->name);

	data = kmalloc(sizeof(struct mydemo_private_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	sprintf(data->name, "private_data_%d", minor);

	data->device = device;
	file->private_data = data;

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
	struct mydemo_private_data *data = file->private_data;
	struct mydemo_device *device = data->device;

	if (kfifo_is_empty(&device->mydemo_fifo)) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		printk("%s: pid = %d, going to sleep\n", __func__, current->pid);
		ret = wait_event_interruptible(device->read_queue, !kfifo_is_empty(&device->mydemo_fifo));

		if (ret)
			return ret;
	}

	ret = kfifo_to_user(&device->mydemo_fifo, buf, count, &actual_readed);
	if (ret)
	return -EIO;

	if (!kfifo_is_full(&device->mydemo_fifo)) {
		wake_up_interruptible(&device->write_queue);
		kill_fasync(&device->fasync, SIGIO, POLLOUT);
	}

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
	struct mydemo_private_data *data = file->private_data;
	struct mydemo_device *device = data->device;

	if (kfifo_is_full(&device->mydemo_fifo)) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		printk("%s: pid = %d, going to sleep\n", __func__, current->pid);
		ret = wait_event_interruptible(device->write_queue, !kfifo_is_full(&device->mydemo_fifo));
		if (ret)
			return ret;
	}

	ret = kfifo_from_user(&device->mydemo_fifo, buf, count, &actual_write);
	if (ret)
		return -EIO;

	if (!kfifo_is_empty(&device->mydemo_fifo)) {
		wake_up_interruptible(&device->read_queue);
		kill_fasync(&device->fasync, SIGIO, POLLIN);
	}

	printk("%s: actual_write = %d, ppos = %lld\n", __func__, actual_write,
			*ppos);

	return actual_write;
}

static unsigned int demodrv_poll(struct file *file, poll_table *wait)
{
	int mask = 0;
	struct mydemo_private_data *data = file->private_data;
	struct mydemo_device *device = data->device;

	poll_wait(file, &device->read_queue, wait);
	poll_wait(file, &device->write_queue, wait);

	if (!kfifo_is_empty(&device->mydemo_fifo))
		mask |= POLLIN | POLLRDNORM;
	if (!kfifo_is_full(&device->mydemo_fifo))
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static int demodrv_fasync(int fd, struct file *file, int on)
{
	struct mydemo_private_data *data = file->private_data;
	struct mydemo_device *device = data->device;

	return fasync_helper(fd, file, on, &device->fasync);
}

static const struct file_operations demodrv_fops = {
	.owner = THIS_MODULE,
	.open = demodrv_open,
	.release = demodrv_release,
	.read = demodrv_read,
	.write = demodrv_write,
	.poll = demodrv_poll,
	.fasync = demodrv_fasync,
};

static int __init my_demo_init(void)
{
	int ret;
	int i;
	struct mydemo_device *device;
	int major, minor;

	ret = alloc_chrdev_region(&dev, 0, MYDEMO_MAX_DEVICES, DEMO_NAME);
	if (ret) {
		printk("failed to allocate char device region");
		return ret;
	}
	major = MAJOR(dev);
	minor = MINOR(dev);

	demo_cdev = cdev_alloc();
	if (!demo_cdev) {
		printk("cdev_alloc failed\n");
		return ret;
	}

	cdev_init(demo_cdev, &demodrv_fops);

	ret = cdev_add(demo_cdev, dev, MYDEMO_MAX_DEVICES);
	if (ret) {
		printk("cdev_add failed\n");
		goto cdev_fail;
	}

	my_class = class_create(THIS_MODULE, "my_class");
	if (IS_ERR(my_class)) {
		printk("Failed to create class\n");
		ret = PTR_ERR(my_class);
		goto class_fail;
	}

	for (i = 0; i < MYDEMO_MAX_DEVICES; i++) {
		device = kmalloc(sizeof(struct mydemo_device), GFP_KERNEL);
		if (!device) {
			ret = -ENOMEM;
			goto free_device;
		}
		sprintf(device->name, "%s%d", DEMO_NAME, i);
		mydemo_device[i] = device;
		init_waitqueue_head(&device->read_queue);
		init_waitqueue_head(&device->write_queue);

		ret = kfifo_alloc(&device->mydemo_fifo,
						MYDEMO_FIFO_SIZE,
						GFP_KERNEL);
		if (ret) {
			ret = -ENOMEM;
			goto free_kfifo;
		}

		dev = MKDEV(major, minor++);
		device_create(my_class, NULL, dev, NULL, device->name);

		printk("mydemo_fifo = %p\n", &device->mydemo_fifo);
	}

	printk("succeeded register char device: %s\n", DEMO_NAME);

	return 0;

free_kfifo:
	for (i = 0; i < MYDEMO_MAX_DEVICES; i++)
		if (&mydemo_device[i]->mydemo_fifo)
			kfifo_free(&mydemo_device[i]->mydemo_fifo);

free_device:
	for (i = 0; i < MYDEMO_MAX_DEVICES; i++)
		if (mydemo_device[i])
			kfree(mydemo_device[i]);

class_fail:
	cdev_del(demo_cdev);

cdev_fail:
	unregister_chrdev_region(dev, MYDEMO_MAX_DEVICES);

	return ret;
}

static void __exit my_demo_exit(void)
{
	int i;
	int major = MAJOR(dev);
	int minor = MINOR(dev);
	printk("removing device\n");

	for (i = 0; i < MYDEMO_MAX_DEVICES; i++)
		if (&mydemo_device[i]->mydemo_fifo)
			kfifo_free(&mydemo_device[i]->mydemo_fifo);

	for (i = 0; i < MYDEMO_MAX_DEVICES; i++)
		if (mydemo_device[i])
			kfree(mydemo_device[i]);

	if (my_class) {
		for (i = 0; i < MYDEMO_MAX_DEVICES; i++) {
			dev = MKDEV(major, minor--);
			device_destroy(my_class, dev);
		}
		class_destroy(my_class);
	}

	if (demo_cdev)
		cdev_del(demo_cdev);

	unregister_chrdev_region(dev, MYDEMO_MAX_DEVICES);
}

module_init(my_demo_init);
module_exit(my_demo_exit);
MODULE_LICENSE("GPL v2");
