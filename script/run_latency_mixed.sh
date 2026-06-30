#!/bin/bash
trap "echo -e '\n[!] Detected Ctrl+C, aborting entire script!'; exit 1" SIGINT
taskset -pc 0 $$ > /dev/null

RUNS=20
MIXED_OPS=100000
POOL_SIZES=(10000 50000 100000 250000 500000 1000000)
FILE_DIST="../trace_data/latency_results_mixed.csv"

echo "PoolSize,Run,Phase,Tree,Metric,Value_ns" > $FILE_DIST

echo "Starting Mixed Latency Benchmark..."

for POOL in "${POOL_SIZES[@]}"; do
    echo "Testing: Pool Size = $POOL (Ops = $MIXED_OPS)..."
    
    for (( i=1; i<=RUNS; i++ )); do

        dmesg -c > /dev/null

        echo "mixed $POOL $MIXED_OPS" | tee /proc/rbtree_latency_cmd > /dev/null
        sleep 1 
        
        MIXED_LINE=$(dmesg | grep "Avg Op Latency" | tail -n 1)

        #  Avg Op Latency    |       292 ns |       283 ns
        RB_MIXED=$(echo "$MIXED_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_MIXED=$(echo "$MIXED_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        if [ -z "$RB_MIXED" ]; then continue; fi

        echo "$POOL,$i,Mixed,RB Tree,Latency,$RB_MIXED" >> $FILE_DIST
        echo "$POOL,$i,Mixed,WAVL Tree,Latency,$WAVL_MIXED" >> $FILE_DIST
        
    done
done
echo "Mixed Latency tests completed! Saved to $FILE_DIST"