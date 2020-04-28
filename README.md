# Malloc
A study on memory allocators in C++

Look at this space for useful FAQ's on dealing with memory !

# Usage
- ```module load gcc-9.2``` (if you are in a SLURM machine)
- ```mkdir build```
- ```make``` to make the program and run.
- ```make debug``` to build program for debugging.

# Description

Running the above commands will create a build binary and run our set of tests against each allocator. It will run a test of a 1000 randomly generated allocations and deallocations. This is worked upon by 4 threads. This parameter can be changed in the main function in main.cpp , namely the variables n_th and prob_size.

In each folder under include, we have the different allocators, namely Hoard, Jemalloc and Tcmalloc.

## Note:

The allocators might take time if proper cores/compute aren't available in the cims machine. Using appropriate sbatch request should mitigate this.
