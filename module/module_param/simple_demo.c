#include <linux/module.h>
#include <linux/init.h>

static int debug = 1;
/* /sys/module/simple_demo/parameters/debug */
/* perm is set 0644 in sysfs, S_IRUGO|S_IWUSR */
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "enable debugging information");

#define dprintk(args...) \
	if (debug) { \
		printk(KERN_DEBUG args); \
	}

/* /sys/module/simple_demo/parameters/mytest */
static int mytest = 100;
module_param(mytest, int, 0644);
MODULE_PARM_DESC(mytest, "test for module parameter");

static int __init my_test_init(void)
{
	dprintk("my test kernel module init\n");
	dprintk("module parameter = %d\n", mytest);

	return 0;
}

static void __exit my_test_exit(void)
{
	printk("goodbye\n");
}

module_init(my_test_init);
module_exit(my_test_exit);
MODULE_LICENSE("GPL v2");