#!/bin/bash
trap "echo -e '\n[!] Detected Ctrl+C, aborting entire script!'; exit 1" SIGINT
taskset -pc 0 $$ > /dev/null

RUNS=20
MIXED_OPS=100000
POOL_SIZES=(10000 50000 100000 250000 500000 1000000)
FILE_DIST="../../trace_data/latency/latency_results_mixed.csv"


echo "PoolSize,Run,Tree,Insert_ns,Lookup_ns,Erase_ns" > $FILE_DIST

echo "Starting Mixed Detailed Latency Benchmark..."

for POOL in "${POOL_SIZES[@]}"; do
    echo "Testing: Pool Size = $POOL (Ops = $MIXED_OPS)..."
    
    for (( i=1; i<=RUNS; i++ )); do

        dmesg -c > /dev/null

        echo "mixed $POOL $MIXED_OPS" | tee /proc/rbtree_latency_cmd > /dev/null
        sleep 1 
        

        DMESG_OUT=$(dmesg)
        

        INS_LINE=$(echo "$DMESG_OUT" | grep "Avg Insert Latency" | tail -n 1)
        LKP_LINE=$(echo "$DMESG_OUT" | grep "Avg Lookup Latency" | tail -n 1)
        DEL_LINE=$(echo "$DMESG_OUT" | grep "Avg Erase Latency" | tail -n 1)


        RB_INS=$(echo "$INS_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        RB_LKP=$(echo "$LKP_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        RB_DEL=$(echo "$DEL_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')


        WAVL_INS=$(echo "$INS_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        WAVL_LKP=$(echo "$LKP_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        WAVL_DEL=$(echo "$DEL_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        

        if [ -z "$RB_INS" ] || [ -z "$RB_LKP" ] || [ -z "$RB_DEL" ]; then 
            echo "  [!] Warning: Missing data in run $i, skipping..."
            continue
        fi

        echo "$POOL,$i,RB Tree,$RB_INS,$RB_LKP,$RB_DEL" >> $FILE_DIST
        echo "$POOL,$i,WAVL Tree,$WAVL_INS,$WAVL_LKP,$WAVL_DEL" >> $FILE_DIST
        
    done
done
echo "Mixed Latency tests completed! Saved to $FILE_DIST"