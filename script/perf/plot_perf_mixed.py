import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../../graph/perf"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)


try:
    df = pd.read_csv('../../trace_data/perf/perf_results_mixed.csv')
except FileNotFoundError:
    print("Error: perf_results_mixed.csv not found!")
    exit()

metrics = ['L1 Misses', 'Branch Misses']
my_palette = {'RB Tree': '#e63946', 'WAVL Tree': '#1d3557'}

fig, axes = plt.subplots(1, 2, figsize=(18, 7))
plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')


def format_k(x, pos):
    if x >= 1000000:
        return f'{int(x/1000000)}M'
    elif x >= 1000:
        return f'{int(x/1000)}K'
    return str(int(x))

for i, metric in enumerate(metrics):
    ax = axes[i]
    metric_data = df[df['Metric'] == metric]
    
    sns.lineplot(x='PoolSize', y='Value', hue='Tree', data=metric_data, ax=ax,
                 errorbar='sd', marker='o', markersize=9, linewidth=2.5, palette=my_palette)

    ax.set_title(f'Hardware Events: {metric}\n(Steady-State 1M Mixed Ops)', fontsize=16, pad=15)
    ax.set_ylabel('Total Event Count', fontsize=14)
    ax.set_xlabel('Tree Size / Pool Size (Nodes)', fontsize=14)
    
    ax.yaxis.set_major_formatter(comma_fmt)
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(format_k))
    

    ax.set_xticks([10000, 50000, 100000, 250000, 500000, 1000000])
    ax.legend(title='Data Structure', fontsize=12)

plt.tight_layout(pad=3.0)
filename = os.path.join(output_dir, 'perf_mixed_comparison.png')
plt.savefig(filename, dpi=300)
print(f"Done! Saved graph to: {filename}")