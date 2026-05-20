#include <linux/module.h>
#include <linux/rbtree.h> 
#include "wavl_tree_augmented.h"



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
        if (wavl_rank_diff(parent, node) != 0) //check 0 violation
        break;
        if(node==parent->rb_left){ //node on left
            sibling=parent->rb_right;
            if(wavl_rank_diff(parent, sibling)==1){
                // 0,1 case go promote
                wavl_promote(parent);
                node=parent;
                continue;
            }
            else if(!(node->rb_right)||wavl_rank_diff(node,node->rb_right)==2){ //single rotate
                wavl_rotate_right(parent,root); //rotate  
                wavl_demote(parent);  //demote z
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
                wavl_promote(y);      // promote y
                wavl_demote(node);    // demote x
                wavl_demote(parent); // demote z
                break; //tree is balanced
            }
        }
        else{   //node on right
            sibling=parent->rb_left;
            if(wavl_rank_diff(parent, sibling)==1){
                // 0,1 case go promote
                wavl_promote(parent);
                node=parent;
                continue;
            }
            else if(!(node->rb_left)||wavl_rank_diff(node,node->rb_left)==2){  //single rotate
                wavl_rotate_left(parent,root); //rotate 
                wavl_demote(parent);  //demote z
                break; //tree is balanced
            }
            else { //double rotate
                struct rb_node *y = node->rb_left;
                wavl_rotate_right(node,root);
                wavl_rotate_left(parent,root);
                wavl_promote(y); //promote y
                wavl_demote(node);//demote x
                wavl_demote(parent); //demot z 
                break; //tree is balanced
            }
        }
    }
}
EXPORT_SYMBOL(wavl_insert_color);
static const struct wavl_augment_callbacks dummy_callbacks = { NULL, NULL, NULL };
static void dummy_rotate(struct rb_node *old, struct rb_node *new) {}


void wavl_erase(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *rebalance;
	rebalance = __wavl_erase_augmented(node, root, &dummy_callbacks);
	if (rebalance)
		____wavl_erase_color(rebalance, root, dummy_rotate);
}
void ____wavl_erase_color(struct rb_node *rebalance_node, struct rb_root *root, void (*augment_rotate)(struct rb_node *old, struct rb_node *new)) {
    
}
EXPORT_SYMBOL(wavl_erase);