# Market Analyzer (C / Dear ImGui via cimgui)

A C project for a stock market analyzer using [Dear ImGui](https://github.com/ocornut/imgui) through [cimgui](https://github.com/cimgui/cimgui), with GLFW/OpenGL for rendering and [ImPlot](https://github.com/epezent/implot) for charting.

Stock price data is fetched using a Yahoo Finance chart interface (`src/yfinance/`) with no API key required.

## Showcase
![Chart view](docs/img/showcase1.png)

## Features
- Integrated single-window UI with selector and chart sections
- Fetches daily OHLCV data from Yahoo Finance (`1d`, `6mo` by default)
- Candlestick chart rendering with fitted axes
- Reusable Yahoo Finance C interface (`src/yfinance/`)

## Architecture
- `src/main.c` — application entry point
- `src/app/` — app lifecycle, UI composition, and top-level orchestration
- `src/platform/` — GLFW/OpenGL + ImGui/ImPlot backend setup and frame lifecycle
- `src/fetch/` — asynchronous stock/suggestion fetching service and UI-facing status snapshots
- `src/chart/` — chart state and OHLC data management
- `src/windows/` — UI sections (`stock_selector`, `stock_data`)
- `src/plot/` — C wrapper over ImPlot for C UI code
- `src/yfinance/` — Yahoo Finance client/fetch/parse interface

## Dependencies
Fetched/built by CMake where configured:
- GLFW
- Dear ImGui (docking)
- cimgui
- ImPlot

System dependencies:
- OpenGL
- libcurl
- json-c
- pkg-config

## Build
```bash
cmake -S . -B build
cmake --build build -j4
```

## Run
```bash
./build/market-analyzer
```

Or:
```bash
cmake --build build --target run
```

## Usage
1. Launch the app.
2. Enter a ticker symbol (e.g. `AAPL`, `MSFT`) in the selector section.
3. Click **View data** to fetch and display the candlestick chart and available company/chart metadata.

## Notes
- Yahoo Finance’s chart endpoint is unofficial and may change, breaking this project
