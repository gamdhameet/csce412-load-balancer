# CodeHarbor load balancer

CSCE 412 Project 4. This C++17 simulation puts processing and streaming requests in separate FIFO queues. Each queue has its own server pool. A source-IP filter runs before routing.

## Build and run

```sh
make
./load-balancer
```

The program prompts for the initial total server count and number of clock cycles. At least two servers are needed because the two job types have separate pools. For a repeatable quiet run:

```sh
./load-balancer --servers 20 --cycles 100000 --seed 412 \
  --mode quiet --log logs/quiet-20x100000.txt
```

For event-level output, use `--mode verbose`. The log path is optional; without one, verbose events go to the terminal. Run commands from this directory so `config.txt` is found, or pass `--config PATH`.

## Model

The initial queues hold 100 accepted requests per initial server. Later arrivals happen randomly according to `arrival_probability`. The seed makes a run repeatable. Task lengths, source and destination IPs, and job types are generated from the settings in `config.txt`. A small configurable share of source IPs is sampled from a blocked test range to make filter behavior visible.

Every clock cycle, the program checks for arrivals, advances active work, and assigns waiting requests to idle servers. Every `scale_interval` cycles, each pool independently adds one server when its queue exceeds 80 times its server count. It removes one idle server when its queue falls below 50 times its server count. The configured minimum and maximum server counts still apply.

The summary gives initial and final queue sizes, task-time range, generated and filtered requests, completed work, active and idle servers, and the capacity-change history. The processing and streaming pools have separate counters. A request already assigned to a server remains active when the run ends; it is counted as assigned but not completed.

## Files

- `include/`, `src/`: C++ request, worker, filter, balancer, switch, configuration, and driver code
- `config.txt`: run settings and blocked IPv4 CIDR ranges
- `docs/mainpage.md`: Doxygen main page
- `Doxyfile`: HTML documentation settings (`make docs`)

`make clean` removes the local executable, object files, and generated Doxygen HTML.
`make package` writes a source-only ZIP to `dist/`.
