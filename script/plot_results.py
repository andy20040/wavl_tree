import pandas as pd
import matplotlib.pyplot as plt
import os


try:
    df = pd.read_csv('results_all.csv')
except FileNotFoundError:
    print(" results_all.csv not found！")
    exit()

target_modes = ['seq', 'reverse', 'seq_rev', 'rev_seq']


plt.style.use('seaborn-v0_8-whitegrid')

print("starting plot diagram...")


for mode in target_modes:
    if mode not in df['Mode'].values:
        print(f" '{mode}' not found , skipped")
        continue


    mode_data = df[df['Mode'] == mode]
    

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # ==========================================
    # (Total Path)
    # ==========================================
    ax1.plot(mode_data['D'], mode_data['RB_Tot_Path'], 
             marker='o', markersize=5, color='red', linewidth=2, label='Red-Black Tree')
    ax1.plot(mode_data['D'], mode_data['WAVL_Tot_Path'], 
             marker='s', markersize=5, color='blue', linewidth=2, linestyle='--', label='WAVL Tree')
    
    ax1.set_title(f'[{mode.upper()}] Cumulative Rebalancing Path Length', fontsize=14, pad=15)
    ax1.set_xlabel('Number of Deletions (D)', fontsize=12)
    ax1.set_ylabel('Total Path Steps (Nodes)', fontsize=12)
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.set_ylim(bottom=0)
    ax1.legend(fontsize=11)

    # ==========================================
    #  (Total Rotations)
    # ==========================================
    ax2.plot(mode_data['D'], mode_data['RB_Tot_Rot'], 
             marker='o', markersize=6, color='red', linewidth=3, label='Red-Black Tree')
    ax2.plot(mode_data['D'], mode_data['WAVL_Tot_Rot'], 
             marker='x', markersize=5, color='blue', linewidth=2, linestyle='--', alpha=0.8, label='WAVL Tree')
    
    ax2.set_title(f'[{mode.upper()}] Cumulative Rotations', fontsize=14, pad=15)
    ax2.set_xlabel('Number of Deletions (D)', fontsize=12)
    ax2.set_ylabel('Total Rotations', fontsize=12)
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.set_ylim(bottom=0)
    ax2.legend(fontsize=11)


    plt.tight_layout()
    filename = f'{mode}_comparison.png'
    plt.savefig(filename, dpi=300)
    print(f"saved : {filename}")

    plt.close()

print("All diagram saved !")