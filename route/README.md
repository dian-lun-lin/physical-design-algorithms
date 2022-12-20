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


| Input       | wirelength |
|-------------|------------|
| case1       | 249 |
| case2       | 499 | 
| case3       | 780 | 
| case4       | 17093 | 
| case5       | 2418476 | 
| case6       | 34300909 | 
| case7       | 548777096 | 
| case8       | 7669941016 | 
| case100000  | 24375982283 |
| case200000  | 34477180072 |
| case500000  | 602386 |  


# Challenges
During theimplementation, I need to understand how to use flute3 to solve the problem. The main challenge is that flute3 is not well documented. I need to look into the source code to understand the usage of flute3.  


