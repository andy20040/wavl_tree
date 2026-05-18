#include <linux/module.h>
#include <linux/init.h>
#include <linux/rbtree.h>


extern void wavl_insert_color(struct rb_node *node, struct rb_root *root);


struct my_wavl_node {
    int key;
    struct rb_node node;
};


struct rb_root my_tree = RB_ROOT;

#define TEST_NODES_COUNT 7
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
            return -1; //duplicate keys
    }


    rb_link_node(&data->node, parent, new);

    wavl_insert_color(&data->node, root);

    return 0;
}

//module entry
static int __init wavl_test_init(void) {
    int i;
    struct my_wavl_node *root_node;

    pr_info("WAVL Tree Test: loading module...\n");

    //sequential insert 1 to 7
    for (i = 0; i < TEST_NODES_COUNT; i++) {
        test_nodes[i].key = i + 1; // keys: 1, 2, 3, 4, 5, 6, 7
        pr_info("WAVL Tree Test: insert Key = %d\n", test_nodes[i].key);
        
        if (my_wavl_insert(&my_tree, &test_nodes[i]) != 0) {
            pr_err("WAVL Tree Test: insert fail have duplicate keys! \n");
            return -1;
        }
    }

    pr_info("WAVL Tree Test: insert complete!\n");

    // without  rebalancing, root will be 1
    // If the logic is correct root will be 4
    if (my_tree.rb_node) {
        root_node = container_of(my_tree.rb_node, struct my_wavl_node, node);
        pr_info("WAVL Tree Test: root key is  -> %d\n", root_node->key);
        
        if (root_node->key == 4) {
            pr_info("Tree is balanced\n");
        } else {
            pr_info("Tree is not balanced\n");
        }
    }

    return 0; 
}

// module exit

static void __exit wavl_test_exit(void) {
    pr_info("WAVL Tree Test: module unloaded successfully \n");
}

module_init(wavl_test_init);
module_exit(wavl_test_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Yen Yu Chen");
MODULE_DESCRIPTION("WAVL Tree Insertion Test");