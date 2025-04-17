import pandas as pd
import matplotlib.pyplot as plt

data_path = "./data/mpmc.csv"
save_path = "./output/mpmc.png"

# Read the CSV data into a DataFrame
df = pd.read_csv(data_path)

# Set the Number of producers as the index for easier plotting
df.set_index('num_channel', inplace=True)

# Plot the data
plt.figure()
plt.plot(df.index, df['multiple'], marker='o', linestyle='-', color='brown', label='Channel base')
plt.plot(df.index, df['single'], marker='s', linestyle='-', color='darkblue', label='Lock base')

# Adding labels and title
plt.xlabel('Number of Channels')
plt.ylabel('Throughput(MB/s)')
plt.title('Throughput - Number of Channels')
plt.legend()

# Show the graph
plt.tight_layout()
plt.savefig(save_path)
plt.show()