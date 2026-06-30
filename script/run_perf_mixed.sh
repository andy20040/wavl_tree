#!/bin/bash
trap "echo -e '\n[!] Detected Ctrl+C, aborting entire script!'; exit 1" SIGINT
taskset -pc 0 $$ > /dev/null

RUNS=20
MIXED_OPS=100000
POOL_SIZES=(10000 50000 100000 250000 500000 1000000) 
FILE_DIST="../trace_data/perf_results_mixed.csv"

echo "PoolSize,Run,Tree,Metric,Value" > $FILE_DIST

echo "Starting Perf Hardware Counter for Mixed Workload..."

for POOL in "${POOL_SIZES[@]}"; do
    echo "Testing: Pool Size = $POOL (Ops = $MIXED_OPS)..."
    
    for (( i=1; i<=RUNS; i++ )); do
        
        # 1.  RB Tree 
        dmesg -c > /dev/null
        perf stat -x ',' -e L1-dcache-load-misses,branch-misses \
            -- sh -c "echo 'mixed_rb $POOL $MIXED_OPS' > /proc/rbtree_latency_cmd" 2> perf_rb.log
        
        RB_L1=$(grep "L1-dcache-load-misses" perf_rb.log | cut -d',' -f1)
        RB_BR=$(grep "branch-misses" perf_rb.log | cut -d',' -f1)

        echo "$POOL,$i,RB Tree,L1 Misses,$RB_L1" >> $FILE_DIST
        echo "$POOL,$i,RB Tree,Branch Misses,$RB_BR" >> $FILE_DIST

        # 2. WAVL Tree
        dmesg -c > /dev/null
        perf stat -x ',' -e L1-dcache-load-misses,branch-misses \
            -- sh -c "echo 'mixed_wavl $POOL $MIXED_OPS' > /proc/rbtree_latency_cmd" 2> perf_wavl.log
        
        WAVL_L1=$(grep "L1-dcache-load-misses" perf_wavl.log | cut -d',' -f1)
        WAVL_BR=$(grep "branch-misses" perf_wavl.log | cut -d',' -f1)

        echo "$POOL,$i,WAVL Tree,L1 Misses,$WAVL_L1" >> $FILE_DIST
        echo "$POOL,$i,WAVL Tree,Branch Misses,$WAVL_BR" >> $FILE_DIST
    done
done
echo "Mixed Perf tests completed! Saved to $FILE_DIST"