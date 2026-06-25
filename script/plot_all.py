import pandas as pd
import matplotlib.pyplot as plt
import os

# 1. 讀取資料
try:
    df = pd.read_csv('results_all.csv')
except FileNotFoundError:
    print("error：results_all.csv not found")
    exit()


target_modes = ['seq', 'reverse', 'seq_rev', 'rev_seq']
phases = ['Insert', 'Delete']


plt.style.use('seaborn-v0_8-whitegrid')

print("ploting diagram..")


for mode in target_modes:
    mode_data = df[df['Mode'] == mode]
    
    if mode_data.empty:
        print(f"skipped: {mode} no data")
        continue

    for phase in phases:
        subset = mode_data[mode_data['Phase'] == phase]
        
        if subset.empty:
            continue


        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

        # (Path Length) ---
        ax1.plot(subset['D'], subset['RB_Tot_Path'], 
                 marker='o', markersize=5, color='red', linewidth=2, label='RB Tree')
        ax1.plot(subset['D'], subset['WAVL_Tot_Path'], 
                 marker='s', markersize=5, color='blue', linewidth=2, linestyle='--', label='WAVL Tree')
        
        ax1.set_title(f'[{mode.upper()} - {phase}] Total Path Length', fontsize=14, pad=10)
        ax1.set_xlabel('Number of Operations (D)', fontsize=12)
        ax1.set_ylabel('Cumulative Path Length', fontsize=12)
        ax1.grid(True, linestyle='--', alpha=0.7)
        ax1.set_ylim(bottom=0)
        ax1.legend(fontsize=11)

        # (Total Rotations) ---
        ax2.plot(subset['D'], subset['RB_Tot_Rot'], 
                 marker='o', markersize=6, color='red', linewidth=2, label='RB Tree')
        ax2.plot(subset['D'], subset['WAVL_Tot_Rot'], 
                 marker='x', markersize=5, color='blue', linewidth=2, linestyle='--', alpha=0.8, label='WAVL Tree')
        
        ax2.set_title(f'[{mode.upper()} - {phase}] Total Rotations', fontsize=14, pad=10)
        ax2.set_xlabel('Number of Operations (D)', fontsize=12)
        ax2.set_ylabel('Cumulative Rotations', fontsize=12)
        ax2.grid(True, linestyle='--', alpha=0.7)
        ax2.set_ylim(bottom=0)
        ax2.legend(fontsize=11)

        # 存檔
        filename = f'{mode}_{phase.lower()}_comparison.png'
        plt.tight_layout()
        plt.savefig(filename, dpi=300)
        print(f"diagram saved: {filename}")
        
        plt.close()

print("all saved")