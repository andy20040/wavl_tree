#!/bin/bash

RUNS=100
N_FIXED=1000000
D_FIXED=1000000
FILE_DIST="../results_random_dist.csv"

echo "Run,Phase,Tree,Metric,Value" > $FILE_DIST

echo "starting..."

for (( i=1; i<=RUNS; i++ )); do
    sudo dmesg -c > /dev/null
    echo "random $N_FIXED $D_FIXED" | sudo tee /proc/rbtree_test_cmd > /dev/null
    sleep 3
    
    ROT_LINES=$(dmesg | grep "Total Rotation Counts")
    PATH_LINES=$(dmesg | grep "Total Rebalancing Path Len")
    
    #  Insert Phase
    RB_INS_ROT=$(echo "$ROT_LINES" | head -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_INS_ROT=$(echo "$ROT_LINES" | head -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    RB_INS_PATH=$(echo "$PATH_LINES" | head -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_INS_PATH=$(echo "$PATH_LINES" | head -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    
    #  Delete Phase
    RB_DEL_ROT=$(echo "$ROT_LINES" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_DEL_ROT=$(echo "$ROT_LINES" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')
    RB_DEL_PATH=$(echo "$PATH_LINES" | tail -n 1 | awk -F'|' '{print $2}' | tr -d ' ')
    WAVL_DEL_PATH=$(echo "$PATH_LINES" | tail -n 1 | awk -F'|' '{print $3}' | tr -d ' ')

    # write to  CSV ( Rotations , Path)
    echo "$i,Insert,RB Tree,Rotations,$RB_INS_ROT" >> $FILE_DIST
    echo "$i,Insert,WAVL Tree,Rotations,$WAVL_INS_ROT" >> $FILE_DIST
    echo "$i,Insert,RB Tree,Path,$RB_INS_PATH" >> $FILE_DIST
    echo "$i,Insert,WAVL Tree,Path,$WAVL_INS_PATH" >> $FILE_DIST
    
    echo "$i,Delete,RB Tree,Rotations,$RB_DEL_ROT" >> $FILE_DIST
    echo "$i,Delete,WAVL Tree,Rotations,$WAVL_DEL_ROT" >> $FILE_DIST
    echo "$i,Delete,RB Tree,Path,$RB_DEL_PATH" >> $FILE_DIST
    echo "$i,Delete,WAVL Tree,Path,$WAVL_DEL_PATH" >> $FILE_DIST
    

    if (( i % 10 == 0 )); then
        echo "  complete $i / $RUNS runs"
    fi
done

echo "all complete !"