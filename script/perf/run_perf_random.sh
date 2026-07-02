#!/bin/bash
trap "echo -e '\n[!] Detected Ctrl+C, aborting entire script!'; exit 1" SIGINT
taskset -pc 0 $$ > /dev/null
RUNS=50  
N_FIXED=10000
RATIOS=(10 20 30 40 50 60 70 80 90 100) 
FILE_DIST="../../trace_data/perf/perf_results_random.csv"


echo "Ratio,Run,Tree,Metric,Value" > $FILE_DIST

echo "Starting Perf Hardware Counter Benchmark..."

for RATIO in "${RATIOS[@]}"; do
    D_VAL=$(( N_FIXED * RATIO / 100 ))
    echo "Testing: ${RATIO}% Deletion (D=$D_VAL)..."
    
    for (( i=1; i<=RUNS; i++ )); do
        

        dmesg -c > /dev/null
        perf stat -x ',' -e L1-dcache-load-misses,branch-misses \
            -- sh -c "echo 'random_rb $N_FIXED $D_VAL' > /proc/rbtree_latency_cmd" 2> perf_rb.log
        
        RB_L1=$(grep "L1-dcache-load-misses" perf_rb.log | cut -d',' -f1)
        RB_BR=$(grep "branch-misses" perf_rb.log | cut -d',' -f1)

        echo "$RATIO,$i,RB Tree,L1 Misses,$RB_L1" >> $FILE_DIST
        echo "$RATIO,$i,RB Tree,Branch Misses,$RB_BR" >> $FILE_DIST
        dmesg -c > /dev/null
        perf stat -x ',' -e L1-dcache-load-misses,branch-misses \
            -- sh -c "echo 'random_wavl $N_FIXED $D_VAL' > /proc/rbtree_latency_cmd" 2> perf_wavl.log
        
        WAVL_L1=$(grep "L1-dcache-load-misses" perf_wavl.log | cut -d',' -f1)
        WAVL_BR=$(grep "branch-misses" perf_wavl.log | cut -d',' -f1)

        echo "$RATIO,$i,WAVL Tree,L1 Misses,$WAVL_L1" >> $FILE_DIST
        echo "$RATIO,$i,WAVL Tree,Branch Misses,$WAVL_BR" >> $FILE_DIST

    done
done
echo "All Perf tests completed! Saved to $FILE_DIST"