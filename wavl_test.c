#include <linux/module.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/random.h> 
#include <linux/proc_fs.h>   
#include <linux/uaccess.h>   
#include <linux/string.h>
#define PROC_NAME "wavl_cmd"
extern void wavl_insert_color(struct rb_node *node, struct rb_root *root);
extern void wavl_erase(struct rb_node *node, struct rb_root *root);

struct my_wavl_node {
    int key;
    int in_tree;
    struct rb_node node;
};

struct rb_root my_tree = RB_ROOT;

#define TEST_NODES_COUNT 20  //total node to insert
#define DELETE_NODES_COUNT 10 //total node to delete
#define ACTION_COUNT 50 //total actions for mixed
static struct my_wavl_node test_nodes[TEST_NODES_COUNT];
static struct proc_dir_entry *wavl_proc_ent;

static int my_wavl_insert(struct rb_root *root, struct my_wavl_node *data) {
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    if (data->in_tree) return -1; //if already on tree stop insertion
    while (*new) { //find place to insert
        struct my_wavl_node *this = container_of(*new, struct my_wavl_node, node);
        parent = *new;
        if (data->key < this->key)
            new = &((*new)->rb_left);
        else if (data->key > this->key)
            new = &((*new)->rb_right);
        else
            new = &((*new)->rb_right);//same key
    }

    rb_link_node(&data->node, parent, new);
    wavl_insert_color(&data->node, root);
    data->in_tree = 1;
    return 0;
}
static void my_wavl_erase(struct rb_root *root, struct my_wavl_node *data) {
    if (data->in_tree) {
        wavl_erase(&data->node, root);
        data->in_tree = 0; // node gone
    }
}
static void print_root_key(const char *test_name) {
    if (my_tree.rb_node) {
        struct my_wavl_node *root_node = container_of(my_tree.rb_node, struct my_wavl_node, node);
        pr_info("WAVL [%s]:  Root Key is -> %d\n", test_name, root_node->key);
    }
}
static void print_tree_inorder(struct rb_root *root) {
    struct rb_node *node;
    int count = 0;
    
    pr_info("=== Inorder Result===\n");
    

    for (node = rb_first(root); node; node = rb_next(node)) {
        struct my_wavl_node *my_node = container_of(node, struct my_wavl_node, node);
        pr_info("  Key = %d\n", my_node->key);
    }
    pr_info("=====================================\n");
}
static void run_test(const char *test_type) {
    int i;
    my_tree = RB_ROOT; 
    for (i = 0; i < TEST_NODES_COUNT; i++) {
        test_nodes[i].in_tree = 0;//reset nodes
    }
    if (strcmp(test_type, "seq", 3) == 0) {
        pr_info("[Running] Sequential Insert 1 ~ %d...\n", TEST_NODES_COUNT);
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = i + 1; 
            my_wavl_insert(&my_tree, &test_nodes[i]);
        }
    } 
    else if (strcmp(test_type, "rev", 3) == 0) {
        pr_info("[Running] Reverse Insert %d ~ 1...\n", TEST_NODES_COUNT);
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = TEST_NODES_COUNT - i; 
            my_wavl_insert(&my_tree, &test_nodes[i]);
        }
    } 
    else if (strcmp(test_type, "rand", 4) == 0) {
        pr_info("[Running] Random Insert...(including duplicate numbers)\n");
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = get_random_u32() % 10; 
            my_wavl_insert(&my_tree, &test_nodes[i]);
        }
    } 
    else if (strcmp(test_type, "rand_del", 8) == 0) {
        pr_info("[Running] Random Delete Test...insert 1 to %d first\n",TEST_NODES_COUNT);
        //insert 1 to TEST_NODES_COUNT
        for (i = 0; i < TEST_NODES_COUNT; i++) {
            test_nodes[i].key = i + 1;
            my_wavl_insert(&my_tree, &test_nodes[i]);
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
            }
        }
    }
    else if (strcmp(test_type, "mixed", 5) == 0) {
        pr_info("[Running] Mixed Insert/Delete  Test...\n"); 
        for (i = 0; i < ACTION_COUNT; i++) {
            int idx = get_random_u32() % TEST_NODES_COUNT;
            //reverse action
            if (!test_nodes[idx].in_tree) {
                test_nodes[idx].key = get_random_u32() % 100;
                pr_info("  -> [Action %d] Inserting key %d (idx: %d)\n", i+1, test_nodes[idx].key, idx);
                my_wavl_insert(&my_tree, &test_nodes[idx]);
            } 
            else {
                pr_info("  -> [Action %d] Deleting key %d (idx: %d)\n", i+1, test_nodes[idx].key, idx);
                my_wavl_erase(&my_tree, &test_nodes[idx]);
            }
        }
    }
    else {
        pr_info("[ERROR] unknown instruction: %s\n", test_type);
        pr_info("Support auguments: seq, rev, rand\n");
        return;
    }
    
    print_tree_inorder(&my_tree);
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
MODULE_DESCRIPTION("WAVL Tree Stress Test");