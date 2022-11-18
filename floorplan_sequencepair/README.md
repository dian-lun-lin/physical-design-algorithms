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
4. I run SA at most 100 times.
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
~$ ./sp [alpha] [input_block_file] [input_net_file] [output_file] [num_threads=1]
```

For example, to enable eight threads:

```bash
~$ ./sp 0.5 input_pa2/2.block input_pa2/2.net 2.out 8
```

# Experimental Results
I implement SPF using C++17 and compile SPF using GCC-8 with optimization -O3 enabled. I run SPF on twhuang-server-01. All data is in average of ten runs.

##  Single thread
The following table shows runtime of my SPF implementaion on each benchmark using one thread. My SPF implementation can find inbound solution for all benchamrks. 


| Input       | Total cost | runtime |
|-------------|------------|---------|
| 1           | 360550     | 0.13s   |
| 2           | 360350     | 15.6s   |
| 3           | 107200849  | 633.17s |
| ami33       | 712069     | 1.3s    |
| ami49       | 21291598   | 3.4s    |
| apte        | 24308574   | 0.2s    |
| hp          | 4795343    | 0.7s    |
| xerox       | 11020250   | 0.4s    |


##  Eight threads
The following table shows runtime of my SPF implementaion on benchmark 2 and 3 using eight threads. The benchmark 2 and 3 are the largest bechmarks that require the longest runtime to find a valid solution.
For benchmark 2, we can clearly see my implementation using eight threads acheves times speed-up compared to single thread implementaion.
For benchmark 3, the runtime improvement is not significant since a valid solution in benchmark 3 is extremely hard to find. 
The SA keep finding local minimum and cannot jump out of the local minimum.


| Input       | Total cost | runtime |
|-------------|------------|---------|
| 2           | 360350     | 4.11s   |
| 3           | 



# Challenges
During the SPF implementation, I encounter two challenges:

 1.  Debugging SPF is much more diffcult than FM partitiong
 
 In my implemetation, bugs typically occur at sequence pair-to-graph transformation. However, this type of bug is very hard to find since SA does not reconize the bug. You can still get cost based on your wrong transformation results. The only way to debug is to print out the sequence pair and manully check whether the corresponding graph is correct.
 
 2.  Tuning SA is extremely challenging

SA is very senstive to parameters and the cost function.
Results can be significantly different by changing only one parameter. 
Trial and error is the only way to fine-tune parameters and cost function.
 

 


