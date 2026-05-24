#include <linux/module.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/random.h> 
#include <linux/proc_fs.h>   
#include <linux/uaccess.h>   
#include <linux/string.h>
#include "wavl_tree_augmented.h"
#define PROC_NAME "wavl_cmd"
static int nodecount=0;

struct my_wavl_node {
    int key;
    int in_tree;
    int subtree_max;
    struct rb_node node;
};

struct rb_root_cached my_tree = RB_ROOT_CACHED;


static inline bool my_wavl_less(struct rb_node *node_new, const struct rb_node *node_parent)
{
    struct my_wavl_node *n = container_of(node_new, struct my_wavl_node, node);
    struct my_wavl_node *p = container_of(node_parent, struct my_wavl_node, node);
    
    return n->key < p->key; 
}


static inline int compute_node_max(struct my_wavl_node *node) {
    return node->key; //define augment  value is the key itself
}

WAVL_DECLARE_CALLBACKS_MAX(static, my_aug_callbacks,struct my_wavl_node, node, int, subtree_max, compute_node_max)
#define TEST_NODES_COUNT 20  //total node to insert
#define DELETE_NODES_COUNT 10 //total node to delete
#define ACTION_COUNT 50 //total actions for mixed
static struct my_wavl_node test_nodes[TEST_NODES_COUNT];
static struct proc_dir_entry *wavl_proc_ent;

static int my_wavl_insert(struct rb_root_cached *root, struct my_wavl_node *data) {
    struct rb_node **link = &root->rb_root.rb_node;
    struct rb_node *parent = NULL;
    bool leftmost = true;

    if (data->in_tree) return -1;

    while (*link) {
        parent = *link;
        if (my_wavl_less(&data->node, parent)) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
            leftmost = false;
        }
    }
    data->subtree_max = data->key;
    rb_link_node(&data->node, parent, link);
    if (parent) {
        my_aug_callbacks.propagate(parent, NULL);
    }
    wavl_insert_augmented_cached(&data->node, root, leftmost, &my_aug_callbacks);
    
    data->in_tree = 1;
    nodecount++;
    return 0;
}

static void my_wavl_erase(struct rb_root_cached *root, struct my_wavl_node *data) {
    if (data->in_tree) {
        if (root->rb_leftmost == &data->node)
            root->rb_leftmost = rb_next(&data->node);
        wavl_erase_augmented(&data->node, &root->rb_root, &my_aug_callbacks);
        data->in_tree = 0;
        nodecount--;
    }
}
static void print_tree_inorder(struct rb_root *root) {
    struct rb_node *node;
    pr_info("=== Inorder Result===\n");
    

    for (node = rb_first(root); node; node = rb_next(node)) {
        struct my_wavl_node *my_node = container_of(node, struct my_wavl_node, node);
        pr_info("  Key = %d\n", my_node->key);
    }
    pr_info("=====================================\n");
}
static void print_tree_reverse(struct rb_root *root) {
    struct rb_node *node;
    pr_info("=== Reverse Inorder Result ===\n");
    for (node = rb_last(root); node; node = rb_prev(node)) {
        struct my_wavl_node *my_node = container_of(node, struct my_wavl_node, node);
        pr_info("  Key = %d\n", my_node->key);
    }
    pr_info("=====================================\n");
}


static int verify_leftmost(struct rb_root_cached *root) {
    struct rb_node *curr = root->rb_root.rb_node;
    struct rb_node *true_leftmost = NULL;

    // move to left deepest
    while (curr) {
        true_leftmost = curr;
        curr = curr->rb_left;
    }

    if (root->rb_leftmost != true_leftmost) {
        pr_err("[ERROR] Leftmost Cache error！should be %p, not %p\n", true_leftmost, root->rb_leftmost);
        return -1;
    }
    return 0;
}
static int verify_augmented_metadata(struct rb_node *node, int *error_flag) {
    struct my_wavl_node *m;
    int true_max, l_max, r_max;

    if (!node) return 0; 

    m = container_of(node, struct my_wavl_node, node);
    true_max = m->key; // default max itself

    if (node->rb_left) {
        l_max = verify_augmented_metadata(node->rb_left, error_flag);
        if (l_max > true_max) true_max = l_max;
    }
    if (node->rb_right) {
        r_max = verify_augmented_metadata(node->rb_right, error_flag);
        if (r_max > true_max) true_max = r_max;
    }

    if (m->subtree_max != true_max) {
        pr_err("[ERROR] Augmented data error ！Key %d recorded max is %d，but actual subtree  max is %d\n", 
               m->key, m->subtree_max, true_max);
        *error_flag = 1;
    }

    return true_max;
}
static int verify_wavl_properties(struct rb_root_cached *root) {
    struct rb_node *node;
    int count = 0;
    int error = 0; 

    for (node = rb_first(&root->rb_root); node; node = rb_next(node)) {
        struct my_wavl_node *my_node = container_of(node, struct my_wavl_node, node);
        
        unsigned long diff_l = wavl_rank_diff(node, node->rb_left);
        unsigned long diff_r = wavl_rank_diff(node, node->rb_right);

        if (node->rb_left && wavl_parent(node->rb_left) != node) {
            pr_err("[ERROR] Parent mismatch! Key %d left\n", my_node->key);
            error = 1;
        }
        if (node->rb_right && wavl_parent(node->rb_right) != node) {
            pr_err("[ERROR] Parent mismatch! Key %d right\n", my_node->key);
            error = 1;
        }

        if (diff_l != 1 && diff_l != 2) {
            pr_err("[ERROR] Rank Violation! Key %d left diff is %lu\n", my_node->key, diff_l);
            error = 1;
        }
        if (diff_r != 1 && diff_r != 2) {
            pr_err("[ERROR] Rank Violation! Key %d right diff is %lu\n", my_node->key, diff_r);
            error = 1;
        }
        count++;
    }

    if (verify_leftmost(root) != 0) error = 1;
    verify_augmented_metadata(root->rb_root.rb_node, &error);

    if (error) return -1; 

    pr_info("[OK] WAVL Invariants Verified. Rank, Leftmost, Augmented check passed! (%d nodes)\n", count);
    return 0;
}






static void run_test(const char *test_type) {
    struct my_wavl_node *new_node=NULL;
    int i,insertion_count=0,deletion_count=0,help=0,replace=0;
    nodecount=0;
    my_tree = RB_ROOT_CACHED; 
    for (i = 0; i < TEST_NODES_COUNT; i++) {
        test_nodes[i].in_tree = 0;//reset nodes
    }
    if (strcmp(test_type, "seq") == 0) {
        pr_info("[Running] Sequential Insert 1 ~ %d...\n", TEST_NODES_COUNT);
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = i + 1; 
            my_wavl_insert(&my_tree, &test_nodes[i]);
            insertion_count++;
        }
    } 
    else if (strcmp(test_type, "rev") == 0) {
        pr_info("[Running] Reverse Insert %d ~ 1...\n", TEST_NODES_COUNT);
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = TEST_NODES_COUNT - i; 
            my_wavl_insert(&my_tree, &test_nodes[i]);
            insertion_count++;
        }
    } 
    else if (strcmp(test_type, "rand") == 0) {
        pr_info("[Running] Random Insert...(including duplicate numbers)\n");
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = get_random_u32() % 10; 
            my_wavl_insert(&my_tree, &test_nodes[i]);
            insertion_count++;
        }
    } 
    else if (strcmp(test_type, "rand_del") == 0) {
        pr_info("[Running] Random Delete Test...insert 1 to %d first\n",TEST_NODES_COUNT);
        //insert 1 to TEST_NODES_COUNT
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = i + 1;
            my_wavl_insert(&my_tree, &test_nodes[i]);
            insertion_count++;
        }
        // random delete
        //create list to delete
        int indices[TEST_NODES_COUNT];
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            indices[i] = i;
        }
        // shuffle
        for (i = TEST_NODES_COUNT - 1; i > 0; i--) {
            int j = get_random_u32() % (i + 1);
            
            // swap i j
            int temp = indices[i];
            indices[i] = indices[j];
            indices[j] = temp;
        }
        //delete node
        for (i = 0; i < DELETE_NODES_COUNT; i++) {
            int idx = indices[i]; 
            if (test_nodes[idx].in_tree) {
                pr_info("  -> Deleting key %d (Array Index: %d)\n", test_nodes[idx].key, idx);
                my_wavl_erase(&my_tree, &test_nodes[idx]);
                deletion_count++;
            }
        }
    }
    else if (strcmp(test_type, "mixed") == 0) {
        pr_info("[Running] Mixed Insert/Delete  Test...\n"); 
        for (i = 0; i < ACTION_COUNT; i++) {
            int idx = get_random_u32() % TEST_NODES_COUNT;
            //reverse action
            if (!test_nodes[idx].in_tree) {
                test_nodes[idx].key = get_random_u32() % 100;
                pr_info("  -> [Action %d] Inserting key %d (idx: %d)\n", i+1, test_nodes[idx].key, idx);
                my_wavl_insert(&my_tree, &test_nodes[idx]);
                insertion_count++;
            } 
            else {
                pr_info("  -> [Action %d] Deleting key %d (idx: %d)\n", i+1, test_nodes[idx].key, idx);
                my_wavl_erase(&my_tree, &test_nodes[idx]);
                deletion_count++;
            }
        }
    }
    else if (strcmp(test_type, "replace") == 0) {
        replace=1;
        struct my_wavl_node *target;
        pr_info("[Running] Replace Node Test...\n");
        
        // insert first
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = i + 1;
            my_wavl_insert(&my_tree, &test_nodes[i]);
        }
        
        // choose a node
        target = &test_nodes[5];
        pr_info("  -> replace Key %d to a new node (Key 999)\n", target->key);
        
        // replace
        new_node = kmalloc(sizeof(*new_node), GFP_KERNEL); 
        new_node->key = 999;
        new_node->in_tree = 1;
        RB_CLEAR_NODE(&new_node->node);//init rb_node empty
        wavl_replace_node_cached(&target->node, &new_node->node, &my_tree);
        target->in_tree = 0;
        
        my_aug_callbacks.copy(&target->node, &new_node->node); // copy subtree_max to new node
        my_aug_callbacks.propagate(&new_node->node, NULL);     // key changed go propagate
    }
    else if (strcmp(test_type, "help") == 0) {
        pr_info("=== WAVL Tree Test Available Instructions ===\n");
        pr_info("  echo \"seq\"      > /proc/wavl_cmd  (Sequential Insert)\n");
        pr_info("  echo \"rev\"      > /proc/wavl_cmd  (Reverse Insert)\n");
        pr_info("  echo \"rand\"     > /proc/wavl_cmd  (Random Insert)\n");
        pr_info("  echo \"rand_del\" > /proc/wavl_cmd  (Random Delete Test)\n");
        pr_info("  echo \"mixed\"    > /proc/wavl_cmd  (Mixed Insert/Delete  Test)\n");
        pr_info("  echo \"replace\"    > /proc/wavl_cmd  (replace  Test)\n");
        pr_info("=============================================\n");
        help=1;
    }
    else {
        pr_info("[ERROR] unknown instruction: %s\n", test_type);
        pr_info("Support auguments: seq, rev, rand,rand_del,mixed,help\n");
        return;
    }
    if(!help){
        print_tree_inorder(&my_tree.rb_root);
        print_tree_reverse(&my_tree.rb_root);
        pr_info("total insertion count: %d \n",insertion_count);
        pr_info("total deletion count: %d \n",deletion_count);
        pr_info("node count: %d\n",nodecount);
        if (verify_wavl_properties(&my_tree) != 0) {
            pr_err(">>> WAVL Tree is BROKEN! <<<\n");
        }
        pr_info("=====================================\n");
        help=0;
        if(replace){
            replace=0;
            my_wavl_erase(&my_tree, new_node);
            kfree(new_node); //free node
        }
    }

}
static ssize_t wavl_proc_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[32]; 
    size_t copy_len = count < sizeof(buf) ? count : sizeof(buf) - 1;

    if (copy_from_user(buf, ubuf, copy_len)) {
        return -EFAULT; 
    }
    
    buf[copy_len] = '\0'; 

    // echo write have \n we need to remove it
    strim(buf);
    
    run_test(buf);

    return count; 
}
static const struct proc_ops wavl_proc_ops = {
    .proc_write = wavl_proc_write,
};

static int __init wavl_test_init(void) {
    pr_info("Loading Test Module ...\n");

    wavl_proc_ent = proc_create(PROC_NAME, 0666, NULL, &wavl_proc_ops);
    if (!wavl_proc_ent) {
        pr_err("unable to create  /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    pr_info("Create successful /proc/%s\n", PROC_NAME);
    pr_info("enter: echo \"seq\" > /proc/%s to test\n", PROC_NAME);
    
    return 0; 
}

static void __exit wavl_test_exit(void) {
    proc_remove(wavl_proc_ent);
    pr_info("WAVL Tree Test Ended\n");
}

module_init(wavl_test_init);
module_exit(wavl_test_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Yen Yu Chen");
MODULE_DESCRIPTION("WAVL Tree  Test");