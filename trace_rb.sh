#!/bin/bash
TRACE_DIR=/sys/kernel/tracing  


echo 0 > $TRACE_DIR/tracing_on
echo > $TRACE_DIR/set_ftrace_filter
echo > $TRACE_DIR/set_ftrace_pid

#set function
echo my_rb_insert_wrapper > $TRACE_DIR/set_ftrace_filter
echo my_rb_erase > $TRACE_DIR/set_ftrace_filter

echo 1 > $TRACE_DIR/function_profile_enabled

echo "start ... ( Waiting for  60 seconds )"
sleep 60


echo 0 > $TRACE_DIR/function_profile_enabled

cat $TRACE_DIR/trace_stat/function0 | head -n 5