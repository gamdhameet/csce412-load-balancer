/** @file WebServer.cpp @brief Server assignment and clock updates. */
#include "WebServer.h"

#include <stdexcept>

namespace codeharbor {

void WebServer::assign(const Request& request) {
    if (busy()) throw std::logic_error("cannot assign a busy server");
    if (request.time <= 0) throw std::invalid_argument("request time must be positive");
    current_ = request;
    remaining_ = request.time;
}

bool WebServer::tick() {
    if (!busy()) return false;
    --remaining_;
    if (remaining_ != 0) return false;
    current_.reset();
    return true;
}

} // namespace codeharbor
