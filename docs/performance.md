# TrafficSim performance benchmarks

## Purpose

These benchmarks measure the scalability of road lookup, route calculation,
vehicle updates, statistics collection, and complete simulations.

Benchmarks are executed manually in GitHub Actions to avoid local application
control restrictions and to provide a recorded, reproducible environment.

## Methodology

The benchmark suite uses:

- Release builds with C++20.
- Four vehicle counts: 100, 1,000, 5,000, and 10,000.
- Three repetitions for every benchmark and vehicle count.
- Average wall-clock and process CPU times.
- Maximum resident memory for the complete benchmark process.
- A dedicated GitHub-hosted runner for each execution.

The individual benchmark scenarios are:

| Benchmark | Work performed |
|---|---|
| Road lookup | 100 road lookups per vehicle |
| Routing | One Dijkstra route calculation per vehicle over a 100-road linear network |
| Vehicle update | Ten simulation steps of 0.1 seconds |
| Statistics | One road-statistics observation per vehicle |
| Full simulation | Five simulated seconds with a 0.1-second time step |

Fixture creation is excluded from individual component timings. The full
simulation benchmark includes spawning, routing, vehicle updates, traffic
monitoring, and statistics collection.

## Optimized run environment

The optimized measurements were produced by commit
`3619e95e098be7d8315ce8e166ddbb236ab51a50`.

- Runner: GitHub-hosted `ubuntu-24.04`
- CPU: AMD EPYC 9V74, 4 logical processors
- Memory: 15 GiB
- Compiler: GCC 13.3.0
- CMake: 3.31.6
- Ninja: 1.13.2
- Build type: Release
- Test validation: 199 tests passed
- Test duration: 0.78 seconds

## Optimized results

All values are averages of three repetitions.

| Vehicles | Road lookup (ms) | Routing (ms) | Vehicle update (ms) | Statistics (ms) |
|---:|---:|---:|---:|---:|
| 100 | 0.07 | 1.05 | 0.11 | 0.01 |
| 1,000 | 0.57 | 10.08 | 1.22 | 0.09 |
| 5,000 | 2.56 | 49.92 | 7.93 | 0.41 |
| 10,000 | 5.27 | 99.80 | 17.05 | 0.68 |

| Vehicles | Full simulation (ms) | Simulated seconds per real second |
|---:|---:|---:|
| 100 | 0.97 | 5,149.29 |
| 1,000 | 11.38 | 439.29 |
| 5,000 | 65.83 | 75.96 |
| 10,000 | 137.26 | 36.43 |

The complete 60-sample benchmark required 1.25 seconds of wall time, used
approximately 99% of one CPU core, and reached a maximum resident set size of
8,924 KiB.

## Identified bottleneck

The initial profile showed quadratic growth in the full simulation:

| Vehicles | Initial full simulation (ms) | Optimized (ms) | Observed speedup |
|---:|---:|---:|---:|
| 100 | 0.93 | 0.97 | 0.95x |
| 1,000 | 33.32 | 11.38 | 2.93x |
| 5,000 | 779.67 | 65.83 | 11.84x |
| 10,000 | 3,077.16 | 137.26 | 22.42x |

`VehicleManager::addVehicle()` previously searched the complete vehicle vector
for every insertion. Spawning `n` vehicles therefore performed approximately
`O(n²)` identifier comparisons.

`VehicleManager` now maintains an `unordered_map` from vehicle identifier to
vector index. Adding, finding, and retrieving a vehicle are average `O(1)`
operations. The index is updated after arrived vehicles are removed and is
cleared when the manager resets.

In the optimized run, increasing the workload from 5,000 to 10,000 vehicles
increased full-simulation time by approximately 2.08x, demonstrating nearly
linear scaling at the largest measured sizes.

## Comparison limitation

GitHub assigned different processors to the two workflow executions:

- Initial run: Intel Xeon Platinum 8573C
- Optimized run: AMD EPYC 9V74

Consequently, the before-and-after speedup values are observational rather than
a controlled hardware comparison. The removal of quadratic behavior is also
supported by the optimized run's internal scaling and by the unchanged
component benchmark structure.

## Reproduction

Run the `Performance benchmarks` workflow manually from GitHub Actions. The
workflow:

1. Builds and executes the complete Debug test suite.
2. Builds the benchmark executable in Release mode.
3. Runs a five-sample smoke benchmark.
4. Runs all 60 performance samples.
5. Uploads CSV results, process metrics, and environment details as an artifact.

Generated results are intentionally excluded from version control.