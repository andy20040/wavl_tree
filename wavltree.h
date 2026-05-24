/* wavl_tree.h */
#ifndef _WAVL_TREE_H
#define _WAVL_TREE_H

#include <linux/rbtree.h> //
extern void wavl_replace_node(struct rb_node *victim, struct rb_node *new,struct rb_root *root);
extern void wavl_replace_node_rcu(struct rb_node *victim, struct rb_node *new,struct rb_root *root);
void wavl_insert(struct rb_node *node, struct rb_root *root);
void wavl_erase(struct rb_node *node, struct rb_root *root);

#define wavl_first_cached(root) (root)->rb_leftmost

static inline void wavl_insert_cached(struct rb_node *node,
					  struct rb_root_cached *root,
					  bool leftmost)
{
	if (leftmost)
		root->rb_leftmost = node;
	wavl_insert(node, &root->rb_root);
}


static inline struct rb_node *
wavl_erase_cached(struct rb_node *node, struct rb_root_cached *root)
{
	struct rb_node *leftmost = NULL;

	if (root->rb_leftmost == node)
		leftmost = root->rb_leftmost = rb_next(node);

	wavl_erase(node, &root->rb_root);

	return leftmost;
}

static inline void wavl_replace_node_cached(struct rb_node *victim,
					  struct rb_node *new,
					  struct rb_root_cached *root)
{
	if (root->rb_leftmost == victim)
		root->rb_leftmost = new;
	wavl_replace_node(victim, new, &root->rb_root);
}

static __always_inline struct rb_node *
wavl_add_cached(struct rb_node *node, struct rb_root_cached *tree,
          bool (*less)(struct rb_node *, const struct rb_node *))
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
    wavl_insert_cached(node, tree, leftmost); 

    return leftmost ? node : NULL;
}


static __always_inline void
wavl_add(struct rb_node *node, struct rb_root *tree,
       bool (*less)(struct rb_node *, const struct rb_node *))
{
    struct rb_node **link = &tree->rb_node;
    struct rb_node *parent = NULL;

    while (*link) {
        parent = *link;
        if (less(node, parent))
            link = &parent->rb_left;
        else
            link = &parent->rb_right;
    }

    rb_link_node(node, parent, link);
    wavl_insert(node, tree); 
}


static __always_inline struct rb_node *
wavl_find_add(struct rb_node *node, struct rb_root *tree,
        int (*cmp)(struct rb_node *, const struct rb_node *))
{
    struct rb_node **link = &tree->rb_node;
    struct rb_node *parent = NULL;
    int c;

    while (*link) {
        parent = *link;
        c = cmp(node, parent);

        if (c < 0)
            link = &parent->rb_left;
        else if (c > 0)
            link = &parent->rb_right;
        else
            return parent; 
    }

    rb_link_node(node, parent, link);
    wavl_insert(node, tree);
    return NULL;
}

#endif /* _WAVL_TREE_H */