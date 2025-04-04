<p align="center">
  <img src="./assets/images/secure_ivshmem_logo.png" alt="Project Logo" width="400" height="200" style="object-fit: cover;"/>
</p>

<h1 align="center">Secure IVSHMEM</h1>

<p align="center">
  Secure and Performant, the new IVSHMEM Communication Protocol
</p>

---

<!-- ## 🚀 Features

- Feature 1
- Feature 2
- Feature 3 -->


This document introduces the core ideas behind our approach to enhancing IVSHMEM performance, with a focus on security, efficiency, and dynamic resource management. The full details are available in the main README: [./ivshmem_perf/README.md](./ivshmem_perf/README.md)

## Problem Overview

1. **Security Concerns:**
   - IVSHMEM is inherently insecure—malicious processes can corrupt the shared memory.

2. **Inefficient Synchronization:**
   - Mutex locks used in multi-producer, multi-consumer environments significantly degrade performance.

3. **Complex Access Control:**
   - Implementing fine-grained access control for each VM or process can prevent attacks but adds complexity for developers.

4. **Inefficient Data Copying:**
   - Repeated copying of data into the IVSHMEM region due to mutex-based read and write operations is inefficient.

## Proposed Solution

### Core Concept

Develop a kernel module that centrally controls access to the IVSHMEM region for all VMs. Instead of granting direct memory access, processes interact with IVSHMEM exclusively through a defined API. This design secures shared memory and minimizes performance penalties by reducing lock usage.

### Communication Mechanisms

- **Interrupts and Polling:**
  - Communication occurs via doorbell interrupts or polling mechanisms.
  
- **Primary APIs:**
  - `ivshmem_send(ptr buffer, int size)`
  - `ivshmem_recv(ptr buffer, int size)`

### Buffer Allocation and Management

- **Channel-Based Buffer Assignment:**
  - Each sender is allocated a dedicated channel within IVSHMEM.
  - Example: For a total IVSHMEM size of 4 MB:
    - Process A on VM1 sending 5 GB is assigned a 1 MB channel.
    - Process C on VM2 sending 5 GB is also assigned a 1 MB channel.
    - The remaining 2 MB is reserved for future allocations.

- **Control Section:**
  - Maintains metadata about buffer assignments and channel usage.
  
  **Shared Variables:**
  - **`free_start_offset` (int):** Points to the next available space for new buffer allocation.
  - **`num_active_channel` (int):** Tracks the number of active channels.
  - **`control_mutex` (Mutex):** Ensures thread-safe modifications during resizing.
  - **`channels` (Array):** Holds metadata for each channel.

  **Channel Structure:**
  - **`sender_vm` (int):** Sender's VM identifier.
  - **`sender_pid` (int):** Sender's process identifier.
  - **`receiver_vm` (int):** Receiver's VM identifier.
  - **`buff_offset` (int):** Offset where the channel begins.
  - **`buff_size` (int):** Allocated buffer size.
  - **`data_size` (int):** Current data amount in the channel.
  - **`tail` and `head` (Indices):** For ring buffer management.
  - **`head_touch` (int):** Counter for monitoring activity, especially during resizing.

### Data Transmission Strategy

- **Lock-Free Operations:**
  - Data is transmitted within dedicated channels without locks, ensuring isolation between channels.

- **Ring Buffer Implementation:**
  - Each 1 MB channel is subdivided into multiple 4 KB blocks.
  - A ring buffer mechanism using head and tail indices allows continuous, efficient data read/write operations without excessive data copying.

- **Dynamic Buffer Resizing:**
  - Every N interrupts (e.g., 100), the system rebalances channel allocations:
    - Active channels may be expanded (e.g., from 1 MB to 1.5 MB).
    - Inactive channels can be downsized or removed.
  - The control section is locked using `control_mutex` during this rebalancing.

- **New Channel Assignment:**
  - When a new communication request is initiated, a new channel is created:
    - A new control section entry is made.
    - `free_start_offset` is incremented accordingly.

## Benefits

1. **Enhanced Security:**
   - Processes are confined to their assigned IVSHMEM regions, reducing the risk of unauthorized access.

2. **Optimized Throughput:**
   - Dynamic buffer partitioning allows for resizing based on traffic, enhancing throughput.
   - The ring buffer mechanism minimizes repetitive data copying.

3. **Minimal Locking Overhead:**
   - Locks are used only during infrequent rebalancing operations, allowing most operations to run lock-free in a multi-process environment.
