# IAE_Project

## Introduction
This repository contains the implementation, benchmarking, and analysis of algorithms for finding Biconnected Components (Articulation Points) in undirected graphs. Specifically, we compare:
- **Jen-Schmidt Algorithm**: Computes Open Ear / Chain Decompositions using a highly optimal, sequential DFS approach with strict $O(V + E)$ complexity.
- **Slota-Madduri Algorithm**: A parallel-friendly BFS and exclusion-search approach to discover articulation points. Highly scalable for robust graphical structures but can suffer $O(V \cdot (V + E))$ bottlenecks in certain tree-like extremes.

The goal is to benchmark and profile their execution times and memory usages across different network topologies (dense, highly connected, large, real-world, small, sparse, and tree-like graphs).

## How to Run

### Prerequisites
- GCC / G++ (with OpenMP support for Slota-Madduri parallelization)
- Python 3.x (for running the dataset evaluation script and generating plots)
- Matplotlib / Pandas (if the graphing script is used)

### Setup & Instructions

1. **Extract the Datasets**
   Unzip the `dataset.zip` file so that the `dataset` directory exists alongside your code files.
   ```bash
   unzip dataset.zip
   ```

2. **Run The Benchmarks**
   We have provided a Python runner script that automatically compiles the C++ codes and runs the datasets through them to collect results.
   
   Execute the script by running:
   ```bash
   cd code
   python3 run_dataset.py
   ```
   
   *(Note: The python script internally issues the `g++ -O2 -std=c++17 -fopenmp slota-dataset.cpp -o slota_dataset` and equivalent combinations so you don't have to compile them manually unless modifying them directly).*

3. **Check the Output**
   The execution times and memory usages will be produced in the resultant log files (or printed directly relative to how the script is outputting).