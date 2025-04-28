import pandas as pd
import matplotlib.pyplot as plt
import mtl_config

data_path = "./data/shm_basic.csv"
save_path = "./output/shm_basic.png"

# Read the CSV data into a DataFrame
df = pd.read_csv(data_path)


# Plot the data
plt.figure()

plt.plot(
    df['shm_size(log2)'],
    df['ivshmem'],
    marker='o',      # circle marker at each point
    linestyle='-',   # keep the connecting line
    label='IVSHMEM',
    color='darkblue'  # color for the line
)
plt.plot(
    df['shm_size(log2)'],
    df['secure_ivshmem'],
    marker='s',      # square marker at each point
    linestyle='-',
    label='Secure IVSHMEM',
    color='brown'    # color for the line
)

plt.xlabel('Message Size (log2 Bytes)')
plt.ylabel('Bandwidth(MB/s)')
plt.legend()

plt.tight_layout()
plt.savefig(save_path)
plt.show()