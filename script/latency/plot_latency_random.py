import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../../graph/latency"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

try:
    df = pd.read_csv('../../trace_data/latency/latency_results_random.csv')
except FileNotFoundError:
    print("Error: latency_results_random.csv not found")
    exit()


phases = ['Insert', 'Lookup', 'Delete', 'Traversal']
my_palette = {'RB Tree': '#e63946', 'WAVL Tree': '#1d3557'}


fig, axes = plt.subplots(1, 4, figsize=(28, 6))

plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')

for i, phase in enumerate(phases):
    phase_data = df[df['Phase'] == phase]
    if phase_data.empty:
        continue
    
    ax = axes[i]
    sns.lineplot(x='Ratio', y='Value_ns', hue='Tree', data=phase_data, ax=ax,
                 errorbar='sd', marker='o', markersize=8, linewidth=2.5, palette=my_palette)


    if phase == 'Traversal':
        ax.set_title(f'[{phase} Phase] Total Latency vs Deletion Ratio', fontsize=14, pad=15)
        ax.set_ylabel('Total Latency (ns)', fontsize=12)
    else:
        ax.set_title(f'[{phase} Phase] Avg Latency vs Deletion Ratio', fontsize=14, pad=15)
        ax.set_ylabel('Average Latency (ns)', fontsize=12)
        
    ax.set_xlabel('Deletion Ratio (%)', fontsize=12)
    ax.yaxis.set_major_formatter(comma_fmt)
    ax.set_xticks(range(10, 101, 10)) 
    ax.legend(title='Data Structure', fontsize=11)
    

    min_val = phase_data['Value_ns'].min()
    max_val = phase_data['Value_ns'].max()
    margin = (max_val - min_val) * 0.15
    if margin == 0:
        margin = min_val * 0.05 if min_val != 0 else 10
    ax.set_ylim(max(0, min_val - margin), max_val + margin)

filename = os.path.join(output_dir, 'latency_random_all_phases.png')
plt.tight_layout(pad=2.0)
plt.savefig(filename, dpi=300)
print(f"Done! Saved graph to: {filename}")