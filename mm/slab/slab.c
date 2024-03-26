#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>

static char *kbuf;
static int size = 20;
static int align = 8;
static struct kmem_cache *my_slab;

static int __init my_init(void)
{
	/* Create a kmem_cache */
	my_slab = kmem_cache_create("my_slab", size, align, 0, NULL);
	if (!my_slab) {
		pr_err("kmem_cache_create failed\n");
		return -ENOMEM;
	}
	pr_info("created memory cache correctly\n");

	/* Alloc a slab object */
	kbuf = kmem_cache_alloc(my_slab, GFP_ATOMIC);
	if (!kbuf) {
		pr_err(" failed to alloc a slab object\n");
		(void)kmem_cache_destroy(my_slab);
		return -1;
	}
	pr_info(" successfully allocated a slab object\n");
	return 0;
}

static void __exit my_exit(void)
{
	/* Free the slab object */
	kmem_cache_free(my_slab, kbuf);
	pr_info("freed a memory slab object\n");

	/* Destroy the kmem_cache */
	(void)kmem_cache_destroy(my_slab);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL v2");