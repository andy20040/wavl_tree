#!/bin/bash
TRACE_DIR=/sys/kernel/tracing


echo 0 > $TRACE_DIR/tracing_on
echo > $TRACE_DIR/set_ftrace_filter
echo > $TRACE_DIR/set_ftrace_pid


echo $$ > $TRACE_DIR/set_ftrace_pid


echo rb_erase > $TRACE_DIR/set_ftrace_filter
echo wavl_erase >> $TRACE_DIR/set_ftrace_filter
echo rb_insert_color >> $TRACE_DIR/set_ftrace_filter
echo wavl_insert >> $TRACE_DIR/set_ftrace_filter


echo function_graph > $TRACE_DIR/current_tracer


echo 1 > $TRACE_DIR/tracing_on
exec "$@"
echo 0 > $TRACE_DIR/tracing_on