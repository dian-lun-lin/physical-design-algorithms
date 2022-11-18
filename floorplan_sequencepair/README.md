# Fixed-outline Floorplanning


## Problem Description

This programming assignment asks you to write a fixed-outline chip floorplanner that can handle hard macros. Given a set of rectangular macros and a set of nets, the floorplanner places all macros within a rectangular chip without any overlaps. We assume that the lower-left corner of this chip is the origin (0,0), and no space (channel) is needed between two different macros. The objective is to minimize the area of chip bounding box and the total net wirelength. The total wirelength $W$ of a set of nets $N$ can be computed by:

$$
W = \sum_{n_i \in N} HPWL(n\_i)
$$

where HPWL denotes the half-perimeter wirelength, i.e., half of bounding box length, of net $n_i$. The objective for this problem is to minimize:

$$
Cost = \alpha A/A_{norm} + (1 - \alpha) W/W_{norm}
$$

where $A$ denotes the bounding-box area of the floorplan, Anorm is the average area, $W$ is the total wire length, Wnorm is the average wire length, and $\alpha$, $0 ≦ \alpha ≦ 1$, is a user defined ratio to balance the final area and wirelength. To compute $A_{norm}$ and $W_{norm}$, we can perturb the initial solution $m$ times to obtain $m$ floorplans and compute the average area $A_{norm}$ and the average wire length $W_{norm}$ of these floorplans. The value $m$ is proportional to the problem size. 

Note that a floorplan which cannot fit into the given outline is not acceptable.

# How to Compile & Run

There are serveral details in my sequenece-pair floorplanning (SPF) implementaion:
 

1. I randomly initialize sequence pair and apply simulated annealing (SA) to improve cost.
3. I apply openmp to find solution in parallel. 
Each thread will create its sequence pair and perform SA. 
After SA, I will find the best solution and update each thread's sequence pair to the best.
2. By default, the number of thread is one.
4. I run SA at most 30 times.
5. My cost function only considers whether the solution is legal.
I will jump out of SA loop once I find a legal solution.
6. Finally, I will apply compress() to get the best result.

To compile, you need GNU C++ Compiler at least v7.0 with -std=c++17. I recommend out-of-source build with cmake:

```bash
~$ mkdir build
~$ cd build
~$ cmake ../
~$ make
```

You will see an executable file `sp` under `bin/`.
To run sp, you can simply type:

```bash
~$ cd bin
~$ ./sp [alpha] [input_file] [output_file] [num_threads=1]
```

For example, to enable eight threads:

```bash
~$ ./sp 0.5 input_pa2/2.block input_pa2/2.net 2.out 8
```

# Experimental Results
I implement SPF using C++17 and compile SPF using GCC-8 with optimization -O3 enabled. I run SPF on twhuang-server-01

##  Single thread
The following table shows runtime of my SPF implementaion on each benchmark using one thread. My SPF implementation can find inbound solution for all benchamrks. In input_0.dat, the algorithm requries 9 seconds to finish. That is because input_0.dat contains the largest number of cells, which induces noticable overhead of updating buckets at each iteration within a pass.


| Input       | Total cost | runtime |
|-------------|------------|---------|


##  Eight threads
The following table shows runtime and cut size of our F-M on each benchmark using eight threads. We can clearly see my implementation can get signifcant improvement by using multiple CPU threads. Take as an example, the n is **6.8x** compared to initial partition.





# Challenges
During the F-M implementation, I encounter three challenges:

 1. Tuning SA is extremely challenging 
 2. Debugging SPF is much more diffcult than FM partitiong.
 
Apparently, the fist challenge is due to a bug in my code. To identify where the bug is, I carefully go through a simple testcase (i.e., input_6.dat) and print out
the updated gain of each cell at each iteration within a pass. The print-out results show I did not correctly update the gain of each cell.
For the second and thrid challengs, I dive into the F-M paper and figure out the implemntation details.


