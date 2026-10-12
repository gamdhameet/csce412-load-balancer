/** @file LoadBalancer.h @brief FIFO dispatch and dynamic server capacity. */
#pragma once

#include "WebServer.h"

#include <cstddef>
#include <iosfwd>
#include <queue>
#include <string>
#include <vector>

namespace codeharbor {

/** A single change to the number of allocated servers. */
struct ScaleEvent {
    unsigned cycle;
    std::string action;
    std::size_t servers;
    std::size_t waiting;
};

/** Cumulative work and capacity measurements for one balancer. */
struct BalancerStats {
    std::size_t accepted = 0;
    std::size_t discarded = 0;
    std::size_t assigned = 0;
    std::size_t completed = 0;
    std::size_t peak_queue = 0;
    std::size_t servers_added = 0;
    std::size_t servers_removed = 0;
    std::size_t peak_servers = 0;
    std::size_t minimum_servers = 0;
};

/** Owns a request queue and a resizable collection of web servers. */
class LoadBalancer {
public:
    /** Construct a named pool with initial capacity and scaling limits. */
    LoadBalancer(std::string name, std::size_t servers, std::size_t min_servers,
                 std::size_t max_servers, std::size_t max_queue, unsigned scale_interval);

    /** Enqueue a request, or discard it if the configured queue limit is reached. */
    bool enqueue(const Request& request);

    /** Process one clock tick, assign work, and check scaling when due. */
    void tick(unsigned cycle, std::ostream* verbose);

    /** Name of this job pool. */
    const std::string& name() const { return name_; }
    /** Number of requests waiting for a server. */
    std::size_t waiting() const { return queue_.size(); }
    /** Number of allocated servers. */
    std::size_t server_count() const { return servers_.size(); }
    /** Number of servers with active work. */
    std::size_t busy_count() const;
    /** Number of allocated servers currently idle. */
    std::size_t idle_count() const { return servers_.size() - busy_count(); }
    /** Cumulative counters. */
    const BalancerStats& stats() const { return stats_; }
    /** Ordered history of capacity changes. */
    const std::vector<ScaleEvent>& scale_events() const { return scale_events_; }

private:
    void scale(unsigned cycle, std::ostream* verbose);

    std::string name_;
    std::queue<Request> queue_;
    std::vector<WebServer> servers_;
    std::size_t min_servers_;
    std::size_t max_servers_;
    std::size_t max_queue_;
    unsigned scale_interval_;
    unsigned next_id_ = 1;
    BalancerStats stats_;
    std::vector<ScaleEvent> scale_events_;
};

} // namespace codeharbor
