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
#include <linux/vmalloc.h>
#include "trace_data/timer_data.h"     
#define PROC_NAME "rbtree_test_cmd"
DEFINE_PER_CPU(u64, baseline_rotations);
DEFINE_PER_CPU(u64, baseline_path_length);
struct rb_root my_test_tree = RB_ROOT; //rbtree
struct rb_root my_wavl_tree = RB_ROOT; //wavl_tree

struct my_node {
    unsigned long long key;
    struct rb_node node;
};
static inline void dummy_propagate(struct rb_node *node, struct rb_node *stop) {}
static inline void dummy_copy(struct rb_node *old, struct rb_node *new) {}
static inline void dummy_rotate(struct rb_node *old, struct rb_node *new) {}

static const struct rb_augment_callbacks dummy_callbacks = {
	.propagate = dummy_propagate,
	.copy = dummy_copy,
	.rotate = dummy_rotate
};





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
static inline void rb_set_black(struct rb_node *rb)
{
	rb->__rb_parent_color += RB_BLACK;
}

noinline void my_rb_insert_wrapper(struct rb_node *node, struct rb_root *root,
	    void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{   // __rb_insert
    struct rb_node *parent = rb_red_parent(node), *gparent, *tmp;

        while (true) {
            /*
            * Loop invariant: node is red.
            */
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
                    this_cpu_inc(baseline_path_length);
                    this_cpu_inc(baseline_path_length);
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
                    this_cpu_inc(baseline_path_length);
                    this_cpu_inc(baseline_path_length);
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

static __always_inline void
____myrb_erase_color(struct rb_node *parent, struct rb_root *root,
	void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{
	struct rb_node *node = NULL, *sibling, *tmp1, *tmp2;

	while (true) {
		/*
		 * Loop invariants:
		 * - node is black (or NULL on first iteration)
		 * - node is not the root (parent is not NULL)
		 * - All leaf paths going through parent and node have a
		 *   black node count that is 1 lower than other leaf paths.
		 */
		sibling = parent->rb_right;
		if (node != sibling) {	/* node == parent->rb_left */
			if (rb_is_red(sibling)) {
				/*
				 * Case 1 - left rotate at parent
				 *
				 *     P               S
				 *    / \             / \
				 *   N   s    -->    p   Sr
				 *      / \         / \
				 *     Sl  Sr      N   Sl
				 */
				tmp1 = sibling->rb_left;
				WRITE_ONCE(parent->rb_right, tmp1);
				WRITE_ONCE(sibling->rb_left, parent);
				rb_set_parent_color(tmp1, parent, RB_BLACK);
				__rb_rotate_set_parents(parent, sibling, root,
							RB_RED);
				augment_rotate(parent, sibling);
                this_cpu_inc(baseline_rotations);
				sibling = tmp1;
			}
			tmp1 = sibling->rb_right;
			if (!tmp1 || rb_is_black(tmp1)) {
				tmp2 = sibling->rb_left;
				if (!tmp2 || rb_is_black(tmp2)) {
					/*
					 * Case 2 - sibling color flip
					 * (p could be either color here)
					 *
					 *    (p)           (p)
					 *    / \           / \
					 *   N   S    -->  N   s
					 *      / \           / \
					 *     Sl  Sr        Sl  Sr
					 *
					 * This leaves us violating 5) which
					 * can be fixed by flipping p to black
					 * if it was red, or by recursing at p.
					 * p is red when coming from Case 1.
					 */
					rb_set_parent_color(sibling, parent,
							    RB_RED);
					if (rb_is_red(parent))
						rb_set_black(parent);
					else {
						node = parent;
						parent = rb_parent(node);
                        this_cpu_inc(baseline_path_length);
						if (parent)
							continue;
					}
					break;
				}
				/*
				 * Case 3 - right rotate at sibling
				 * (p could be either color here)
				 *
				 *   (p)           (p)
				 *   / \           / \
				 *  N   S    -->  N   sl
				 *     / \             \
				 *    sl  Sr            S
				 *                       \
				 *                        Sr
				 *
				 * Note: p might be red, and then both
				 * p and sl are red after rotation(which
				 * breaks property 4). This is fixed in
				 * Case 4 (in __rb_rotate_set_parents()
				 *         which set sl the color of p
				 *         and set p RB_BLACK)
				 *
				 *   (p)            (sl)
				 *   / \            /  \
				 *  N   sl   -->   P    S
				 *       \        /      \
				 *        S      N        Sr
				 *         \
				 *          Sr
				 */
				tmp1 = tmp2->rb_right;
				WRITE_ONCE(sibling->rb_left, tmp1);
				WRITE_ONCE(tmp2->rb_right, sibling);
				WRITE_ONCE(parent->rb_right, tmp2);
				if (tmp1)
					rb_set_parent_color(tmp1, sibling,
							    RB_BLACK);
				augment_rotate(sibling, tmp2);
                this_cpu_inc(baseline_rotations);
				tmp1 = sibling;
				sibling = tmp2;
			}
			/*
			 * Case 4 - left rotate at parent + color flips
			 * (p and sl could be either color here.
			 *  After rotation, p becomes black, s acquires
			 *  p's color, and sl keeps its color)
			 *
			 *      (p)             (s)
			 *      / \             / \
			 *     N   S     -->   P   Sr
			 *        / \         / \
			 *      (sl) sr      N  (sl)
			 */
			tmp2 = sibling->rb_left;
			WRITE_ONCE(parent->rb_right, tmp2);
			WRITE_ONCE(sibling->rb_left, parent);
			rb_set_parent_color(tmp1, sibling, RB_BLACK);
			if (tmp2)
				rb_set_parent(tmp2, parent);
			__rb_rotate_set_parents(parent, sibling, root,
						RB_BLACK);
			augment_rotate(parent, sibling);
            this_cpu_inc(baseline_rotations);
			break;
		} else {
			sibling = parent->rb_left;
			if (rb_is_red(sibling)) {
				/* Case 1 - right rotate at parent */
				tmp1 = sibling->rb_right;
				WRITE_ONCE(parent->rb_left, tmp1);
				WRITE_ONCE(sibling->rb_right, parent);
				rb_set_parent_color(tmp1, parent, RB_BLACK);
				__rb_rotate_set_parents(parent, sibling, root,
							RB_RED);
				augment_rotate(parent, sibling);
                this_cpu_inc(baseline_rotations);
				sibling = tmp1;
			}
			tmp1 = sibling->rb_left;
			if (!tmp1 || rb_is_black(tmp1)) {
				tmp2 = sibling->rb_right;
				if (!tmp2 || rb_is_black(tmp2)) {
					/* Case 2 - sibling color flip */
					rb_set_parent_color(sibling, parent,
							    RB_RED);
					if (rb_is_red(parent))
						rb_set_black(parent);
					else {
						node = parent;
						parent = rb_parent(node);
                        this_cpu_inc(baseline_path_length);
						if (parent)
							continue;
					}
					break;
				}
				/* Case 3 - left rotate at sibling */
				tmp1 = tmp2->rb_left;
				WRITE_ONCE(sibling->rb_right, tmp1);
				WRITE_ONCE(tmp2->rb_left, sibling);
				WRITE_ONCE(parent->rb_left, tmp2);
				if (tmp1)
					rb_set_parent_color(tmp1, sibling,
							    RB_BLACK);
				augment_rotate(sibling, tmp2);
                this_cpu_inc(baseline_rotations);
				tmp1 = sibling;
				sibling = tmp2;
			}
			/* Case 4 - right rotate at parent + color flips */
			tmp2 = sibling->rb_right;
			WRITE_ONCE(parent->rb_left, tmp2);
			WRITE_ONCE(sibling->rb_right, parent);
			rb_set_parent_color(tmp1, sibling, RB_BLACK);
			if (tmp2)
				rb_set_parent(tmp2, parent);
			__rb_rotate_set_parents(parent, sibling, root,
						RB_BLACK);
			augment_rotate(parent, sibling);
            this_cpu_inc(baseline_rotations);
			break;
		}
	}
}

/* Non-inline version for rb_erase_augmented() use */
static void __myrb_erase_color(struct rb_node *parent, struct rb_root *root,
	void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{
	____myrb_erase_color(parent, root, augment_rotate);
}



void my_rb_erase(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *rebalance;
	rebalance = __rb_erase_augmented(node, root, &dummy_callbacks);
	if (rebalance)
		____myrb_erase_color(rebalance, root, dummy_rotate);
}












static void my_rb_insert_color(struct rb_node *node, struct rb_root *root)
{
	my_rb_insert_wrapper(node, root, dummy_rotate);
}
static struct my_node* insert_wavl_node(unsigned long long key)
{
    struct rb_node **new = &(my_wavl_tree.rb_node), *parent = NULL;
    struct my_node *data = kmalloc(sizeof(struct my_node), GFP_KERNEL);
    
    if (!data) return NULL;
    data->key = key;

    while (*new) {
        struct my_node *this = container_of(*new, struct my_node, node);
        parent = *new;
        if (key < this->key) new = &((*new)->rb_left);
        else new = &((*new)->rb_right); 
    }
    rb_link_node(&data->node, parent, new);
    wavl_insert(&data->node, &my_wavl_tree); 
    return data;
}
static struct my_node* insert_rb_node(unsigned long long key) //rbtree
{
    struct rb_node **new = &(my_test_tree.rb_node), *parent = NULL;
    struct my_node *data = kmalloc(sizeof(struct my_node), GFP_KERNEL);
    
    if (!data) return  NULL;
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
    return data;
}
//for record
static struct my_node* search_rb_node(unsigned long long key) {
    struct rb_node *node = my_test_tree.rb_node;
    while (node) {
        struct my_node *data = container_of(node, struct my_node, node);
        if (key < data->key) node = node->rb_left;
        else if (key > data->key) node = node->rb_right;
        else return data;
    }
    return NULL;
}

static struct my_node* search_wavl_node(unsigned long long key) {
    struct rb_node *node = my_wavl_tree.rb_node;
    while (node) {
        struct my_node *data = container_of(node, struct my_node, node);
        if (key < data->key) node = node->rb_left;
        else if (key > data->key) node = node->rb_right;
        else return data;
    }
    return NULL;
}






















static ssize_t rbtree_proc_write(struct file *file, const char __user *buf_user, size_t count, loff_t *ppos)
{
    char buf[32]; 
    size_t copy_len;
    int cpu, i; 
    copy_len = (count < sizeof(buf)) ? count : (sizeof(buf) - 1);
    if (copy_from_user(buf, buf_user, copy_len)) {
        return -EFAULT;
    }
    buf[copy_len] = '\0';
    strim(buf);
    u32 random_key;
    
    /*  Insert  */
    u64 rb_ins_rots = 0, rb_ins_path = 0;
    u64 wavl_ins_rots = 0, wavl_ins_path = 0;
    
    /* Delete */
    u64 rb_del_rots = 0, rb_del_path = 0;
    u64 wavl_del_rots = 0, wavl_del_path = 0;
    
    const int TOTAL_OPERATIONS = 10000;
    const int DELETE_OPERATIONS = 5000;
    

    struct my_node **rb_nodes = vmalloc(TOTAL_OPERATIONS * sizeof(struct my_node *));
    struct my_node **wavl_nodes = vmalloc(TOTAL_OPERATIONS * sizeof(struct my_node *));
    int *indices = kmalloc_array(TOTAL_OPERATIONS , sizeof(int), GFP_KERNEL); /* index for shuffle */
    if (strcmp(buf, "real_trace") == 0) {
        u64 rb_rots = 0, rb_path = 0;
        u64 wavl_rots = 0, wavl_path = 0;
        u64 delete_misses = 0;
        u64 total_inserts = 0; 
        u64 total_deletes = 0;
        int i;
        int WARMUP_COUNT = TRACE_SIZE / 5;
        pr_info("[RB-Test] Replaying Real Kernel Trace (%d ops)...\n", TRACE_SIZE);

        for (i = 0; i < TRACE_SIZE; i++) {
            unsigned long long key = real_trace[i].key;

            if (real_trace[i].is_insert) {
                total_inserts++;
                insert_rb_node(key);
                insert_wavl_node(key);
            } 
            else {
                struct my_node *rb_target = search_rb_node(key);
                struct my_node *wavl_target = search_wavl_node(key);

                if (rb_target && wavl_target) { // found
                    total_deletes++;
                    my_rb_erase(&rb_target->node, &my_test_tree);
                    kfree(rb_target);
                    
                    wavl_erase(&wavl_target->node, &my_wavl_tree);
                    kfree(wavl_target);
                } else {
                    delete_misses++; //not found
                }
            }
            if (i == WARMUP_COUNT) {
                for_each_possible_cpu(cpu) {
                    per_cpu(baseline_rotations, cpu) = 0;
                    per_cpu(baseline_path_length, cpu) = 0;
                    per_cpu(wavl_rotations, cpu) = 0;
                    per_cpu(wavl_path_length, cpu) = 0;
                }
            }
        }

        for_each_possible_cpu(cpu) {
            rb_rots += per_cpu(baseline_rotations, cpu);
            rb_path += per_cpu(baseline_path_length, cpu);
            wavl_rots += per_cpu(wavl_rotations, cpu);
            wavl_path += per_cpu(wavl_path_length, cpu);
        }

        pr_info("==================================================\n");
        pr_info("          [ Real Kernel Trace Replay ]\n");
        pr_info("==================================================\n");
        pr_info("Metric                     |  RB Tree  | WAVL Tree\n");
        pr_info("--------------------------------------------------\n");
        pr_info("Total Rotation Counts      | %9llu | %9llu\n", rb_rots, wavl_rots);
        pr_info("Total Rebalancing Path Len | %9llu | %9llu\n", rb_path, wavl_path);
        pr_info("==================================================\n");
        pr_info("Total Inserts          : %llu\n", total_inserts);
        pr_info("Total Successful Delete  : %llu\n", total_deletes);
        pr_info("Total Delete Misses: %llu\n", delete_misses);
        struct my_node *pos, *n;
        rbtree_postorder_for_each_entry_safe(pos, n, &my_test_tree, node) {
            kfree(pos);
        }
        rbtree_postorder_for_each_entry_safe(pos, n, &my_wavl_tree, node) {
            kfree(pos);
        }
        my_wavl_tree = RB_ROOT;
        my_test_tree = RB_ROOT;
    }
    else{
        if (!rb_nodes || !wavl_nodes || !indices) {
            pr_err("Memory allocation failed!\n");
            
            if (rb_nodes) vfree(rb_nodes);
            if (wavl_nodes) vfree(wavl_nodes);
            if (indices) kfree(indices); 
            
            return -ENOMEM; 
        }
        /* ==========================================
        * Insert 
        * ========================================== */
        for_each_possible_cpu(cpu) {
            per_cpu(baseline_rotations, cpu) = 0;
            per_cpu(baseline_path_length, cpu) = 0;
            per_cpu(wavl_rotations, cpu) = 0;
            per_cpu(wavl_path_length, cpu) = 0;
        }

        pr_info("[RB-Test] Inserting %d nodes...\n", TOTAL_OPERATIONS);
        
        for (i = 0; i < TOTAL_OPERATIONS; i++) {
            //random_key = get_random_u32() % 1000000;
            //rb_nodes[i]  = insert_rb_node(random_key);   
            rb_nodes[i]  = insert_rb_node(i);
            //wavl_nodes[i] = insert_wavl_node(random_key);
            wavl_nodes[i] = insert_wavl_node(i);
            indices[i] = i; 
        }
        

        for_each_possible_cpu(cpu) {
            rb_ins_rots += per_cpu(baseline_rotations, cpu);
            rb_ins_path += per_cpu(baseline_path_length, cpu);
            wavl_ins_rots += per_cpu(wavl_rotations, cpu);
            wavl_ins_path += per_cpu(wavl_path_length, cpu);
        }

        /* ==========================================
        * shuffle
        * ========================================== */
        // for (i = TOTAL_OPERATIONS - 1; i > 0; i--) {
        //     u32 j = get_random_u32() % (i + 1);
            
        //     int temp = indices[i];
        //     indices[i] = indices[j];
        //     indices[j] = temp;
        // }

        /* ==========================================
        * delete
        * ========================================== */
        for_each_possible_cpu(cpu) {
            per_cpu(baseline_rotations, cpu) = 0;
            per_cpu(baseline_path_length, cpu) = 0;
            per_cpu(wavl_rotations, cpu) = 0;
            per_cpu(wavl_path_length, cpu) = 0;
        }

        pr_info("[RB-Test] Deleting %d nodes...\n", DELETE_OPERATIONS);
        for (i = 0; i < DELETE_OPERATIONS; i++) {
            int target_idx = indices[i]; 
            
            if (rb_nodes[target_idx]) {
                my_rb_erase(&rb_nodes[target_idx]->node, &my_test_tree);
                kfree(rb_nodes[target_idx]);
                rb_nodes[target_idx] = NULL;
            }
            if (wavl_nodes[target_idx]) {
                wavl_erase(&wavl_nodes[target_idx]->node, &my_wavl_tree);
                kfree(wavl_nodes[target_idx]);
                wavl_nodes[target_idx] = NULL;
            }
        }


        for_each_possible_cpu(cpu) {
            rb_del_rots += per_cpu(baseline_rotations, cpu);
            rb_del_path += per_cpu(baseline_path_length, cpu);
            wavl_del_rots += per_cpu(wavl_rotations, cpu);
            wavl_del_path += per_cpu(wavl_path_length, cpu);
        }


        pr_info("==================================================\n");
        pr_info("               [ Insert Phase ]\n");
        pr_info("==================================================\n");
        pr_info("Metric                     |  RB Tree  | WAVL Tree\n");
        pr_info("--------------------------------------------------\n");
        pr_info("Total Rotation Counts      | %9llu | %9llu\n", rb_ins_rots, wavl_ins_rots);
        pr_info("Average Rotation Counts    | %5llu.%03llu | %5llu.%03llu\n", 
                rb_ins_rots / TOTAL_OPERATIONS, ((rb_ins_rots * 1000) / TOTAL_OPERATIONS) % 1000,
                wavl_ins_rots / TOTAL_OPERATIONS, ((wavl_ins_rots * 1000) / TOTAL_OPERATIONS) % 1000);
        pr_info("Total Rebalancing Path Len | %9llu | %9llu\n", rb_ins_path, wavl_ins_path);

        pr_info("==================================================\n");
        pr_info("               [ Delete Phase ]\n");
        pr_info("==================================================\n");
        pr_info("Metric                     |  RB Tree  | WAVL Tree\n");
        pr_info("--------------------------------------------------\n");
        pr_info("Total Rotation Counts      | %9llu | %9llu\n", rb_del_rots, wavl_del_rots);
        pr_info("Average Rotation Counts    | %5llu.%03llu | %5llu.%03llu\n", 
                rb_del_rots / DELETE_OPERATIONS, ((rb_del_rots * 1000) / DELETE_OPERATIONS) % 1000,
                wavl_del_rots / DELETE_OPERATIONS, ((wavl_del_rots * 1000) / DELETE_OPERATIONS) % 1000);
        pr_info("Total Rebalancing Path Len | %9llu | %9llu\n", rb_del_path, wavl_del_path);
        pr_info("==================================================\n");


        vfree(rb_nodes);
        vfree(wavl_nodes);
        kfree(indices);

        struct my_node *pos, *n;
        rbtree_postorder_for_each_entry_safe(pos, n, &my_test_tree, node) {
            kfree(pos);
        }
        rbtree_postorder_for_each_entry_safe(pos, n, &my_wavl_tree, node) {
            kfree(pos);
        }
        my_wavl_tree = RB_ROOT;
        my_test_tree = RB_ROOT;
    }
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
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("[RB-Test] module unloaded \n");
}

module_init(rbtree_wrapper_init);
module_exit(rbtree_wrapper_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Yen Yu");
MODULE_DESCRIPTION("A wrapper module for tracing native rbtree");