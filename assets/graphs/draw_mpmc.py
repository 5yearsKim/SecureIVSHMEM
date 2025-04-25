import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

data_path = "./data/mpmc.csv"
save_path = "./output/mpmc.png"

# Read and index by number of channels
df = pd.read_csv(data_path)
df.set_index('num_channel', inplace=True)

# Positions for grouped bars
x = np.arange(len(df.index))
bar_width = 0.35

plt.figure()
plt.bar(x + bar_width/2, df['multiple'], width=bar_width, color='brown', label='Channel based')
plt.bar(x - bar_width/2, df['single'], width=bar_width, color='darkblue', label='Lock based')

# Labels, ticks, title, legend
plt.xlabel('Number of Channels')
plt.ylabel('Throughput (MB/s)')
plt.xticks(x, df.index)  # label ticks with the channel counts
plt.title('Throughput – Number of Channels')
plt.legend()
plt.tight_layout()

# Save and show
plt.savefig(save_path)
plt.show()
