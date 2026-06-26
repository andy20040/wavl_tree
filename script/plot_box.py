import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "graph"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)


try:
    df = pd.read_csv('results_random_dist.csv')
except FileNotFoundError:
    print(" results_random_dist.csv not found")
    exit()

plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')

# 繪製兩種階段：Insert 和 Delete
for phase in ['Insert', 'Delete']:
    phase_data = df[df['Phase'] == phase]
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))

    # (Path) 
    path_data = phase_data[phase_data['Metric'] == 'Path']
    sns.boxplot(x='Tree', y='Value', data=path_data, ax=ax1, width=0.5, 
                palette={'RB Tree': '#ff9999', 'WAVL Tree': '#99ccff'})
    ax1.set_title(f'[{phase}] Distribution of Path Length', fontsize=14, pad=10)
    ax1.set_ylabel('Total Rebalancing Path Length')
    ax1.set_xlabel('')
    ax1.yaxis.set_major_formatter(comma_fmt)

    # (Rotations) 
    rot_data = phase_data[phase_data['Metric'] == 'Rotations']
    sns.boxplot(x='Tree', y='Value', data=rot_data, ax=ax2, width=0.5, 
                palette={'RB Tree': '#ff9999', 'WAVL Tree': '#99ccff'})
    ax2.set_title(f'[{phase}] Distribution of Rotations', fontsize=14, pad=10)
    ax2.set_ylabel('Total Rotations')
    ax2.set_xlabel('')
    ax2.yaxis.set_major_formatter(comma_fmt)

    # 
    filename = os.path.join(output_dir, f'random_distribution_{phase.lower()}.png')
    plt.tight_layout()
    plt.savefig(filename, dpi=300)
    print(f"complete: {filename}")

plt.close()