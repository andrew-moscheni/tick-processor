# Tick Processor

A high-performance financial tick data pipeline in C++17.
Ingests real-time trade data, computes rolling statistics,
and detects price anomalies using Welford's online algorithm.

Built as a portfolio project demonstrating C++17, Linux systems
programming, concurrent data pipelines, and applied statistics.

## Features

- CSV parser using memory-mapped-style streaming reads
- Welford's online algorithm for numerically stable rolling
  mean, variance, and standard deviation
- Z-score anomaly detection (flags trades beyond 3σ)
- Thread-safe producer/consumer queue using std::mutex and
  std::condition_variable
- 14 unit tests across parser, stats engine, and queue
- Benchmarked at 60,908,739 ticks/sec (single-threaded stats engine)

## Project structure
tick-processor/  
├── include/          # Header files  
│   ├── tick.h        # Tick struct and parse_csv declaration  
│   ├── stats_engine.h  
│   ├── tick_queue.h  
│   └── pipeline.h  
├── src/              # Implementation  
│   ├── parser.cpp  
│   ├── stats_engine.cpp  
│   ├── tick_queue.cpp  
│   ├── pipeline.cpp  
│   ├── main.cpp  
│   └── benchmark.cpp  
├── tests/  
│   └── test_main.cpp  
└── CMakeLists.txt  
## Build

Requires GCC 11+, CMake 3.14+, and libgtest-dev on Ubuntu.

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# Process one day of BTC/USDT tick data
./tick_processor

# Run the test suite
./run_tests

# Run the benchmark (1M synthetic ticks)
./benchmark
```

## Benchmark results

Measured on AMD Ryzen 5 & Ubuntu version 24.04.1.

| Mode | Ticks | Time | Throughput |
|---|---|---|---|
| Stats engine (single-threaded) | 1,000,000 | 16.42 ms | 60,908,739 ticks/sec |
| Full pipeline (two threads) | 1,000,000 | 179.44 ms | 5,572,769 ticks/sec |

The single-threaded stats engine outperforms the full pipeline
because Welford's algorithm is cheap (~3 arithmetic ops per tick)
and the two-thread pipeline pays mutex/condition_variable overhead
on every tick. In a real system the producer would be doing
heavier I/O work, making the concurrency benefit larger.

## Key implementation notes

**Welford's algorithm** — updates mean and variance in a single
pass with O(1) memory. Numerically stable for large datasets
unlike the naive two-pass approach.

**Bounded queue** — the producer blocks when the queue reaches
capacity, preventing unbounded memory growth if the consumer
falls behind.

**Z-score anomaly detection** — flags any trade whose price is
more than 3 standard deviations from the rolling mean. On
2024-01-15 BTC/USDT data this detected 210 anomalies
out of 487,203 trades (~0.000431).

## Future Implementations

The next stage of this project is to create a streaming pipeline to gather live market data and implement it in this tick processor. Once that is completed, common pricing models (momentum-based, Black-Scholes model, binomial model, etc.) will be created and added as modules in this program.

## Data source

Binance public trade data:
https://data.binance.vision/data/spot/daily/trades/BTCUSDT/
