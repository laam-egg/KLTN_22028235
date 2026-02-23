# procbench Benchmarking for ember2024-fe-cpp

## Overview

Go to each `tcXX.json` file in this directory to see details of each test case.

The results are stored in [`results.json`](./results.json) and [`results.html`](./results.html) files.

## Run the benchmark yourself

For each `tcXX.json` file in this directory:
- Replace the PE file paths with valid ones on your machine.
- Also pay attention to the path of the `efe_demo` executable - it
    differs based on the platform. The current files assume a Linux
    environment.

Then, install procbench:

```sh
pip install procbench==0.1.1
```

Run it:

```sh
cd <project_root>
cd benchmarks/procbench

procbench run tc01.json tc02.json tc03.json tc04.json tc05.json -o results.json
procbench export html results.json results.html
```

Then open `results.html` in your browser to see the performance results.
