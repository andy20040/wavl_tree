#include <linux/module.h>
#include <linux/rbtree.h> 


#define WAVL_PARITY_MASK 1UL       
#define WAVL_PARENT_MASK ~1UL    

// get node parity
static inline unsigned long wavl_parity(struct rb_node *node) {
    return node->__rb_parent_color & WAVL_PARITY_MASK;
}

// get parent pointer
static inline struct rb_node *wavl_parent(struct rb_node *node) {
    return (struct rb_node *)(node->__rb_parent_color & WAVL_PARENT_MASK);
}

// set parity without changing parent 
static inline void wavl_set_parity(struct rb_node *node, unsigned long parity) {
    node->__rb_parent_color = (node->__rb_parent_color & WAVL_PARENT_MASK) | (parity & WAVL_PARITY_MASK);
}
static inline void wavl_flip_parity(struct rb_node *node) {
    node->__rb_parent_color ^= WAVL_PARITY_MASK;
}
static inline unsigned long wavl_rank_diff(struct rb_node *parent, struct rb_node *child) {
    unsigned long p_parity = wavl_parity(parent);
    unsigned long c_parity = child ? wavl_parity(child) : 1UL; 
    return (p_parity != c_parity) ? 1UL : 2UL;
}
static inline void wavl_change_child(struct rb_node *old, struct rb_node *new,struct rb_node *parent, struct rb_root *root)
{
	if (parent) {
		if (parent->rb_left == old)
			WRITE_ONCE(parent->rb_left, new);
		else
			WRITE_ONCE(parent->rb_right, new);
	} else
		WRITE_ONCE(root->rb_node, new);
}
static inline void wavl_set_parent(struct rb_node *node,struct rb_node *parent){
    node->__rb_parent_color = ((unsigned long )parent& WAVL_PARENT_MASK)|wavl_parity(node);
}
static void wavl_rotate_left(struct rb_node *node, struct rb_root *root) {
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = wavl_parent(node);
    if ((node->rb_right = right->rb_left))
        wavl_set_parent(right->rb_left, node);
    wavl_set_parent(right,parent);
    wavl_change_child(node,right,parent,root);
    WRITE_ONCE(right->rb_left,node);
    wavl_set_parent(node,right);
}

static void wavl_rotate_right(struct rb_node *node, struct rb_root *root) {
    struct rb_node *left = node->rb_left;
    struct rb_node *parent = wavl_parent(node);
    if ((node->rb_left = left->rb_right))
        wavl_set_parent(left->rb_right, node);
    wavl_set_parent(left,parent);
    wavl_change_child(node,left,parent,root);
    WRITE_ONCE(left->rb_right,node);
    wavl_set_parent(node,left);
}


void wavl_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link) {
    node->__rb_parent_color = (unsigned long)parent; // 初始 Rank 差值設為 0
    node->rb_left = node->rb_right = NULL;
    *rb_link = node;
}
EXPORT_SYMBOL(wavl_link_node);




void wavl_insert_color(struct rb_node *node, struct rb_root *root) {
    struct rb_node *parent = rb_red_parent(node), *gparent, *sibling;
    while(true){
        if (unlikely(!parent)) {
			break;
		}
        if (wavl_rank_diff(parent, node) != 0) //check 0-violation
        break;
        gparent=wavl_parent(parent);
        if(node==parent->rb_left){ //node on left
            sibling=parent->rb_right;
            if(wavl_rank_diff(sibling)==1){
                // 0,1 case go promote
                wavl_flip_parity(parent);
                node=parent;
                parent=gparent;
                continue;
            }
        }
        else{   //node on right
            sibling=parent->rb_left;
            if(wavl_rank_diff(sibling)==1){
                // 0,1 case go promote
                wavl_flip_parity(parent);
                node=parent;
                parent=gparent;
                continue;
            }
        }
    }
}
EXPORT_SYMBOL(wavl_insert_color);

// 刪除後的重新平衡：處理 3-1 或 2-2 違規
void wavl_erase(struct rb_node *node, struct rb_root *root) {
    // TODO: WAVL 的刪除與 Rank Demotion 邏輯 (這是最難的部分，建議最後寫)
}
EXPORT_SYMBOL(wavl_erase);