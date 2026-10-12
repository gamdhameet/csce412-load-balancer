/** @file Switch.cpp @brief Admission control and job-type routing. */
#include "Switch.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace codeharbor {

Switch::Switch(std::size_t total_servers, std::size_t min_servers_per_pool,
               std::size_t max_servers_per_pool, std::size_t max_queue_per_pool,
               unsigned scale_interval, IpFilter filter)
    : filter_(std::move(filter)),
      processing_("processing", (total_servers + 1) / 2, min_servers_per_pool,
                  max_servers_per_pool, max_queue_per_pool, scale_interval),
      streaming_("streaming", total_servers / 2, min_servers_per_pool,
                 max_servers_per_pool, max_queue_per_pool, scale_interval) {
    if (total_servers < 2 * min_servers_per_pool)
        throw std::invalid_argument("too few servers for two pools");
}

RouteResult Switch::route(const Request& request, unsigned cycle, std::ostream* verbose) {
    ++generated_count_;
    if (filter_.blocks(request.ip_in)) {
        ++blocked_count_;
        if (verbose) *verbose << "cycle " << cycle << " blocked source="
                              << request.ip_in << '\n';
        return RouteResult::blocked;
    }
    LoadBalancer* target = nullptr;
    if (request.type == 'P') target = &processing_;
    else if (request.type == 'S') target = &streaming_;
    else throw std::invalid_argument("unknown job type");
    if (!target->enqueue(request)) {
        if (verbose) *verbose << "cycle " << cycle << " " << target->name()
                              << " discard queue_full source=" << request.ip_in << '\n';
        return RouteResult::discarded;
    }
    if (verbose) *verbose << "cycle " << cycle << " " << target->name()
                          << " arrival source=" << request.ip_in
                          << " destination=" << request.ip_out
                          << " time=" << request.time
                          << " waiting=" << target->waiting() << '\n';
    return RouteResult::accepted;
}

void Switch::tick(unsigned cycle, std::ostream* verbose) {
    processing_.tick(cycle, verbose);
    streaming_.tick(cycle, verbose);
}

} // namespace codeharbor
