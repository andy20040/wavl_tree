#!/bin/bash


N=1000000


MODES=("seq" "reverse" "seq_rev" "rev_seq")


echo "Mode,N,D,Phase,RB_Tot_Rot,WAVL_Tot_Rot,RB_Tot_Path,WAVL_Tot_Path" > results_all.csv

echo "start testing..."


for MODE in "${MODES[@]}"; do
    echo "=================================================="
    echo " testing mode: $MODE "
    echo "=================================================="
    

    for (( D=10000; D<=1000000; D+=20000 )); do

        sudo dmesg -c > /dev/null
        
        echo "$MODE $N $D" | sudo tee /proc/rbtree_test_cmd > /dev/null
        

        sleep 3
        
        ROT_LINES=$(dmesg | grep "Total Rotation Counts")
        PATH_LINES=$(dmesg | grep "Total Rebalancing Path Len")
        
        # Insert Phase ---
        RB_INS_ROT=$(echo "$ROT_LINES" | head -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_INS_ROT=$(echo "$ROT_LINES" | head -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        RB_INS_PATH=$(echo "$PATH_LINES" | head -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_INS_PATH=$(echo "$PATH_LINES" | head -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        
        #Delete Phase ---
        RB_DEL_ROT=$(echo "$ROT_LINES" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_DEL_ROT=$(echo "$ROT_LINES" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        RB_DEL_PATH=$(echo "$PATH_LINES" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_DEL_PATH=$(echo "$PATH_LINES" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        echo "$MODE,$N,$D,Insert,$RB_INS_ROT,$WAVL_INS_ROT,$RB_INS_PATH,$WAVL_INS_PATH" >> results_all.csv
        echo "$MODE,$N,$D,Delete,$RB_DEL_ROT,$WAVL_DEL_ROT,$RB_DEL_PATH,$WAVL_DEL_PATH" >> results_all.csv
    done
done

echo "data saved to results_all.csv"