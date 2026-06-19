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
        this_cpu_inc(wavl_path_length);
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
                wavl_rotate_right(parent,root,augment_rotate); //rotate  
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
                wavl_rotate_left(parent,root,augment_rotate); //rotate 
                wavl_demote(parent);  //demote z
                break; //tree is balanced
            }
            else { //double rotate
                struct rb_node *y = node->rb_left;
                wavl_rotate_right(node,root,augment_rotate);
                wavl_rotate_left(parent,root,augment_rotate);
                wavl_promote(y); //promote y
                wavl_demote(node);//demote x
                wavl_demote(parent); //demot z 
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
static __always_inline void ____wavl_erase(struct rb_node *rebalance_node, struct rb_root *root, void (*augment_rotate)(struct rb_node *old, struct rb_node *new)) {
    struct rb_node *x = rebalance_node; // p(x)
    struct rb_node  *sibling;

    while (true) {
        this_cpu_inc(wavl_path_length);
        unsigned long diff_l = wavl_rank_diff(x, x->rb_left);
        unsigned long diff_r = wavl_rank_diff(x, x->rb_right);
        // 2-2 leaf
        if (diff_l == 2 && diff_r == 2 && wavl_is_leaf(x)) {
            wavl_demote(x);
            x = wavl_parent(x);
            if (!x) break;
            continue;
        }
        if (diff_l != 3 && diff_r != 3) {
            break;
        }
        bool x_on_left = (diff_l == 3);
        
        if (x_on_left) { //x on left
            sibling = x->rb_right;
            
            // y is 2-child 
            if (wavl_rank_diff(x, sibling) == 2) {
                wavl_demote(x);       // demote p(x)
                x = wavl_parent(x);   // keep check top layer
                if (!x) break;
                continue;
            }
            
            // y is 1-child and  2,2
            if (wavl_rank_diff(sibling, sibling->rb_left) == 2 && 
                wavl_rank_diff(sibling, sibling->rb_right) == 2) {
                wavl_demote(x);       // demote p(x)
                wavl_demote(sibling); // demote y
                x = wavl_parent(x);   // keep check top layer
                if (!x) break;
                continue;
            }

            // Rotate
            struct rb_node *z = x;
            struct rb_node *y = sibling;
            struct rb_node *v = y->rb_left;
            struct rb_node *w = y->rb_right;

            // Rotate: w is 1-child 
            if (wavl_rank_diff(y, w) == 1) {
                wavl_rotate_left(z, root,augment_rotate);
                wavl_promote(y);      // promote y
                wavl_demote(z);       // demote z
                if (wavl_is_leaf(z)) {
                    wavl_demote(z);   
                }
                break; // tree is balanced
            }
            // Double Rotate: w is 2-child ( v is 1-child)
            else {
                wavl_rotate_right(y, root,augment_rotate);
                wavl_rotate_left(z, root,augment_rotate);
                
                wavl_promote(v);      // promote v
                wavl_promote(v);      // promote v 
                wavl_demote(y);       // demote y
                wavl_demote(z);       // demote z
                wavl_demote(z);       // demote z 
                break; // tree is balanced
            }
        } 
        else { //node on right
            sibling = x->rb_left;
            
            if (wavl_rank_diff(x, sibling) == 2) {
                wavl_demote(x);       
                x = wavl_parent(x);   
                if (!x) break;
                continue;
            }
            
            if (wavl_rank_diff(sibling, sibling->rb_left) == 2 && 
                wavl_rank_diff(sibling, sibling->rb_right) == 2) {
                wavl_demote(x);       
                wavl_demote(sibling); 
                x = wavl_parent(x);   
                if (!x) break;
                continue;
            }

            struct rb_node *z = x;
            struct rb_node *y = sibling;
            struct rb_node *v = y->rb_right; 
            struct rb_node *w = y->rb_left;  

            if (wavl_rank_diff(y, w) == 1) {
                wavl_rotate_right(z, root,augment_rotate);
                wavl_promote(y);      
                wavl_demote(z);       
                if (wavl_is_leaf(z)) {
                    wavl_demote(z);   
                }
                break; 
            }
   
            else {
                wavl_rotate_left(y, root,augment_rotate);
                wavl_rotate_right(z, root,augment_rotate);
                
                wavl_promote(v);
                wavl_promote(v);
                wavl_demote(y);
                wavl_demote(z);
                wavl_demote(z);
                break; 
            }
        }
    }
}
void wavl_insert(struct rb_node *node, struct rb_root *root)
{
	__wavl_insert(node, root, dummy_rotate);
}
//EXPORT_SYMBOL(wavl_insert);
void __wavl_erase(struct rb_node *parent, struct rb_root *root,void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{
	____wavl_erase(parent, root, augment_rotate);
}
//EXPORT_SYMBOL(__wavl_erase);
void wavl_erase(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *rebalance;
	rebalance = __wavl_erase_augmented(node, root, &dummy_callbacks);
	if (rebalance)
		____wavl_erase(rebalance, root, dummy_rotate);
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