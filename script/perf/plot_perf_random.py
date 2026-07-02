import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../../graph/perf"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

try:
    df = pd.read_csv('../../trace_data/perf/perf_results_random.csv')
except FileNotFoundError:
    print("Error: perf_results_random.csv not found")
    exit()

plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')


metrics = ['L1 Misses', 'Branch Misses']
my_palette = {'RB Tree': '#e63946', 'WAVL Tree': '#1d3557'}


fig, axes = plt.subplots(1, 2, figsize=(16, 6))

for i, metric in enumerate(metrics):
    metric_data = df[df['Metric'] == metric]
    if metric_data.empty:
        continue
    
    ax = axes[i]
    sns.lineplot(x='Ratio', y='Value', hue='Tree', data=metric_data, ax=ax,
                 errorbar='sd', marker='o', markersize=8, linewidth=2.5, palette=my_palette)

    ax.set_title(f'Hardware Events: {metric}', fontsize=14, pad=15)
    ax.set_xlabel('Deletion Ratio (%)', fontsize=12)
    ax.set_ylabel('Total Event Count', fontsize=12)
    ax.yaxis.set_major_formatter(comma_fmt)
    ax.set_xticks(range(10, 101, 10)) 
    ax.legend(title='Data Structure', fontsize=11)
    

    min_val = metric_data['Value'].min()
    max_val = metric_data['Value'].max()
    margin = (max_val - min_val) * 0.15
    if margin == 0:
        margin = min_val * 0.05 if min_val != 0 else 10
    ax.set_ylim(max(0, min_val - margin), max_val + margin)

filename = os.path.join(output_dir, 'perf_random_comparison.png')
plt.tight_layout(pad=2.0)
plt.savefig(filename, dpi=300)
print(f"Done! Saved graph to: {filename}")