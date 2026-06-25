#!/bin/bash


N=1000000


echo "N,D,RB_Tot_Rot,WAVL_Tot_Rot,RB_Tot_Path,WAVL_Tot_Path" > results_total.csv




for (( D=10000; D<=1000000; D+=20000 )); do
    echo "======================================"
    echo "測試 N = $N, D = $D ..."
    
    # clear log
    sudo dmesg -c > /dev/null
    

    echo "seq $N $D" | sudo tee /proc/rbtree_test_cmd > /dev/null
    
    sleep 3
    
    RB_TOT_ROT=$(dmesg | grep "Total Rotation Counts" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_TOT_ROT=$(dmesg | grep "Total Rotation Counts" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    RB_TOT_PATH=$(dmesg | grep "Total Rebalancing Path Len" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_TOT_PATH=$(dmesg | grep "Total Rebalancing Path Len" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    if [ -z "$RB_TOT_PATH" ] || [ -z "$WAVL_TOT_PATH" ]; then
        echo "  [error] can found data skip one iteration"
        continue
    fi
    echo "$N,$D,$RB_TOT_ROT,$WAVL_TOT_ROT,$RB_TOT_PATH,$WAVL_TOT_PATH" >> results_total.csv
    
    echo "  -> RB totatl rotation: $RB_TOT_ROT, WAVL totatl rotation: $WAVL_TOT_ROT"
    echo "  -> RB total rebalancing path: $RB_TOT_PATH, WAVL total rebalancing path: $WAVL_TOT_PATH"
done

echo "complete! saved to results_total.csv"