#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/random.h> 
#include <linux/rbtree_augmented.h>
#include "wavl_tree_augmented.h"
#define PROC_NAME "rbtree_test_cmd"
DEFINE_PER_CPU(u64, baseline_rotations);
DEFINE_PER_CPU(u64, baseline_path_length);

struct rb_root my_test_tree = RB_ROOT; //rbtree
struct rb_root my_wavl_tree = RB_ROOT; //wavl_tree

struct my_node {
    int key;
    struct rb_node node;
};
static void dummy_rotate(struct rb_node *old, struct rb_node *new) {}

static inline struct rb_node *rb_red_parent(struct rb_node *red)
{
	return (struct rb_node *)red->__rb_parent_color;
}
static inline void
__rb_rotate_set_parents(struct rb_node *old, struct rb_node *new,
			struct rb_root *root, int color)
{
	struct rb_node *parent = rb_parent(old);
	new->__rb_parent_color = old->__rb_parent_color;
	rb_set_parent_color(old, new, color);
	__rb_change_child(old, new, parent, root);
}

static noinline void my_rb_insert_wrapper(struct rb_node *node, struct rb_root *root,
	    void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{   // __rb_insert
    struct rb_node *parent = rb_red_parent(node), *gparent, *tmp;

        while (true) {
            /*
            * Loop invariant: node is red.
            */
            this_cpu_inc(baseline_path_length); //go up 1 layer every loop
            if (unlikely(!parent)) {
                /*
                * The inserted node is root. Either this is the
                * first node, or we recursed at Case 1 below and
                * are no longer violating 4).
                */
                rb_set_parent_color(node, NULL, RB_BLACK);
                break;
            }

            /*
            * If there is a black parent, we are done.
            * Otherwise, take some corrective action as,
            * per 4), we don't want a red root or two
            * consecutive red nodes.
            */
            if(rb_is_black(parent))
                break;

            gparent = rb_red_parent(parent);

            tmp = gparent->rb_right;
            if (parent != tmp) {	/* parent == gparent->rb_left */
                if (tmp && rb_is_red(tmp)) {
                    /*
                    * Case 1 - node's uncle is red (color flips).
                    *
                    *       G            g
                    *      / \          / \
                    *     p   u  -->   P   U
                    *    /            /
                    *   n            n
                    *
                    * However, since g's parent might be red, and
                    * 4) does not allow this, we need to recurse
                    * at g.
                    */
                    rb_set_parent_color(tmp, gparent, RB_BLACK);
                    rb_set_parent_color(parent, gparent, RB_BLACK);
                    node = gparent;
                    parent = rb_parent(node);
                    rb_set_parent_color(node, parent, RB_RED);
                    continue;
                }

                tmp = parent->rb_right;
                if (node == tmp) {
                    /*
                    * Case 2 - node's uncle is black and node is
                    * the parent's right child (left rotate at parent).
                    *
                    *      G             G
                    *     / \           / \
                    *    p   U  -->    n   U
                    *     \           /
                    *      n         p
                    *
                    * This still leaves us in violation of 4), the
                    * continuation into Case 3 will fix that.
                    */
                    tmp = node->rb_left;
                    WRITE_ONCE(parent->rb_right, tmp);
                    WRITE_ONCE(node->rb_left, parent);
                    if (tmp)
                        rb_set_parent_color(tmp, parent,
                                    RB_BLACK);
                    rb_set_parent_color(parent, node, RB_RED);
                    augment_rotate(parent, node);
                    this_cpu_inc(baseline_rotations); //rotate once
                    parent = node;
                    tmp = node->rb_right;
                }

                /*
                * Case 3 - node's uncle is black and node is
                * the parent's left child (right rotate at gparent).
                *
                *        G           P
                *       / \         / \
                *      p   U  -->  n   g
                *     /                 \
                *    n                   U
                */
                WRITE_ONCE(gparent->rb_left, tmp); /* == parent->rb_right */
                WRITE_ONCE(parent->rb_right, gparent);
                if (tmp)
                    rb_set_parent_color(tmp, gparent, RB_BLACK);
                __rb_rotate_set_parents(gparent, parent, root, RB_RED);
                augment_rotate(gparent, parent);
                this_cpu_inc(baseline_rotations); //rotate once
                break;
            } else {
                tmp = gparent->rb_left;
                if (tmp && rb_is_red(tmp)) {
                    /* Case 1 - color flips */
                    rb_set_parent_color(tmp, gparent, RB_BLACK);
                    rb_set_parent_color(parent, gparent, RB_BLACK);
                    node = gparent;
                    parent = rb_parent(node);
                    rb_set_parent_color(node, parent, RB_RED);
                    continue;
                }

                tmp = parent->rb_left;
                if (node == tmp) {
                    /* Case 2 - right rotate at parent */
                    tmp = node->rb_right;
                    WRITE_ONCE(parent->rb_left, tmp);
                    WRITE_ONCE(node->rb_right, parent);
                    if (tmp)
                        rb_set_parent_color(tmp, parent,
                                    RB_BLACK);
                    rb_set_parent_color(parent, node, RB_RED);
                    augment_rotate(parent, node);
                    this_cpu_inc(baseline_rotations); //rotate once
                    parent = node;
                    tmp = node->rb_left;
                }

                /* Case 3 - left rotate at gparent */
                WRITE_ONCE(gparent->rb_right, tmp); /* == parent->rb_left */
                WRITE_ONCE(parent->rb_left, gparent);
                if (tmp)
                    rb_set_parent_color(tmp, gparent, RB_BLACK);
                __rb_rotate_set_parents(gparent, parent, root, RB_RED);
                augment_rotate(gparent, parent);
                this_cpu_inc(baseline_rotations); //rotate once
                break;
            }
        }
}


static void my_rb_insert_color(struct rb_node *node, struct rb_root *root)
{
	my_rb_insert_wrapper(node, root, dummy_rotate);
}
static void insert_wavl_node(int key)
{
    struct rb_node **new = &(my_wavl_tree.rb_node), *parent = NULL;
    struct my_node *data = kmalloc(sizeof(struct my_node), GFP_KERNEL);
    
    if (!data) return;
    data->key = key;

    while (*new) {
        struct my_node *this = container_of(*new, struct my_node, node);
        parent = *new;
        if (key < this->key) new = &((*new)->rb_left);
        else new = &((*new)->rb_right); 
    }
    rb_link_node(&data->node, parent, new);
    wavl_insert(&data->node, &my_wavl_tree); 
}
static void insert_rb_node(int key) //rbtree
{
    struct rb_node **new = &(my_test_tree.rb_node), *parent = NULL;
    struct my_node *data = kmalloc(sizeof(struct my_node), GFP_KERNEL);
    
    if (!data) return;
    data->key = key;
    while (*new) {
        struct my_node *this = container_of(*new, struct my_node, node);
        parent = *new;
        if (key < this->key)
            new = &((*new)->rb_left);
        else if (key > this->key)
            new = &((*new)->rb_right);
        else {
            new = &((*new)->rb_right); //default insert right
        }
    }
    rb_link_node(&data->node, parent, new);
    //target
    my_rb_insert_color(&data->node, &my_test_tree);
}

static ssize_t rbtree_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int i, cpu;
    u32 random_key;
    u64 rb_rots = 0, rb_path = 0;
    u64 wavl_rots = 0, wavl_path = 0;
    const int TOTAL_OPERATIONS = 1000; 
    for_each_possible_cpu(cpu) {
        per_cpu(baseline_rotations, cpu) = 0;
        per_cpu(baseline_path_length, cpu) = 0;
        per_cpu(wavl_rotations, cpu) = 0;
        per_cpu(wavl_path_length, cpu) = 0;
    }

    pr_info("[RB-Test] insert %d nodes...\n", TOTAL_OPERATIONS);
    
    for (i = 0; i < TOTAL_OPERATIONS; i++) {
        random_key = get_random_u32() % 1000000;
        insert_rb_node(random_key);   
        insert_wavl_node(random_key);
    }
    
    for_each_possible_cpu(cpu) {
        rb_rots += per_cpu(baseline_rotations, cpu);
        rb_path += per_cpu(baseline_path_length, cpu);
        wavl_rots += per_cpu(wavl_rotations, cpu);
        wavl_path += per_cpu(wavl_path_length, cpu);
    }

    pr_info("========================================\n");
    pr_info("           [ result graph ]\n");
    pr_info("========================================\n");
    pr_info("Pointer    |  RB Tree  |  WAVL Tree\n");
    pr_info("----------------------------------------\n");
    pr_info("Total Rotation Counts   | %11llu | %10llu\n", rb_rots, wavl_rots);
    pr_info("Average Rotation Counts | %7llu.%03llu | %6llu.%03llu\n", 
            rb_rots / TOTAL_OPERATIONS, ((rb_rots * 1000) / TOTAL_OPERATIONS) % 1000,
            wavl_rots / TOTAL_OPERATIONS, ((wavl_rots * 1000) / TOTAL_OPERATIONS) % 1000);
    pr_info("Total Rebalancing Path Len   | %11llu | %10llu\n", rb_path, wavl_path);
    pr_info("========================================\n");
    return count;
}

static const struct proc_ops rbtree_proc_ops = {
    .proc_write = rbtree_proc_write,
};

static int __init rbtree_wrapper_init(void)
{
    proc_create(PROC_NAME, 0666, NULL, &rbtree_proc_ops);
    pr_info("[RB-Test] %s module loaded\n", PROC_NAME);
    return 0;
}

static void __exit rbtree_wrapper_exit(void)
{
    struct my_node *pos, *n;
    remove_proc_entry(PROC_NAME, NULL);
    
    rbtree_postorder_for_each_entry_safe(pos, n, &my_test_tree, node) {
        kfree(pos);
    }
    rbtree_postorder_for_each_entry_safe(pos, n, &my_wavl_tree, node) {
        kfree(pos);
    }
    pr_info("[RB-Test] module unloaded \n");
}

module_init(rbtree_wrapper_init);
module_exit(rbtree_wrapper_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Yen Yu");
MODULE_DESCRIPTION("A wrapper module for tracing native rbtree");