# Multi-Asset Option Pricing

## Goal

This C++ project prices multi-asset options with Monte Carlo simulation and
computes finite-difference deltas. It also builds a dynamically rebalanced
hedging portfolio from a realized market trajectory.

The implemented option types are:

- `basket`
- `asian`
- `performance`

The sample cases in `data/` cover one-asset and multi-asset configurations.

## Requirements

- The PNL library (https://github.com/pnlnum/pnl/releases)
- `nlohmann_json`
- GoogleTest for the unit tests 

## Build

From the project root:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/matieres/5MMPCPD/pnl \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Main executables

### Price at time 0

```bash
./build/price0 data/call.json
```

`price0` reads one option/model JSON file and writes a JSON result to stdout:

```json
{
  "price": 0.0,
  "priceStdDev": 0.0,
  "delta": [],
  "deltaStdDev": []
}
```

### Build a hedging portfolio

```bash
./build/hedge data/call_market.txt data/call.json
```

`hedge` takes a market-path text file followed by the option/model JSON file.
It computes the portfolio at the initial date and at every hedging date, then
writes `{"portfolio": [...]}` to stdout. The market path must contain one row
per hedging date, including the initial row, and one column per asset.

## Input files

An option JSON file contains the model and simulation parameters used by both
executables. Required fields are:

```json
{
  "model size": 2,
  "strike": 100.0,
  "spot": [100.0, 100.0],
  "maturity": 1.0,
  "volatility": [0.2, 0.2],
  "interest rate": 0.05,
  "correlation": 0.0,
  "option type": "basket",
  "payoff coefficients": [0.5, 0.5],
  "timestep number": 12,
  "sample number": 50000,
  "hedging dates number": 12,
  "fd step": 0.1
}
```

## Tests

Run the complete CTest suite after building:

```bash
ctest --test-dir build --output-on-failure
```

The suite includes the GoogleTest target `unit_tests` and the legacy tests for
the date helper, JSON reader, and pricer. They cover JSON parsing and output,
option payoffs, Black-Scholes paths, Monte Carlo pricing/deltas, and portfolio
construction.

Individual test executables can also be run directly:

```bash
./build/test_json_reader test/data/json_reader_input.json
./build/test_compute_lastdate
./build/test_pricer
./build/unit_tests
```

## Batch and comparison scripts

### `testForPCPD.py`

Runs all JSON cases in a data directory with `price0` or `hedge`, stores the
obtained JSON files, and compares them with the matching `*_expected_*.json`
files.

Price tests:

```bash
python3 testForPCPD.py \
  --exec build/price0 \
  --datadir data \
  --outdir resultats_tests \
  --price
```

Hedge tests:

```bash
python3 testForPCPD.py \
  --exec build/hedge \
  --datadir data \
  --marketdir data \
  --outdir resultats_tests \
  --hedge
```

Important options:

- `--exec PATH`: executable to run. If omitted with `--toplevel`, all matching executables are searched there.
- `--datadir PATH`: directory containing input JSON cases.
- `--marketdir PATH`: directory containing hedge market files; defaults to `--datadir`.
- `--outdir PATH`: directory for generated outputs and comparison reports.
- `--toplevel ROOT`: uses `ROOT/Executables` and `ROOT/Tests`, taking precedence over the other path options.
- `--compare-only`: skips execution and compares outputs already present in `--outdir`.
- Exactly one of `--price` or `--hedge` is required.

The script uses a 60-second timeout per price case and a 500-second timeout
per hedge case. It writes obtained files under
`<outdir>/Outputs/` and comparison reports under `<outdir>/Results/`.

### `scripts/compare_prices.py`

Runs `price0` on every non-expected JSON file, prints a comparison table, and
returns a non-zero status when a price or delta differs from its reference by
at least three expected standard deviations.

```bash
python3 scripts/compare_prices.py \
  --exec build/price0 \
  --datadir data \
  --timeout 60 \
  --json price_report.json
```

All arguments are optional. Defaults are `build/price0`, `data`, a 60-second
timeout, and no report file. The script compares price distance, delta
distance, standard-deviation ratio, and execution time.

## Directory guide

```text
src/                  C++ pricing, Monte Carlo, JSON, and portfolio code
test/                 Legacy tests, unit tests, and test input data
data/                 Input cases, market paths, and expected JSON results
scripts/              Additional Python comparison utilities
resultats_tests/      Checked-in/generated batch outputs and dashboards
CMakeLists.txt        Build targets, dependencies, and CTest registration
testForPCPD.py        Batch price/hedge runner and result comparator
```

The `data/` cases generally have this group of files: `<case>.json`,
`<case>_market.txt`, `<case>_expected_price.json`,
`<case>_expected_hedge.json`, and `<case>_expected_portfolio.json`.
