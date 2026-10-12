/** @file Switch.h @brief Routes job types to independent load balancers. */
#pragma once

#include "IpFilter.h"
#include "LoadBalancer.h"

#include <cstddef>
#include <iosfwd>

namespace codeharbor {

/** Admission and routing result for one generated request. */
enum class RouteResult { accepted, blocked, discarded };

/** Runs separate processing and streaming pools behind one IP filter. */
class Switch {
public:
    /** Create two pools whose initial server counts add to total_servers. */
    Switch(std::size_t total_servers, std::size_t min_servers_per_pool,
           std::size_t max_servers_per_pool, std::size_t max_queue_per_pool,
           unsigned scale_interval, IpFilter filter);

    /** Apply the IP filter and queue an allowed request by job type. */
    RouteResult route(const Request& request, unsigned cycle, std::ostream* verbose);

    /** Advance both pools by one cycle. */
    void tick(unsigned cycle, std::ostream* verbose);

    /** Processing pool and its counters. */
    const LoadBalancer& processing() const { return processing_; }
    /** Streaming pool and its counters. */
    const LoadBalancer& streaming() const { return streaming_; }
    /** Total number of source addresses blocked. */
    std::size_t blocked_count() const { return blocked_count_; }
    /** Total generated requests submitted to the switch. */
    std::size_t generated_count() const { return generated_count_; }
    /** Current combined queue depth. */
    std::size_t waiting() const { return processing_.waiting() + streaming_.waiting(); }

private:
    IpFilter filter_;
    LoadBalancer processing_;
    LoadBalancer streaming_;
    std::size_t blocked_count_ = 0;
    std::size_t generated_count_ = 0;
};

} // namespace codeharbor
