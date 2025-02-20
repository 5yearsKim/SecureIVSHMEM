# Secure IVSHMEM IDF Proposal

## 1. Title of the invention

Secure End-to-End Channel Allocation Protocol for IVSHMEM Communication


## 2. Names of the inventors

Intel IA Korea


## 3. Resubmision Responses:

N/A

## 4. Techonlogy Background

inter vm communication

inter vm shared memory communication

Generic IVSHMEM Device Driver


### 4.1 Problem Definition – What technical problem did you solve?


insecure nature of ivshmem

    threatening models

        Eave Dropping

        MITM Attack

        Relay Attack

Performance Degradation of Separating Channels
    Channel Separation -> performance degradation 
    Dynamic Channel Allocation 
        - Hypervisor has no way to trust the endpoint (Relay Attack)
        - Hard to enforce granual access control for each channel


-> we propose a protocol and kernel module for securely and dynamically allocating a end-to-end communication channel on IVSHMEM device.




### 4.2 Previous Solution (if any): Describe any previous solution used to solve the problem


mempipe
xensocket
grant table
sivshm


## 5. Overview of the invention


### 5.1 Short Summary – In 1-3 sentences, describe the core of your solution


### 5.2 Advantages – In 1-3 sentences, describe the value of the invention to Intel or to our
customers.


## 6. DETECTABILITY

### 6.1 Please describe in detail how your invention is detectable in a final product.

#### A. If your invention results in a specific structural feature please describe the appearance of that feature (e.g., include SEM/TEMs of actual features if available).


communication protocol
implementation
    user level library
    kernel module


#### B. If there are visual inspection and/or reverse engineering techniques that can be used to identify the feature, please describe them.



#### C. If documentation such as product literature would show usage of the invention, please let us know what to look for in that regard.


## 7. DETAILS OF THE INVENTION


### 7.1 Provide details that help us fully understand your invention, including details on how you solved the technical problem, and at least one figure. You may also provide flowcharts, graphs, slides, or data to support your description. Where appropriate, please provide and explain any empirical support, such as experimental data or simulation results, that can demonstrate the viability of your invention.
