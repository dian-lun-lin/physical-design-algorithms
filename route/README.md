# Steiner-Tree Construction

Due: 12/16

## Problem Description

This programming assignment asks you to implement a Steiner-tree router 
that can connect pins for a single net on a single-layer chip. 
Given a set of pins for a single net, the Steiner-tree router routes all pins within a chip. 
Pins are connected by horizontal lines (H-line) and/or vertical lines (V-line). 
Steiner points are allowed to be used during routing.
The objective of Steiner-tree routing is to minimize the total routing wirelength. 
The total routing wirelength $W$ of a set of $P$ can be computed by the following:

$$
W = \sum_{pi \in P} w(p_i) + d
$$

where $p_i$ denotes an H-line or a V-line in the line segment set $P$ and $w(p_i)$ denotes the real routing wirelength of $p_i$. Here, $d$ denotes the disjoing cost evaluated by the following:

$$
d = 2 \times U \times H
$$

where $U$ is the number of unconnected pins and $H$ is the half perimeter wirelength (HPWL) of the chip boundary.

Note that a route which has any net routed out of the chip boundary is a failed result.

# How to Compile & Run

I use Flute3 to sovle the problem

To compile, you need GNU C++ Compiler at least v7.0 with -std=c++17. I recommend out-of-source build with cmake:

```bash
~$ mkdir build
~$ cd build
~$ cmake ../
~$ make
```

You will see an executable file `stree` under `bin/`.
To run the program, you can simply type:

```bash
~$ cd bin
~$ ./fm [input file name] [output file name] 
```


# Experimental Results
I implement stree using C++17 and compile F-M using GCC-8 with optimization -O3 enabled. I run the program on twhuang-server-01


The following table shows runtime of my F-M implementation on each benchmark for one pass. My F-M implementation can finish all benchamrks **within 10 seconds**. In input_0.dat, the algorithm requries 9 seconds to finish. That is because input_0.dat contains the largest number of cells, which induces noticable overhead of updating buckets at each iteration within a pass.


| Input       | cutsize | runtime |
|-------------|---------|---------|
| input_0.dat | 42256   | 9.00s   |
| input_1.dat | 2508    | 0.02s   |
| input_2.dat | 4247    | 0.04s   |
| input_3.dat | 35008   | 0.40s   |
| input_4.dat | 97944   | 0.89s   |
| input_5.dat | 288264  | 2.96s   |
| input_6.dat | 2       | 0.004s  |


# Challenges
During the F-M implementation, I encounter three challenges:

 1. The cut size cannot converage for multiple passes. 
 2. The overhead of inserting/erasing cells into bucket lists is large.
 3. The overhead of updating gain of each cell is large.
 
Apparently, the fist challenge is due to a bug in my code. To identify where the bug is, I carefully go through a simple testcase (i.e., input_6.dat) and print out
the updated gain of each cell at each iteration within a pass. The print-out results show I did not correctly update the gain of each cell.
For the second and thrid challengs, I dive into the F-M paper and figure out the implemntation details.

