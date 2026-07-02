import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

output_dir = "../../graph/latency"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

try:
    #  CSV ( PoolSize, Tree, Insert_ns, Lookup_ns, Erase_ns)
    df = pd.read_csv('../../trace_data/latency/latency_results_mixed.csv')
except FileNotFoundError:
    print("Error: latency_results_mixed.csv not found!")
    exit()


df_melted = df.melt(id_vars=['PoolSize', 'Tree'],
                    value_vars=['Insert_ns', 'Lookup_ns', 'Erase_ns'],
                    var_name='Operation', value_name='Latency_ns')


df_melted['Operation'] = df_melted['Operation'].replace({
    'Insert_ns': 'Insert',
    'Lookup_ns': 'Lookup',
    'Erase_ns': 'Erase'
})

my_palette = {'RB Tree': '#e63946', 'WAVL Tree': '#1d3557'}

plt.style.use('seaborn-v0_8-whitegrid')


g = sns.FacetGrid(df_melted, col="Operation", sharey=False, height=5, aspect=1.2)
g.map_dataframe(sns.lineplot, x='PoolSize', y='Latency_ns', hue='Tree',
                marker='D', markersize=9, linewidth=3, palette=my_palette)


g.fig.suptitle('[Mixed Phase] Detailed Operation Latency vs Tree Size', fontsize=18, y=1.05)
g.set_axis_labels('Tree Size / Pool Size (Nodes)', 'Average Latency (ns)')
g.add_legend(title='Data Structure', fontsize=12)

def format_k(x, pos):
    if x >= 1000000:
        return f'{int(x/1000000)}M'
    elif x >= 1000:
        return f'{int(x/1000)}K'
    return str(int(x))


for ax in g.axes.flat:
    ax.yaxis.set_major_formatter(ticker.StrMethodFormatter('{x:,.0f}'))
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(format_k))
    ax.set_xticks([10000, 50000, 100000, 250000, 500000, 1000000])

plt.tight_layout()
filename = os.path.join(output_dir, 'latency_mixed_detailed_comparison.png')
g.savefig(filename, dpi=300)
print(f"Done! Saved graph to: {filename}")