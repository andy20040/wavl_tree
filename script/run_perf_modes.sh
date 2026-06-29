#!/bin/bash

RUNS=20  # 硬體計數器穩定度高，20次足以消除雜訊
N_FIXED=1000000
RATIOS=(10 20 30 40 50 60 70 80 90 100) 
MODES=("seq" "reverse" "seq_rev" "rev_seq")
FILE_DIST="../trace_data/perf_results_modes.csv"

echo "Mode,Ratio,Run,Tree,Metric,Value" > $FILE_DIST
echo "Starting Perf Hardware Counter for All Modes..."

for MODE in "${MODES[@]}"; do
    echo "=========================================="
    echo " Testing Mode: $MODE "
    echo "=========================================="
    
    for RATIO in "${RATIOS[@]}"; do
        D_VAL=$(( N_FIXED * RATIO / 100 ))
        echo "  -> Deletion Ratio: ${RATIO}% (D=$D_VAL)..."
        
        for (( i=1; i<=RUNS; i++ )); do
            
            # 1. 測量 RB Tree
            sudo dmesg -c > /dev/null
            sudo perf stat -x ',' -e L1-dcache-load-misses,branch-misses \
                -- sudo sh -c "echo '${MODE}_rb $N_FIXED $D_VAL' > /proc/rbtree_latency_cmd" 2> perf_rb.log
            
            RB_L1=$(grep "L1-dcache-load-misses" perf_rb.log | cut -d',' -f1)
            RB_LLC=$(grep "LLC-load-misses" perf_rb.log | cut -d',' -f1)
            RB_BR=$(grep "branch-misses" perf_rb.log | cut -d',' -f1)

            echo "$MODE,$RATIO,$i,RB Tree,L1 Misses,$RB_L1" >> $FILE_DIST
            echo "$MODE,$RATIO,$i,RB Tree,LLC Misses,$RB_LLC" >> $FILE_DIST
            echo "$MODE,$RATIO,$i,RB Tree,Branch Misses,$RB_BR" >> $FILE_DIST

            # 2. 測量 WAVL Tree
            sudo dmesg -c > /dev/null
            sudo perf stat -x ',' -e L1-dcache-load-misses,branch-misses \
                -- sudo sh -c "echo '${MODE}_wavl $N_FIXED $D_VAL' > /proc/rbtree_latency_cmd" 2> perf_wavl.log
            
            WAVL_L1=$(grep "L1-dcache-load-misses" perf_wavl.log | cut -d',' -f1)
            WAVL_LLC=$(grep "LLC-load-misses" perf_wavl.log | cut -d',' -f1)
            WAVL_BR=$(grep "branch-misses" perf_wavl.log | cut -d',' -f1)

            echo "$MODE,$RATIO,$i,WAVL Tree,L1 Misses,$WAVL_L1" >> $FILE_DIST
            echo "$MODE,$RATIO,$i,WAVL Tree,LLC Misses,$WAVL_LLC" >> $FILE_DIST
            echo "$MODE,$RATIO,$i,WAVL Tree,Branch Misses,$WAVL_BR" >> $FILE_DIST

        done
    done
done
echo "All Mode Perf tests completed! Saved to $FILE_DIST"