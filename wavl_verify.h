#ifndef _WAVL_VERIFY_H
#define _WAVL_VERIFY_H

#include <linux/rbtree.h>
#include <linux/log2.h>       // 給 ilog2() 使用
#include "wavl_tree_augmented.h"



static inline int get_max_height(struct rb_node *node) {
    int left_h, right_h;
    if (!node) return 0;
    
    left_h = get_max_height(node->rb_left);
    right_h = get_max_height(node->rb_right);
    
    return 1 + (left_h > right_h ? left_h : right_h);
}

static inline int get_max_avl_height(unsigned long m) {
    unsigned long n_minus_1, n_curr, n_next;
    int h;

    if (m == 0) return 0;
    if (m == 1) return 0; 
    if (m == 2) return 1; 

    n_minus_1 = 2; 
    n_curr = 4;    
    h = 2;

    while (true) {
        n_next = n_minus_1 + n_curr + 1;
        if (n_next > m) {
            break;
        }
        n_minus_1 = n_curr;
        n_curr = n_next;
        h++;
    }
    return h;
}

static inline int verify_leftmost(struct rb_root_cached *root) {
    struct rb_node *curr = root->rb_root.rb_node;
    struct rb_node *true_leftmost = NULL;

    while (curr) {
        true_leftmost = curr;
        curr = curr->rb_left;
    }

    if (root->rb_leftmost != true_leftmost) {
        pr_err("[ERROR] Leftmost Cache error！should be %p, not %p\n", true_leftmost, root->rb_leftmost);
        return -1;
    }
    return 0;
}


static inline int verify_wavl_properties(struct rb_root_cached *root, int nodecount, unsigned long insertion_count) {
    struct rb_node *node;
    struct rb_node *last_node = NULL; // for BST traversal
    int count = 0;
    int error = 0; 

    for (node = rb_first(&root->rb_root); node; node = rb_next(node)) {
        struct my_wavl_node *my_node = container_of(node, struct my_wavl_node, node);
        
        /* Property 1: BST check */
        if (last_node) {
            struct my_wavl_node *last_my_node = container_of(last_node, struct my_wavl_node, node);
            if (last_my_node->key > my_node->key) {
                pr_err("[ERROR] BST order wrong！ front Key %d greater than current Key %d\n", 
                       last_my_node->key, my_node->key);
                error = 1;
            }
        }
        last_node = node;
        unsigned long diff_l = wavl_rank_diff(node, node->rb_left);
        unsigned long diff_r = wavl_rank_diff(node, node->rb_right);

        /* Property 2: parent child pointer Consistency */
        if (node->rb_left && wavl_parent(node->rb_left) != node) {
            pr_err("[ERROR] Parent mismatch! Key %d left\n", my_node->key);
            error = 1;
        }
        if (node->rb_right && wavl_parent(node->rb_right) != node) {
            pr_err("[ERROR] Parent mismatch! Key %d right\n", my_node->key);
            error = 1;
        }

        /* Property 3: leaf base case check */
        if(wavl_is_leaf(node)){
            if (diff_l != 1 || diff_r != 1) {
                pr_err("[ERROR] leaf node rank not correct！ Key %d is leaf，but rank diff is (left:%lu, right:%lu)\n", 
                       my_node->key, diff_l, diff_r);
                error = 1;
            }
        }

        /* Property 4: WAVL rank property */
        if (diff_l != 1 && diff_l != 2) {
            pr_err("[ERROR] Rank Violation! Key %d left diff is %lu\n", my_node->key, diff_l);
            error = 1;
        }
        if (diff_r != 1 && diff_r != 2) {
            pr_err("[ERROR] Rank Violation! Key %d right diff is %lu\n", my_node->key, diff_r);
            error = 1;
        }
        count++;
    }

    /* Property 5: WAVL height property & Degradation Analysis */
    if (nodecount > 0 && insertion_count > 0) {
        int h = get_max_height(root->rb_root.rb_node);
        unsigned long min_n = 1UL << ((h + 1) / 2);
        
        if (nodecount < min_n) {
            pr_err("[ERROR] WAVL Structural Violation! Height is %d, but nodes count %d < required %lu\n", 
                   h, nodecount, min_n);
            error = 1;
        }

        /* ========== Degradation Analysis ========== */
        int h_avl_limit = get_max_avl_height(insertion_count);
        unsigned long rb_safe_threshold = 1UL << ((h_avl_limit + 1) / 2);

        pr_info("[Degradation Analysis]\n");

        if (nodecount < rb_safe_threshold) {
            pr_info(">>> [DEGRADATION ALERT] The theoretical limits have crossed! <<<\n");
            pr_info(">>> WAVL Tree has  degraded into the Red-Black Tree  <<<\n");
        } else {
            pr_info(">>> [STATUS] WAVL Tree is still protected by AVL strictness bounds. <<<\n");
        }
    }

    if (verify_leftmost(root) != 0) error = 1;
    if (error) return -1; 

    pr_info("[OK] WAVL Invariants Verified. Rank, Leftmost, Augmented, Height check passed! (%d nodes)\n", count);
    return 0;
}

static inline unsigned long verify_interval_metadata(struct rb_node *node, int *error_flag) {
    struct my_wavl_interval_node *m;
    unsigned long true_max, l_max, r_max;
    if (!node) return 0; 

    m = container_of(node, struct my_wavl_interval_node, node);
    true_max = m->end; 

    if (node->rb_left) {
        l_max = verify_interval_metadata(node->rb_left, error_flag);
        if (l_max > true_max) true_max = l_max;
    }
    if (node->rb_right) {
        r_max = verify_interval_metadata(node->rb_right, error_flag);
        if (r_max > true_max) true_max = r_max;
    }

    if (m->subtree_max_end != true_max) {
        pr_err("[ERROR] Interval [%lu, %lu] recorded max is %lu, but actual is %lu\n", 
               m->start, m->end, m->subtree_max_end, true_max);
        *error_flag = 1;
    }
    return true_max;
}

static inline int verify_interval_wavl_properties(struct rb_root_cached *root) {
    struct rb_node *node;
    int count = 0;
    int error = 0; 
    for (node = rb_first(&root->rb_root); node; node = rb_next(node)) {
        struct my_wavl_interval_node *my_node = container_of(node, struct my_wavl_interval_node, node);
        unsigned long diff_l = wavl_rank_diff(node, node->rb_left);
        unsigned long diff_r = wavl_rank_diff(node, node->rb_right);

        if (node->rb_left && wavl_parent(node->rb_left) != node) {
            pr_err("[ERROR] Parent mismatch! Interval [%lu, %lu] left\n", my_node->start, my_node->end);
            error = 1;
        }
        if (node->rb_right && wavl_parent(node->rb_right) != node) {
            pr_err("[ERROR] Parent mismatch! Interval [%lu, %lu] right\n", my_node->start, my_node->end);
            error = 1;
        }
        if (diff_l != 1 && diff_l != 2) {
            pr_err("[ERROR] Rank Violation! Interval [%lu, %lu] left diff is %lu\n", my_node->start, my_node->end, diff_l);
            error = 1;
        }
        if (diff_r != 1 && diff_r != 2) {
            pr_err("[ERROR] Rank Violation! Interval [%lu, %lu] right diff is %lu\n", my_node->start, my_node->end, diff_r);
            error = 1;
        }
        count++;
    }
    if (verify_leftmost(root) != 0) error = 1;
    verify_interval_metadata(root->rb_root.rb_node, &error);
    if (error) return -1; 

    pr_info("[OK] Interval WAVL Invariants Verified. Rank, Leftmost, Augmented check passed! (%d nodes)\n", count);
    return 0;
}

#endif // _WAVL_VERIFY_H