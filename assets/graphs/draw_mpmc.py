import pandas as pd
import matplotlib.pyplot as plt

data_path = "./data/mpmc.csv"
save_path = "./output/mpmc.png"

# Read the CSV data into a DataFrame
df = pd.read_csv(data_path)

# Set the Number of producers as the index for easier plotting
df.set_index('Number of producers', inplace=True)

# Plot the data
plt.figure(figsize=(8, 6))
plt.plot(df.index, df['Channel base'], marker='o', linestyle='-', color='blue', label='Channel base')
plt.plot(df.index, df['Lock base'], marker='s', linestyle='-', color='red', label='Lock base')

# Adding labels and title
plt.xlabel('Number of Producers')
plt.ylabel('Throughput(MB/s)')
plt.title('Throughput - Number of Producers')
plt.legend()
plt.grid(True)

# Show the graph
plt.tight_layout()
plt.savefig(save_path)
