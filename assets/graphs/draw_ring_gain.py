import pandas as pd
import matplotlib.pyplot as plt

data_path = "./data/ring_buffer.csv"
save_path = "./output/ring_buffer.png"

# Read the CSV data into a DataFrame
df = pd.read_csv(data_path)

# Prepare bar positions
x = df['shm_size(log2)']
bar_width = 0.4

# Create grouped bar chart
plt.figure()
plt.bar(x - bar_width/2, df['message'], width=bar_width, color='darkblue', label='message based')
plt.bar(x + bar_width/2, df['ring_buffer'], width=bar_width, color='brown', label='ring buffer based')

# Labels and ticks
plt.xlabel('Message Size (log₂ Bytes)')
plt.ylabel('Bandwidth(MB/s)')
plt.xticks(x)
plt.legend()
plt.title('Throughput - Ring Buffer vs Message Based')

plt.tight_layout()
plt.savefig(save_path)
plt.show()