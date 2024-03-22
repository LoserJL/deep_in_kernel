#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/rbtree.h>

struct my_rbtype {
	struct rb_node node;
	unsigned long runtime;
};

static struct my_rbtype node0 = {.runtime = 0x1};
static struct my_rbtype node1 = {.runtime = 0x2};
static struct my_rbtype node2 = {.runtime = 0x3};
static struct my_rbtype node3 = {.runtime = 0x5};
static struct my_rbtype node4 = {.runtime = 0x4};
static struct my_rbtype node5 = {.runtime = 0x7};
static struct my_rbtype node6 = {.runtime = 0x8};
static struct my_rbtype node7 = {.runtime = 0x9};
static struct my_rbtype node8 = {.runtime = 0x129};

/* root for rbtree */
struct rb_root my_rbtree = RB_ROOT;

/* Insert private node into rbtree */
static int rbtree_insert(struct rb_root *root, struct my_rbtype *node)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct my_rbtype *this = rb_entry(*new, struct my_rbtype, node);
		int result;

		/* Compare runtime */
		result = this->runtime - node->runtime;

		/* Setup parent */
		parent = *new;

		if (result > 0)
			new = &((*new)->rb_left);
		else if (result < 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}

	/* Add new node ang rebalance tree */
	rb_link_node(&node->node, parent, new);
	rb_insert_color(&node->node, root);

	return 1;
}

/* Search private node on rbtree */
struct my_rbtype *rbtree_search(struct rb_root *root, unsigned long runtime)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct my_rbtype *this = rb_entry(node, struct my_rbtype, node);
		int result;

		result = this->runtime - runtime;

		if (result > 0)
			node = node->rb_left;
		else if (result < 0)
			node = node->rb_right;
		else
			return this;
	}

	return NULL;
}

static int __init rbtree_demo_init(void)
{
	struct rb_node *np;

	/* Insert rb_node */
	rbtree_insert(&my_rbtree, &node0);
	rbtree_insert(&my_rbtree, &node1);
	rbtree_insert(&my_rbtree, &node2);
	rbtree_insert(&my_rbtree, &node3);
	rbtree_insert(&my_rbtree, &node4);
	rbtree_insert(&my_rbtree, &node5);
	rbtree_insert(&my_rbtree, &node6);
	rbtree_insert(&my_rbtree, &node7);
	rbtree_insert(&my_rbtree, &node8);

	/* Traverser all node on rbtree */
	for (np = rb_first(&my_rbtree); np; np = rb_next(np))
		printk("RB: %#lx\n", rb_entry(np, struct my_rbtype, node)->runtime);

	/* erase rb_node */
	rb_erase(&node6.node, &my_rbtree);

	printk("Re- Iterate over RBTree.\n");
	for (np = rb_first(&my_rbtree); np; np = rb_next(np))
		printk("RB: %#lx\n", rb_entry(np, struct my_rbtype, node)->runtime);

	return 0;
}

static void __exit rbtree_demo_exit(void)
{
	struct rb_node *np;
	struct my_rbtype *rb_node;

	for (np = rb_first(&my_rbtree); np; np = rb_next(np)) {
		rb_node = rb_entry(np, struct my_rbtype, node);
		if (rb_node)
			rb_erase(&rb_node->node, &my_rbtree);
	}
	printk("RBTree exit.\n");
}

module_init(rbtree_demo_init);
module_exit(rbtree_demo_exit);
MODULE_LICENSE("GPL v2");