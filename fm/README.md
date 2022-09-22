# F-M Circuit Partitioning

## Problem Description

Let $C = c_1, c_2, c_3, ..., c_n$ be a set of $n$ cells and $N = n_1, n_2, n_3, ..., n_m$ be a set of $m$ nets. Each net $n_i$ connects a subset of the cells in $C$. Your job in this programming assignment is to implement the 2-way [Fiduccia-Mattheyses algorithm](https://en.wikipedia.org/wiki/Fiduccia%E2%80%93Mattheyses_algorithm) that partitions the set $C$ of $n$ cells into two disjoing, balanced groups, $G_1$ and $G_2$, such that the overall cut size is minimized. No cell replication is allowed. The cust size $s$ is given by the number of nets among $G_1$ and $G_2$. For a given balance factor $r$, where $0 < r < 1$, the objective is to minimize the cut size $s$ while satisfying the following constraint:

$$
n\times(1-r)/2 \leq |G_1|, |G_2| \leq n\times(1+r)/2 
$$

# How to Compile & Run

To compile F-M partitioning algorithm, you need :
 * GNU C++ Compiler at least v7.0 with -std=c++17.
```bash
~$ g++ --version     # GNU must present with version at least v7.0
```
I recommend out-of-source build with cmake:
```bash
~$ mkdir build
~$ cd build
~$ cmake ../
~$ make
```
You will see an executable file `fm` under `bin/`.
To run F-M, you can simply type:

```bash
~$ cd bin
~$ ./fm [input file name] [output file name]
```

For example:
```bash
~$ ./fm input_pa1/input_0.dat output_0.dat
```



+ A section listing partition results in terms of cut size and runtime for each *PASSED* benchmark 
+ A section outlining the challenges you encountered and solved during the implementation


# Experimental Results
We implement F-M using C++17 and compile F-M using GCC-8 with optimization -O3 enabled.
We run F-M (**single CPU core**) on a Ubuntu Linux 5.0.0-21-generic x86 64-bit machine with 40 Intel Xeon Gold 6138 CPU cores at 2.00 GHz and 256 GB RAM.


## One single pass
Table 1 shows runtime of our F-M on each benchmark for one pass. 

Table 1:
 Input | cut_size | runtime |
| :-: | :-: | :-: |
| input_0.dat |  42239  |  7.69s |
| input_1.dat | 2569  | 0.01s |
| input_2.dat |  4283  |  0.02s  |
| input_3.dat |  35052 | 0.35s |
| input_4.dat |  99017 | 0.72s |
| input_5.dat |  290729 | 2.26s |
| input_6.dat |  2 | 0.003s |

## Best cut size
Table 2 shows runtime of our F-M on each benchmark for one pass.

 Input | cut_size | # passes | runtime
| :-: | :-: | :-: |
| input_0.dat |  
| input_1.dat | 
| input_2.dat |  
| input_3.dat | 
| input_4.dat | 
| input_5.dat |  
| input_6.dat | 
# Challenges






