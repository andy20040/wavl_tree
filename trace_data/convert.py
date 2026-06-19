# convert.py
import sys

input_file = "cfs_trace.txt"
output_file = "cfs_data.h"
max_ops = 10000  # data count

try:
    with open(input_file, "r") as f:
        lines = f.readlines()
except FileNotFoundError:
    print(f"{input_file} Not found！")
    sys.exit(1)

output = "#ifndef _TRACE_DATA_H\n#define _TRACE_DATA_H\n\n"
output += "struct trace_op {\n    int is_insert;\n    unsigned long long key;\n};\n\n"
output += "static struct trace_op real_trace[] = {\n"

count = 0
for line in lines:
    parts = line.strip().split()
    if len(parts) == 2 and parts[0] in ['I', 'D']:
        is_insert = 1 if parts[0] == 'I' else 0
        key = parts[1]
        output += f"    {{{is_insert}, {key}ULL}},\n"
        count += 1
        if count >= max_ops:
            break

output += "};\n\n"
output += f"#define TRACE_SIZE {count}\n"
output += "#endif // _TRACE_DATA_H\n"

with open(output_file, "w") as f:
    f.write(output)

print(f"Convert Success！Handle {count} Operations，Generate {output_file}")