# Parallel Monte Carlo Risk Engine

A high-performance C program for estimating the risk profile of a European call option using a parallel Monte Carlo simulation engine. The project distributes independent simulation paths across multiple worker processes, aggregates the results in a controller process, and reports key risk metrics such as mean PnL, standard deviation, and Value at Risk (VaR).

## Project Description

This project models a one-day risk calculation for a long European call position under the Black-Scholes framework. It simulates geometric Brownian motion (GBM) price paths, computes the option's profit-and-loss (PnL) under a one-day horizon, and then estimates the statistical distribution of those outcomes.

The application is designed for performance and observability:

- Worker processes are created with `fork()` to parallelize Monte Carlo simulation.
- Each worker receives a task through a pipe and writes results back through another pipe.
- The controller aggregates results using `select()` so workers can finish asynchronously.
- A deterministic base seed can be supplied to make runs reproducible for benchmarking and testing.
- The output includes the model inputs, the Black-Scholes theoretical call price at time zero, and risk metrics for 95% and 99% VaR.

This makes the project useful for studying both numerical finance and multiprocessing design in C.

## Key Features

- Parallel Monte Carlo simulation using multiple worker processes
- European call option pricing under Black-Scholes assumptions
- One-day PnL simulation based on GBM dynamics
- Risk metrics:
  - Mean PnL
  - Standard deviation of PnL
  - 95% and 99% VaR
- Reproducible seeded execution
- Command-line interface for model inputs and simulation size

## Repository Structure

- `main.c` — command-line parsing and result reporting
- `controller.c` / `controller.h` — process orchestration and aggregation logic
- `worker.c` / `worker.h` — worker simulation tasks
- `mc.c` / `mc.h` — Monte Carlo path generation and GBM logic
- `option_pricing.c` / `option_pricing.h` — Black-Scholes pricing helpers
- `stats.c` / `stats.h` — statistics and VaR calculations
- `protocol.h` — shared IPC message definitions
- `util.c` / `util.h` — common helper functions
- `Makefile` — build and run targets

## Requirements

- GCC
- GNU Make
- POSIX-compliant environment (Linux or macOS; the project uses `fork()`, `pipe()`, and `select()`)

## Build

```bash
make
```

This produces the executable `risk_engine`.

## Run

```bash
./risk_engine 4 50000 100 100 0.05 0.2 0.25
```

Example arguments:

- `num_workers`: number of worker processes (minimum 3)
- `num_sims`: total Monte Carlo simulations
- `S0`: current underlying price
- `K`: strike price
- `r`: annual risk-free rate
- `sigma`: annual volatility
- `T_years`: maturity in years
- `seed` (optional): base random seed

## Example with custom seed

```bash
./risk_engine 4 50000 100 100 0.05 0.2 0.25 42
```

## Notes

The program is designed as a practical C systems project that combines numerical finance, probabilistic simulation, and parallel process communication. It is particularly suited for understanding how Monte Carlo risk estimation can be decomposed across worker processes while preserving reproducibility and numerical accuracy.
