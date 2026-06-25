#!/bin/bash

# insert node count
N=1000000

# different delete count
D_COUNTS=(10000 50000 100000 200000 500000 800000 1000000)

echo "N,D,RB_Avg_Rot,WAVL_Avg_Rot,RB_Avg_Path,WAVL_Avg_Path" > results.csv

echo "start testing  (fixed N=$N，different D)..."

for D in "${D_COUNTS[@]}"; do
    echo "======================================"
    echo "testing N = $N, D = $D ..."
    
    # clear log
    sudo dmesg -c > /dev/null
    
    echo "seq $N $D" | sudo tee /proc/rbtree_test_cmd > /dev/null
    
    # wait kernel module
    sleep 3
    
    # use grep and awk 
    
    RB_AVG_ROT=$(dmesg | grep "Average Rotation Counts" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_AVG_ROT=$(dmesg | grep "Average Rotation Counts" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    

    RB_TOT_PATH=$(dmesg | grep "Total Rebalancing Path Len" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_TOT_PATH=$(dmesg | grep "Total Rebalancing Path Len" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    RB_AVG_PATH=$(echo "scale=3; $RB_TOT_PATH / $D" | bc)
    WAVL_AVG_PATH=$(echo "scale=3; $WAVL_TOT_PATH / $D" | bc)

    echo "$N,$D,$RB_AVG_ROT,$WAVL_AVG_ROT,$RB_AVG_PATH,$WAVL_AVG_PATH" >> results.csv
    
    echo "  -> RB average rotation: $RB_AVG_ROT, WAVL rotation: $WAVL_AVG_ROT"
    echo "  -> RB average path: $RB_AVG_PATH, WAVL average path: $WAVL_AVG_PATH"
done

echo "complete result saved to results.csv"