#!/bin/bash


MODES=("seq" "reverse" "seq_rev" "rev_seq")


FILE_ALL="../results_all.csv"


echo "Mode,N,D,Phase,RB_Tot_Rot,WAVL_Tot_Rot,RB_Tot_Path,WAVL_Tot_Path" > $FILE_ALL

# =====================================================================
#  (Insert Phase)
# =====================================================================
echo ""
echo "=================================================="
echo " insert "
echo "=================================================="
D_FIXED=1

for MODE in "${MODES[@]}"; do
    echo " testing : $MODE (Insert Scaling)..."
    for (( N=10000; N<=1000000; N+=20000 )); do
        
        sudo dmesg -c > /dev/null
        echo "$MODE $N $D_FIXED" | sudo tee /proc/rbtree_test_cmd > /dev/null
        sleep 2
        
        ROT_LINES=$(dmesg | grep "Total Rotation Counts")
        PATH_LINES=$(dmesg | grep "Total Rebalancing Path Len")
        
        RB_INS_ROT=$(echo "$ROT_LINES" | head -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_INS_ROT=$(echo "$ROT_LINES" | head -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        RB_INS_PATH=$(echo "$PATH_LINES" | head -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_INS_PATH=$(echo "$PATH_LINES" | head -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        
        if [ -z "$RB_INS_PATH" ]; then
            continue
        fi
        
        echo "$MODE,$N,$D_FIXED,Insert,$RB_INS_ROT,$WAVL_INS_ROT,$RB_INS_PATH,$WAVL_INS_PATH" >> $FILE_ALL
    done
done

# =====================================================================
# (Delete Phase)
# =====================================================================
echo ""
echo "=================================================="
echo " delete"
echo "=================================================="
N_FIXED=1000000

for MODE in "${MODES[@]}"; do
    echo "  testing : $MODE (Delete Scaling)..."
    for (( D=10000; D<=1000000; D+=20000 )); do
        
        sudo dmesg -c > /dev/null
        echo "$MODE $N_FIXED $D" | sudo tee /proc/rbtree_test_cmd > /dev/null
        sleep 3
        
        ROT_LINES=$(dmesg | grep "Total Rotation Counts")
        PATH_LINES=$(dmesg | grep "Total Rebalancing Path Len")
        
        RB_DEL_ROT=$(echo "$ROT_LINES" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_DEL_ROT=$(echo "$ROT_LINES" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        RB_DEL_PATH=$(echo "$PATH_LINES" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_DEL_PATH=$(echo "$PATH_LINES" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        
        echo "$MODE,$N_FIXED,$D,Delete,$RB_DEL_ROT,$WAVL_DEL_ROT,$RB_DEL_PATH,$WAVL_DEL_PATH" >> $FILE_ALL
    done
done

echo ""
echo "saved to : $FILE_ALL"