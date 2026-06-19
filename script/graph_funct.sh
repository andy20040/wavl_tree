#!/bin/bash
# graph_funct.sh
TRACE_DIR=/sys/kernel/tracing
echo > $TRACE_DIR/trace
echo $$ > $TRACE_DIR/set_ftrace_pid
echo > $TRACE_DIR/set_ftrace_filter
echo function_graph > $TRACE_DIR/current_tracer
echo schedule > $TRACE_DIR/set_graph_function
echo funcgraph-tail > $TRACE_DIR/trace_options
exec "$@"