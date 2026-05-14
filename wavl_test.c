// 檔名：wavl_test.c
#include <linux/module.h> // module_init 等巨集在這裡
#include <linux/init.h>   // __init, __exit 在這裡
#include <linux/rbtree.h> // 為了使用 struct rb_node 等結構

// 1. 宣告你在 wavl_tree.c 裡面寫好的函數，這樣編譯器才認得
extern void wavl_link_node(struct rb_node *, struct rb_node *, struct rb_node **);
extern void wavl_insert_color(struct rb_node *, struct rb_root *);

// ==========================================================
// 這是你的「進入點」函數：當打指令 sudo insmod wavl_test.ko 時會執行
// ==========================================================
static int __init wavl_test_init(void) {
    // pr_info 就等於是核心版的 printf，印出的東西要用 dmesg 指令看
    pr_info("WAVL Tree Test: 模組開始載入...\n");

    // ----- 在這裡寫你的測試邏輯 -----
    // 1. 宣告一棵空的樹根
    // struct rb_root my_tree = RB_ROOT;
    
    // 2. 宣告幾個虛擬的節點，手動串起來
    // 3. 呼叫 wavl_insert_color() 看看你的 0-1 違規修復邏輯會不會當機

    pr_info("WAVL Tree Test: 測試完成，邏輯沒有觸發 Kernel Panic！\n");
    
    return 0; // 回傳 0 代表成功掛載。回傳負數代表掛載失敗。
}

// ==========================================================
// 這是你的「離開點」函數：當打指令 sudo rmmod wavl_test 時會執行
// ==========================================================
static void __exit wavl_test_exit(void) {
    // 如果你在 init 裡面有向核心要記憶體 (kmalloc)，就要在這裡釋放 (kfree)
    pr_info("WAVL Tree Test: 模組已卸載，清理完畢。\n");
}

// ==========================================================
// 向核心註冊這兩個點 (最關鍵的兩行)
// ==========================================================
module_init(wavl_test_init);
module_exit(wavl_test_exit);

// 核心非常重視版權，沒有標註 GPL 授權，核心會拒絕掛載並噴出警告
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("NCKU Student"); // 可以填上你的名字
MODULE_DESCRIPTION("WAVL Tree Implementation Test Module");