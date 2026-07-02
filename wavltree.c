#include <linux/module.h>
#include <linux/rbtree.h> 
#include "wavl_tree_augmented.h"

DEFINE_PER_CPU(u64, wavl_rotations);
DEFINE_PER_CPU(u64, wavl_path_length);

static void wavl_rotate_left(struct rb_node *node, struct rb_root *root, void (*augment_rotate)(struct rb_node *old, struct rb_node *new)) {
    this_cpu_inc(wavl_rotations);
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = wavl_parent(node);
    if (WARN_ON_ONCE(!right)) return;
    if ((node->rb_right = right->rb_left))
        wavl_set_parent(right->rb_left, node);
    wavl_set_parent(right,parent);
    wavl_change_child(node,right,parent,root);
    WRITE_ONCE(right->rb_left,node);
    wavl_set_parent(node,right);
    augment_rotate(node, right); //augment fix
}

static void wavl_rotate_right(struct rb_node *node, struct rb_root *root, void (*augment_rotate)(struct rb_node *old, struct rb_node *new)) {
    this_cpu_inc(wavl_rotations);
    struct rb_node *left = node->rb_left;
    struct rb_node *parent = wavl_parent(node);
    if (WARN_ON_ONCE(!left)) return;
    if ((node->rb_left = left->rb_right))
        wavl_set_parent(left->rb_right, node);
    wavl_set_parent(left,parent);
    wavl_change_child(node,left,parent,root);
    WRITE_ONCE(left->rb_right,node);
    wavl_set_parent(node,left);
    augment_rotate(node, left);
}



static __always_inline void __wavl_insert(struct rb_node *node, struct rb_root *root,void (*augment_rotate)(struct rb_node *old, struct rb_node *new)) {
    struct rb_node *parent, *sibling;
    while(true){
        parent = wavl_parent(node);
        if (unlikely(!parent)) {
			break;
		}
        if (wavl_parity(node) != wavl_parity(parent)) break; //check 0 violation 
        bool is_left = (node == parent->rb_left);
        sibling = is_left ? parent->rb_right : parent->rb_left;
        if (wavl_parity(parent) != wavl_parity(sibling)) { // 0 , 1 violoation 
            wavl_flip_parity(parent); // Promote 
            this_cpu_inc(wavl_path_length);
            node = parent;
            continue;
        }
        // 0 , 2 violation
        if(is_left){ //node on left
            sibling=parent->rb_right;
            if(wavl_parity(node) == wavl_parity(node->rb_right)){ //single rotate rank diff=2
                wavl_rotate_right(parent,root,augment_rotate); //rotate  
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
                wavl_rotate_left(node,root,augment_rotate);
                wavl_rotate_right(parent,root,augment_rotate);
                /*
                 *       parent(z)                 parent(z)                        y
                 *      /0      \2                /        \                    /       \
                 *    node(x)    D    ==>        y          D    ==>          node(x)    z
                 *   /    \1                   /   \                         /   \     /   \
                 *  A      y                 node   C                       A     B   C     D
                 *       /   \              /    \
                 *      B     C            A      B
                */
                wavl_flip_parity(y);      // Promote y
                wavl_flip_parity(node);   // Demote x
                wavl_flip_parity(parent); // Demote z
                break; //tree is balanced
            }
        }
        else{   //node on right
            sibling=parent->rb_left;
            if(wavl_parity(node) == wavl_parity(node->rb_left)){  //single rotate rank diff=2
                wavl_rotate_left(parent,root,augment_rotate); //rotate 
                wavl_flip_parity(parent);  //demote z
                break; //tree is balanced
            }
            else { //double rotate
                struct rb_node *y = node->rb_left;
                wavl_rotate_right(node,root,augment_rotate);
                wavl_rotate_left(parent,root,augment_rotate);
                wavl_flip_parity(y);      // Promote y
                wavl_flip_parity(node);   // Demote x
                wavl_flip_parity(parent); // Demote z
                break; //tree is balanced
            }
        }
    }
}
static inline void dummy_propagate(struct rb_node *node, struct rb_node *stop) {}
static inline void dummy_copy(struct rb_node *old, struct rb_node *new) {}
static inline void dummy_rotate(struct rb_node *old, struct rb_node *new) {}
static const struct wavl_augment_callbacks dummy_callbacks = {
	.propagate = dummy_propagate,
	.copy = dummy_copy,
	.rotate = dummy_rotate
};
static __always_inline void ____wavl_erase(struct rb_node *rebalance_node, struct rb_root *root, void (*augment_rotate)(struct rb_node *old, struct rb_node *new), bool is_3_child, bool hole_on_left) {
    struct rb_node *x = rebalance_node; 

    while (x) {
        // =============== 3-child ===============
        if (is_3_child) {
            struct rb_node *sibling = hole_on_left ? x->rb_right : x->rb_left;
            if (unlikely(!sibling)) break; 
            unsigned long s_p = sibling->__rb_parent_color & WAVL_PARITY_MASK; //sibling parity
            //  Sibling 2-child 
            if (wavl_parity(x) == s_p) {
                struct rb_node *p = wavl_parent(x);
                if (p) { 
                    is_3_child = (wavl_parity(x) == wavl_parity(p));
                    hole_on_left = (x == p->rb_left);
                }
                wavl_flip_parity(x); // Demote x
                x = p;
                this_cpu_inc(wavl_path_length);
                continue;
            }
            
            if (wavl_parity(sibling->rb_left) == s_p && wavl_parity(sibling->rb_right) == s_p) { //  2,2 node  demote twice
                struct rb_node *p = wavl_parent(x);
                if (p) {
                    is_3_child = (wavl_parity(x) == wavl_parity(p));
                    hole_on_left = (x == p->rb_left);
                }
                wavl_flip_parity(x);
                wavl_flip_parity(sibling);
                x = p;
                this_cpu_inc(wavl_path_length);
                continue;
            }
            
            if (hole_on_left) {
                struct rb_node *y = sibling;
                if (wavl_parity(y->rb_right) != s_p) { //single rotate
                    wavl_rotate_left(x, root, augment_rotate);
                    wavl_flip_parity(y); // Promote y
                    wavl_flip_parity(x); // Demote x
                    if (!x->rb_left && !x->rb_right && wavl_parity(x) == 1) 
                        wavl_flip_parity(x); //  remove 2,2 leaf
                    break;
                } else {  //double rotate 
                    wavl_rotate_right(y, root, augment_rotate);
                    wavl_rotate_left(x, root, augment_rotate);
                    wavl_flip_parity(y); //  Demote y！
                    break;
                }
            } else { 
                struct rb_node *y = sibling;    
                if (wavl_parity(y->rb_left) != s_p) { //single rotate
                    wavl_rotate_right(x, root, augment_rotate);
                    wavl_flip_parity(y); 
                    wavl_flip_parity(x); 
                    if (!x->rb_left && !x->rb_right && wavl_parity(x) == 1) 
                        wavl_flip_parity(x); 
                    break;
                } else { //double rotate 
                    wavl_rotate_left(y, root, augment_rotate);
                    wavl_rotate_right(x, root, augment_rotate); //no need to write code if double demote/ promote
                    wavl_flip_parity(y); 
                    break;
                }
            }
        } 
        // =============== 2,2 Leaf  ===============
        else if (!x->rb_left && !x->rb_right && wavl_parity(x) == 1) {
            struct rb_node *p = wavl_parent(x);
            if (p) {
                is_3_child = (wavl_parity(x) == wavl_parity(p));
                hole_on_left = (x == p->rb_left);
            }
            wavl_flip_parity(x);
            x = p;
            this_cpu_inc(wavl_path_length);
            continue;
        } else {
            break; 
        }
    }
}
void wavl_insert(struct rb_node *node, struct rb_root *root)
{
	__wavl_insert(node, root, dummy_rotate);
}
//EXPORT_SYMBOL(wavl_insert);
void __wavl_erase(struct rb_node *parent, struct rb_root *root,void (*augment_rotate)(struct rb_node *old, struct rb_node *new),bool is_3_child, bool hole_on_left)
{
	____wavl_erase(parent, root, augment_rotate, is_3_child, hole_on_left);
}
//EXPORT_SYMBOL(__wavl_erase);
void wavl_erase(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *victim = node;
    struct rb_node *p_victim;
    struct rb_node *w; //node to come up
    bool is_3_child = false;
    bool hole_on_left = false;
    //successor
    if (node->rb_left && node->rb_right) {
        victim = node->rb_right;
        while (victim->rb_left) {
            victim = victim->rb_left;
        }
    }

    w = victim->rb_left ? victim->rb_left : victim->rb_right;
    p_victim = wavl_parent(victim);
    
    if (p_victim) {
        is_3_child = (wavl_parity(p_victim) != wavl_parity(w));
        hole_on_left = (victim == p_victim->rb_left);
    }
    struct rb_node *rebalance = __wavl_erase_augmented(node, root, &dummy_callbacks);
    if (rebalance) {
        ____wavl_erase(rebalance, root, dummy_rotate, is_3_child, hole_on_left);
    }
}
//EXPORT_SYMBOL(wavl_erase);

void __wavl_insert_augmented(struct rb_node *node, struct rb_root *root,
	void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{
	__wavl_insert(node, root,augment_rotate);
}
//EXPORT_SYMBOL(__wavl_insert_augmented);

void wavl_replace_node(struct rb_node *victim, struct rb_node *new,
		     struct rb_root *root)
{
	struct rb_node *parent = wavl_parent(victim);

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;

	/* Set the surrounding nodes to point to the replacement */
	if (victim->rb_left)
		wavl_set_parent(victim->rb_left, new);
	if (victim->rb_right)
		wavl_set_parent(victim->rb_right, new);
	wavl_change_child(victim, new, parent, root);
}

void wavl_replace_node_rcu(struct rb_node *victim, struct rb_node *new,
			 struct rb_root *root)
{
	struct rb_node *parent = wavl_parent(victim);

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;

	/* Set the surrounding nodes to point to the replacement */
	if (victim->rb_left)
		wavl_set_parent(victim->rb_left, new);
	if (victim->rb_right)
		wavl_set_parent(victim->rb_right, new);

	/* Set the parent's pointer to the new node last after an RCU barrier
	 * so that the pointers onwards are seen to be set correctly when doing
	 * an RCU walk over the tree.
	 */
	__wavl_change_child_rcu(victim, new, parent, root);
}
//EXPORT_SYMBOL(wavl_replace_node_rcu);