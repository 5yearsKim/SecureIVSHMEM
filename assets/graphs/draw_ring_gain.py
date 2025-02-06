import pandas as pd
import matplotlib.pyplot as plt

data_path = "./data/ring_data.csv"
save_path = "./output/ring.png"

# Read the CSV data into a DataFrame
df = pd.read_csv(data_path)

# Create a bar graph
plt.figure(figsize=(6, 4))
bars = plt.bar(df['Method'], df['Throughput'], color=['blue', 'green'], width=0.4)

# Add labels and title
plt.xlabel('Communication Method')
plt.ylabel('Throughput(MB/s)')
plt.title('Comparison: Alternative Read/Write vs. Ring Buffer')
plt.ylim(0, max(df['Throughput']) * 1.2)

# Add text labels on top of each bar
for bar in bars:
    height = bar.get_height()
    plt.annotate(f'{height}',
                 xy=(bar.get_x() + bar.get_width() / 2, height),
                 xytext=(0, 3),  # 3 points vertical offset
                 textcoords="offset points",
                 ha='center', va='bottom')

plt.tight_layout()
plt.savefig(save_path)
