import pandas as pd
import matplotlib.pyplot as plt
import mtl_config

data_path = "./data/ring_buffer.csv"
save_path = "./output/ring_buffer.png"

# Read the CSV data into a DataFrame
df = pd.read_csv(data_path)

# X axis: message size (log2)
x = df['shm_size(log2)']

plt.figure()
plt.plot(x, df['message'], marker='o', linestyle='-', color='darkblue', label='Alternate Flag Mode')
plt.plot(x, df['ring_buffer'], marker='s', linestyle='-', color='brown', label='Ring Buffer')

# Labels, ticks, title, legend
plt.xlabel('Message Size (log₂ Bytes)')
plt.ylabel('Bandwidth (MB/s)')
plt.xticks(x)  # ensure all sizes appear on the x-axis
plt.title('Throughput – Ring Buffer vs Alternate Flag')
plt.legend()
plt.tight_layout()

# Save and show
plt.savefig(save_path)
plt.show()
