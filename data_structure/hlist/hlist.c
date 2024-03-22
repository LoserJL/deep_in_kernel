/*
 * 1.问题描述：将关键字 {19, 14, 23, 01, 68, 20, 84, 27, 55, 11, 10, 79}，
 * 	 用H(x)=x%p,散列到哈希表中，使用链地址法处理冲突。
 * */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

/*
 * H(x)=x%p
 * @ p <= m，m是哈希表的长度
 * */

#define NR_HASH	13 //m
#define P		13 //p
#define NR_KEYS 12

#define HASH_FUN(x) ((x)%(P))

int keys[NR_KEYS] = {19, 14, 23, 1, 68, 20, 84, 27, 55, 11, 10, 79};

struct hash_term {
	int key;
	struct hlist_node list;
};

struct hlist_head hash[NR_HASH]; /* 哈希表 */

static int __init hash_init(void)
{
	int i;
	int addr;
	struct hash_term *p;
	struct hash_term *tpos;
	struct hlist_node *pos, *n;

	for (i = 0; i < NR_HASH; i++) {
		INIT_HLIST_HEAD(&hash[i]); /* 初始化哈希表 */
	}

	for (i = 0; i < NR_KEYS; i++) {
		addr = HASH_FUN(keys[i]); /* 由哈希函数获得地址 */

		p = (struct hash_term *)kmalloc(sizeof(struct hash_term), GFP_KERNEL);
		p->key = keys[i];

		hlist_add_head(&p->list, &hash[addr]); /* 头插法存放关键字节点 */
	}

	for (i = 0; i < NR_HASH; i++) {
		printk("print hash table: %d\t", i);

		hlist_for_each_entry(tpos, &hash[i], list) {
			printk("%d\t", tpos->key);
		}

		printk("\n");
	}

	printk("delete 79 ...\n");
	for (i = 0; i < NR_HASH; i++) {
		hlist_for_each_safe(pos, n, &hash[i]) {
			if (hlist_entry(pos, struct hash_term, list)->key == 79) {
				hlist_del(pos);
				kfree(hlist_entry(pos, struct hash_term, list));
			}
		}
	}

	for (i = 0; i < NR_HASH; i++) {
		printk("print hash table: %d\t", i);

		hlist_for_each_entry(tpos, &hash[i], list) {
			printk("%d\t", tpos->key);
		}

		printk("\n");
	}

	return 0;
}

static void __exit hash_exit(void)
{
	struct hlist_node *pos;
	struct hlist_node *n;
	int i;

	printk("destroy hash table...\n");

	for (i = 0; i < NR_HASH; i++) {
		hlist_for_each_safe(pos, n, &hash[i]) {
			hlist_del(pos);
			kfree(hlist_entry(pos, struct hash_term, list));
		}
	}
}

module_init(hash_init);
module_exit(hash_exit);

MODULE_LICENSE("GPL v2");
