import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../graph"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

try:
    df = pd.read_csv('../latency_results_modes.csv')
except FileNotFoundError:
    try:
        df = pd.read_csv('latency_results_modes.csv') 
    except:
        print("Error: latency_results_modes.csv not found")
        exit()

target_modes = ['seq', 'reverse', 'seq_rev', 'rev_seq']
phases = ['Insert', 'Lookup', 'Delete']

plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')

for mode in target_modes:
    mode_data = df[df['Mode'] == mode]
    if mode_data.empty:
        continue

    for phase in phases:
        subset = mode_data[mode_data['Phase'] == phase]
        if subset.empty:
            continue

        fig, ax = plt.subplots(figsize=(10, 6))

        if phase == 'Insert':
            x_data = subset['N']
            x_label = 'Number of Inserted Nodes (N)'
        else:
            x_data = subset['D']
            x_label = 'Number of Lookups/Deletions (D)'

        ax.plot(x_data, subset['RB_Latency_ns'], 
                 marker='o', markersize=6, color='#e63946', linewidth=2, label='RB Tree')
        ax.plot(x_data, subset['WAVL_Latency_ns'], 
                 marker='s', markersize=6, color='#1d3557', linewidth=2, linestyle='--', label='WAVL Tree')
        
        ax.set_title(f'[{mode.upper()} - {phase}] Execution Latency', fontsize=14, pad=15)
        ax.set_xlabel(x_label, fontsize=12) 
        ax.set_ylabel('Latency (ns)', fontsize=12)
        ax.grid(True, linestyle='--', alpha=0.7)
        
        max_lat = max(subset['RB_Latency_ns'].max(), subset['WAVL_Latency_ns'].max())
        ax.set_ylim(bottom=0, top=max_lat * 1.1)
        ax.legend(fontsize=11)
        
        ax.xaxis.set_major_locator(ticker.MaxNLocator(nbins=5))
        ax.yaxis.set_major_locator(ticker.MaxNLocator(nbins=5))
        ax.xaxis.set_major_formatter(comma_fmt)
        ax.yaxis.set_major_formatter(comma_fmt)
        ax.tick_params(axis='x', rotation=30, labelsize=10) 
        ax.tick_params(axis='y', labelsize=10)

        filename = os.path.join(output_dir, f'latency_{mode}_{phase.lower()}.png')
        plt.tight_layout(pad=2.0) 
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        print(f"Saved: {filename}")
        
        plt.close()

print("All latency plots generated!")