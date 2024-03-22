#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

/* header of bitmap */
#include <linux/bitmap.h>

static int __init bitmap_demo_init(void)
{
	unsigned long bitmap = 0xf000000;

	/* Set range bits on bitmap */
	bitmap_set(&bitmap, 5, 8);
	printk("Bitmap: %#lx\n", bitmap);

	return 0;
}

static void __exit bitmap_demo_exit(void)
{
	printk("Bitmap exit\n");
}

module_init(bitmap_demo_init);
module_exit(bitmap_demo_exit);

MODULE_LICENSE("GPL v2");