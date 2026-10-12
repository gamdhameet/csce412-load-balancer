/** @file WebServer.h @brief One simulated worker. */
#pragma once

#include "Request.h"

#include <optional>

namespace codeharbor {

/** Processes at most one request at a time. */
class WebServer {
public:
    /** Give a stable identifier to this server. */
    explicit WebServer(unsigned id) : id_(id) {}

    /** Assign work to an idle server; throws if already busy. */
    void assign(const Request& request);

    /** Advance one clock cycle; return true if work finished on this tick. */
    bool tick();

    /** Whether this server currently owns a request. */
    bool busy() const { return current_.has_value(); }

    /** Identifier used in verbose events. */
    unsigned id() const { return id_; }

    /** Remaining cycles for the active request, or zero when idle. */
    int remaining() const { return remaining_; }

private:
    unsigned id_;
    std::optional<Request> current_;
    int remaining_ = 0;
};

} // namespace codeharbor
