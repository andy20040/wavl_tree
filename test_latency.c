#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>     
#include <linux/vmalloc.h>   
#include <linux/random.h>   
#include "wavl_tree_augmented.h"
#include "trace_data/timer_data.h"     
#include "trace_data/cfs_data.h"

#define PROC_NAME "rbtree_latency_cmd"

struct rb_root my_test_tree = RB_ROOT; // RB Tree
struct rb_root my_wavl_tree = RB_ROOT; // WAVL Tree

struct my_node {
    unsigned long long key;
    struct rb_node node;
};


static u32 my_xorshift32(u32 *state) {
    u32 x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

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
    char cmd[16] = {0};
    int req_ins = 10000;
    int req_del = 5000;
    int parsed;
    size_t copy_len = (count < sizeof(buf)) ? count : (sizeof(buf) - 1);
    
    if (copy_from_user(buf, buf_user, copy_len)) return -EFAULT;
    buf[copy_len] = '\0';
    strim(buf);

    parsed = sscanf(buf, "%15s %d %d", cmd, &req_ins, &req_del);

    /* ==========================================================
     *  (CFS / Timer)
     * ========================================================== */
    if (strcmp(cmd, "timer_trace") == 0 || strcmp(cmd, "cfs_trace") == 0) {
        u64 t_start, t_end;
        u64 rb_ins_time = 0, wavl_ins_time = 0;
        u64 rb_del_time = 0, wavl_del_time = 0;
        u64 rb_lookup_time = 0, wavl_lookup_time = 0;
        u64 rb_trav_time = 0, wavl_trav_time = 0;
        
        u64 total_inserts = 0, total_deletes = 0, delete_misses = 0;
        int i;
        int is_cfs = (strcmp(cmd, "cfs_trace") == 0);
        int trace_size = is_cfs ? CFS_TRACE_SIZE : TRACE_SIZE;
        
        pr_info("[Latency-Test] Starting %s replay (%d ops)...\n", cmd, trace_size);

        for (i = 0; i < trace_size; i++) {
            unsigned long long key = is_cfs ? cfs_trace_data[i].key : real_trace[i].key;
            int is_insert = is_cfs ? cfs_trace_data[i].is_insert : real_trace[i].is_insert;

            if (is_insert) {
                struct my_node *rb_new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
                struct my_node *wavl_new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
                if (!rb_new || !wavl_new) continue;
                rb_new->key = key; wavl_new->key = key;

                t_start = ktime_get_ns(); do_rb_insert(rb_new, &my_test_tree); t_end = ktime_get_ns();
                rb_ins_time += (t_end - t_start);

                t_start = ktime_get_ns(); do_wavl_insert(wavl_new, &my_wavl_tree); t_end = ktime_get_ns();
                wavl_ins_time += (t_end - t_start);

                total_inserts++;
            } else {
                t_start = ktime_get_ns(); struct my_node *rb_target = do_search(key, &my_test_tree); t_end = ktime_get_ns();
                rb_lookup_time += (t_end - t_start);

                t_start = ktime_get_ns(); struct my_node *wavl_target = do_search(key, &my_wavl_tree); t_end = ktime_get_ns();
                wavl_lookup_time += (t_end - t_start);

                if (rb_target && wavl_target) {
                    t_start = ktime_get_ns(); rb_erase(&rb_target->node, &my_test_tree); t_end = ktime_get_ns();
                    rb_del_time += (t_end - t_start); kfree(rb_target);

                    t_start = ktime_get_ns(); wavl_erase(&wavl_target->node, &my_wavl_tree); t_end = ktime_get_ns();
                    wavl_del_time += (t_end - t_start); kfree(wavl_target);

                    total_deletes++;
                } else {
                    delete_misses++;
                }
            }
        }

        /* Traversal (In-order) */
        struct rb_node *node;
        volatile unsigned long long dummy_sum = 0; 
        t_start = ktime_get_ns();
        for (node = rb_first(&my_test_tree); node; node = rb_next(node)) dummy_sum += container_of(node, struct my_node, node)->key;
        t_end = ktime_get_ns(); rb_trav_time = (t_end - t_start);

        dummy_sum = 0;
        t_start = ktime_get_ns();
        for (node = rb_first(&my_wavl_tree); node; node = rb_next(node)) dummy_sum += container_of(node, struct my_node, node)->key;
        t_end = ktime_get_ns(); wavl_trav_time = (t_end - t_start);

        pr_info("==================================================\n");
        pr_info("     [ Trace Latency Benchmark (ns) ]\n");
        pr_info("==================================================\n");
        pr_info("Workload Type     : %s\n", cmd);
        pr_info("Total Inserts     : %llu\n", total_inserts);
        pr_info("Total Deletes     : %llu\n", total_deletes);
        pr_info("==================================================\n");
        pr_info("Metric (Avg/Total)|    Native RB   |    WAVL Tree\n");
        pr_info("--------------------------------------------------\n");
        pr_info("Avg Insert Latency| %9llu ns | %9llu ns\n", total_inserts > 0 ? rb_ins_time / total_inserts : 0, total_inserts > 0 ? wavl_ins_time / total_inserts : 0);
        pr_info("Avg Lookup Latency| %9llu ns | %9llu ns\n", (total_deletes + delete_misses) > 0 ? rb_lookup_time / (total_deletes + delete_misses) : 0, (total_deletes + delete_misses) > 0 ? wavl_lookup_time / (total_deletes + delete_misses) : 0);
        pr_info("Avg Erase Latency | %9llu ns | %9llu ns\n", total_deletes > 0 ? rb_del_time / total_deletes : 0, total_deletes > 0 ? wavl_del_time / total_deletes : 0);
        pr_info("Total Traversal   | %9llu ns | %9llu ns\n", rb_trav_time, wavl_trav_time);
        pr_info("==================================================\n");

        struct my_node *pos, *n;
        rbtree_postorder_for_each_entry_safe(pos, n, &my_test_tree, node) kfree(pos);
        rbtree_postorder_for_each_entry_safe(pos, n, &my_wavl_tree, node) kfree(pos);
        my_test_tree = RB_ROOT; my_wavl_tree = RB_ROOT;
    }
    /* ==========================================================
     *  (Random / Seq / Reverse)
     * ========================================================== */
    else if (strcmp(cmd, "random") == 0 || strcmp(cmd, "randomseed") == 0 || strcmp(cmd, "seq") == 0 ||
             strcmp(cmd, "reverse") == 0 || strcmp(cmd, "seq_rev") == 0 || strcmp(cmd, "rev_seq") == 0||strcmp(cmd, "lookup") == 0) {
        
        int is_random  = (strcmp(cmd, "randomseed") == 0);
        int full_rand  = (strcmp(cmd, "random") == 0);
        int is_seq     = (strcmp(cmd, "seq") == 0);
        int is_rev     = (strcmp(cmd, "reverse") == 0);
        int is_seq_rev = (strcmp(cmd, "seq_rev") == 0);
        int is_rev_seq = (strcmp(cmd, "rev_seq") == 0);
        int look_up    =  (strcmp(cmd, "lookup")==0) ;
        int TOTAL_OPERATIONS = req_ins;
        int DELETE_OPERATIONS = (req_del > req_ins) ? req_ins : req_del;
        u32 prng_state = 123456789;
        int i;
        
        u64 t_start, t_end;
        u64 rb_ins_time = 0, wavl_ins_time = 0;
        u64 rb_del_time = 0, wavl_del_time = 0;
        u64 rb_lookup_time = 0, wavl_lookup_time = 0;
        u64 rb_trav_time = 0, wavl_trav_time = 0;
        
        struct my_node **rb_nodes = vmalloc(TOTAL_OPERATIONS * sizeof(struct my_node *));
        struct my_node **wavl_nodes = vmalloc(TOTAL_OPERATIONS * sizeof(struct my_node *));
        int *indices = vmalloc(TOTAL_OPERATIONS * sizeof(int));

        if (!rb_nodes || !wavl_nodes || !indices) {
            pr_err("Memory allocation failed!\n");
            if (rb_nodes) vfree(rb_nodes);
            if (wavl_nodes) vfree(wavl_nodes);
            if (indices) vfree(indices);
            return -ENOMEM; 
        }

        pr_info("[Latency-Test] Generating %s workload (Inserts: %d, Deletes: %d)...\n", cmd, TOTAL_OPERATIONS, DELETE_OPERATIONS);

        /* 1. Insert Phase */
        for (i = 0; i < TOTAL_OPERATIONS; i++) {
            u64 key;
            unsigned int rand_val;
            struct my_node *rb_new, *wavl_new;

            if (is_random) key = my_xorshift32(&prng_state) % 1000000;
            else if(full_rand) { get_random_bytes(&rand_val, sizeof(rand_val)); key = rand_val % 1000000; }
            else if (is_rev || is_rev_seq) key = TOTAL_OPERATIONS - 1 - i;
            else key = i;


            rb_new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
            wavl_new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
            rb_new->key = key; wavl_new->key = key;
            rb_nodes[i] = rb_new; wavl_nodes[i] = wavl_new;


            t_start = ktime_get_ns();
            do_rb_insert(rb_new, &my_test_tree);
            t_end = ktime_get_ns();
            rb_ins_time += (t_end - t_start);


            t_start = ktime_get_ns();
            do_wavl_insert(wavl_new, &my_wavl_tree);
            t_end = ktime_get_ns();
            wavl_ins_time += (t_end - t_start);

            indices[i] = i;
        }

        /* 2. Traversal Phase (In-order) */
        {
            struct rb_node *node;
            volatile unsigned long long dummy_sum = 0; 
            t_start = ktime_get_ns();
            for (node = rb_first(&my_test_tree); node; node = rb_next(node)) dummy_sum += container_of(node, struct my_node, node)->key;
            t_end = ktime_get_ns(); rb_trav_time = (t_end - t_start);

            dummy_sum = 0;
            t_start = ktime_get_ns();
            for (node = rb_first(&my_wavl_tree); node; node = rb_next(node)) dummy_sum += container_of(node, struct my_node, node)->key;
            t_end = ktime_get_ns(); wavl_trav_time = (t_end - t_start);
        }

        /* ==========================================
        * Setup Deletion Indices 
        * ========================================== */
        for (i = 0; i < TOTAL_OPERATIONS; i++) {
            if (is_random || full_rand||look_up) {  
                indices[i] = i;
            } else {
                int target_key;
                if (is_seq || is_rev_seq) {
                    target_key = i; 
                } else {
                    target_key = TOTAL_OPERATIONS - 1 - i; 
                }
                if (is_rev || is_rev_seq) {
                    indices[i] = TOTAL_OPERATIONS - 1 - target_key;
                } else {
                    indices[i] = target_key;
                }
            }
        }
        /* ==========================================
        * shuffle
        * ========================================== */

        if (is_random || full_rand ||look_up) { 
            for (i = TOTAL_OPERATIONS - 1; i > 0; i--) {
                u32 j = my_xorshift32(&prng_state) % (i + 1);
                int temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
        if (look_up) {
            for (i = 0; i < DELETE_OPERATIONS; i++) {
                int target_idx = indices[i]; 
                u64 target_key = rb_nodes[target_idx]->key; 

                t_start = ktime_get_ns(); 
                do_search(target_key, &my_test_tree); 
                t_end = ktime_get_ns();
                rb_lookup_time += (t_end - t_start);

                t_start = ktime_get_ns(); 
                do_search(target_key, &my_wavl_tree); 
                t_end = ktime_get_ns();
                wavl_lookup_time += (t_end - t_start);
            }

            pr_info("==================================================\n");
            pr_info("     [ Pure Lookup Benchmark (Read-Only) ]\n");
            pr_info("==================================================\n");
            pr_info("Tree Size (Nodes) : %d\n", TOTAL_OPERATIONS);
            pr_info("Total Lookups     : %d\n", DELETE_OPERATIONS);
            pr_info("--------------------------------------------------\n");
            pr_info("Metric (Avg)      |    Native RB   |    WAVL Tree\n");
            pr_info("--------------------------------------------------\n");
            pr_info("Avg Lookup Latency| %9llu ns | %9llu ns\n", 
                DELETE_OPERATIONS > 0 ? rb_lookup_time / DELETE_OPERATIONS : 0, 
                DELETE_OPERATIONS > 0 ? wavl_lookup_time / DELETE_OPERATIONS : 0);
            pr_info("==================================================\n");

        } 
        else {
            for (i = 0; i < DELETE_OPERATIONS; i++) {
                int target_idx = indices[i]; 
                
                if (rb_nodes[target_idx]) {
                    t_start = ktime_get_ns();
                    rb_erase(&rb_nodes[target_idx]->node, &my_test_tree);
                    t_end = ktime_get_ns();
                    rb_del_time += (t_end - t_start);
                    
                    kfree(rb_nodes[target_idx]); 
                    rb_nodes[target_idx] = NULL; 
                }
                if (wavl_nodes[target_idx]) {
                    t_start = ktime_get_ns();
                    wavl_erase(&wavl_nodes[target_idx]->node, &my_wavl_tree);
                    t_end = ktime_get_ns();
                    wavl_del_time += (t_end - t_start);

                    kfree(wavl_nodes[target_idx]);
                    wavl_nodes[target_idx] = NULL;
                }
            }

            pr_info("==================================================\n");
            pr_info("     [ Bulk Latency Benchmark (ns) ]\n");
            pr_info("==================================================\n");
            pr_info("Workload Type     : %s\n", cmd);
            pr_info("Total Inserts     : %d\n", TOTAL_OPERATIONS);
            pr_info("Total Deletes     : %d\n", DELETE_OPERATIONS);
            pr_info("==================================================\n");
            pr_info("Metric (Avg/Total)|    Native RB   |    WAVL Tree\n");
            pr_info("--------------------------------------------------\n");
            pr_info("Avg Insert Latency| %9llu ns | %9llu ns\n", TOTAL_OPERATIONS > 0 ? rb_ins_time / TOTAL_OPERATIONS : 0, TOTAL_OPERATIONS > 0 ? wavl_ins_time / TOTAL_OPERATIONS : 0);
            pr_info("Avg Erase Latency | %9llu ns | %9llu ns\n", DELETE_OPERATIONS > 0 ? rb_del_time / DELETE_OPERATIONS : 0, DELETE_OPERATIONS > 0 ? wavl_del_time / DELETE_OPERATIONS : 0);
            pr_info("Total Traversal   | %9llu ns | %9llu ns\n", rb_trav_time, wavl_trav_time);
            pr_info("==================================================\n");
        }

        /* ==========================================================
         *  (Cleanup)
         * ========================================================== */
        vfree(rb_nodes);
        vfree(wavl_nodes);
        vfree(indices);

        struct my_node *pos, *n;
        rbtree_postorder_for_each_entry_safe(pos, n, &my_test_tree, node) {
            kfree(pos);
        }
        rbtree_postorder_for_each_entry_safe(pos, n, &my_wavl_tree, node) {
            kfree(pos);
        }
        my_test_tree = RB_ROOT;
        my_wavl_tree = RB_ROOT;
    }
    else {
        pr_err("[Latency-Test] Unknown command.\n");
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
MODULE_DESCRIPTION("Comprehensive Latency measurement module");