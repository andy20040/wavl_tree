import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker  # 新增這行：用來處理數字格式化


try:
    df = pd.read_csv('results_all.csv')
except FileNotFoundError:
    print(" results_all.csv not found ")
    exit()


target_modes = ['seq', 'reverse', 'seq_rev', 'rev_seq']
phases = ['Insert', 'Delete']


plt.style.use('seaborn-v0_8-whitegrid')

print("ploting diagram...")


comma_fmt = ticker.StrMethodFormatter('{x:,.0f}')


for mode in target_modes:
    mode_data = df[df['Mode'] == mode]
    
    if mode_data.empty:
        continue

    for phase in phases:
        subset = mode_data[mode_data['Phase'] == phase]
        
        if subset.empty:
            continue

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

        #  (Path Length) ---
        ax1.plot(subset['D'], subset['RB_Tot_Path'], 
                 marker='o', markersize=5, color='red', linewidth=2, label='RB Tree')
        ax1.plot(subset['D'], subset['WAVL_Tot_Path'], 
                 marker='s', markersize=5, color='blue', linewidth=2, linestyle='--', label='WAVL Tree')
        
        ax1.set_title(f'[{mode.upper()} - {phase}] Total Path Length', fontsize=14, pad=15)
        ax1.set_xlabel('Number of Operations (D)', fontsize=12)
        ax1.set_ylabel('Cumulative Path Length', fontsize=12)
        ax1.grid(True, linestyle='--', alpha=0.7)
        max_path = max(subset['RB_Tot_Path'].max(), subset['WAVL_Tot_Path'].max())
        ax1.set_ylim(bottom=0, top=max_path * 1.1)
        ax1.legend(fontsize=11)
        
        ax1.xaxis.set_major_formatter(comma_fmt)
        ax1.yaxis.set_major_formatter(comma_fmt)
        ax1.tick_params(axis='x', rotation=30, labelsize=10) 
        ax1.tick_params(axis='y', labelsize=10)

        # (Total Rotations) ---
        ax2.plot(subset['D'], subset['RB_Tot_Rot'], 
                 marker='o', markersize=6, color='red', linewidth=2, label='RB Tree')
        ax2.plot(subset['D'], subset['WAVL_Tot_Rot'], 
                 marker='x', markersize=5, color='blue', linewidth=2, linestyle='--', alpha=0.8, label='WAVL Tree')
        
        ax2.set_title(f'[{mode.upper()} - {phase}] Total Rotations', fontsize=14, pad=15)
        ax2.set_xlabel('Number of Operations (D)', fontsize=12)
        ax2.set_ylabel('Cumulative Rotations', fontsize=12)
        ax2.grid(True, linestyle='--', alpha=0.7)
        max_rot = max(subset['RB_Tot_Rot'].max(), subset['WAVL_Tot_Rot'].max())
        ax2.set_ylim(bottom=0, top=max_rot * 1.1)
        ax2.legend(fontsize=11)
        ax2.xaxis.set_major_formatter(comma_fmt)
        ax2.yaxis.set_major_formatter(comma_fmt)
        ax2.tick_params(axis='x', rotation=30, labelsize=10) # X軸傾斜 30 度
        ax2.tick_params(axis='y', labelsize=10)

        filename = f'{mode}_{phase.lower()}_comparison.png'

        plt.tight_layout(pad=2.0) 
        plt.savefig(filename, dpi=300, bbox_inches='tight') # bbox_inches='tight' 保證邊緣不被切除
        print(f" saved: {filename}")
        
        plt.close()

print("all saved")