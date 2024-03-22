#include <linux/list.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#define N 10	/* 链表节点数 */

struct num_list {
	int num;				/* 数据 */
	struct list_head list;	/* 指向双向链表前后节点的指针 */
};

struct num_list num_head;	/* 头结点 */

static int __init double_list_init(void)
{
	/* 初始化头结点 */
	struct num_list *list_node;	/* 每次申请链表节点时所用的指针 */
	struct list_head *pos;
	struct num_list *p;
	int i;

	printk("double list is starting...\n");

	INIT_LIST_HEAD(&num_head.list);

	/* 这里可以看出，初始化之后，prev与next都指向自己 */
	printk("%p - %p - %p\n", &num_head.list, num_head.list.prev, num_head.list.next);
	/* prev->next和next->prev也是自己 */
	printk("%p - %p\n", num_head.list.prev->next, num_head.list.next->prev);
	/* 同样地，prev->prev和next->next也是自己 */
	printk("%p - %p\n", num_head.list.prev->prev, num_head.list.next->next);
	/* 总之，咋样都是自己 */
	printk("%p - %p\n", num_head.list.prev->prev->next->prev, num_head.list.next->next->prev);


	/* 建立N个节点，依次接入到链表中 */
	for (i = 0; i < N; i++) {
		/* kmalloc()在内核空间申请内存，类似C库的malloc() */
		list_node = (struct num_list *)kmalloc(sizeof(struct num_list), GFP_KERNEL);
		list_node->num = i+1;
		list_add_tail(&list_node->list, &num_head.list); //注意，一定不同于一般操作，这里都是对嵌入的list_head操作
		printk("Node %d is added to the double list...\n", i+1);
	}

	/* 遍历链表 */
	i = 1;
	list_for_each(pos, &num_head.list) {
		p = list_entry(pos, struct num_list, list); //其实就是container_of
		printk("Node %d's data:%d\n", i, p->num);
		i++;
	}

	return 0;
}

static void __exit double_list_exit(void)
{
	struct list_head *pos, *n;
	struct num_list *p;
	int i;

	/* 依次删除N个节点 */
	i = 1;
	list_for_each_safe(pos, n, &num_head.list) {		/* 为了安全删除节点而进行的遍历 */
		list_del(pos);									/* 从双链表中删除当前节点 */
		p = list_entry(pos, struct num_list, list);		/* 得到当前数据节点的首地址，即指针 */
		kfree(p);										/* 释放该数据节点所占空间 */
		printk("Node %d has removed from duble list...\n", i++);
	}
	printk("double list is exiting...\n");
}

module_init(double_list_init)
module_exit(double_list_exit);

MODULE_LICENSE("GPL v2");
