#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/radix-tree.h>
#include <linux/xarray.h>

/* radix tree node type */
struct my_radix_tree_node {
	char *name;
	unsigned long id;
};

/* Radix tree root */
static struct radix_tree_root rt_root;

/* node */
static struct my_radix_tree_node node0 = {.name = "IDA", .id = 0x20000};
static struct my_radix_tree_node node1 = {.name = "IDB", .id = 0x60000};
static struct my_radix_tree_node node2 = {.name = "IDC", .id = 0x80000};
static struct my_radix_tree_node node3 = {.name = "IDD", .id = 0x30000};
static struct my_radix_tree_node node4 = {.name = "IDE", .id = 0x90000};

/* Reference lib/xarray.c */
void xa_dump_node(const struct radix_tree_node *node)
{
	unsigned i, j;

	if (!node)
		return;
	if ((unsigned long)node & 3) {
		pr_cont("node %px\n", node);
		return;
	}

	pr_cont("node %px %s %d parent %px shift %d count %d values %d "
		"array %px list %px %px marks",
		node, node->parent ? "offset" : "max", node->offset,
		node->parent, node->shift, node->count, node->nr_values,
		node->array, node->private_list.prev, node->private_list.next);
	for (i = 0; i < XA_MAX_MARKS; i++)
		for (j = 0; j < XA_MARK_LONGS; j++)
			pr_cont(" %lx", node->marks[i][j]);
	pr_cont("\n");
}

void xa_dump_index(unsigned long index, unsigned int shift)
{
	if (!shift)
		pr_info("%lu: ", index);
	else if (shift >= BITS_PER_LONG)
		pr_info("0-%lu: ", ~0UL);
	else
		pr_info("%lu-%lu: ", index, index | ((1UL << shift) - 1));
}

void xa_dump_entry(const void *entry, unsigned long index, unsigned long shift)
{
	if (!entry)
		return;

	xa_dump_index(index, shift);

	if (xa_is_node(entry)) {
		if (shift == 0) {
			pr_cont("%px\n", entry);
		} else {
			unsigned long i;
			struct xa_node *node = xa_to_node(entry);
			xa_dump_node(node);
			for (i = 0; i < XA_CHUNK_SIZE; i++)
				xa_dump_entry(node->slots[i],
				      index + (i << node->shift), node->shift);
		}
	} else if (xa_is_value(entry))
		pr_cont("value %ld (0x%lx) [%px]\n", xa_to_value(entry),
						xa_to_value(entry), entry);
	else if (!xa_is_internal(entry))
		pr_cont("%px\n", entry);
	else if (xa_is_retry(entry))
		pr_cont("retry (%ld)\n", xa_to_internal(entry));
	else if (xa_is_sibling(entry))
		pr_cont("sibling (slot %ld)\n", xa_to_sibling(entry));
	else if (xa_is_zero(entry))
		pr_cont("zero (%ld)\n", xa_to_internal(entry));
	else
		pr_cont("UNKNOWN ENTRY (%px)\n", entry);
}

static void my_xa_dump(const struct radix_tree_root *xa)
{
	void *entry = xa->xa_head;
	unsigned int shift = 0;

	pr_info("rt_root: %px head %px flags %x marks %d %d %d\n", xa, entry,
			xa->xa_flags, xa_marked(xa, XA_MARK_0),
			xa_marked(xa, XA_MARK_1), xa_marked(xa, XA_MARK_2));
	if (xa_is_node(entry))
		shift = xa_to_node(entry)->shift + XA_CHUNK_SHIFT;
	xa_dump_entry(entry, 0, shift);
}

static int __init radix_demo_init(void)
{
	struct my_radix_tree_node *np;

	/* Initialize Radix-tree root */
	INIT_RADIX_TREE(&rt_root, GFP_ATOMIC);
	my_xa_dump(&rt_root);

	/* Insert node into Radix-tree */
	radix_tree_insert(&rt_root, node0.id, &node0);
	my_xa_dump(&rt_root);
	radix_tree_insert(&rt_root, node1.id, &node1);
	my_xa_dump(&rt_root);
	radix_tree_insert(&rt_root, node2.id, &node2);
	my_xa_dump(&rt_root);
	radix_tree_insert(&rt_root, node3.id, &node3);
	my_xa_dump(&rt_root);
	radix_tree_insert(&rt_root, node4.id, &node4);
	my_xa_dump(&rt_root);

	/* search struct node by id */
	np = radix_tree_lookup(&rt_root, 0x60000);
	BUG_ON(!np);
	printk("Radix tree : %s id %#lx\n", np->name, np->id);

	/* delete a node from radix tree */
	radix_tree_delete(&rt_root, np->id);
	my_xa_dump(&rt_root);

	/* search struct node by id */
	np = radix_tree_lookup(&rt_root, 0x60000);
	if (np)
		printk("Radix tree : %s id %#lx\n", np->name, np->id);
	else
		printk("The node is deleted\n");

	return 0;
}

static __exit void radix_demo_exit(void)
{
	printk("Radix tree: exit\n");
}

module_init(radix_demo_init);
module_exit(radix_demo_exit);
MODULE_LICENSE("GPL v2");