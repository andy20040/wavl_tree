import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../graph"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

try:
    df = pd.read_csv('results_ratio.csv')
except FileNotFoundError:
    print("results_ratio.csv not found")
    exit()


df_delete = df[df['Phase'] == 'Delete']

plt.style.use('seaborn-v0_8-whitegrid')
comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# (Path Length)
path_data = df_delete[df_delete['Metric'] == 'Path']
sns.boxplot(x='Ratio', y='Value', hue='Tree', data=path_data, ax=ax1, 
            width=0.6, palette={'RB Tree': '#ff9999', 'WAVL Tree': '#99ccff'})

ax1.set_title('[Delete Phase] Rebalancing Path vs Deletion Ratio', fontsize=14, pad=15)
ax1.set_xlabel('Deletion Ratio (%)', fontsize=12)
ax1.set_ylabel('Total Rebalancing Path Length', fontsize=12)
ax1.yaxis.set_major_formatter(comma_fmt)
ax1.legend(title='Data Structure')

#  (Rotations)
rot_data = df_delete[df_delete['Metric'] == 'Rotations']
sns.boxplot(x='Ratio', y='Value', hue='Tree', data=rot_data, ax=ax2, 
            width=0.6, palette={'RB Tree': '#ff9999', 'WAVL Tree': '#99ccff'})

ax2.set_title('[Delete Phase] Rotations vs Deletion Ratio', fontsize=14, pad=15)
ax2.set_xlabel('Deletion Ratio (%)', fontsize=12)
ax2.set_ylabel('Total Rotations', fontsize=12)
ax2.yaxis.set_major_formatter(comma_fmt)
ax2.legend(title='Data Structure')

# 存檔
filename = os.path.join(output_dir, 'deletion_ratio_analysis.png')
plt.tight_layout(pad=2.0)
plt.savefig(filename, dpi=300)
print(f"done: {filename}")