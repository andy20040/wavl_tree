/* wavl_tree.h */
#ifndef _WAVL_TREE_H
#define _WAVL_TREE_H

#include <linux/rbtree.h> //

void wavl_insert(struct rb_node *node, struct rb_root *root);
void wavl_erase(struct rb_node *node, struct rb_root *root);

#endif /* _WAVL_TREE_H */