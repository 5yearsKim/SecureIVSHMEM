# IVSHMEM Performance Measurement

This project is designed to measure the performance of IVSHMEM throughput in various aspects. The easiest way to test the shared memory communication is by using a simple device driver for a shared memory device.

## Setup

1. **Move to the Home Directory**

   ```sh
   cd ivshmem_perf
   ```

2. **Load the Kernel Module for the Shared Memory Device Driver**

   ```sh
   cd hw
   ./build_and_load.sh
   ```

3. **Verify the Shared Memory Device**

   ```sh
   ls -al /dev/myshm
   ```

## Build the Project

1. **Prepare the Build Directory and Build**

   ```sh
   mv ivshmem_perf
   mkdir -p build
   cd build
   cmake ..
   make
   ```

## Running the Applications

1. **Run the Trusted Host**

   Open a new terminal and run the trusted host application. This application controls the config space for multiple VMs.

   ```sh
   ./trusted_main
   ```

2. **Run the Writer Application**

   ```sh
   ./writer/writer
   ```

3. **Run the Reader Application**

   ```sh
   ./reader/reader
   ```

