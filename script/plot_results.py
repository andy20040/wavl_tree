import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('results.csv')


plt.style.use('seaborn-v0_8-whitegrid')
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# rebalancing path
ax1.plot(df['D'], df['RB_Avg_Path'], marker='o', color='red', label='Red-Black Tree')
ax1.plot(df['D'], df['WAVL_Avg_Path'], marker='s', color='blue', label='WAVL Tree')
ax1.set_title('Average Rebalancing Path Length per Deletion', fontsize=14)
ax1.set_xlabel('Number of Deletions (D)', fontsize=12)
ax1.set_ylabel('Average Path Length (Nodes)', fontsize=12)
ax1.legend(fontsize=12)
ax1.grid(True, linestyle='--', alpha=0.7)

#rotation count
ax2.plot(df['D'], df['RB_Avg_Rot'], marker='o', color='red', label='Red-Black Tree')
ax2.plot(df['D'], df['WAVL_Avg_Rot'], marker='s', color='blue', label='WAVL Tree')
ax2.set_title('Average Rotations per Deletion', fontsize=14)
ax2.set_xlabel('Number of Deletions (D)', fontsize=12)
ax2.set_ylabel('Average Rotations', fontsize=12)
ax2.legend(fontsize=12)
ax2.grid(True, linestyle='--', alpha=0.7)

plt.tight_layout()
plt.savefig('rebalancing_performance.png', dpi=300)
print("saved to rebalancing_performance.png")
# plt.show()