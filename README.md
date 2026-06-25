# Alpaca Terminal

A real-time market data terminal in C++17. Connects to Alpaca's
WebSocket feed, ingests live trade and bar data for any set of
tickers, and renders a continuously updating table directly in
the CLI — overwriting each frame in-place to keep the display
compact and readable.

Built as a portfolio project demonstrating C++17, WebSocket
networking with libcurl, concurrent data pipelines, and
ANSI terminal rendering.

## Features

- Live WebSocket connection to Alpaca's IEX data feed via libcurl
- Subscribes to both bar (`b`/`d`) and trade (`t`) message types,
  updating OHLCV fields between bar intervals for real-time close prices
- Thread-safe producer/consumer architecture using `std::mutex`,
  `std::condition_variable`, and `std::atomic`
- In-place CLI rendering with ANSI escape sequences — rewrites
  the table every 500ms without scrolling
- Color-coded close price (green if close ≥ open, red if below)
- Displays: symbol, open, close, high, low, volume, trade count,
  VWAP, and last updated timestamp
- Graceful shutdown on Ctrl+C — restores cursor and cleans up
  the WebSocket connection

## Project structure

```
src/
├── main.cpp   # Full implementation (single file)
└── CMakeLists.txt
```

## Dependencies

- GCC 11+ or Clang 13+
- CMake 3.15+
- libcurl 7.86+ (WebSocket support required)
- nlohmann/json (auto-fetched by CMake if not found on system)

> **libcurl version:** WebSocket support (`curl_ws_send` / `curl_ws_recv`)
> was added in libcurl 7.86.0. Check your version with `curl --version`.
> If you're on Ubuntu 22.04 or earlier you may need to build curl from source.

## Credentials

Add your Alpaca API key and secret to the top of `main.cpp`:

```cpp
const char* API_KEY        = "YOUR_API_KEY";
const char* API_SECRET_KEY = "YOUR_API_SECRET";
```

> **Important:** move these into a separate config file and add it to
> `.gitignore` before committing. The comment in the source flags this.

The program connects to the IEX feed by default (`stream.data.alpaca.markets/v2/iex`).
To use the SIP feed (paid subscription), update `WS_URL` accordingly.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# Stream live data for one or more tickers
./src AAPL MSFT TSLA

# Any number of tickers is supported
./src SPY QQQ NVDA AMZN META
```

Press `Ctrl+C` to exit cleanly.

## How it works

The program splits into two threads on startup:

**Processor thread** — owns the network connection. Authenticates
with Alpaca, subscribes to bars and trades for the requested tickers,
then loops receiving WebSocket frames. Bar messages (`b`/`d`) populate
all OHLCV fields at once; trade messages (`t`) update the close price,
high, low, and volume tick-by-tick between bar arrivals.

**Client (render) thread** — waits on a condition variable until the
processor signals that the WebSocket handshake is complete, then enters
a 500ms render loop. Each iteration builds the full table into an
`ostringstream` before printing, then repositions the cursor using
`\033[{N}A\033[J` to overwrite the previous frame without scrolling.

Both threads share a `std::map<string, BarData>` protected by a mutex.
The `dirty` flag on each `BarData` entry tracks which rows have changed
since the last render, available for future use (e.g. row flash on update).

## Data source

Alpaca Markets WebSocket API:
https://docs.alpaca.markets/reference/stocklatestbars