#include <linux/module.h>
#include <linux/rbtree.h> 

/* ==========================================================
 * 利用 __rb_parent_color 的最低 2 個 bits 來儲存 Rank 差值 (Rank Difference)
 * WAVL 的 Rank 差值通常是 1 或 2，剛好可以用 2 bits (01 或 10) 存。
 * ========================================================== */
#define WAVL_RANK_MASK 3UL       // 二進位 11
#define WAVL_PARENT_MASK ~3UL    // 濾掉最後兩個 bits，取得乾淨指標

// 取得 Rank 差值
static inline unsigned long wavl_rank_diff(struct rb_node *node) {
    // 防呆：如果是 NULL，依照 WAVL 定義，外部虛擬節點的 rank 差值通常視為 1 或特定值
    if (!node) return 1; 
    return node->__rb_parent_color & WAVL_RANK_MASK;
}

// 取得 Parent 指標
static inline struct rb_node *wavl_parent(struct rb_node *node) {
    return (struct rb_node *)(node->__rb_parent_color & WAVL_PARENT_MASK);
}

// 同時設定 Parent 與 Rank 差值
static inline void wavl_set_parent_rank(struct rb_node *node, struct rb_node *parent, unsigned long rank_diff) {
    node->__rb_parent_color = (unsigned long)parent | (rank_diff & WAVL_RANK_MASK);
}

// 設定單一節點的 Rank 差值 (不改變 parent)
static inline void wavl_set_rank_diff(struct rb_node *node, unsigned long rank_diff) {
    node->__rb_parent_color = (node->__rb_parent_color & WAVL_PARENT_MASK) | (rank_diff & WAVL_RANK_MASK);
}

/* ==========================================================
 * 戰區二：基礎樹狀操作 (Rotations & BST Logic)
 * 這裡你需要實作左旋與右旋，注意旋轉時要更新 rank_diff
 * ========================================================== */

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

    while (true) {
        parent = wavl_parent(node);
        if (!parent) {
            // 到達 Root，Root 的 Rank 差值強制為 1 (或不理它)
            break;
        }

        // 判斷 node 是左子節點還是右子節點
        if (parent->rb_left == node) {
            sibling = parent->rb_right;
            // TODO: WAVL 插入邏輯 (左側)
            // 1. 如果 sibling 的 rank_diff == 1 -> Rank Promotion (提拔 Parent)
            // 2. 如果 sibling 的 rank_diff == 2 -> 進行單旋轉或雙旋轉修復 -> 旋轉後可直接 break!
        } else {
            sibling = parent->rb_left;
            // TODO: WAVL 插入邏輯 (右側，與左側完全對稱)
        }
        
        break; // 開發初期先 break，避免無窮迴圈當機
    }
}
EXPORT_SYMBOL(wavl_insert_color);

// 刪除後的重新平衡：處理 3-1 或 2-2 違規
void wavl_erase(struct rb_node *node, struct rb_root *root) {
    // TODO: WAVL 的刪除與 Rank Demotion 邏輯 (這是最難的部分，建議最後寫)
}
EXPORT_SYMBOL(wavl_erase);