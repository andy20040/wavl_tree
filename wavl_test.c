#include <linux/module.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/random.h> 

extern void wavl_insert_color(struct rb_node *node, struct rb_root *root);

struct my_wavl_node {
    int key;
    struct rb_node node;
};

struct rb_root my_tree = RB_ROOT;

#define TEST_NODES_COUNT 20  
static struct my_wavl_node test_nodes[TEST_NODES_COUNT];

static int my_wavl_insert(struct rb_root *root, struct my_wavl_node *data) {
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    
    while (*new) {
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
    return 0;
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
        pr_info("  Key = %d\n", ++count, my_node->key);
    }
    pr_info("=====================================\n");
}
//module entry
static int __init wavl_test_init(void) {
    int i, inserted;
    u32 rand_val;

    pr_info("===================================\n");
    pr_info("Test start\n");
    pr_info("===================================\n");


    pr_info("[Test 1] Seqential Insert 1 ~ %d...\n", TEST_NODES_COUNT);
    my_tree = RB_ROOT; // reset root
    for (i = 0; i < TEST_NODES_COUNT; i++) {
        test_nodes[i].key = i + 1; 
        my_wavl_insert(&my_tree, &test_nodes[i]);
    }
    print_tree_inorder(&my_tree);

    //-------------------------------------------------------------------
    pr_info("[Test 2] Reverse Insert %d ~ 1...\n", TEST_NODES_COUNT);
    my_tree = RB_ROOT; // reset root
    for (i = 0; i < TEST_NODES_COUNT; i++) {
        test_nodes[i].key = TEST_NODES_COUNT - i; 
        my_wavl_insert(&my_tree, &test_nodes[i]);
    }
    print_tree_inorder(&my_tree);

    //--------------------------------------------------------------------
    pr_info("[Test 3] Random insert %d diffrent numbers...\n", TEST_NODES_COUNT);
    my_tree = RB_ROOT; 
    inserted = 0;      // nums of insertion
    
    // get rid of the duplicate number 
    while (inserted < TEST_NODES_COUNT) {
        rand_val = get_random_u32() % 100;
        
        test_nodes[inserted].key = rand_val;
        
        if (my_wavl_insert(&my_tree, &test_nodes[inserted]) == 0) 
            inserted++;     
    }
    print_tree_inorder(&my_tree);
    //---------------------------------------------------------------------
    pr_info("[Test 4] Duplicate Key Policy Test...\n");
    my_tree = RB_ROOT; 

    for (i = 0; i < TEST_NODES_COUNT; i++) {
        rand_val = get_random_u32() % 10; 
        test_nodes[i].key = rand_val;
        my_wavl_insert(&my_tree, &test_nodes[i]);
    }
    print_tree_inorder(&my_tree);

    pr_info("===================================\n");
    pr_info("Tests Ended\n");
    pr_info("===================================\n");
    return 0; 
}

static void __exit wavl_test_exit(void) {
    pr_info("WAVL Tree Test: 模組卸載完畢。\n");
}

module_init(wavl_test_init);
module_exit(wavl_test_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Yen Yu Chen");
MODULE_DESCRIPTION("WAVL Tree Stress Test");