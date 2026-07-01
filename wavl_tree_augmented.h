#ifndef _WAVL_TREE_AUGMENTED_H
#define _WAVL_TREE_AUGMENTED_H
#include "wavltree.h"
#include <linux/rcupdate.h>
#define WAVL_PARITY_MASK 1UL       
#define WAVL_PARENT_MASK ~1UL    

struct wavl_augment_callbacks {
    void (*propagate)(struct rb_node *node, struct rb_node *stop);
    void (*copy)(struct rb_node *old, struct rb_node *new);
    void (*rotate)(struct rb_node *old, struct rb_node *new);
};

extern void __wavl_insert_augmented(struct rb_node *node, struct rb_root *root,void (*augment_rotate)(struct rb_node *old, struct rb_node *new));
extern void __wavl_erase(struct rb_node *parent, struct rb_root *root,void (*augment_rotate)(struct rb_node *old, struct rb_node *new),bool is_3_child, bool hole_on_left);
static inline void wavl_insert_augmented(struct rb_node *node, struct rb_root *root,
                      const struct wavl_augment_callbacks *augment)
{
    __wavl_insert_augmented(node, root, augment->rotate);
}

static inline void wavl_insert_augmented_cached(struct rb_node *node,struct rb_root_cached *root, bool newleft,const struct wavl_augment_callbacks *augment)
{
    if (newleft)
        root->rb_leftmost = node;
    wavl_insert_augmented(node, &root->rb_root, augment);
}

static __always_inline struct rb_node *
wavl_add_augmented_cached(struct rb_node *node, struct rb_root_cached *tree,
			bool (*less)(struct rb_node *, const struct rb_node *),
			const struct wavl_augment_callbacks *augment)
{
	struct rb_node **link = &tree->rb_root.rb_node;
	struct rb_node *parent = NULL;
	bool leftmost = true;

	while (*link) {
		parent = *link;
		if (less(node, parent)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = false;
		}
	}

	rb_link_node(node, parent, link);
	augment->propagate(parent, NULL); /* suboptimal */
	wavl_insert_augmented_cached(node, tree, leftmost, augment);

	return leftmost ? node : NULL;
}




static inline unsigned long wavl_parity(const struct rb_node *node) {
    return node ? (node->__rb_parent_color & WAVL_PARITY_MASK) : 1UL; 
}

static inline void wavl_flip_parity(struct rb_node *node) {
    node->__rb_parent_color ^= WAVL_PARITY_MASK;
}



static inline struct rb_node *wavl_parent(struct rb_node *node) {
    return (struct rb_node *)(node->__rb_parent_color & WAVL_PARENT_MASK);
}

static bool wavl_is_leaf(struct rb_node *node) {
    if(!node) return false;
    return (!(node->rb_right) && !(node->rb_left));
}


static inline void wavl_set_parity(struct rb_node *node, unsigned long parity) {
    node->__rb_parent_color = (node->__rb_parent_color & WAVL_PARENT_MASK) | (parity & WAVL_PARITY_MASK);
}




static inline void wavl_change_child(struct rb_node *old, struct rb_node *new, struct rb_node *parent, struct rb_root *root) {
	if (parent) {
		if (parent->rb_left == old)
			WRITE_ONCE(parent->rb_left, new);
		else
			WRITE_ONCE(parent->rb_right, new);
	} else
		WRITE_ONCE(root->rb_node, new);
}

static inline void
__wavl_change_child_rcu(struct rb_node *old, struct rb_node *new,
		      struct rb_node *parent, struct rb_root *root)
{
	if (parent) {
		if (parent->rb_left == old)
			rcu_assign_pointer(parent->rb_left, new);
		else
			rcu_assign_pointer(parent->rb_right, new);
	} else
		rcu_assign_pointer(root->rb_node, new);
}



static inline void wavl_set_parent(struct rb_node *node, struct rb_node *parent) {
    node->__rb_parent_color = ((unsigned long)parent & WAVL_PARENT_MASK) | wavl_parity(node);
}




static inline struct rb_node *
__wavl_erase_augmented(struct rb_node *node, struct rb_root *root,
                       const struct wavl_augment_callbacks *augment) 
{
    struct rb_node *child = node->rb_right;
    struct rb_node *tmp = node->rb_left;
    struct rb_node *parent;
    struct rb_node *rebalance_node = NULL; 

    if (!tmp) {
        /* Case 1: no left child */
        parent = wavl_parent(node);
        wavl_change_child(node, child, parent, root);
        if (child) wavl_set_parent(child, parent);
        
        tmp = parent; // for augmented
    } else if (!child) {
        /* Case 2: no right child */
        parent = wavl_parent(node); 
        wavl_change_child(node, tmp, parent, root);
        if (tmp) wavl_set_parent(tmp, parent);

        tmp = parent; // for augmented
    } else {
        /* Case 3: two children, find the Successor */
        struct rb_node *successor = child, *child2;
        tmp = child->rb_left;
        if (!tmp) {
            parent = successor;
            child2 = successor->rb_right;
            /*
            node
                \
               child (successor)
              /     \
            tmp     child2
            
            
            */

            if (augment->copy) augment->copy(node, successor);
        } 
        else {
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rb_left;
            } while (tmp);
            /*
                    parent                                 /
                  /                       ===>           successor
                successor                               /         \...      
                /        \                            NULL
             tmp       child2

            */
            child2 = successor->rb_right;
            WRITE_ONCE(parent->rb_left, child2);
            WRITE_ONCE(successor->rb_right, child);
            wavl_set_parent(child, successor);

            if (augment->copy) augment->copy(node, successor);
            if (augment->propagate) augment->propagate(parent, successor);
        }
        //replace and link
        tmp = node->rb_left; 
        WRITE_ONCE(successor->rb_left, tmp);//replace's node left subtree
        wavl_set_parent(tmp, successor);
        struct rb_node *old_parent = wavl_parent(node); 
        wavl_change_child(node, successor, old_parent, root); 
        
        if (child2) wavl_set_parent(child2, parent);
        
        wavl_set_parent(successor, old_parent);
        wavl_set_parity(successor, wavl_parity(node)); 

        tmp = successor; // for augmented
    }

    // check whether parent violate
    rebalance_node = parent;

    if (augment->propagate) augment->propagate(tmp, NULL);
    
    return rebalance_node;
}
static __always_inline void
wavl_erase_augmented(struct rb_node *node, struct rb_root *root,
		   const struct wavl_augment_callbacks *augment)
{
    struct rb_node *victim = node;
    struct rb_node *p_victim;
    struct rb_node *w; 
    bool is_3_child = false;
    bool hole_on_left = false;
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
	struct rb_node *rebalance = __wavl_erase_augmented(node, root, augment);
    if (rebalance)
		__wavl_erase(rebalance, root, augment->rotate, is_3_child, hole_on_left);
}


static __always_inline void
wavl_erase_augmented_cached(struct rb_node *node, struct rb_root_cached *root,
			  const struct wavl_augment_callbacks *augment)
{
	if (root->rb_leftmost == node)
		root->rb_leftmost = rb_next(node);
	wavl_erase_augmented(node, &root->rb_root, augment);
}


#define WAVL_DECLARE_CALLBACKS(WAVLSTATIC, WAVLNAME,                \
                 WAVLSTRUCT, WAVLFIELD, WAVLAUGMENTED, WAVLCOMPUTE) \
static inline void                                                  \
WAVLNAME ## _propagate(struct rb_node *rb, struct rb_node *stop)    \
{                                                                   \
    while (rb != stop) {                                            \
        WAVLSTRUCT *node = rb_entry(rb, WAVLSTRUCT, WAVLFIELD);     \
        if (WAVLCOMPUTE(node, true))                                \
            break;                                                  \
        rb = wavl_parent(&node->WAVLFIELD);                         \
    }                                                               \
}                                                                   \
static inline void                                                  \
WAVLNAME ## _copy(struct rb_node *rb_old, struct rb_node *rb_new)   \
{                                                                   \
    WAVLSTRUCT *old = rb_entry(rb_old, WAVLSTRUCT, WAVLFIELD);      \
    WAVLSTRUCT *new = rb_entry(rb_new, WAVLSTRUCT, WAVLFIELD);      \
    new->WAVLAUGMENTED = old->WAVLAUGMENTED;                        \
}                                                                   \
static void                                                         \
WAVLNAME ## _rotate(struct rb_node *rb_old, struct rb_node *rb_new) \
{                                                                   \
    WAVLSTRUCT *old = rb_entry(rb_old, WAVLSTRUCT, WAVLFIELD);      \
    WAVLSTRUCT *new = rb_entry(rb_new, WAVLSTRUCT, WAVLFIELD);      \
    new->WAVLAUGMENTED = old->WAVLAUGMENTED;                        \
    WAVLCOMPUTE(old, false);                                        \
}                                                                   \
WAVLSTATIC const struct wavl_augment_callbacks WAVLNAME = {         \
    .propagate = WAVLNAME ## _propagate,                            \
    .copy = WAVLNAME ## _copy,                                      \
    .rotate = WAVLNAME ## _rotate                                   \
};
//find max augment
#define WAVL_DECLARE_CALLBACKS_MAX(WAVLSTATIC, WAVLNAME, WAVLSTRUCT, WAVLFIELD, \
				 WAVLTYPE, WAVLAUGMENTED, WAVLCOMPUTE)	      \
static inline bool WAVLNAME ## _compute_max(WAVLSTRUCT *node, bool exit)      \
{									      \
	WAVLSTRUCT *child;						      \
	WAVLTYPE max = WAVLCOMPUTE(node);				      \
	if (node->WAVLFIELD.rb_left) {					      \
		child = rb_entry(node->WAVLFIELD.rb_left, WAVLSTRUCT, WAVLFIELD);   \
		if (child->WAVLAUGMENTED > max)				      \
			max = child->WAVLAUGMENTED;			      \
	}								      \
	if (node->WAVLFIELD.rb_right) {					      \
		child = rb_entry(node->WAVLFIELD.rb_right, WAVLSTRUCT, WAVLFIELD);  \
		if (child->WAVLAUGMENTED > max)				      \
			max = child->WAVLAUGMENTED;			      \
	}								      \
	if (exit && node->WAVLAUGMENTED == max)				      \
		return true;						      \
	node->WAVLAUGMENTED = max;					      \
	return false;							      \
}									      \
WAVL_DECLARE_CALLBACKS(WAVLSTATIC, WAVLNAME,				      \
		     WAVLSTRUCT, WAVLFIELD, WAVLAUGMENTED, WAVLNAME ## _compute_max)


#endif