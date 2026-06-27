#!/bin/bash

RUNS=50
N_FIXED=1000000
RATIOS=(10 20 30 40 50 60 70 80 90 100) 
FILE_DIST="../trace_data/latency_results_random.csv"


echo "Ratio,Run,Phase,Tree,Metric,Value_ns" > $FILE_DIST

echo "Starting Random Latency Benchmark..."

for RATIO in "${RATIOS[@]}"; do
    D_VAL=$(( N_FIXED * RATIO / 100 ))
    echo "Testing: ${RATIO}% Deletion (D=$D_VAL)..."
    
    for (( i=1; i<=RUNS; i++ )); do

        sudo dmesg -c > /dev/null
        echo "random $N_FIXED $D_VAL" | sudo tee /proc/rbtree_latency_cmd > /dev/null
        sleep 1 
        INS_LINE=$(dmesg | grep "Avg Insert Latency" | tail -n 1)
        LOOK_LINE=$(dmesg | grep "Avg Lookup Latency" | tail -n 1)
        DEL_LINE=$(dmesg | grep "Avg Erase Latency" | tail -n 1)


        # format : Avg Insert Latency|       292 ns |       283 ns
        RB_INS=$(echo "$INS_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_INS=$(echo "$INS_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        RB_LOOK=$(echo "$LOOK_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_LOOK=$(echo "$LOOK_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        RB_DEL=$(echo "$DEL_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_DEL=$(echo "$DEL_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')

        echo "$RATIO,$i,Insert,RB Tree,Latency,$RB_INS" >> $FILE_DIST
        echo "$RATIO,$i,Insert,WAVL Tree,Latency,$WAVL_INS" >> $FILE_DIST
        
        echo "$RATIO,$i,Lookup,RB Tree,Latency,$RB_LOOK" >> $FILE_DIST
        echo "$RATIO,$i,Lookup,WAVL Tree,Latency,$WAVL_LOOK" >> $FILE_DIST
        
        echo "$RATIO,$i,Delete,RB Tree,Latency,$RB_DEL" >> $FILE_DIST
        echo "$RATIO,$i,Delete,WAVL Tree,Latency,$WAVL_DEL" >> $FILE_DIST

        if (( i % 10 == 0 )); then
            echo "  Completed $i / $RUNS runs"
        fi
    done
done
echo "All Random Latency tests completed! Saved to $FILE_DIST"