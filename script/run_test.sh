#!/bin/bash


N=1000000


MODES=("seq" "reverse" "seq_rev" "rev_seq")


echo "Mode,N,D,RB_Tot_Rot,WAVL_Tot_Rot,RB_Tot_Path,WAVL_Tot_Path" > results_all.csv

echo "start testing..."


for MODE in "${MODES[@]}"; do
    echo "=================================================="
    echo " testing mode: $MODE "
    echo "=================================================="
    

    for (( D=10000; D<=1000000; D+=20000 )); do

        sudo dmesg -c > /dev/null
        
        echo "$MODE $N $D" | sudo tee /proc/rbtree_test_cmd > /dev/null
        

        sleep 3
        
        RB_TOT_ROT=$(dmesg | grep "Total Rotation Counts" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_TOT_ROT=$(dmesg | grep "Total Rotation Counts" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        RB_TOT_PATH=$(dmesg | grep "Total Rebalancing Path Len" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
        WAVL_TOT_PATH=$(dmesg | grep "Total Rebalancing Path Len" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
        
        if [ -z "$RB_TOT_PATH" ] || [ -z "$WAVL_TOT_PATH" ]; then
            echo "  [error] $MODE D=$D data not found , skipped"
            continue
        fi
        echo "$MODE,$N,$D,$RB_TOT_ROT,$WAVL_TOT_ROT,$RB_TOT_PATH,$WAVL_TOT_PATH" >> results_all.csv
    done
done

echo "data saved to results_all.csv"