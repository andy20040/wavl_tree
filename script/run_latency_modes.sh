#!/bin/bash
# script for recording insert delete latency and traversal time
trap "echo -e '\n[!] Detected Ctrl+C, aborting entire script!'; exit 1" SIGINT
MODES=("seq" "reverse" "seq_rev" "rev_seq")
FILE_ALL="../trace_data/latency_results_modes.csv"

echo "Mode,N,D,Phase,RB_Latency_ns,WAVL_Latency_ns" > $FILE_ALL

# =====================================================================
#  Insert Phase (fixed D，changing N)
# =====================================================================
echo "=================================================="
echo " Starting Mode Scaling Benchmark (Insert Phase)"
echo "=================================================="
D_FIXED=1

for MODE in "${MODES[@]}"; do
    echo " Testing: $MODE (Insert Scaling)..."
    for (( N=10000; N<=1000000; N+=20000 )); do
        
        dmesg -c > /dev/null
        echo "$MODE $N $D_FIXED" |  tee /proc/rbtree_latency_cmd > /dev/null
        sleep 0.5
        
        INS_LINE=$(dmesg | grep "Avg Insert Latency" | head -n 1)
        
        RB_INS=$(echo "$INS_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_INS=$(echo "$INS_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        if [ -z "$RB_INS" ]; then continue; fi
        
        echo "$MODE,$N,$D_FIXED,Insert,$RB_INS,$WAVL_INS" >> $FILE_ALL
    done
done

# =====================================================================
#  Delete Phase (fixed N=1,000,000，changing D)
# =====================================================================
echo "=================================================="
echo " Starting Mode Scaling Benchmark (Delete Phase & Traversal)"
echo "=================================================="
N_FIXED=1000000

for MODE in "${MODES[@]}"; do
    echo " Testing: $MODE (Delete & Traversal Scaling)..."
    for (( D=10000; D<=1000000; D+=20000 )); do
        
        dmesg -c > /dev/null
        echo "$MODE $N_FIXED $D" |  tee /proc/rbtree_latency_cmd > /dev/null
        sleep 1
        
        LOOK_LINE=$(dmesg | grep "Avg Lookup Latency" | tail -n 1)
        DEL_LINE=$(dmesg | grep "Avg Erase Latency" | tail -n 1)
        TRAV_LINE=$(dmesg | grep "Total Traversal" | tail -n 1)
        
        RB_LOOK=$(echo "$LOOK_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_LOOK=$(echo "$LOOK_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        RB_DEL=$(echo "$DEL_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_DEL=$(echo "$DEL_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        RB_TRAV=$(echo "$TRAV_LINE" | awk -F'|' '{print $2}' | awk '{print $1}')
        WAVL_TRAV=$(echo "$TRAV_LINE" | awk -F'|' '{print $3}' | awk '{print $1}')
        
        echo "$MODE,$N_FIXED,$D,Lookup,$RB_LOOK,$WAVL_LOOK" >> $FILE_ALL
        echo "$MODE,$N_FIXED,$D,Delete,$RB_DEL,$WAVL_DEL" >> $FILE_ALL
        echo "$MODE,$N_FIXED,$D,Traversal,$RB_TRAV,$WAVL_TRAV" >> $FILE_ALL 
    done
done

echo "All Mode Latency tests completed! Saved to $FILE_ALL"