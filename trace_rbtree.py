#!/usr/bin/env python3
from bcc import BPF
import time


bpf_source = """
#include <uapi/linux/ptrace.h>

// per cpu array (index 0)
BPF_PERCPU_ARRAY(insert_counts, u32, 1);
int trace_rb_insert_color(struct pt_regs *ctx) {
    u32 key = 0;
    u32 *val = insert_counts.lookup(&key);
    if (val) {
        (*val)++;
    }
    return 0;
}
"""



b = BPF(text=bpf_source)


b.attach_kprobe(event="rb_insert_color", fn_name="trace_rb_insert_color")



try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nAggregating results...")


counts_map = b.get_table("insert_counts")
total_calls = 0

for key, per_cpu_values in counts_map.items():
    if key.value == 0:
        for cpu_id, count in enumerate(per_cpu_values):
            calls = count.value
            print(f"CPU {cpu_id:02d}: {calls:>10} calls")
            total_calls += calls

print("-" * 25)
print(f"Total   : {total_calls:>10} calls")