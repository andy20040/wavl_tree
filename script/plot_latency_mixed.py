import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../graph"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)


try:
    df = pd.read_csv('../trace_data/latency_results_mixed.csv')
except FileNotFoundError:
    print("Error: latency_results_mixed.csv not found!")
    exit()

my_palette = {'RB Tree': '#e63946', 'WAVL Tree': '#1d3557'}

plt.figure(figsize=(10, 7))
plt.style.use('seaborn-v0_8-whitegrid')
ax = plt.gca()


sns.lineplot(x='PoolSize', y='Value_ns', hue='Tree', data=df, ax=ax,
             errorbar='sd', marker='D', markersize=9, linewidth=3, palette=my_palette)

ax.set_title('[Mixed Phase] Average Operation Latency vs Tree Size', fontsize=16, pad=15)
ax.set_ylabel('Average Latency per Operation (ns)', fontsize=14)
ax.set_xlabel('Tree Size / Pool Size (Nodes)', fontsize=14)


ax.yaxis.set_major_formatter(ticker.StrMethodFormatter('{x:,.0f}'))

def format_k(x, pos):
    if x >= 1000000:
        return f'{int(x/1000000)}M'
    elif x >= 1000:
        return f'{int(x/1000)}K'
    return str(int(x))

ax.xaxis.set_major_formatter(ticker.FuncFormatter(format_k))
ax.set_xticks([10000, 50000, 100000, 250000, 500000, 1000000])
ax.legend(title='Data Structure', fontsize=12)

min_val = df['Value_ns'].min()
max_val = df['Value_ns'].max()
margin = (max_val - min_val) * 0.15
if margin == 0:
    margin = min_val * 0.05
ax.set_ylim(max(0, min_val - margin), max_val + margin)

plt.tight_layout()
filename = os.path.join(output_dir, 'latency_mixed_comparison.png')
plt.savefig(filename, dpi=300)
print(f"Done! Saved graph to: {filename}")