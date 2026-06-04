#!/bin/bash
# trace_process.sh
TRACE_DIR=/sys/kernel/tracing
echo $$ > $TRACE_DIR/set_ftrace_pid
# echo every function to filter
echo handle_mm_fault > $TRACE_DIR/set_ftrace_filter
echo function > $TRACE_DIR/current_tracer
exec "$@"