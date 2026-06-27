#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>     
#include "wavl_tree_augmented.h"
#include "trace_data/timer_data.h"     
#include "trace_data/cfs_data.h"

#define PROC_NAME "rbtree_latency_cmd"

struct rb_root my_test_tree = RB_ROOT; //RB Tree
struct rb_root my_wavl_tree = RB_ROOT; // WAVL Tree

struct my_node {
    unsigned long long key;
    struct rb_node node;
};


static inline void do_rb_insert(struct my_node *data, struct rb_root *root) {
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    while (*new) {
        struct my_node *this = container_of(*new, struct my_node, node);
        parent = *new;
        if (data->key < this->key) new = &((*new)->rb_left);
        else new = &((*new)->rb_right); 
    }
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root); 
}

static inline void do_wavl_insert(struct my_node *data, struct rb_root *root) {
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    while (*new) {
        struct my_node *this = container_of(*new, struct my_node, node);
        parent = *new;
        if (data->key < this->key) new = &((*new)->rb_left);
        else new = &((*new)->rb_right);
    }
    rb_link_node(&data->node, parent, new);
    wavl_insert(&data->node, root);
}

static inline struct my_node* do_search(unsigned long long key, struct rb_root *root) {
    struct rb_node *node = root->rb_node;
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
    size_t copy_len = (count < sizeof(buf)) ? count : (sizeof(buf) - 1);
    
    if (copy_from_user(buf, buf_user, copy_len)) return -EFAULT;
    buf[copy_len] = '\0';
    strim(buf);

    if (strcmp(buf, "timer_trace") == 0 || strcmp(buf, "cfs_trace") == 0) {
        u64 t_start, t_end;
        u64 rb_ins_time = 0, wavl_ins_time = 0;
        u64 rb_del_time = 0, wavl_del_time = 0;
        u64 rb_lookup_time = 0, wavl_lookup_time = 0;
        u64 rb_trav_time = 0, wavl_trav_time = 0;
        
        u64 total_inserts = 0, total_deletes = 0, delete_misses = 0;
        int i;
        int is_cfs = (strcmp(buf, "cfs_trace") == 0);
        int trace_size = is_cfs ? CFS_TRACE_SIZE : TRACE_SIZE;
        
        pr_info("[Latency-Test] Starting %s replay (%d ops)...\n", buf, trace_size);

        for (i = 0; i < trace_size; i++) {
            unsigned long long key = is_cfs ? cfs_trace_data[i].key : real_trace[i].key;
            int is_insert = is_cfs ? cfs_trace_data[i].is_insert : real_trace[i].is_insert;

            if (is_insert) {
                /* pre allocate */
                struct my_node *rb_new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
                struct my_node *wavl_new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
                if (!rb_new || !wavl_new) continue;
                rb_new->key = key;
                wavl_new->key = key;

                //  RB Tree Insert
                t_start = ktime_get_ns();
                do_rb_insert(rb_new, &my_test_tree);
                t_end = ktime_get_ns();
                rb_ins_time += (t_end - t_start);

                //  WAVL Tree Insert
                t_start = ktime_get_ns();
                do_wavl_insert(wavl_new, &my_wavl_tree);
                t_end = ktime_get_ns();
                wavl_ins_time += (t_end - t_start);

                total_inserts++;
            } else {
                //  RB Tree Lookup
                t_start = ktime_get_ns();
                struct my_node *rb_target = do_search(key, &my_test_tree);
                t_end = ktime_get_ns();
                rb_lookup_time += (t_end - t_start);

                //  WAVL Tree Lookup
                t_start = ktime_get_ns();
                struct my_node *wavl_target = do_search(key, &my_wavl_tree);
                t_end = ktime_get_ns();
                wavl_lookup_time += (t_end - t_start);

                if (rb_target && wavl_target) {
                    //  RB Tree Erase
                    t_start = ktime_get_ns();
                    rb_erase(&rb_target->node, &my_test_tree);
                    t_end = ktime_get_ns();
                    rb_del_time += (t_end - t_start);
                    kfree(rb_target);

                    //  WAVL Tree Erase
                    t_start = ktime_get_ns();
                    wavl_erase(&wavl_target->node, &my_wavl_tree);
                    t_end = ktime_get_ns();
                    wavl_del_time += (t_end - t_start);
                    kfree(wavl_target);

                    total_deletes++;
                } else {
                    delete_misses++;
                }
            }
        }

        /* Traversal (In-order) */
        struct rb_node *node;
        volatile unsigned long long dummy_sum = 0; // avoid compiler optimization

        t_start = ktime_get_ns();
        for (node = rb_first(&my_test_tree); node; node = rb_next(node)) {
            dummy_sum += container_of(node, struct my_node, node)->key;
        }
        t_end = ktime_get_ns();
        rb_trav_time = (t_end - t_start);

        dummy_sum = 0;
        t_start = ktime_get_ns();
        for (node = rb_first(&my_wavl_tree); node; node = rb_next(node)) {
            dummy_sum += container_of(node, struct my_node, node)->key;
        }
        t_end = ktime_get_ns();
        wavl_trav_time = (t_end - t_start);

        pr_info("==================================================\n");
        pr_info("     [ Hardware Latency Benchmark (ns) ]\n");
        pr_info("==================================================\n");
        pr_info("Workload Type     : %s\n", buf);
        pr_info("Total Inserts     : %llu\n", total_inserts);
        pr_info("Total Deletes     : %llu\n", total_deletes);
        pr_info("==================================================\n");
        pr_info("Metric (Avg/Total)|    Native RB   |    WAVL Tree\n");
        pr_info("--------------------------------------------------\n");
        
        pr_info("Avg Insert Latency| %9llu ns | %9llu ns\n", 
                total_inserts > 0 ? rb_ins_time / total_inserts : 0, 
                total_inserts > 0 ? wavl_ins_time / total_inserts : 0);

        pr_info("Avg Lookup Latency| %9llu ns | %9llu ns\n", 
                (total_deletes + delete_misses) > 0 ? rb_lookup_time / (total_deletes + delete_misses) : 0, 
                (total_deletes + delete_misses) > 0 ? wavl_lookup_time / (total_deletes + delete_misses) : 0);

        pr_info("Avg Erase Latency | %9llu ns | %9llu ns\n", 
                total_deletes > 0 ? rb_del_time / total_deletes : 0, 
                total_deletes > 0 ? wavl_del_time / total_deletes : 0);

        pr_info("Total Traversal   | %9llu ns | %9llu ns\n", rb_trav_time, wavl_trav_time);
        pr_info("==================================================\n");

        struct my_node *pos, *n;
        rbtree_postorder_for_each_entry_safe(pos, n, &my_test_tree, node) kfree(pos);
        rbtree_postorder_for_each_entry_safe(pos, n, &my_wavl_tree, node) kfree(pos);
        my_test_tree = RB_ROOT;
        my_wavl_tree = RB_ROOT;
    }
    else {
        pr_err("[Latency-Test] Unknown command. Use 'cfs_trace' or 'timer_trace'\n");
        return -EINVAL;
    }

    return count;
}

static const struct proc_ops rbtree_proc_ops = {
    .proc_write = rbtree_proc_write,
};

static int __init rbtree_latency_init(void) {
    proc_create(PROC_NAME, 0666, NULL, &rbtree_proc_ops);
    pr_info("[Latency-Test] %s module loaded\n", PROC_NAME);
    return 0;
}

static void __exit rbtree_latency_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("[Latency-Test] module unloaded\n");
}

module_init(rbtree_latency_init);
module_exit(rbtree_latency_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Yen Yu");
MODULE_DESCRIPTION("Latency measurement module for Native RB-Tree and WAVL-Tree");