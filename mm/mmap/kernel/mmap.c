#include <linux/module.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pfn.h>
#include <linux/highmem-internal.h>

static dev_t dev;
static struct cdev *cdev;
static struct class *class;
struct page *page;

static int my_open(struct inode *inode, struct file *filp)
{
	printk("Open mmap_test file\n");

	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf,
					size_t size, loff_t *pos)
{
	return 0;
}

static ssize_t my_write(struct file *filp, const char __user *buf,
					size_t size, loff_t *pos)
{

	return 0;
}

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long pfn = page_to_pfn(page);
	unsigned long paddr = PFN_PHYS(pfn);
	char *vaddr = (char *)kmap(page);
	int i;

	for (i = 0; i < 100; i++)
		vaddr[i] = i + 1;
	kunmap(page);

	if (remap_pfn_range(vma, vma->vm_start, pfn,
					PAGE_SIZE, vma->vm_page_prot)) {
		printk("Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
				paddr, paddr + PAGE_SIZE, vma->vm_start, vma->vm_start + PAGE_SIZE);
		return -EAGAIN;
	}
	printk("Succeessed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
			paddr, paddr + PAGE_SIZE, vma->vm_start, vma->vm_start + PAGE_SIZE);
	return 0;
}

static const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.read = my_read,
	.write = my_write,
	.mmap = my_mmap,
};

static int __init my_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, "mmap_test");
	if (ret) {
		printk("Failed to allocate char device region");
		return ret;
	}

	cdev = cdev_alloc();
	if (!cdev) {
		printk("cdev_alloc failed\n");
		return -1;
	}

	cdev_init(cdev, &my_fops);

	ret = cdev_add(cdev, dev, 1);
	if (ret) {
		printk("cdev_add failed\n");
		goto cdev_add_fail;
	}

	class = class_create(THIS_MODULE, "my_class");
	if (IS_ERR(class)) {
		printk("Failed to create class\n");
		ret = PTR_ERR(class);
		goto class_fail;
	}

	device_create(class, NULL, dev, NULL, "mmap_test");
	printk("Created the /dev/mmap_test file\n");

	page = alloc_page(GFP_KERNEL);
	printk("Allocated a page\n");

	return 0;

class_fail:
	cdev_del(cdev);

cdev_add_fail:
	unregister_chrdev_region(dev, 1);

	return ret;
}

static void __exit my_exit(void)
{
	printk("Removing device\n");

	if (page) {
		__free_page(page);
		page = NULL;
	}
	printk("freed page\n");

	if (class) {
		device_destroy(class, dev);
		class_destroy(class);
	}

	if (cdev)
		cdev_del(cdev);

	unregister_chrdev_region(dev, 1);

	printk("mmap test exited\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL v2");