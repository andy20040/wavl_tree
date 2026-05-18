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



void wavl_insert_color(struct rb_node *node, struct rb_root *root) {
    struct rb_node *parent, *sibling;
    while(true){
        parent = wavl_parent(node);
        if (unlikely(!parent)) {
			break;
		}
        if (wavl_parity(parent) != wavl_parity(node)) //check 0 violation
        break;
        if(node==parent->rb_left){ //node on left
            sibling=parent->rb_right;
            if(wavl_rank_diff(parent, sibling)==1){
                // 0,1 case go promote
                wavl_flip_parity(parent);
                node=parent;
                continue;
            }
            else if(!(node->rb_right)||wavl_rank_diff(node,node->rb_right)==2){ //single rotate
                wavl_rotate_right(parent,root); //rotate  
                wavl_flip_parity(parent);  //demote z
                /*
                 *       parent(z)
                 *      /0     \2
                 *    node      C
                 *   /   \2
                 *  A     B
                 * 
                 * 
                */
               break; //tree is balanced
            }
            else { //double rotate
                struct rb_node *y = node->rb_right;
                wavl_rotate_left(node,root);
                wavl_rotate_right(parent,root);
                /*
                 *       parent(z)                 parent(z)                        y
                 *      /0      \2                /        \                    /       \
                 *    node(x)    D    ==>        y          D    ==>          node(x)    z
                 *   /    \1                   /   \                         /   \     /   \
                 *  A      y                 node   C                       A     B   C     D
                 *       /   \              /    \
                 *      B     C            A      B
                */
                wavl_flip_parity(y); //promote y
                wavl_flip_parity(node);//demote x
                wavl_flip_parity(parent); //demot z 
                break; //tree is balanced
            }
        }
        else{   //node on right
            sibling=parent->rb_left;
            if(wavl_rank_diff(parent, sibling)==1){
                // 0,1 case go promote
                wavl_flip_parity(parent);
                node=parent;
                continue;
            }
            else if(!(node->rb_left)||wavl_rank_diff(node,node->rb_left)==2){  //single rotate
                wavl_rotate_left(parent,root); //rotate 
                wavl_flip_parity(parent);  //demote z
                break; //tree is balanced
            }
            else { //double rotate
                struct rb_node *y = node->rb_left;
                wavl_rotate_right(node,root);
                wavl_rotate_left(parent,root);
                wavl_flip_parity(y); //promote y
                wavl_flip_parity(node);//demote x
                wavl_flip_parity(parent); //demot z 
                break; //tree is balanced
            }
        }
    }
}
EXPORT_SYMBOL(wavl_insert_color);
