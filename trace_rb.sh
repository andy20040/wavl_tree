#!/bin/bash
TRACE_DIR=/sys/kernel/tracing  # 或者是 /sys/kernel/debug/tracing


echo 0 > $TRACE_DIR/tracing_on
echo > $TRACE_DIR/set_ftrace_filter
echo > $TRACE_DIR/set_ftrace_pid

#set function
echo rb_insert_color > $TRACE_DIR/set_ftrace_filter
echo rb_erase >> $TRACE_DIR/set_ftrace_filter

echo 1 > $TRACE_DIR/function_profile_enabled

echo "start ... ( Waiting for  10 seconds )"
sleep 10


echo 0 > $TRACE_DIR/function_profile_enabled

cat $TRACE_DIR/trace_stat/function0 | head -n 5