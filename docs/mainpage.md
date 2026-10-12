/**
@mainpage CodeHarbor load balancer

@section overview Overview

This project models a company's load balancer over discrete clock cycles. A request
contains source and destination IPv4 addresses, a processing duration, and a job
type. The IP filter rejects blocked sources. The switch sends admitted processing
and streaming jobs to separate FIFO queues and server pools.

@section workflow Workflow

The driver reads `config.txt` and the user's initial server count and run length.
It creates 100 queued requests per initial server. Each cycle can generate another
request, advances work already assigned to servers, and gives waiting requests to
idle servers. At the configured interval, each pool changes by at most one server
to keep its queue near the 50–80 requests per server band. A busy server is never
removed.

@section files Code map

- Request and RequestGenerator: request data and reproducible randomized input.
- IpFilter: blocked source IPv4 CIDR ranges.
- WebServer: one in-progress request and its remaining time.
- LoadBalancer: FIFO dispatch, counters, and server scaling.
- Switch: firewall admission and P/S job routing.
- Config: readable run settings.
- main.cpp: input, clock loop, logging, and summary.

@section usage Build and run

`make` builds the C++17 executable. `./load-balancer --servers 20 --cycles 100000
--seed 412 --mode quiet --log logs/quiet-20x100000.txt` runs the required example.
Use `--mode verbose` for an event log. The same seed and settings reproduce the
same request stream and final counts.
*/
