import pandas as pd
import matplotlib.pyplot as plt


df = pd.read_csv('results_total.csv')


plt.style.use('seaborn-v0_8-whitegrid')
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# ---rebalancing path ---
ax1.plot(df['D'], df['RB_Tot_Path'], marker='o', markersize=4, color='red', label='Red-Black Tree')
ax1.plot(df['D'], df['WAVL_Tot_Path'], marker='s', markersize=4, color='blue', label='WAVL Tree')
ax1.set_title('Cumulative Rebalancing Path Length', fontsize=14)
ax1.set_xlabel('Number of Deletions (D)', fontsize=12)
ax1.set_ylabel('Total Path Steps (Nodes)', fontsize=12)
ax1.legend(fontsize=12)
ax1.grid(True, linestyle='--', alpha=0.7)

ax1.set_ylim(bottom=0)

# --- Total Rotations ---
ax2.plot(df['D'], df['RB_Tot_Rot'], marker='o', markersize=4, color='red', label='Red-Black Tree')
ax2.plot(df['D'], df['WAVL_Tot_Rot'], marker='s', markersize=4, color='blue', label='WAVL Tree')
ax2.set_title('Cumulative Rotations', fontsize=14)
ax2.set_xlabel('Number of Deletions (D)', fontsize=12)
ax2.set_ylabel('Total Rotations', fontsize=12)
ax2.legend(fontsize=12)
ax2.grid(True, linestyle='--', alpha=0.7)
ax2.set_ylim(bottom=0)
plt.tight_layout()
plt.savefig('rebalancing_totals.png', dpi=300)
print("saved to  rebalancing_totals.png")