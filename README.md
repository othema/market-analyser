# Market Analyzer (C / Dear ImGui via cimgui)

A C project for a stock market analyzer using [Dear ImGui](https://github.com/ocornut/imgui) through [cimgui](https://github.com/cimgui/cimgui), with GLFW/OpenGL for rendering and [ImPlot](https://github.com/epezent/implot) for charting.

Stock price data is fetched using the Yahoo Finance chart API.

## Showcase
![Chart view](docs/img/showcase1.png)

## Features
- Candlestick/line chart rendering
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
