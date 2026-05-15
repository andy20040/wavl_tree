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

static void wavl_rotate_left(struct rb_node *node, struct rb_root *root) {
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = wavl_parent(node);

    // 1. 處理 node 與 right 的子節點交換
    if ((node->rb_right = right->rb_left))
        wavl_set_parent_rank(right->rb_left, node, wavl_rank_diff(right->rb_left));
    right->rb_left = node;

    // 2. 處理 parent 指標的更新
    wavl_set_parent_rank(right, parent, wavl_rank_diff(right)); // right 接替 node 的位置
    if (parent) {
        if (node == parent->rb_left)
            parent->rb_left = right;
        else
            parent->rb_right = right;
    } else {
        root->rb_node = right;
    }
    
    // 3. 處理 node 被降級到 right 下方的狀態
    wavl_set_parent_rank(node, right, wavl_rank_diff(node));
}

static void wavl_rotate_right(struct rb_node *node, struct rb_root *root) {
    // TODO: 參考左旋邏輯，實作對稱的右旋
}

// 純 BST 的插入節點 (直接從 lib/rbtree.c 抄過來，把 rb_parent 換成 wavl_parent 即可)
void wavl_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link) {
    node->__rb_parent_color = (unsigned long)parent; // 初始 Rank 差值設為 0
    node->rb_left = node->rb_right = NULL;
    *rb_link = node;
}
EXPORT_SYMBOL(wavl_link_node);

/* ==========================================================
 * 戰區三：你的主戰場 (WAVL Rebalancing)
 * ========================================================== */

// 插入後的重新平衡：處理 0-1 違規
void wavl_insert_color(struct rb_node *node, struct rb_root *root) {
    struct rb_node *parent, *sibling;

    // 新插入的節點預設與 parent 的 rank 差值為 0 (產生 0-1 違規)
    wavl_set_rank_diff(node, 0); 

    while (node != root->rb_node) {
    struct rb_node *parent = rb_parent(node);
    
    // 如果跟爸爸的差值是 1 或 2，代表已經平衡，直接結束
    if (wavl_rank_diff(parent, node) > 0)
        break; 

    // 以下是差值為 0 的處理 (0-child violation)
    struct rb_node *sibling = wavl_sibling(node);

    // 【第一招：Promote】 (對應圖片第一排)
    if (wavl_rank_diff(parent, sibling) == 1) {
        wavl_promote(parent); // 爸爸 Rank + 1 (如果是存 Parity 就 XOR 1)
        node = parent;        // 違規可能上傳，往上爬繼續 while
        continue;
    }

    // 【準備旋轉】(因為 sibling diff == 2)
    // 判斷是 LL 型還是 LR 型，決定用單旋還是雙旋
    if ( node 是 parent 的左小孩 ) {
        if ( node 的左小孩 rank diff == 1 ) {
            // 【第二招：單旋轉】
            // ... 執行右旋，調 Rank ...
        } else {
            // 【第三招：雙旋轉】
            // ... 執行左旋再右旋，調 Rank ...
        }
    } else {
        // ... 對稱的右半邊邏輯 ...
    }
    
    // 旋轉完必定完全平衡，直接跳出迴圈！
    break; 
    }
}
EXPORT_SYMBOL(wavl_insert_color);

// 刪除後的重新平衡：處理 3-1 或 2-2 違規
void wavl_erase(struct rb_node *node, struct rb_root *root) {
    // TODO: WAVL 的刪除與 Rank Demotion 邏輯 (這是最難的部分，建議最後寫)
}
EXPORT_SYMBOL(wavl_erase);