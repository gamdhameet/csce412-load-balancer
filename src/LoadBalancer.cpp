/** @file LoadBalancer.cpp @brief Queue dispatch, work completion, and scaling. */
#include "LoadBalancer.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace codeharbor {

LoadBalancer::LoadBalancer(std::string name, std::size_t servers,
                           std::size_t min_servers, std::size_t max_servers,
                           std::size_t max_queue, unsigned scale_interval)
    : name_(std::move(name)), min_servers_(min_servers), max_servers_(max_servers),
      max_queue_(max_queue), scale_interval_(scale_interval) {
    if (servers < min_servers || servers > max_servers || max_queue == 0 || scale_interval == 0)
        throw std::invalid_argument("invalid balancer capacity");
    for (std::size_t i = 0; i < servers; ++i) servers_.emplace_back(next_id_++);
    stats_.peak_servers = stats_.minimum_servers = servers;
}

bool LoadBalancer::enqueue(const Request& request) {
    if (queue_.size() == max_queue_) {
        ++stats_.discarded;
        return false;
    }
    queue_.push(request);
    ++stats_.accepted;
    stats_.peak_queue = std::max(stats_.peak_queue, queue_.size());
    return true;
}

std::size_t LoadBalancer::busy_count() const {
    return static_cast<std::size_t>(std::count_if(servers_.begin(), servers_.end(),
                                                 [](const WebServer& server) { return server.busy(); }));
}

void LoadBalancer::tick(unsigned cycle, std::ostream* verbose) {
    for (auto& server : servers_) {
        if (server.tick()) {
            ++stats_.completed;
            if (verbose) *verbose << "cycle " << cycle << " " << name_
                                  << " complete server=" << server.id() << '\n';
        }
    }
    for (auto& server : servers_) {
        if (queue_.empty()) break;
        if (!server.busy()) {
            const Request request = queue_.front();
            queue_.pop();
            server.assign(request);
            ++stats_.assigned;
            if (verbose) *verbose << "cycle " << cycle << " " << name_
                                  << " assign server=" << server.id()
                                  << " source=" << request.ip_in
                                  << " time=" << request.time
                                  << " waiting=" << queue_.size() << '\n';
        }
    }
    if (cycle % scale_interval_ == 0) scale(cycle, verbose);
}

void LoadBalancer::scale(unsigned cycle, std::ostream* verbose) {
    const std::size_t count = servers_.size();
    std::string action;
    if (queue_.size() > 80 * count && count < max_servers_) {
        servers_.emplace_back(next_id_++);
        ++stats_.servers_added;
        stats_.peak_servers = std::max(stats_.peak_servers, servers_.size());
        action = "add";
    } else if (queue_.size() < 50 * count && count > min_servers_) {
        const auto idle = std::find_if(servers_.begin(), servers_.end(),
                                       [](const WebServer& server) { return !server.busy(); });
        if (idle != servers_.end()) {
            servers_.erase(idle);
            ++stats_.servers_removed;
            stats_.minimum_servers = std::min(stats_.minimum_servers, servers_.size());
            action = "remove";
        }
    }
    if (!action.empty()) {
        scale_events_.push_back({cycle, action, servers_.size(), queue_.size()});
        if (verbose) *verbose << "cycle " << cycle << " " << name_
                              << " scale=" << action << " servers=" << servers_.size()
                              << " waiting=" << queue_.size() << '\n';
    }
}

} // namespace codeharbor
