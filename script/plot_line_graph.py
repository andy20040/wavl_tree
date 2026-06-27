import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os


output_dir = "../graph"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)


try:
    df = pd.read_csv('../trace_data/results_random.csv')
except FileNotFoundError:
    print(" results_random.csv not found")
    exit()


plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')

phases = ['Insert', 'Delete']

my_palette = {'RB Tree': '#e63946', 'WAVL Tree': '#1d3557'}

for phase in phases:
    phase_data = df[df['Phase'] == phase]
    if phase_data.empty:
        continue

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    #  (Path Length) ---
    path_data = phase_data[phase_data['Metric'] == 'Path']
    sns.lineplot(x='Ratio', y='Value', hue='Tree', data=path_data, ax=ax1,
                 errorbar='sd', marker='o', markersize=8, linewidth=2.5, palette=my_palette)

    ax1.set_title(f'[{phase} Phase] Rebalancing Path vs Deletion Ratio', fontsize=14, pad=15)
    ax1.set_xlabel('Deletion Ratio (%)', fontsize=12)
    ax1.set_ylabel('Total Rebalancing Path Length', fontsize=12)
    ax1.yaxis.set_major_formatter(comma_fmt)
    ax1.set_xticks(range(10, 101, 10)) 
    ax1.legend(title='Data Structure', fontsize=11, title_fontsize=12)
    bottom, top = ax1.get_ylim()
    ax1.set_ylim(bottom=0, top=top * 1.1)
    #  (Rotations) 
    rot_data = phase_data[phase_data['Metric'] == 'Rotations']
    sns.lineplot(x='Ratio', y='Value', hue='Tree', data=rot_data, ax=ax2,
                 errorbar='sd', marker='s', markersize=8, linewidth=2.5, palette=my_palette)

    ax2.set_title(f'[{phase} Phase] Rotations vs Deletion Ratio', fontsize=14, pad=15)
    ax2.set_xlabel('Deletion Ratio (%)', fontsize=12)
    ax2.set_ylabel('Total Rotations', fontsize=12)
    ax2.yaxis.set_major_formatter(comma_fmt)
    ax2.set_xticks(range(10, 101, 10))
    ax2.legend(title='Data Structure', fontsize=11, title_fontsize=12)
    bottom, top = ax2.get_ylim()
    ax2.set_ylim(bottom=0, top=top * 1.1)

    filename = os.path.join(output_dir, f'random_{phase.lower()}_phase.png')
    plt.tight_layout(pad=2.0)
    plt.savefig(filename, dpi=300)
    print(f"done: {filename}")

print("all done")